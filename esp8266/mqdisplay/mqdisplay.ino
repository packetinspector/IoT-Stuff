/*
MQTT DISPLAY

No config inside the code is necessary.  Just flash, config is done via web. 
command topic will be displayed on-screen created as <mqtt-clientname>/command

message contains a json payload:
e.g.
// Set Brightness
{"command": "brightness", "command_data": "50" }
// Display text
{"command": "message", "command_data": "My Cool Message" }
// HA Template
{"command": "message", "command_data": " {{ states.sensor.dark_sky_daily_summary.state }} "}
// Whats playing?
{"command": "message", "command_data": " {{ states.media_player.kodi.attributes.media_series_title }} "}
// Show things
{"command": "message", "command_data": "High: {{ states.sensor.dark_sky_daily_high_temperature.state}} \nLow:  {{ states.sensor.dark_sky_daily_low_temperature.state}} \n"}
// Will use new line chars to break
{"command": "message", "command_data": "DarkSky\nTodays Temp\nCurrent: {{ states.sensor.dark_sky_temperature.state }}\nHigh:    {{ states.sensor.dark_sky_daily_high_temperature.state}}\nLow:     {{ states.sensor.dark_sky_daily_low_temperature.state}}"}
// Reconnect to mqtt - Not sure this will ever be needed as you need to be connected...yeah....
{"command": "mqreconnect"}
// Reset your device
{"command": "reset"}

Home Assistant Integration!
This will add itself to home assistant as a Light component.  You can then turn the display ON/OFF and adjust brightness

ToDo:

Add flash effect, fill panel with color off/on
Text color support
Widgets: display image from SPIFFS
Image Downloader: Push URL to download image
//
Use Integrated touch to make "buttons"
*/

// Include application, user and local libraries
#include <FS.h> 
#include "SPI.h"
#include <ArduinoJson.h>

// Your display may very
// #include "TFT_22_ILI9225.h"
#include <TFT_eSPI.h>      // Hardware-specific library
// Additional UI functions
// #include "GfxUi.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
// This override may or may not work, if not edit PubSub
#define MQTT_MAX_PACKET_SIZE 2048
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);

// Display Setup
// #define TFT_RST D4
// #define TFT_RS  D7
// #define TFT_CS  D8  // SS
// #define TFT_SDI D5  // MOSI
// #define TFT_CLK D6  // SCK
// #define TFT_LED D2   // 0 if wired to +5V directly

// !!!!!! This is not part of TFT_eSPI !!!!!!
#define TFT_LED PIN_D0

#define TFT_BRIGHTNESS 200 // Initial brightness of TFT backlight (optional)
// #define TFT_LINE_LENGTH 30
#define TFT_MAX_CHARS 1500

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
// GfxUi ui = GfxUi(&tft);

// Use hardware SPI (faster - on Uno: 13-SCK, 12-MISO, 11-MOSI)
// TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);
// Use software SPI (slower)
// TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_SDI, TFT_CLK, TFT_LED, TFT_BRIGHTNESS);
// End Display Setup

#define HOSTNAME "MQTT-DISPLAY"
//Vars for config
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_username[40];
char mqtt_password[40];
char mqtt_clientname[23] = HOSTNAME;
const char* haPrefix = "homeassistant";
bool shouldSaveConfig = false;
char* strPayload;
boolean willRetain = true;
const char* willMessage = "offline" ;
// Global Vars
// char* commandTopic;  // command topic for messages 
// char* willTopic;
int currentBrightness = TFT_BRIGHTNESS;
boolean displayLightOn = true; 
boolean updatedMessage = false;
boolean updatedisplay = false;
String command = "";
String command_data = "";
String commandTopic = "NOTYET";
String hastateTopic = "NOTYET";
String hasetTopic = "NOTYET";
String currentTopic = "";
DynamicJsonBuffer  jsonBuffer; // allocate dynamic buffer for json work
JsonObject& root = jsonBuffer.createObject(); // create object for json maniupulation

// Functions
void display_line(int x, int y, String s, uint16_t color = TFT_WHITE) {
    // tft.drawText(x,y,s,color);
    //Avoid code changes if switch to different lib (which happened)
    tft.setTextColor(color);
    tft.drawString(s,x,y);
}

void setup_wifi(){
    // Set Hostname
    WiFi.hostname(HOSTNAME);
    //WiFiManager
    WiFiManager wifiManager;
    // Uncomment to start all over
    // wifiManager.resetSettings();
    // Add MQTT settings to config portal
    WiFiManagerParameter custom_mqtt_clientname("clientname", "client name", mqtt_clientname, 23);
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_username("user", "mqtt username", mqtt_username, 40);
    WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 40);
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_username);
    wifiManager.addParameter(&custom_mqtt_password);
    wifiManager.addParameter(&custom_mqtt_clientname);
    wifiManager.autoConnect(HOSTNAME);
    // Wifi Connected now, display results
    WiFi.printDiag(Serial);
    tft.fillScreen(TFT_BLACK);
    display_line(10,10, "Network: " + WiFi.SSID());
    display_line(10,20, "IP Address: " + WiFi.localIP().toString());
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_username, custom_mqtt_username.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    strcpy(mqtt_clientname, custom_mqtt_clientname.getValue());
    delay(1000);
    // display_line(30,20, );
}

// Called if WiFi has not been configured yet
void configModeCallback (WiFiManager *myWiFiManager) {
    tft.fillScreen(TFT_BLACK);
    display_line(10,10, "Wifi Manager");
    display_line(10,20, "Please connect to AP");
    display_line(10,30, myWiFiManager->getConfigPortalSSID(), TFT_ORANGE);
    display_line(10,40, "To setup Wifi Configuration");
}

void saveConfigCallback () {
    // The wifi has been configured and config data captured
    Serial.println("Saving Data...");
    Serial.println("Clearing SPIFFS");
    display_line(10, 50, "Clearing Spiffs...");
    SPIFFS.format();
    display_line(10, 60, "Success");
    delay(1000);
    shouldSaveConfig = true;
}

void display_off() {
  // tft.setBacklight(false);
  // tft.setDisplay(false);
  display_brightness(0);
  displayLightOn = false;
}

void display_on() {
  // tft.setBacklight(true);
  // tft.setDisplay(true);
  display_brightness(currentBrightness);
  displayLightOn = true;
}

void display_rotate(int rotation) {
  tft.setRotation(rotation);
}

void display_brightness(int brightness){
  // Not included in lib.  Set the voltage to dim backlight.  Range
  // is 0..1023 but most things are 0..255
  if (brightness != 0) {
    currentBrightness = brightness;
    analogWrite(TFT_LED, int(((float)brightness/255) * 1022));
  } else {
    // I want to save the current brightness state so leave it untocuhed.
    analogWrite(TFT_LED, brightness);
  }
}


void setup_display(){
    // tft.begin();
    // tft.fillScreen(TFT_BLACK);
    // // tft.setBacklightBrightness(TFT_BRIGHTNESS);
    // tft.setOrientation(3);
    // tft.setFont(Terminal6x8);
    // tft.setBackgroundColor(COLOR_BLACK);
    // display_line(10,10, "Initializing Device...", TFT_WHITE);

    pinMode(TFT_LED, OUTPUT);
    display_brightness(currentBrightness);
    // analogWrite(TFT_LED, 1023);

    // Initialise the TFT screen
    tft.init();

    // Set the rotation before we calibrate
    tft.setRotation(0);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    display_line(10,10, "Initializing Device...", TFT_WHITE);
}
void load_config() {
    //clean FS, for testing
    //SPIFFS.format();

    //read configuration from FS json
    Serial.println("mounting FS...");

    if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        // DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_username, json["mqtt_username"]);
          strcpy(mqtt_password, json["mqtt_password"]);
          strcpy(mqtt_clientname, json["mqtt_clientname"]);
          tft.fillScreen(TFT_BLACK);
          display_line(10,10, "Config Loaded");
          display_line(10,20, "MQTT Server: " + String(mqtt_server));
          display_line(10,30, "MQTT Port: " + String(mqtt_port));
          display_line(10,40, "MQTT User: " + String(mqtt_username));
          display_line(10,50, "MQTT Pass: ........");
          display_line(10,60, "MQTT Clientname: " + String(mqtt_clientname));
          delay(1500);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
    } else {
    Serial.println("failed to mount FS");
    }
}
void save_config(){
     if (SPIFFS.begin()) {
        tft.fillScreen(TFT_BLACK);
        display_line(10,10, "Saving Config...");
        Serial.println("saving config");
        // DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["mqtt_server"] = mqtt_server;
        json["mqtt_port"] = mqtt_port;
        json["mqtt_username"] = mqtt_username;
        json["mqtt_password"] = mqtt_password;
        json["mqtt_clientname"] = mqtt_clientname;
        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
            Serial.println("failed to open config file for writing");
            display_line(10,20, "Failed to save config!");
        }

        json.printTo(Serial);
        json.printTo(configFile);
        configFile.close();
        //end save
        display_line(10, 20, "Success");
        delay(1000);
    } else{
        Serial.println("SPIFFS FAILURE!");
        display_line(10,10, "SPIFFS FAILURE!");
    }
}

void setup_mq() {
    client.setServer(mqtt_server,atoi(mqtt_port));
    client.setCallback(mqcallback);
    // client.connect(HOSTNAME, mqtt_username, mqtt_password, willTopic, 0, willRetain, willMessage)}
}
void mqcallback(char* topic, byte* payload, unsigned int length) {
    Serial.println("Topic:");
    Serial.println(topic);
    currentTopic = String(topic);
    strPayload = ((char*)payload);
    // strcpy(strPayload, (char*)payload);
    Serial.println("Payload:");
    Serial.println(strPayload);
    updatedMessage=true;
}

void mqconnect() {
  // Loop until we're reconnected
  // String commandTopic = mqtt_clientname + "/message";

  // Strings or chars....different methods
  commandTopic = String(mqtt_clientname) + "/command";
  char willTopic[35] = HOSTNAME;
  strcpy(willTopic, mqtt_clientname);
  strcat(willTopic, "/status");
  // HA Things
  String haconfigTopic = String(haPrefix) + "/light/" + String(mqtt_clientname) + "/config";
  hasetTopic = String(mqtt_clientname) + "/set";
  hastateTopic = String(haPrefix) + "/light/" + String(mqtt_clientname) + "/state";
  // HA Light Config
  String haconfigMessage;
  JsonObject& json = jsonBuffer.createObject();
  json["name"] = mqtt_clientname;
  json["platform"] = "mqtt_json";
  json["brightness"] = "true";
  json["command_topic"] = hasetTopic;
  json.printTo(haconfigMessage);
  json.printTo(Serial);
  // Tell Display
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  // Connect!
  while (!client.connected()) {
    tft.fillScreen(TFT_BLACK);
    Serial.print("Attempting MQTT connection as ...");
    Serial.print(mqtt_clientname);
    Serial.print("..");
    display_line(10,10, "Connecting to MQTT...");
    // Attempt to connect
    // if (client.connect(mqtt_clientname, mqtt_username, mqtt_password, willTopic, 0, willRetain, willMessage)) {
    boolean connected = false;
    if (String(mqtt_username) == "") {
       connected = client.connect(mqtt_clientname, willTopic, 0, willRetain, willMessage);
    } else {
       connected = client.connect(mqtt_clientname, mqtt_username, mqtt_password, willTopic, 0, willRetain, willMessage);
    }
    if (connected) {
      Serial.println("connected");
      // Once connected, update status to online - will Message will drop in if we go offline ...
      client.publish(willTopic,"online",true);
      // listen for display commands    
      client.subscribe(commandTopic.c_str());
      // listen for HA
      client.subscribe(hasetTopic.c_str());
      // Publish HA Config
      client.publish(haconfigTopic.c_str(), haconfigMessage.c_str(), true);
      // Publish HA State
      client.publish(hastateTopic.c_str(), haState().c_str(), true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      display_line(10,20, "failed");
      display_line(10,30, "Reason: " + String(client.state()));
      display_line(10,40, "Waiting...");
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  display_line(10,30, "Connection Established");
  display_line(10,40, "Client Name: " + String(mqtt_clientname));
  display_line(10,50, "Topic: " + String(commandTopic));
  display_line(10,70, "    (Waiting for Message)    ", TFT_YELLOW);
}

void parseMessages() {
    // parse the mq message

    // Make sure we don't call this again
    updatedMessage = false; 
    JsonObject& root = jsonBuffer.parseObject(strPayload);
    // Test if parsing succeeds.
    if (!root.success()) {   
        Serial.println("Failed to Parse JSON.");
        return;
    } 
    if (currentTopic == commandTopic) {
        Serial.println("processing commands"); 
        if (root.containsKey("command")) {
            command = root["command"].as<char*>();
        }
        if (root.containsKey("command_data")) {
            command_data = root["command_data"].as<char*>();
        }
        updatedisplay = true;
        // Let the display function deal with it
        return;
    }
    // HA Commands
    // I've already parsed the json, no need to keep relaying it for HA
    if (currentTopic == hasetTopic) {
        Serial.println("processing HA message");
        if (root.containsKey("brightness")) {
            display_brightness(root["brightness"]);
        }
        if (root.containsKey("state")) {
            if (root["state"].as<String>() == "ON") {
                display_on();
            } else{
                display_off();
            }
            Serial.println("Sending state");
            //Serial.println(haState());
            client.publish(hastateTopic.c_str(), haState().c_str(), true);
        }
    }
}

String haState() {
    //Return json for state of display
    String state;
    JsonObject& json = jsonBuffer.createObject();
    json["state"] = displayLightOn ? "ON" : "OFF";
    json["brightness"] = currentBrightness; 
    json.printTo(state);
    return state;
}

void hacommands(){

}

void runCommands(){
    // Do stuff based on the message

    // Make sure we don't call this again
    updatedisplay = false;
    Serial.println("Command: " + command);
    Serial.println("Data: " + command_data);
    // Basic Case...
    if (command == "brightness") {
        // tft.setBacklightBrightness(command_data.toInt());
        display_brightness(command_data.toInt());
    }
    if (command == "message") {
       drawtext(command_data);
    }
    if (command == "rotate") {
       display_rotate(command_data.toInt());
    }
    if (command == "reset") {
      ESP.reset();
    }
    if (command == "mqreconnect") {
        client.disconnect();
        mqconnect();
    }
}

void drawtext(String message){
    // max = 1500; // Font 1
    // 400, font 2
    // 165, font 3

    // tft.setFont(Terminal12x16);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(0,0);
    // int text_length = command_data.length();
    // Serial.print("Writing ");
    // Serial.print(text_length);
    // Serial.println(" characters");
    // int num_lines = ceil((float)text_length / (float)TFT_LINE_LENGTH);
    // Serial.print("Printing ");
    // Serial.print(num_lines);
    // Serial.println(" Lines");
    // for (int i=0; i <=num_lines; i++){
    //     display_line(10, (10*i), command_data.substring(i*TFT_LINE_LENGTH, (i+1)*TFT_LINE_LENGTH));
    // }

    // This can be better...
    if (message.length() < 160) {
      tft.setTextSize(3);
    } else if (message.length() < 400) {
      tft.setTextSize(2);
    } else {
      tft.setTextSize(1);
    }
    // tft.setTextSize(3);
    tft.setTextFont(1);
    tft.print(message);
}

// void drawProgress(uint8_t percentage, String text) {
//   // tft.setFreeFont(&ArialRoundedMTBold_14);

//   tft.setTextDatum(BC_DATUM);
//   tft.setTextColor(TFT_ORANGE, TFT_BLACK);
//   tft.setTextPadding(240);
//   tft.drawString(text, 120, 220);

//   ui.drawProgressBar(10, 225, 240 - 20, 15, percentage, TFT_WHITE, TFT_BLUE);

//   tft.setTextPadding(0);
// }

void start_ota(){
    ArduinoOTA.setHostname(mqtt_clientname);
    ArduinoOTA.begin();
    ArduinoOTA.onStart([]() {
        display_on();
        tft.fillScreen(TFT_BLACK);
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        display_line(10,10,"OTA Update Started!", TFT_RED);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if ((progress % 3) == 1) {
            display_line(10, 30, "**********", TFT_GREEN);
        } else if ((progress % 3) == 0) {
            display_line(10, 30, "**********", TFT_BLUE);
        } else {
            display_line(10, 30, "**********", TFT_RED);
        }
    });

    ArduinoOTA.onEnd([]() {
       display_line(10,50, "Finished. Restarting", TFT_BLUE);
       delay(1000);
    });
}

// Main Program
// Setup
void setup() {
  Serial.begin(115200);
  Serial.println("Go for Serial Communications!");
  // Display stuff
  setup_display();
  // Wifi
  setup_wifi();
  // If new config, save it
  if (shouldSaveConfig) {save_config();}
  // Load config from SPIFFS
  load_config();
  // Start OTA
  start_ota();
  //Setup MQTT
  setup_mq();
  // Let the loop do the rest
}

// Loop
void loop() {
    // OTA
    ArduinoOTA.handle();
    // Keep MQTT Going
    if (!client.connected()) {mqconnect();}
    client.loop();
    if (updatedMessage) {parseMessages();}
    if (updatedisplay) {runCommands();} 
}