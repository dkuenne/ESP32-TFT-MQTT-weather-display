/*
ESP32 TFT MQTT weather display

Author:
Dominik Künne

Based on:
https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Generic/Gradient_Fill/Gradient_Fill.ino
https://github.com/adafruit/Adafruit_MQTT_Library/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino

JSON_OBJECT_SIZE calculator:
https://arduinojson.org/v5/assistant/

JSON code generation:
https://arduinojson.org/v7/assistant/#/step3

Used libraries:
https://github.com/Bodmer/TFT_eSPI
https://github.com/adafruit/Adafruit_MQTT_Library
https://github.com/bblanchon/ArduinoJson
https://github.com/jandrassy/ArduinoOTA

OTA Update:
If OTA update fails after authenticating, local firewall from uploading client must be disabled.
Seems to use random port for connecting back.
https://github.com/esp8266/Arduino/issues/3187
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "WiFi.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>

TFT_eSPI tft = TFT_eSPI();
#define UPDATE_TIMEOUT 300000 // 5 min
unsigned long time_LAST_UPDATE = 0;


// ########## WiFi START ##########
WiFiClient wifiClient;
#define WLAN_SSID "xxxx"
#define WLAN_PASS "xxxx"
// ########## WiFi END ##########


// ########## MQTT START ##########
#define MQTT_SERVER      "xxxx"
#define MQTT_SERVERPORT  1883

Adafruit_MQTT_Client mqtt(&wifiClient, MQTT_SERVER, MQTT_SERVERPORT);
Adafruit_MQTT_Subscribe mqttSensors = Adafruit_MQTT_Subscribe(&mqtt, "sensors");
// ########## MQTT END ##########


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
      Serial.println(mqtt.connectErrorString(ret));
      Serial.println("Retrying MQTT connection in 5 seconds...");
      tft.fillRect(0,214,240,26,TFT_BLACK);
      tft.setCursor(10,214);
      tft.setTextColor(TFT_RED);
      tft.print("MQTT error");
      mqtt.disconnect();
      delay(5000);  // wait 5 seconds
      retries--;
      if (retries == 0) {
        // basically die and wait for WDT to reset me
        while (1);
      }
  }
  Serial.println("MQTT Connected!");
  tft.fillRect(0,214,240,26,TFT_BLACK);
}


void setup(void) {

  // ########## START - Serial init ##########
  Serial.begin(115200);
  //Serial.println(F("Messstation - aussen"));
  // ########## END - Serial init ##########

  // ########## START - TFT init ##########
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(4);
  //tft.fillRectVGradient(0, 0, 240, 240, TFT_SKYBLUE, TFT_BLUE);
  tft.setCursor(192,10);
  tft.print("*C");
  tft.setCursor(192,100);
  tft.print("%H");
  // ########## END - TFT init ##########

  // ########## START - WiFi init ##########
  int maxWifiRetries = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // If too many retries go to deep sleep for 15 seconds
    if(maxWifiRetries == 10){
      WiFi.disconnect();
      ESP.deepSleep(15e6);
    }
    maxWifiRetries++;
  }

  /* WiFi Debug
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  */
  // ########## END - WiFi init ##########

  // ########## START - MQTT init ##########
  mqtt.subscribe(&mqttSensors);
  // ########## END - MQTT init ##########


  // ########## OTA START ##########
  // Port defaults to 3232
  ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("xxxx");

  // No authentication by default
  ArduinoOTA.setPassword("xxxx");

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
        } else {  // U_SPIFFS
          type = "filesystem";
        }

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
          Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
          Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
          Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
          Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
          Serial.println("End Failed");
        }
      });

    ArduinoOTA.begin();
  // ########## OTA END ##########

}

void loop()
{

  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &mqttSensors) {

        /* MQTT Debug
        //{"source":"weather","location":"location","sensor":"SHT41_Temperature","value":  4.16}
        //{"source":"weather","location":"location","sensor":"SHT41_Humidity","value": 64.86}

        //Serial.print(F("Got: "));
        //Serial.println((char *)mqttSensors.lastread);
        */
        
        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, mqttSensors.lastread, 150);

        if (error) {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
          return;
        }

        const char* source = doc["source"]; // "weather"
        const char* location = doc["location"]; // "location"
        const char* sensor = doc["sensor"]; // "SHT41_Temperature"
        float value = doc["value"]; // 4.16
        const char* sensorTemperature = "SHT41_Temperature";
        const char* sensorHumidity = "SHT41_Humidity";

        Serial.print(sensor);
        Serial.print(", ");
        Serial.println(value);

        tft.setTextFont(6);
        tft.setTextColor(TFT_WHITE);

        // Display temperature
        if(strcmp(sensor, sensorTemperature) == 0){
          tft.fillRect(0,10,192,48,TFT_BLACK);
          tft.setCursor(10,10);
          tft.print(value);
        }
        // Display humidity
        if(strcmp(sensor, sensorHumidity) == 0){
          tft.fillRect(0,100,192,48,TFT_BLACK);
          tft.setCursor(10,100);
          tft.print(value);
        }

        // Reset timeout
        time_LAST_UPDATE = millis();
    }
  }

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  if (!mqtt.ping())
  {
    mqtt.disconnect();
  }

  // Timeout check, show error if last update too far gone
  if ((millis() - time_LAST_UPDATE) > UPDATE_TIMEOUT)
  {
    Serial.println("Timeout error!");
    tft.fillRect(0, 214, 240, 26, TFT_BLACK);
    tft.setCursor(10, 214);
    tft.setTextColor(TFT_RED);
    tft.print("Timeout error");
  }

  ArduinoOTA.handle();
  
  delay(1000);

}
