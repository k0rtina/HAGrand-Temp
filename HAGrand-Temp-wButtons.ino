////////////////////////////////////////////////////////////////////////////////////////////////////
// Modify these values for your environment
const char* wifiSSID = "putssidhere";                    // Your WiFi network name
const char* wifiPassword = "putwifipasswordhere";             // Your WiFi network password
const char* otaPassword = "otapw";                      // OTA update password
const char* mqttServer = "192.168.1.100";             // Your MQTT server IP address
const char* mqttUser = "homeassistant";                 // mqtt username, set to "" for no user
const char* mqttPassword = "mqttpassword";               // mqtt password, set to "" for no password
const String mqttNode = "mynode";                     // Your unique hostname for this device
const String mqttDiscoveryPrefix = "homeassistant";     // Home Assistant MQTT Discovery, see https://home-assistant.io/docs/mqtt/discovery/

const String mqttDiscoBinaryStateTopic = mqttDiscoveryPrefix + "/binary_sensor/" + mqttNode + "/state";
const String mqttDiscoBinaryConfigTopic = mqttDiscoveryPrefix + "/binary_sensor/" + mqttNode + "/config";
const String mqttDiscoSensorTopic = mqttDiscoveryPrefix + "/sensor/" + mqttNode + "/" + mqttNode + "sensor1/set";

//Names for the TWO objects temperature and humidity you can change these in customize.yaml
const String mqttDiscoTemperatureTopicConfig = mqttDiscoveryPrefix + "/sensor/" + mqttNode + "/" + mqttNode + "Temperature/config";
const String mqttDiscoHumidityTopicConfig = mqttDiscoveryPrefix + "/sensor/" + mqttNode + "/" + mqttNode + "Humidity/config";
const String mqttDiscoTemperatureTopic = mqttDiscoveryPrefix + "/sensor/" + mqttNode + "/" + mqttNode + "Temperature/set";
const String mqttDiscoHumidityTopic = mqttDiscoveryPrefix + "/sensor/" + mqttNode + "/" + mqttNode + "Humidity/set";
const String mqttDiscoTemperatureConfigPayload = "{\"name\": \"" + mqttNode + "Temperature\", \"device_class\": \"sensor\", \"state_topic\": \"" + mqttDiscoTemperatureTopic + "\", \"unit_of_measurement\": \"°C\", \"qos\": \"0\"}";
const String mqttDiscoHumidityConfigPayload = "{\"name\": \"" + mqttNode + "Humidity\", \"device_class\": \"sensor\", \"state_topic\": \"" + mqttDiscoHumidityTopic + "\", \"unit_of_measurement\": \"%\", \"qos\": \"0\"}";

////////////////////////////////////////////////////////////////////////////////////////////////////
// Set LED "twinkle" time for maximum daylight visibility
const unsigned long twinkleInterval = 50;
unsigned long twinkleTimer = millis();

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

#define OLED_RESET 0  // GPIO0
#define MQTT_VERSION MQTT_VERSION_3_1_1

Adafruit_SSD1306 display(OLED_RESET);
SHT3X sht30(0x45);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

  
void setup() {
  // initialize the serial
  Serial.begin(115200);

  // initialize with the I2C addr 0x3C (for the 64x48)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  // Setup buttons on the OLED Screen
  //pinMode(D4, INPUT); // Setting D4 B Button as input
  //pinMode(D3, INPUT); // Setting D3 A Button as input
  
  // Show MQTT topic
  Serial.println(mqttDiscoTemperatureConfigPayload);
  Serial.println(mqttDiscoHumidityConfigPayload);
  
  // Start up networking
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPassword);
  
  // Create server for MQTT
  mqttClient.setServer(mqttServer, 1883);
}

void loop() {
  //Stuff that doesnt need WiFi
  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);

  if(sht30.get()==0){
    display.print("Temp ");
    display.print((char)247); //print the degree symbol... its not technically correct but looks best. See https://en.wikipedia.org/wiki/Degree_symbol
    display.println("C:");
    display.setTextSize(2);
    display.println(sht30.cTemp);
    float p_temperature = sht30.cTemp;
    //Serial.println(p_temperature);
    //display.println(sht30.fTemp); //you could use this if you wanted to display Farenheit
    display.setTextSize(1);
    display.println("Humidity: ");
    display.setTextSize(2);
    display.println(sht30.humidity);
    float p_humidity = sht30.humidity;
    //Serial.println(p_humidity);
    //Serial.println(LED_BUILTIN);
    
    if (isnan(p_humidity) || isnan(p_temperature)) {
      Serial.println("ERROR: Failed to read from temp sensor!");
      return;
    } else {
      //Serial.println(p_humidity);
      //Serial.println(p_temperature);

      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Not Connected!");
      }
      
      ////if the wifi is connected, then do network stuff
      if (WiFi.status() == WL_CONNECTED) {
        // if MQTT client is not conntected, then attempt to conntect 
        if (!mqttClient.connected()) {
         // digitalWrite(LED_BUILTIN, HIGH); //if MQTT is not connected, turn OFF the LED to give the user feedback that something is wrong
          Serial.println("MQTT Not Connected! retrying...");
          mqttConnect();
        }
        //if MQTT client is connected, then attempt to publish
        if (mqttClient.connected()) {
          publishData(p_temperature, p_humidity); //if the WiFi and MQTT are connected, then publish the data to MQTT
          
          //Serial.println(!digitalRead(LED_BUILTIN));
          mqttClient.loop();
          digitalWrite(LED_BUILTIN, LOW);
        }
      } //end of Network connected code
    }
  }
  else
  {
    display.println("Error!");
    display.println("");
    display.println("Sensor Not Connected?");
  }
  display.display();
  digitalWrite(LED_BUILTIN, HIGH); //this will turn the LED on
  
  delay(1000);
   
  // This is the end of the main loop, below we just check if the buttons have been pushed
  
  // If Button 'B' is pushed on the OLED Screen
  if (digitalRead(D4) == LOW)
  {    
    // Clear the buffer.
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.setTextWrap(false);

    if (WiFi.status() == WL_CONNECTED) {
      long rssi = WiFi.RSSI();
      int bars = getBarsSignal(rssi);
      //display signal quality bars
      for (int b=0; b <= bars; b++) {
        display.fillRect(42 + (b*5),48 - (b*5),2,b*5,WHITE); //put the WiFi bars in the bottom right corner of the screen
      }
    }
    
    display.println(wifiSSID);

    if (WiFi.status() == WL_CONNECTED) {
      // if Wifi is connected write to the screen and console
      display.println("WiFi:   OK");
      Serial.println("WiFi Stength: " + (String)WiFi.RSSI() + "dBm");
      Serial.print(wifiSSID);
      Serial.println(" connected, IP: " + WiFi.localIP().toString());
    } else {
      display.println("WiFi:   NA");  
      Serial.print(wifiSSID);
      Serial.println(" NOT connected");
    }
    if (mqttClient.connected()) {
      // if MQTT is connected write to the screen and console
      display.println("MQTT:   OK");
      display.println(mqttNode);
      Serial.print(mqttServer);
      Serial.println(" MQTT server connected");
      Serial.println(mqttNode);
    }else{
      display.println("MQTT:   NA"); 
      Serial.print(mqttServer);
      Serial.println(" MQTT server NOT connected"); 
    }
 
    display.display();
    delay(5000);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Publish the Temperature and the Humidity
void publishData(float p_temperature, float p_humidity) {
  //conver the temp and humidity from float to a string, so that we can publish it
  String strTemperature = (String)p_temperature;
  String strHumidity = (String)p_humidity;
  
  mqttClient.publish(mqttDiscoTemperatureTopic.c_str(), strTemperature.c_str(), true);
  Serial.print("Published Temperature: ");
  Serial.println(strTemperature);
  mqttClient.publish(mqttDiscoHumidityTopic.c_str(), strHumidity.c_str(), true);
  Serial.print("Published Humidity: ");
  Serial.println(strHumidity);
  Serial.println("");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// MQTT connection and subscriptions
void mqttConnect() {
  Serial.println("Attempting MQTT connection to broker: " + String(mqttServer));
  // Attempt to connect to broker, setting last will and testament (for the Device, and the two switches
  if (mqttClient.connect(mqttNode.c_str(), mqttUser, mqttPassword, mqttDiscoBinaryStateTopic.c_str(), 1, 1, "OFF")) {
    // publish MQTT discovery topics and device state
    Serial.println("MQTT discovery Temperature config: [" + mqttDiscoTemperatureTopicConfig + "] : [" + mqttDiscoTemperatureConfigPayload + "]");
    Serial.println("MQTT discovery Humidity config: [" + mqttDiscoHumidityTopicConfig + "] : [" + mqttDiscoHumidityConfigPayload + "]");
    
    mqttClient.publish(mqttDiscoTemperatureTopicConfig.c_str(), mqttDiscoTemperatureConfigPayload.c_str(), true);
    mqttClient.publish(mqttDiscoHumidityTopicConfig.c_str(), mqttDiscoHumidityConfigPayload.c_str(), true);
    Serial.println("MQTT connected");
  }
  else {
    Serial.println("MQTT connection failed, rc=" + String(mqttClient.state()));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// WiFi Bars
int getBarsSignal(long rssi){
  // 5. High quality: 90% ~= -55db
  // 4. Good quality: 75% ~= -65db
  // 3. Medium quality: 50% ~= -75db
  // 2. Low quality: 30% ~= -85db
  // 1. Unusable quality: 8% ~= -96db
  // 0. No signal
  int bars;
  
  if (rssi > -55) { 
    bars = 5;
  } else if (rssi < -55 & rssi > -65) {
    bars = 4;
  } else if (rssi < -65 & rssi > -75) {
    bars = 3;
  } else if (rssi < -75 & rssi > -85) {
    bars = 2;
  } else if (rssi < -85 & rssi > -96) {
    bars = 1;
  } else {
    bars = 0;
  }
  return bars;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// (mostly) boilerplate OTA setup from library examples
void setupOTA() {
  // Start up OTA
  // ArduinoOTA.setPort(8266); // Port defaults to 8266
  ArduinoOTA.setHostname(mqttNode.c_str());
  ArduinoOTA.setPassword(otaPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("ESP OTA:  update start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("ESP OTA:  update complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.println("ESP OTA:  ERROR code " + String(error));
    if (error == OTA_AUTH_ERROR) Serial.println("ESP OTA:  ERROR - Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("ESP OTA:  ERROR - Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("ESP OTA:  ERROR - Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("ESP OTA:  ERROR - Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("ESP OTA:  ERROR - End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("ESP OTA:  Over the Air firmware update ready");
}
