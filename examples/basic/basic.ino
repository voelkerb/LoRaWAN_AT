/***************************************************
 Example file for using the SimpleMQTT library.
 
 License: Creative Common V1. 

 Benjamin Voelker, voelkerb@me.com
 Embedded Systems Engineer
 ****************************************************/

#include "mqtt.h"
// #include "PubSubClient.h"

#if defined(ESP32)
#include "WiFi.h"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#define RELAY_PIN 13

// Wifi credentials
const char* SSID = "YourNetworkName";
const char* PWD =  "YourPassword";

const char * MQTT_SERVER = "broker.mqtt-dashboard.com";
const char * DEVICE_NAME = "myESPRelay";

MQTT mqtt;

#define MAX_TOPIC_LEN 20
char subscribeTopic[MAX_TOPIC_LEN] = {'\0'};
char publishTopic[MAX_TOPIC_LEN] = {'\0'};

#define MAX_STR_LEN 30
char message[MAX_STR_LEN] = {'\0'};

int relayState = -1;

void setup() {
  Serial.begin(115200);
  // Relay to initial state
  setRelay(true);

  // Connect to WiFi
  WiFi.begin(SSID, PWD);
  Serial.print("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConnected to the WiFi network: %s with IP: %s\n", SSID, WiFi.localIP().toString().c_str());


  // Build publish and subscribe topic strings
  snprintf(&subscribeTopic[0], MAX_TOPIC_LEN, "relay/%s/set", DEVICE_NAME);
  snprintf(&publishTopic[0], MAX_TOPIC_LEN, "relay/%s/get", DEVICE_NAME);

  // Set mqtt and callbacks
  mqtt.init((char*)MQTT_SERVER, (char*)DEVICE_NAME);
  mqtt.onConnect = &onMQTTConnect;
  mqtt.onDisconnect = &onMQTTDisconnect;
  mqtt.onMessage = &mqttCallback;

  bool success = mqtt.connect();
  if (!success) Serial.printf("Error connecting to MQTT Server: %s\n", mqtt.ip);
}

void loop() {
  mqtt.update();
}


/****************************************************
 * If MQTT Server connection was successfull
 ****************************************************/
void onMQTTConnect() {
  Serial.printf("MQTT connected to %s\n", mqtt.ip);
  mqtt.subscribe(subscribeTopic);
  // Should also publish current state
  mqtt.publish("info/", (bool)relayState?"true":"false");
}

/****************************************************
 * If MQTT Server disconnected
 ****************************************************/
void onMQTTDisconnect() {
  Serial.printf("MQTT disconnected from %s\n", mqtt.ip);
}


/****************************************************
 * A mqtt msg was received for a specific 
 * topic we subscribed to. See mqttSubscribe function. 
 ****************************************************/
void mqttCallback(char* topic, byte* msg, unsigned int length) {
  if (length >= MAX_STR_LEN) return;
  // Get message as cstr
  memcpy(&message[0], message, length);
  message[length] = '\0';
  Serial.printf("MQTT msg on topic: %s: %s\n", topic, message);

  // Search for last topic separator
  size_t topicLen = strlen(topic);
  // On not found, this will start from the beginning of the topic string
  int lastSep = -1;
  for (size_t i = 0; i < topicLen; i++) {
    if (topic[i] == '\0') break;
    if (topic[i] == '/') lastSep = i;
  }
  char * topicEnd = &topic[lastSep+1];

  // Switch request
  if(strcmp(topicEnd, "set") == 0) {
    // Case insensitive match
    if (strcasecmp(message, "true") == 0) {
      setRelay(true);
    } else if (strcasecmp(message, "false") == 0) {
      setRelay(false);
    } else {
      Serial.printf("Unknown command");
    }
  }
}

void setRelay(bool value) {
  if (relayState != -1 and (int)value == relayState) return;
  relayState = value;
  digitalWrite(RELAY_PIN, value);
  if (mqtt.connected()) {
    mqtt.publish(publishTopic, relayState?"true":"false");
  }
  Serial.printf("Set relay to %s\n", relayState?"on":"off");
}