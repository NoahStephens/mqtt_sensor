#include <Arduino.h>
// system defines
#define TIMER_UPDATE_TIME  1000   // number of mS between button status checks

// uncomment for DENHAC broker, comment out for "brokerX"
// #define DENHAC

// Network and MQTT broker credentials
#ifdef DENHAC
    const char* mqttBroker = "10.51.97.101";  // mosquitto server (wlan0)
#else
    const char* mqttBroker = "10.0.0.254";       // address of brokerX
#endif    

// MQTT Settings
int mqttPort = 1883;

char* clientID = "gassensor"; 
int TopicsLen = 3;
char* Topics[3] = {"denhac/dirty/metels/gassensor/topics", "denhac/dirty/metels/gassensor/changedata", "denhac/dirty/metels/gassensor/changeheartbeat"}; // Topics ClientID (This Device) to register for

// Default Program settings
int heartbeatInv = 3; // heatbeat interval in (sec)
int sensordataInv = 30; // sensor data interval in (sec)
int sensordataPollingInv = 1000; // sensor data polling interval (micro-sec)
int sensordataOnChnage = 0;     // check wether to send data only on data change.

// Sensor data structure

typedef struct {
    int raw;
    long int timestamp;
    int format;
} sensorData;

// Function Definitions
void heartBeatUpdate();
void sensorDataUpdate();
void getSensorData(int &weight);

void reconnect();

void connect_mqtt();


void connect_wifi();

/*  publish's playload to broker
 *  @senderID - the device to post to
 *  @subTopic - Should be 
 *  @payload - Of type JsonObject, the message payload
 */
void publish(String senderID, String subTopic, JsonDocument& payload);

void publishRegisteredFor(String senderId, char* topics[], int topicsLen);

void publishSensorData(String senderID, int data);

void publishHeartbeat(String senderID) ;

/* 
 * register with MQTT broker for topics of interest to this node
 */ 
void register_myself();

/*
 * un subscribed from MQTT topics with the broker.
 */
void deregister_myself();

/*
 * Disconnects from MQTT broker, un subsribes from MQTT broker, and disconnects from WIFI.
 */
void disconnect();

/*
 * This code is called whenever a message previously registered for is
 * received from the broker. Incoming messages are selected by topic,
 * then the payload is parsed and appropriate action taken. (NB: If only
 * a single message type has been registered for, then there's no need to
 * select on a given message type. In practice this may be rare though...)
 *
 * For info on sending MQTT messages inside the callback handler, see
 * https://github.com/bblanchon/ArduinoJson/wiki/Memory-model.
 * 
 * NB: for guidance on creating proper jsonBuffer size for the json object
 * tree, see https://github.com/bblanchon/ArduinoJson/wiki/Memory-model
*/

void processMQTTMessage(char* topic, byte* json_payload, unsigned int length);

void setup();

void loop();