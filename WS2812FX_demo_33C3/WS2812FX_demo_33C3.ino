/*
  Simple WS2812FX demo with push button and serial control. 
  Last minute project for #33c3. 
  Works for me.
  
  Harm Aldick - 2016
  www.aldick.org

  
  FEATURES
    * A lot of blinken modes
    * Select mode by pressing the button
    * Select color by holding button for more than 1 second
    * Full control over serial (115200 baud)
    * Tested with Arduino NANO

  WIREING
    * WS2812 red (Vcc): 5V, black (GND): GND, green (Data-In): PIN 12
    * Switch: PIN11 and PIN10 (set to GND)

  NOTES
    * Uses the Adafruit Neopixel library. Get it here: 
      https://github.com/adafruit/Adafruit_NeoPixel
    * Uses the WS2812FX library. Get it here: 
      https://github.com/kitesurfer1404/WS2812FX

  LICENSE

  The MIT License (MIT)

  Copyright (c) 2016  Harm Aldick 

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
  2016-12-15 initial version
  2016-12-19 fixed lib, included fireworks FX again
  
*/

#include <WS2812FX.h>

#define LED_PIN 12
#define LED_COUNT 5
#define BUTTON_PIN 11
#define BUTTON_GND 10             // this pin will be set to GND, because we don't have enough GNDs on the Nano.

#define DEFAULT_COLOR_INDEX 240   // default index of color wheel. 0-255

#define DEFAULT_BRIGHTNESS 255
#define DEFAULT_SPEED 200
#define DEFAULT_MODE FX_MODE_SINGLE_DYNAMIC

#define BUTTON_DEBOUNCE_TIME 75       // debounce time in ms
#define BUTTON_LONG_PRESS_DELAY 1000  // cycling through colors after pressing the button for this time (ms)

uint8_t color_wheel_index = DEFAULT_COLOR_INDEX;

uint8_t button_state = HIGH;
uint8_t button_previous_state = HIGH;
unsigned long button_last_readtime = 0;
unsigned long button_last_hightime = 0;

String cmd = "";               // String to store incoming serial commands
boolean cmd_complete = false;  // whether the command string is complete


WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

void setup(){
  Serial.begin(115200);
  Serial.println(F("Starting..."));

  Serial.println(F("WS2812FX setup"));
  ws2812fx.init();
  ws2812fx.setMode(DEFAULT_MODE);
  ws2812fx.setColor(color_wheel(color_wheel_index));
  ws2812fx.setSpeed(DEFAULT_SPEED);
  ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  ws2812fx.start();

  Serial.println(F("Button setup"));
  pinMode(BUTTON_PIN, INPUT_PULLUP); // change INPUT_PULLUP to INPUT when there is an external pullup-resistor attached
  pinMode(BUTTON_GND, OUTPUT);
  digitalWrite(BUTTON_GND, LOW);
  
  Serial.println(F("ready!"));

  printModes();
  printUsage();
}


void loop() {
  unsigned long now = millis();

  ws2812fx.service();

  if(now - button_last_readtime > BUTTON_DEBOUNCE_TIME) {
    button_state = digitalRead(BUTTON_PIN);

    if(button_state == HIGH && button_previous_state == LOW && now - button_last_hightime < BUTTON_LONG_PRESS_DELAY) {
      ws2812fx.setMode((ws2812fx.getMode() + 1) % ws2812fx.getModeCount());
    }
  
    if(button_state == LOW && button_previous_state == LOW && now - button_last_hightime > BUTTON_LONG_PRESS_DELAY) {
      color_wheel_index += 2;
      ws2812fx.setColor(color_wheel(color_wheel_index));
    }

    if(button_state == HIGH) {
      button_last_hightime = now;
    }
    
    button_last_readtime = now;
    button_previous_state = button_state;
  }


  // On Atmega32U4 based boards (leonardo, micro) serialEvent is not called
  // automatically when data arrive on the serial RX. We need to do it ourself
  #if defined(__AVR_ATmega32U4__)
  serialEvent();
  #endif

  if(cmd_complete) {
    process_command();
  }
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t color_wheel(uint8_t pos) {
  pos = 255 - pos;
  if(pos < 85) {
    return ws2812fx.Color(255 - pos * 3, 0, pos * 3);
  }
  if(pos < 170) {
    pos -= 85;
    return ws2812fx.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return ws2812fx.Color(pos * 3, 255 - pos * 3, 0);
}



/*
 * Checks received command and calls corresponding functions.
 */
void process_command() {
  if(cmd == F("b+")) { 
    ws2812fx.increaseBrightness(25);
    Serial.print(F("Increased brightness by 25 to: "));
    Serial.println(ws2812fx.getBrightness());
  }

  if(cmd == F("b-")) {
    ws2812fx.decreaseBrightness(25); 
    Serial.print(F("Decreased brightness by 25 to: "));
    Serial.println(ws2812fx.getBrightness());
  }

  if(cmd.startsWith(F("b "))) { 
    uint8_t b = (uint8_t) cmd.substring(2, cmd.length()).toInt();
    ws2812fx.setBrightness(b);
    Serial.print(F("Set brightness to: "));
    Serial.println(ws2812fx.getBrightness());
  }

  if(cmd == F("s+")) { 
    ws2812fx.increaseSpeed(10);
    Serial.print(F("Increased speed by 10 to: "));
    Serial.println(ws2812fx.getSpeed());
  }

  if(cmd == F("s-")) {
    ws2812fx.decreaseSpeed(10); 
    Serial.print(F("Decreased speed by 10 to: "));
    Serial.println(ws2812fx.getSpeed());
  }

  if(cmd.startsWith(F("s "))) {
    uint8_t s = (uint8_t) cmd.substring(2, cmd.length()).toInt();
    ws2812fx.setSpeed(s); 
    Serial.print(F("Set speed to: "));
    Serial.println(ws2812fx.getSpeed());
  }

  if(cmd.startsWith(F("m "))) { 
    uint8_t m = (uint8_t) cmd.substring(2, cmd.length()).toInt();
    ws2812fx.setMode(m);
    Serial.print(F("Set mode to: "));
    Serial.print(ws2812fx.getMode());
    Serial.print(" - ");
    Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
  }

  if(cmd.startsWith(F("c "))) { 
    uint32_t c = (uint32_t) strtol(&cmd.substring(2, cmd.length())[0], NULL, 16);
    ws2812fx.setColor(c); 
    Serial.print(F("Set color to: "));
    Serial.println(ws2812fx.getColor(), HEX);
  }

  cmd = "";              // reset the commandstring
  cmd_complete = false;  // reset command complete
}

/*
 * Prints a usage menu.
 */
void printUsage() {
  Serial.println(F("Usage:"));
  Serial.println();
  Serial.println(F("m <n> \t : select mode <n>"));
  Serial.println();
  Serial.println(F("b+ \t : increase brightness"));
  Serial.println(F("b- \t : decrease brightness"));
  Serial.println(F("b <n> \t : set brightness to <n>"));
  Serial.println();
  Serial.println(F("s+ \t : increase speed"));
  Serial.println(F("s- \t : decrease speed"));
  Serial.println(F("s <n> \t : set speed to <n>"));
  Serial.println();
  Serial.println(F("c 0x007BFF \t : set color to 0x007BFF"));
  Serial.println();
  Serial.println();
  Serial.println(F("Have a nice day."));
  Serial.println(F("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"));
  Serial.println();
}


/*
 * Prints all available WS2812FX blinken modes.
 */
void printModes() {
  Serial.println(F("Supporting the following modes: "));
  Serial.println();
  for(int i=0; i < ws2812fx.getModeCount(); i++) {
    Serial.print(i);
    Serial.print(F("\t"));
    Serial.println(ws2812fx.getModeName(i));
  }
  Serial.println();
}


/*
 * Reads new input from serial to cmd string. Command is completed on \n
 */
void serialEvent() {
  while(Serial.available()) {
    char inChar = (char) Serial.read();
    if(inChar == '\n') {
      cmd_complete = true;
    } else {
      cmd += inChar;
    }
  }
}
