/*
  Simple WS2812 Artnet Node on NodeMCU v3
  Using WiFiManager for easy WiFi setup and DMX settings
  
  Harm Aldick - 2018
  www.aldick.org

  
  MANUAL SETUP
    * Press "flash" button on startup as soon as the first pixel blinks red
    * Hold button until first pixel flashes green
    * Connect to WiFi using password "Blinkenlights"
    * Setup Wifi and DMX settings
    * After reboot, just wait until node connects
    * Send DMX data via Artnet


  WIREING
    * WS2812 red (Vcc): VU (5V USB), black (GND): G, green (Data-In): via 470 Ohm resistor to D2


  LICENSE

  The MIT License (MIT)

  Copyright (c) 2018  Harm Aldick 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  
  CHANGELOG
  2018-11-06 initial version
 
*/

#include <FS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <WiFiUdp.h>
#include <ArtnetWifi.h>           //https://github.com/rstephan/ArtnetWifi
#include <Adafruit_NeoPixel.h>    //https://github.com/adafruit/Adafruit_NeoPixel

#define AP_SSID "WiFi Artnet LED " // + ESP.getChipId()
#define AP_PASSWORD "Blinkenlights"

#define LED_BRIGHTNESS 127  // half is more than enough
#define LED_PIN D2 // 2 = GPIO2 on ESP01; 4 = D2 on NodeMCU v3
#define LED_NUM 16

#define PIN_BUTTON D3

// default values, if not changed via webinterface
#define ARTNET_UNIVERSE 0   // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0 (as QLC+ does).
#define DMX_START_ADDR 1

uint16_t conf_artnet_universe = ARTNET_UNIVERSE;
uint8_t conf_dmx_start_addr = DMX_START_ADDR;

char char_artnet_universe[6];
char char_dmx_start_addr[4];

ArtnetWifi artnet;
Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

bool should_save_config = false;

void save_config_callback() {
  Serial.println("should save config");
  should_save_config = true;
}


void on_DMX_frame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
  if(universe == conf_artnet_universe) {
    Serial.print("DMX: Univ: ");
    Serial.print(universe, DEC);
    Serial.print(", Seq: ");
    Serial.print(sequence, DEC);
    Serial.print(", Data (");
    Serial.print(length, DEC);
    Serial.print("): ");
    
    /*for (int i = 0; i < length; i++) {
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();*/

    if(length >= (conf_dmx_start_addr + 1)) {
      int r = data[conf_dmx_start_addr - 1];
      int g = data[conf_dmx_start_addr];
      int b = data[conf_dmx_start_addr + 1];

      Serial.print("R: ");
      Serial.print(r);
      Serial.print(" G: ");
      Serial.print(g);
      Serial.print(" B: ");
      Serial.print(b);
      Serial.println();

      for(int i=0; i<LED_NUM; i++) {
        leds.setPixelColor(i, r, g, b);
      }
      leds.show();
    }
  }
}


void save_config_to_fs() {
  Serial.println("saving config");

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  itoa(conf_artnet_universe, char_artnet_universe, 10);
  itoa(conf_dmx_start_addr, char_dmx_start_addr, 10);

  json["artnet_universe"] = char_artnet_universe;
  json["dmx_start_addr"] = char_dmx_start_addr;

  /*json["ip"] = WiFi.localIP().toString();
  json["gateway"] = WiFi.gatewayIP().toString();
  json["subnet"] = WiFi.subnetMask().toString();*/

  File configFile = SPIFFS.open("/config.json", "w");
  if(configFile) {
    Serial.print("writing json to config file: ");
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  } else {
    Serial.println("failed to open config file for writing");
  }
}


void read_config_from_fs() {
  Serial.println("mounting FS...");

  if(SPIFFS.begin()) {
    Serial.println("mounted file system");
    if(SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if(configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if(json.success()) {
          Serial.println("\nparsed json");

          if(json["artnet_universe"]) {
            conf_artnet_universe = atoi(json["artnet_universe"]);
            Serial.print("Artnet Universe: ");
            Serial.println(conf_artnet_universe);
          }
          if(json["dmx_start_addr"]) {
            conf_dmx_start_addr = atoi(json["dmx_start_addr"]);
            Serial.print("DMX Start Addr: ");
            Serial.println(conf_dmx_start_addr);
          }

          /*if(json["ip"]) {
            Serial.println("setting custom ip from config");
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            Serial.println(static_ip);
          } else {
            Serial.println("no custom ip in config");
          }*/
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}


void blink_led(uint8_t n, uint8_t r, uint8_t g, uint8_t b) {
  for(int i=0; i<n; i++) {
    leds.setPixelColor(0, r, g, b);
    leds.show();
    delay(250);
    leds.setPixelColor(0, 0, 0, 0);
    leds.show();
    delay(250);
  }
}


void connect_wifi(bool on_demand) {
  Serial.println("connecting WiFi...");
  WiFiManagerParameter custom_artnet_universe("universe", "Artnet Universe", char_artnet_universe, 5);
  WiFiManagerParameter custom_dmx_start_addr("dmx", "DMX Start Addr.", char_dmx_start_addr, 3);

  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(save_config_callback);
  wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 0, 1), IPAddress(10, 0, 0, 1), IPAddress(255, 255, 255, 0));
  wifiManager.addParameter(&custom_artnet_universe);
  wifiManager.addParameter(&custom_dmx_start_addr);

  String chip_id = String(ESP.getChipId());
  String string_ssid = AP_SSID + chip_id;
  char char_ssid[string_ssid.length() + 1];
  memset(char_ssid, 0, string_ssid.length() + 1);
  for(int i=0; i<string_ssid.length(); i++) {
    char_ssid[i] = string_ssid.charAt(i);
  }

  bool wifi_success = false;
  
  if(on_demand) {
    wifi_success = wifiManager.startConfigPortal(char_ssid, AP_PASSWORD);
  } else {
    wifi_success = wifiManager.autoConnect(char_ssid, AP_PASSWORD);
  }

  if(!wifi_success) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.reset();
      delay(5000);
  }
   
  Serial.println("WiFi connected");

  if(should_save_config) {
    Serial.println("settings changed");
    conf_artnet_universe = atoi(custom_artnet_universe.getValue());
    conf_dmx_start_addr = atoi(custom_dmx_start_addr.getValue());
    save_config_to_fs();
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  leds.begin();
  leds.setBrightness(LED_BRIGHTNESS);

  //clean FS, for testing
  //Serial.println("Erasing SPIFFS");
  //SPIFFS.format();

  read_config_from_fs();

  blink_led(8, 255, 0, 0);

  if(digitalRead(PIN_BUTTON) == HIGH) {
    Serial.println("auto WiFi");
    blink_led(2, 0, 0, 255);
    connect_wifi(false);
  } else {
    Serial.println("setup WiFi");
    blink_led(2, 0, 255, 0);  
    connect_wifi(true);
  }

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Artnet Universe: ");
  Serial.println(conf_artnet_universe);
  Serial.print("DMX Start Address: ");
  Serial.println(conf_dmx_start_addr);

  blink_led(2, 0, 255, 0);

  artnet.setArtDmxCallback(on_DMX_frame);
  artnet.begin();
}

void loop() {
  artnet.read();
}
