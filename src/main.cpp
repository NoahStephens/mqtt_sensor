#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <TickTwo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <main.h>

WiFiClient wfClient;                // create a wifi client
PubSubClient psClient(wfClient);  // create a pub-sub object (must be
					// associated with a wifi client)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP); // create time server client

bool sendStatusFlag;
int publishCount;

// char buffer to store incoming/outgoing messages
char json_msgBuffer[200];

void heartBeatUpdate() {
  publishHeartbeat(clientID);
}

void sensorDataUpdate() {
	int weight;
	getSensorData(weight);
  	publishSensorData(clientID, weight);
}

void getSensorData(int &weight) 
{
	weight = 1;
}

// create a Ticker object to periodically check the debounced pushbutton
// Ticker Timer;
/** create a Ticker object
	 *
	 * @param callback the name of the function to call
	 * @param timer interval length in ms or us
	 * @param repeat default 0 -> endless, repeat > 0 -> number of repeats
	 * @param resolution default MICROS for tickers under 70min, use MILLIS for tickers over 70 min
	 *
	 */
TickTwo HeartBeatTimer(heartBeatUpdate, 30000, 0, MILLIS);               // Timer constructor: funct_ptr, repeat
TickTwo SensorDataTimer(sensorDataUpdate, 6000, 0, MILLIS);   // Timer constructor: funct_ptr, repeat, resolution

/**********************************************************
 * Helper functions
 *********************************************************/

void reconnect() 
{
  // Loop until the pub-sub client connects to the MQTT broker
  while (!psClient.connected()) {
	// attempt to connect to MQTT broker
	Serial.print("Connecting to MQTT broker (");
	Serial.print(mqttBroker);
	Serial.print(") as ");
	Serial.print(clientID);
	Serial.print("...");

	if (psClient.connect(clientID)) {
	  Serial.println(" connected");
	  // clientID MUST BE UNIQUE for all connected clients
	  // can also include username, password if broker requires it
	  // (e.g. psClient.connect(clientID, username, password)

	  // once connected, register for topics of interest
	  register_myself();
	}
	else {
	  // reconnect failed so print a console message, wait, and try again
	  Serial.println(" failed.");
	  // wait 5 seconds before retrying
	  delay(5000);
	  }
	}
}

void connect_npt() {
	timeClient.begin();
}
																				 
void connect_mqtt() {
	// specify MQTT broker's domain name (or IP address) and port number
	psClient.setServer(mqttBroker, mqttPort);

	// Set MQTT Callback function  
	psClient.setCallback(processMQTTMessage);

	// connect to MQTT broker
	if (!psClient.connected())
		reconnect();
}

void connect_wifi() 
{
	// in an attempt to remove the annoying garbled text on
	// startup, print a couple of blank lines (with delay)
  	Serial.println();
  	delay(100);
  	Serial.println();
  	delay(100);

	// attempt to connect to the WiFi network
  	Serial.print("Connecting to ");
  	Serial.print(ssid);
  	Serial.print(" network");
 	 delay(10);
  
  	#ifdef DENHAC
		WiFi.begin(ssid);            // Lipscomb WiFi does NOT require a password
  	#else
		WiFi.begin(ssid, password);  // For WiFi networks that DO require a password
  	#endif

  	// advance a "dot-dot-dot" indicator until connected to WiFi network
  	while (WiFi.status() != WL_CONNECTED) {
		delay(300);
		Serial.print(".");
  	}

  	// report to console that WiFi is connected and print IP address
 	Serial.print(" connected as ");
  	Serial.println(WiFi.localIP());
}

void publish(String senderID, String subTopic, JsonDocument& payload) 
{
	serializeJson(payload, json_msgBuffer);
  	String msgTopic = "denhac/dirty/metels/" + senderID + "/" + subTopic;
  	const char *msg = msgTopic.c_str();  // put into char array form (zero copy!!)
  	psClient.publish(msg, json_msgBuffer); // publish msg
	publishCount++;
	Serial.print(publishCount);
  	Serial.println("Message published to Topic: {" + msgTopic + "}");
	// Serial.println("published Payload START #############################");
  	// serializeJsonPretty(payload, Serial); // print payload
  	// Serial.println("published Payload END #############################");
  	Serial.print("\n\r");
}

void publishRegisteredFor(String senderId, char* topics[], int topicsLen) 
{
  	JsonDocument jsonBuffer;
  	jsonBuffer["ClientId"] = senderId;

  	// create a nested array for the json buffer
  	JsonArray frame = jsonBuffer["topics"].to<JsonArray>();

  	// for each topic in topics array
  	for(int i = 0; i < TopicsLen; i++)
		frame.add(topics[i]);

  	// Send message
  	publish(senderId, "registeredFor", jsonBuffer);
}

void publishSensorData(String senderID, int data) 
{
	JsonDocument jsonBuffer;
  	jsonBuffer["clientId"] = senderID; 

  	// create a nested array for the json buffer
  	JsonObject payload = jsonBuffer.add<JsonObject>();
  	payload["weight"] = data;
	payload["updateTime"] = timeClient.getEpochTime();

	// send data message
 	publish(senderID, "data", jsonBuffer);
}

void publishHeartbeat(String senderID) 
{
  /* Usage: To periodically send a heartbeat message from the device. 
   * The interval is set by @heartbeatInvr in main.h.
   * Setting @heartbeatInv will force the client to stop sending heartbeats.
   * Payload Schema: {"clientId":"deviceId"}
   *      @clientId - The deviceId (@clientID)
   */
   
  JsonDocument jsonBuffer;

  // fill tree with message data
  jsonBuffer["clientId"] = senderID;
//  payload["heartbeat"] = heartbeatInv;

  // send data message
  publish(senderID, "heartbeat", jsonBuffer);
}

void register_myself() 
{
	delay(100); // added for stabilization

  Serial.print("Registering for topics...");

  for(int i=0; i<TopicsLen; i++) // Registering for Topics (main.h)
	psClient.subscribe(Topics[i]) ? Serial.print(" {" + String(Topics[i]) + " Successfully registered!}") : Serial.print(" {" + String(Topics[i]) + " Not registered!}");
  
  Serial.println(" .. Done.");
}

void deregister_myself() 
{
  Serial.print("Unregistering from topics...");
  for(int i=0; i< TopicsLen; i++)
	psClient.unsubscribe(Topics[i]) ? Serial.print(" {" + String(Topics[i]) + " Successfully unsubscribed!}") : Serial.print(" {" + String(Topics[i]) + " Not unsubscribed!}");

  Serial.println(" .. Done.");
}

void disconnect() 
{
  deregister_myself();
	// disconnects MQTT and WIFI Client
  while (psClient.connected()) {
	Serial.print("Disconnecting from MQTT broker(");
	Serial.print(mqttBroker);
	Serial.print(") as ");
	Serial.print(clientID);
	Serial.print("...");
  }

	if (!psClient.connected()) {
	  Serial.println(" disconnected");
	}
	else {
	  // reconnect failed so print a console message, wait, and try again
	  Serial.println(" failed.");
	  Serial.println("Trying again in 5 sec. (Are there queued messages?)");
	  // wait 5 seconds before retrying
	  delay(5000);
	  }

	if(wfClient.connected())
	  wfClient.stop();
}

void processMQTTMessage(char* topic, byte* json_payload, unsigned int length) 
{

  // process messages by topic
	Serial.println("Message from Topic {" + String(topic) + "} ... ");
  	if(strcmp(topic, Topics[0]) == 0) {// topic - /topics"topics":["<Topic1_txt>", "<TopicN_txt>"
		publishRegisteredFor(clientID, Topics, TopicsLen);
	}
  	else if(strcmp(topic, Topics[1]) == 0) { // topic - /changedata
		/* Usage: Device asks to change data rate
		* Payload Schema: {"rate":0, "onChange":0, "pollingRate":0}
		*      @rate - Frequency of which sensor data is published (int - seconds)
		*      @onChange - Bool flag for changing data to only be published if there is a change. ex. sensor val at t1 is 1 and t2 is 2. (bool/int - 0/1)
		*      @pollingRate - Frequency is which the sensor measurement is taken. (int - milliseconds)
		*      Note: if onChange is enabled the sensor will publish data at the rate stored, but only if there is a change.
		*/
		   
	JsonDocument jsonBuffer;
	deserializeJson(jsonBuffer, json_payload);
	  String doc_rate = jsonBuffer["rate"];
	  String doc_on_change = jsonBuffer["onChange"];
	  String doc_polling_rate = jsonBuffer["pollingRate"];

		HeartBeatTimer.interval(doc_rate.toInt());
	//   sensordataCount = doc_rate.toInt();
	  sensordataOnChnage = doc_on_change.toInt();
	  sensordataPollingInv = doc_polling_rate.toInt();

	  Serial.print("data change request: rate: " + doc_rate + " onChange: " + doc_on_change + " pollingRate: " + doc_polling_rate + "... ");
		  }

  else if(strcmp(topic, Topics[2]) == 0) { // topic - /changeheartbeat
	/* Usage: To resc'v a heartbeat message from the node. 
	* Payload Schema: {"Heartbeat":"heartbeart"}
	*      @heartbeat - the new heartbeat interval (int - seconds)
	*/

	JsonDocument jsonBuffer;
	deserializeJson(jsonBuffer, json_payload);

	String doc_heartbeat = jsonBuffer["heartbeat"];
	heartbeatInv = doc_heartbeat.toInt();

  }
  else // unhandled
	Serial.print("Unhandled Topic: "+ String(topic));
  Serial.println("done");
}


void setup() 
{
	Serial.begin(115200);
 
// initialize and connect MQTT Client
	connect_wifi();
	connect_mqtt();
	connect_npt();
  	delay(3000);
  	HeartBeatTimer.start();
  	SensorDataTimer.start();
	publishCount = 0;

  // Timer.attach_ms(TIMER_UPDATE_TIME, updateTimer);
}

void loop() {
	if(!psClient.connected())
		reconnect();

	HeartBeatTimer.update();
	SensorDataTimer.update();
	timeClient.update();

	psClient.loop(); // checks for new messages from the MQTT Broker
}
