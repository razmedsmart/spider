#include <FastLED.h>
#include <arduinoFFT.h>
#include <FastLED.h>
#include <ArduinoOTA.h>
#ifdef ESP32
#include <FS.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>
#endif
#include <ESPAsyncWebSrv.h>
#include <SPIFFSEditor.h>
#include <arduinoFFT.h>
#include "leds.h"
#define debug
#define FASTLED_USING_NAMESPACE
#define FASTLED_ESP8266_D1_PIN_ORDER
#define Power_Per_LED 60
#define TOTAL_NUM_LEDS 1200
#define MAX_POWER_MILLIAMPS = Power_Per_LED * TOTAL_NUM_LEDS

#define BRIGHTNESS          250
#define NUM_LEDS 180
#define NUM_LEDS_BELLY 50 
#define NUM_LEDS_TAIL 50 
#define NUM_LEDS_CLOUD 50 
#define LED_PIN 2 //D3   
#define LED_PIN_BELLY 4 //D5   
#define LED_PIN_TAIL 22   //D4
#define LED_PIN_CLOUD  23   //D6
#define BRIGHTNESS 250
#define SPEED 10
#define MAX_POWER_MILLIAMPS 500
int random_led = 0;
CRGB leds[NUM_LEDS];
CRGB leds_belly[NUM_LEDS_BELLY];
CRGB leds_tail[NUM_LEDS_TAIL];
CRGB leds_cloud[NUM_LEDS_CLOUD];
CRGB ledsx[NUM_LEDS];

// FFPT part
#define SAMPLES 128            
#define SAMPLING_FREQUENCY 1000 //Hz, must be less than 10000 due to ADC
#define xres 16 //32     // number of bands = xres/2
#define yres 14
int MY_ARRAY[]={0, 128, 192, 224, 240, 248, 252, 254, 255,8,16,32,64,80,90,100,110};
//int MY_ARRAY[]={0, 128, 192, 224, 240, 248, 252, 254, 255,0,0,0,0,0,0,0,0};
int MY_MODE_1[]={0, 128, 192, 224, 240, 248, 252, 254, 255};
int MY_MODE_2[]={0, 128, 64, 32, 16, 8, 4, 2, 1};
int MY_MODE_3[]={0, 128, 192, 160, 144, 136, 132, 130, 129};
int MY_MODE_4[]={0, 128, 192, 160, 208, 232, 244, 250, 253};
int MY_MODE_5[]={0, 1, 3, 7, 15, 31, 63, 127, 255};
//const int SAMPLING_FREQUENCY_MSEC= 5;
const int SAMPLE_RATE = 44100;  // 44.1 kHz sampling rate
unsigned long previousMillis = 0;
unsigned long interval = 1000 / SAMPLE_RATE;  // Sampling interval in milliseconds
unsigned int sampling_period_us;
unsigned long microseconds;
//

// Variables to store the options
int opt1 = 0;
int opt2 = 0;
int opt3 = 3;
int opt4 = 0;
int opt5 = 0;
bool pota = false;
bool just_dance = true;

//double vReal[SAMPLES];
//double vImag[SAMPLES];
double vReal[512];
double vImag[512];
char data_avgs[xres];
int yvalue;
int displaycolumn , displayvalue;
int peaks[xres];
arduinoFFT FFT = arduinoFFT();
enum State { TEST, SHOW_UP,  SHOW_DOWN, WAVE, MUSIC, CONFFETI, PRIDE };
// the order of the transition
State showOrder[] = { SHOW_UP,  SHOW_DOWN};

State currentState = TEST;
unsigned long stateChangeTime = 0;
const char* apSSID = "spider";  // Change this to the SSID you want for your AP
const char* apPassword = "1111111111";  // Change this to the password for your AP
AsyncWebServer server(80);
unsigned char newBrightness = 255;
unsigned char currentBrightness = BRIGHTNESS;
unsigned int led_pos = 0;
TaskHandle_t Core0Task;
TaskHandle_t Core1Task;
CRGBArray<NUM_LEDS> ledsy;                              // LED array containing all LEDs
CRGBSet RIGHT (ledsy (0,            NUM_LEDS/2-1)   );  // < subset containing only left  LEDs
CRGBSet R1    (ledsy (0,            NUM_LEDS/4-1)   );  // < subset containing only left  side of left  LEDs
CRGBSet R2    (ledsy (NUM_LEDS/4,   NUM_LEDS/2-1)   );  // < subset containing only right side of left  LEDs
CRGBSet LEFT  (ledsy (NUM_LEDS/2,   NUM_LEDS)       );  // < subset containing only right LEDs
CRGBSet L1    (ledsy (NUM_LEDS/2,   3*NUM_LEDS/4-1) );  // < subset containing only left  side of right LEDs
CRGBSet L2    (ledsy (3*NUM_LEDS/4, NUM_LEDS)       );  // < subset containing only right side of right LEDs

CRGBPalette16 currentPalette, targetPalette;
CRGBPalette16 randomPalette1, randomPalette2;
TBlendType    currentBlending;
uint8_t maxChanges = 24;        // Value for blending between palettes.

uint8_t  _setBrightness = BRIGHTNESS;
CRGB manualColor = 0x000000;
CRGB manualColor_L = 0x000000;
CRGB manualColor_R = 0x000000;
CHSV manualHSV (0, 255, 255);
uint8_t gHue = 0, gHue1 = 0, gHue2 = 0; // rotating "base color" used by many of the patterns
void set_max_value_yres(int val);
int get_max_value_yres();
void set_scale_vreal(int val);
int get_scale_vreal();
uint8_t get_gCurrentPatternNumber();
void set_gCurrentPatternNumber(uint8_t val);
void run_just_dance();
void music(CRGB *_leds, CRGB baseColor, int num_leds);

//SimplePatternList audioPatterns = { audio_spectrum, audioLight };


// FFT
// Define the core numbers for each task
const int coreLoop = 0; // Core 0 for loop()
const int coreFFT = 1;  // Core 1 for run_fft()

CRGB* get_jleds_0()
{
  return leds;
}

int get_jleds_0_num()
{
  return sizeof(leds)/3;
}


void codeForCore0Task(void *pvParameters) {
  Serial.print("Task fft ");
  Serial.println(xPortGetCoreID());
  for (;;)
  {
    run_fft(); 
    //fftLoop();
    //Serial.println();       
    //delayMicroseconds(2);
    delay(1);
  }
}

void codeForCore1Task(void *pvParameters) {
  for (;;)
  {
    if (just_dance)
    {
      run_just_dance();
      continue;
    }
    //Serial.print("Task 1 loop on core ");
    //Serial.println(xPortGetCoreID());
    //delay(100);
    switch (currentState) {
      case TEST:
        EVERY_N_MILLISECONDS(30){  
          FastLED.show();
          led_test();
        };
        if (millis() - stateChangeTime >= 5000) {
          stateChangeTime = millis();
          //fadeOutLeds(2000);
          changeState(SHOW_UP);
        }
        break;

      case SHOW_UP:
        FastLED.clear();
        show_grow(leds, sizeof(leds)/3, true, 5,255, 10);
        show_grow(leds_belly, sizeof(leds_belly)/3, true, 170,255 ,10);
        show_grow(leds_tail, sizeof(leds_tail)/3, true, 85,255, 10);
        show_grow(leds_cloud, sizeof(leds_cloud)/3, true, 42, 255, 10);
        //fadeOutLeds(2000);
        FastLED.show();
        //changeState(SHOW_DOWN);        
        changeState(CONFFETI);        
        break;
      case SHOW_DOWN:
        show_grow(leds_cloud, sizeof(leds_cloud)/3, false, 42, 0, 10);
        show_grow(leds_tail, sizeof(leds_tail)/3, false, 85,0, 10);
        show_grow(leds_belly, sizeof(leds_belly)/3, false, 170,0, 10);
        show_grow(leds, sizeof(leds)/3, false, 5,0, 10);
        changeState(SHOW_UP);
        break;
      case WAVE:
        //wave(leds, sizeof(leds)/3, false, 5,0, 10);
        wave2(leds, sizeof(leds)/3);
        break;
      case CONFFETI:        
        // random colored speckles that blink in and fade smoothly
        fadeToBlackBy( leds, sizeof(leds)/3, 10);
        led_pos = random16(sizeof(leds)/3);
        leds[led_pos] += CHSV( 80 + random8(64), 200, 255);
                FastLED.show();

        if (millis() - stateChangeTime >= 10000) {
            //FastLED.clear();
            fadeOutLeds(2000);
            FastLED.show();
            //changeState(CONFFETI);
            changeState(PRIDE);
          }
      break;
      case PRIDE:
        pride(leds, sizeof(leds)/3);
        if (millis() - stateChangeTime >= 10000) {
            //FastLED.clear();
            FastLED.show();
            //changeState(PRIDE);
            changeState(SHOW_DOWN);
          }
      break;
      case MUSIC:       
        music(leds, CRGB::Red, sizeof(leds)/3);
        break;
    }
  }
}
void printStateEnum(State state) {
  switch (state) {
    case TEST:
      Serial.print("TEST");
      break;
    case SHOW_UP:
      Serial.print("SHOW_UP");
      break;
    case SHOW_DOWN:
      Serial.print("SHOW_DOWN");
      break;
    case WAVE:
      Serial.print("WAVE");
      break;
    case MUSIC:
      Serial.print("MUSIC");
      break;
    case CONFFETI:
      Serial.print("CONFFETI");
      break;
    case PRIDE:
      Serial.print("PRIDE");
      break;
    default:
      Serial.print("Unknown State");
      break;
  }
}


State getNextState() {
  // Define the enum values in an array
  static auto currentShow = 0;

  int numStates = sizeof(showOrder) / sizeof(showOrder[0]);
  Serial.print("num state");Serial.println(numStates);
  Serial.print("curr show");Serial.println(currentShow);
  currentShow = (currentShow + 1) % numStates;
  printStateEnum(showOrder[currentShow]);
  return showOrder[currentShow];
}
void setup_tasks() {
   // Set up Core 0 task handler
  xTaskCreatePinnedToCore(
    codeForCore0Task,
    "Core 0 task",
    10000,
    NULL,
    1,
    &Core0Task,
    0);


  // Set up Core 1 task handler
  xTaskCreatePinnedToCore(
    codeForCore1Task,
    "Core 1 task",
    10000,
    NULL,
    1,
    &Core1Task,
    1);
}

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define LEDX_PINS    13

void ledTestSetup(){
#ifdef debug
    Serial.println("\tStarting ledSetup");
#endif
    FastLED.addLeds< LED_TYPE, LEDX_PINS, COLOR_ORDER >( ledsx, NUM_LEDS ).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(255);
    FastLED.setDither(0);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);    
    //setupNoise();
    fill_solid (ledsx, NUM_LEDS, CRGB::Green);
#ifdef debug
    Serial.println("\tEnding ledSetup");
#endif
}

void setup() {
  fftSetup();
  Serial.begin(9600);   // Initialize serial communication at 9600 baud rate
  Serial.println("Starting");
  sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY));
  pinMode(LED_PIN, OUTPUT);  // Set the button as an input with an internal pull-up resistor
  pinMode(LED_PIN_BELLY, OUTPUT);  // Set the button as an input with an internal pull-up resistor
  pinMode(LED_PIN_TAIL, OUTPUT);  // Set the button as an input with an internal pull-up resistor
  pinMode(LED_PIN_CLOUD, OUTPUT);  // Set the button as an input with an internal pull-up resistor

  //pinMode(buttonPin, INPUT_PULLUP);  // Set the button as an input with an internal pull-up resistor
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.addLeds<WS2811, LED_PIN_BELLY, GRB>(leds_belly, NUM_LEDS_BELLY);
  FastLED.addLeds<WS2811, LED_PIN_TAIL, GRB>(leds_tail, NUM_LEDS_TAIL);
  FastLED.addLeds<WS2811, LED_PIN_CLOUD, GRB>(leds_cloud, NUM_LEDS_CLOUD);
  FastLED.setMaxRefreshRate(10000000,false);
  FastLED.clear();
  //FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,MAX_POWER_MILLIAMPS);
  FastLED.show(0);
  WiFi.softAP(apSSID, apPassword);
  IPAddress apIP = WiFi.softAPIP();
  Serial.println("AP IP address: " + apIP.toString());
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();
  setup_tasks();
  ledTestSetup();
}

void led_test(){
  for (int i = 0; i < NUM_LEDS; i++) 
      leds[i] = CHSV(0+i*5, 255, 255); // Set hue to 0 (red), saturation to 255 (max), and value to 255 (max)
  apply_to_all_led_from_leds();      
  /*for (int i = 0; i < NUM_LEDS_BELLY; i++) 
      leds_belly[i] = CHSV(0+i*5, 255, 255); // Set hue to 0 (red), saturation to 255 (max), and value to 255 (max)
  for (int i = 0; i < NUM_LEDS_TAIL; i++) 
      leds_tail[i] = CHSV(0+i*5, 255, 255); // Set hue to 0 (red), saturation to 255 (max), and value to 255 (max)
  for (int i = 0; i < NUM_LEDS_CLOUD; i++) 
      leds_cloud[i] = CHSV(0+i*5, 255, 255); // Set hue to 0 (red), saturation to 255 (max), and value to 255 (max)
*/
}

void apply_to_all_led_from_leds(){
  for (int i = 0; i < NUM_LEDS_BELLY; i++) 
      leds_belly[i] = leds[map(i,0,NUM_LEDS_BELLY-1,0, sizeof(leds)/3)];
  for (int i = 0; i < NUM_LEDS_TAIL; i++) 
      leds_tail[i] = leds[map(i,0,NUM_LEDS_TAIL-1,0, sizeof(leds)/3)];
  for (int i = 0; i < NUM_LEDS_CLOUD; i++) 
      leds_cloud[i] = leds[map(i,0,NUM_LEDS_CLOUD-1,0, sizeof(leds)/3)];
}
void changeState(State newState) {
  Serial.print("Changing state  "); printStateEnum(currentState); Serial.print("->"); printStateEnum(newState);Serial.println();
  currentState = newState;
  stateChangeTime = millis();
}
void handleRoot(AsyncWebServerRequest *request) {
  // Send the HTML form to set the options
  String html = "<html><body>";
  html += "<h1>Set Options</h1>";
  html += "<form action='/update' method='POST'>";
  html += "Option 1: <input type='number' name='opt1' min='0' max='255' value='" + String(opt1) + "'><br>";
  html += "Option 2: <input type='number' name='opt2' min='0' max='255' value='" + String(opt2) + "'><br>";
  html += "Option 3 max_value_yres 80: <input type='number' name='max_value_yres' min='1' max='255' value='" + String(get_max_value_yres()) + "'><br>";
  html += "Option 4 scale_vreal 8 : <input type='number' name='scale_vreal' min='1' max='255' value='" + String(get_scale_vreal()) + "'><br>";
  html += "Option 5: <input type='number' name='opt5' min='0' max='255' value='" + String(opt5) + "'><br>";
  html += "POTA: <input type='checkbox' name='pota' " + String(pota ? "checked" : "") + "><br>";
  html += "just_dance: <input type='checkbox' name='just_dance' " + String(just_dance ? "checked" : "") + "><br>";
  html += "<input type='submit' value='Submit'>";
  html += "</form>";
  html += "</body></html>";

  request->send(200, "text/html", html);
}
void handleUpdate(AsyncWebServerRequest *request) {
  // Extract the POST data
  if (request->args()) {
    for (size_t i = 0; i < request->args(); i++) {
      String paramName = request->argName(i);
      String paramValue = request->arg(i);
      
      if (paramName == "opt1") {
        opt1 = paramValue.toInt();
        set_gCurrentPatternNumber( opt1%16);
        switch(opt1){
          case 0 : changeState(TEST); break;
          case 1 : changeState(SHOW_UP); break;
          case 2 : changeState(CONFFETI); break;
          case 3 : changeState(PRIDE); break;
          case 4 : changeState(WAVE); break;
        }
      } else if (paramName == "opt2") {
        opt2 = paramValue.toInt();
      } else if (paramName == "max_value_yres") {
        set_max_value_yres( paramValue.toInt());
      } else if (paramName == "scale_vreal") {
        set_scale_vreal( paramValue.toInt() );
      } else if (paramName == "opt5") {
        opt5 = paramValue.toInt();
      } else if (paramName == "pota") {
        pota = (paramValue == "on");
      } else if (paramName == "just_dance") {
        just_dance = (paramValue == "on");
      }
    }
  }

  // Redirect back to the root page after setting the options
  request->redirect("/");
}

void loop() {
  Serial.print("Main loop on core ");
  delay(10000);
}

void fadeOutLeds(int durationMillis) {
  int fadeSteps = 256; // Number of steps for fading (0-255)
  int fadeDelay = durationMillis / fadeSteps;

  for (int i = 255; i >= 0; i--) {
    FastLED.setBrightness(i);
    FastLED.show();
    delay(fadeDelay);
  }

  // Turn off all LEDs
  FastLED.clear();
   FastLED.setBrightness(255);
  FastLED.show();
}

 


CHSV getLEDColor(int displayValue) {

    int hue = map(displayValue, 0, 255, 50, 255); // Maps displayValue from a blue to red hue.
    return CHSV(hue, 255, 255);  // Full saturation and full value for maximum brightness.
}
CRGB getLEDColorRGB(int displayValue) {
    if (displayValue > 200) return CRGB::Red;
    if (displayValue > 100) return CRGB::Yellow;
    return CRGB::Green; 
}


void setColumn(CRGB *_leds, int _displaycolumn, int _displayvalue, int num_leds) {
   int position = _displaycolumn *  yres;
   //Serial.print("setColumn:");Serial.print(_displaycolumn); Serial.print(","); Serial.print(_displayvalue); Serial.println(";");
   CRGB color = CRGB::Black;
   int color_index = 0 ;
   for(int i = 0; i < _displayvalue; i++){
      //color = getLEDColorRGB(MY_ARRAY[i]);
      color = getLEDColor(MY_ARRAY[i]);
      if( i < num_leds){
        _leds[position + i] = color;
      }else{
        Serial.print(i);Serial.println("error;");
      }
   }
   for(int i = _displayvalue; i< yres; i++){
      //if (i < _displayvalue ){
      color = CRGB::Black;
      if( i < num_leds){
        _leds[i+position] = color;
      }else{
        Serial.print(i);Serial.println("error;");
      }
   }
}


void rainbow() {
    // FastLED's built-in rainbow generator
    gHue1++;
    gHue2++;
    fill_rainbow( ledsx, NUM_LEDS/2, gHue1);
    //fill_rainbow( LEFT , NUM_LEDS/2, gHue2);
} // rainbow

void rainbowWithGlitter() {
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter();
} // rainbow with glitter

void rainbow_scaling(){
    for(int i = 0; i <= NUM_LEDS/4; i++){
        R1[i] = CHSV((millis()/77*i+1)%255 + gHue1, 255, 255);
        R2[NUM_LEDS/4-i] = R1[i];
        L1[i] = CHSV((millis()/73*i+1)%255 - gHue2, 255, 255);
        L2[NUM_LEDS/4-i] = L1[i];
    }
} // rainbow scaling

void addGlitter() {
    EVERY_N_MILLISECONDS(1000/30){
        if( random8() < 80) {
            leds[ random16(NUM_LEDS) ] += CRGB::White;
        }
    }
}

void confetti() 
{    // random colored speckles that blink in and fade smoothly
    EVERY_N_MILLISECONDS(1000/30){
        fadeToBlackBy( leds, NUM_LEDS, 30);
        int pos = random16(NUM_LEDS/2);
        // leds[pos] += CHSV( random8(255), 255, 255);
        RIGHT[pos] += CHSV( gHue1 + random8(64), 190+random8(65), 255);
        LEFT [pos] += CHSV( gHue2 + random8(64), 190+random8(65), 255);
    }
}

void sinelon()
{
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( leds, NUM_LEDS, 5);
    int pos1 = beatsin16(11, 0, NUM_LEDS/2-1);
    int pos2 = beatsin16(13, 0, NUM_LEDS/2-1);
    int pos3 = beatsin16( 9, 0, NUM_LEDS/2-1);
    int pos4 = beatsin16(15, 0, NUM_LEDS/2-1);
    LEFT [pos1] = ColorFromPalette(randomPalette1, pos1, 255, LINEARBLEND);   // Use that value for both the location as well as the palette index colour for the pixel.
    RIGHT[pos2] = ColorFromPalette(randomPalette2, pos2, 255, LINEARBLEND);   // Use that value for both the location as well as the palette index colour for the pixel.
    LEFT [pos3] += CHSV( gHue2, 255, 255);
    RIGHT[pos4] += CHSV( gHue1, 255, 255);
}

void dot_beat() {
    uint8_t fadeval = 10;       // Trail behind the LED's. Lower => faster fade.
    // nscale8(leds, NUM_LEDS, fadeval);    // Fade the entire array. Or for just a few LED's, use  nscale8(&leds[2], 5, fadeval);
    fadeToBlackBy( leds, NUM_LEDS, fadeval);

    uint8_t BPM, inner, outer, middle;
    
    BPM = 33;

    inner  = beatsin8(BPM, NUM_LEDS/2/4, NUM_LEDS/2/4*3);    // Move 1/4 to 3/4
    outer  = beatsin8(BPM, 0, NUM_LEDS/2-1);               // Move entire length
    middle = beatsin8(BPM, NUM_LEDS/2/3, NUM_LEDS/2/3*2);   // Move 1/3 to 2/3

    LEFT[outer]  = CHSV( gHue1    , 200, 255);
    LEFT[middle] = CHSV( gHue1+96 , 200, 255);
    LEFT[inner]  = CHSV( gHue1+160, 200, 255);

    BPM = 31;
    
    inner  = beatsin8(BPM, NUM_LEDS/2/4, NUM_LEDS/2/4*3);    // Move 1/4 to 3/4
    outer  = beatsin8(BPM, 0, NUM_LEDS/2-1);               // Move entire length
    middle = beatsin8(BPM, NUM_LEDS/2/3, NUM_LEDS/2/3*2);   // Move 1/3 to 2/3

    RIGHT[outer]  = CHSV( gHue2    , 200, 255);
    RIGHT[middle] = CHSV( gHue2+96 , 200, 255);
    RIGHT[inner]  = CHSV( gHue2+160, 200, 255);

} // dot_beat()

void juggle() {
    // colored dots, weaving in and out of sync with each other
    fadeToBlackBy( leds, NUM_LEDS, 5);
    byte dothue1 = 0, dothue2 = 0;
    for( int i = 0; i < 6; i++) {
        RIGHT[beatsin16(i+7,0,NUM_LEDS/2-1)] |= CHSV(dothue1, 200, 255);
        LEFT [beatsin16(i+5,0,NUM_LEDS/2-1)] |= CHSV(dothue2, 200, 255);
        dothue1 += 32;
        dothue2 -= 32;
        yield();
    }
}

void bpm()
{
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    // CRGBPalette16 palette = PartyColors_p;
    CRGBPalette16 palette = RainbowColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < NUM_LEDS/2; i++) { //9948
        RIGHT[i]              = ColorFromPalette(palette, gHue1+(i*2), beat-gHue1+(i*10));
        LEFT [NUM_LEDS/2-1-i] = ColorFromPalette(palette, gHue2+(i*2), beat-gHue2+(i*10));
        yield();
    }
}

void blendwave() {
    CRGB clr1, clr2;
    uint8_t speed, loc1;

    speed = beatsin8(6,0,255);

    clr1 = blend(CHSV(beatsin8(3,0,255),255,255), CHSV(beatsin8(4,0,255),255,255), speed);
    clr2 = blend(CHSV(beatsin8(4,0,255),255,255), CHSV(beatsin8(3,0,255),255,255), speed);
    loc1 = beatsin8(13,0,NUM_LEDS/2-1);

    fill_gradient_RGB(LEFT, 0, clr2, loc1, clr1);
    fill_gradient_RGB(LEFT, loc1, clr2, NUM_LEDS/2-1, clr1);
    
    speed = beatsin8(7,0,255);

    clr1 = blend(CHSV(beatsin8(4,0,255),255,255), CHSV(beatsin8(5,0,255),255,255), speed);
    clr2 = blend(CHSV(beatsin8(5,0,255),255,255), CHSV(beatsin8(4,0,255),255,255), speed);
    loc1 = beatsin8(11,0,NUM_LEDS/2-1);

    fill_gradient_RGB(RIGHT, 0, clr2, loc1, clr1);
    fill_gradient_RGB(RIGHT, loc1, clr2, NUM_LEDS/2-1, clr1);
} // blendwave()

uint8_t _xhue[NUM_LEDS/2], _yhue[NUM_LEDS/2]; // x/y coordinates for noise function
uint8_t _xsat[NUM_LEDS/2], _ysat[NUM_LEDS/2]; // x/y coordinates for noise function
void setupNoise(){
    for (uint16_t i = 0; i < NUM_LEDS/2; i++) {       // precalculate the lookup-tables:
        uint8_t angle = (i * 256) / NUM_LEDS/2;         // on which position on the circle is the led?
        _xhue[i] = cos8( angle );                         // corrsponding x position in the matrix
        _yhue[i] = sin8( angle );                         // corrsponding y position in the matrix
        _xsat[i] = _yhue[i];                         // corrsponding x position in the matrix
        _ysat[i] = _xhue[i];                         // corrsponding y position in the matrix
    }
}

int scale = 1000;                               // the "zoom factor" for the noise
void noise1() {
    uint8_t _noise, _hue, _sat, _val;
    uint16_t shift_x, shift_y;
    uint32_t real_x, real_y;

    for (uint16_t i = 0; i < NUM_LEDS/2; i++) {

        shift_x = beatsin8(3);                  // the x position of the noise field swings @ 17 bpm
        shift_y = millis() / 100;                // the y position becomes slowly incremented

        real_x = (_xhue[i] + shift_x) * scale;       // calculate the coordinates within the noise field
        real_y = (_yhue[i] + shift_y) * scale;       // based on the precalculated positions

        _noise = inoise16(real_x, real_y, 4223) >> 8;           // get the noise data and scale it down
        
        _hue = _noise * 3;                        // map led color based on noise data
        _sat = 255;
        _val = _noise;

        LEFT[i] = CHSV( _hue, _sat, _val);
        
        shift_x = beatsin8(4);                  // the x position of the noise field swings @ 17 bpm
        shift_y = millis() / 100;                // the y position becomes slowly incremented

        real_x = (_xhue[i] + shift_x) * scale;       // calculate the coordinates within the noise field
        real_y = (_yhue[i] + shift_y) * scale;       // based on the precalculated positions

        _noise = inoise16(real_x, real_y, 4223) >> 8;           // get the noise data and scale it down
        
        _hue = _noise * 3;                        // map led color based on noise data
        _sat = 255;
        _val = _noise;

        RIGHT[i] = CHSV( _hue, _sat, _val);
    }
}

// just moving along one axis = "lavalamp effect"
void noise2() {

    uint8_t _noise, _hue, _sat, _val;
    uint16_t shift_x, shift_y;
    uint32_t real_x, real_y;

    for (uint16_t i = 0; i < NUM_LEDS/2; i++) {

        shift_x = millis() / 47;                 // x as a function of time
        shift_y = 0;

        real_x = (_xhue[i] + shift_x) * scale;       // calculate the coordinates within the noise field
        real_y = (_yhue[i] + shift_y) * scale;       // based on the precalculated positions

        _noise = inoise16(real_x, real_y, 4223) >> 8;           // get the noise data and scale it down
        
        _hue = _noise * 3;                        // map led color based on noise data
        _sat = 255;
        _val = _noise;

        LEFT[i] = CHSV( _hue, _sat, _val);

        shift_x = millis() / 51;                 // x as a function of time
        shift_y = 0;

        real_x = (_xhue[i] + shift_x) * scale;       // calculate the coordinates within the noise field
        real_y = (_yhue[i] + shift_y) * scale;       // based on the precalculated positions

        _noise = inoise16(real_x, real_y, 4223) >> 8;           // get the noise data and scale it down

        _hue = _noise * 3;                        // map led color based on noise data
        _sat = 255;
        _val = _noise;

        RIGHT[i] = CHSV( _hue, _sat, _val);
    }
}

// no x/y shifting but scrolling along z
void noise3() {

    uint8_t _noise, _hue, _sat, _val;
    uint16_t shift_x, shift_y;
    uint32_t real_x, real_y, real_z;

    for (uint16_t i = 0; i < NUM_LEDS/2; i++) {

        shift_x = 0;                             // no movement along x and y
        shift_y = 0;

        real_x = (_xhue[i] + shift_x) * scale;       // calculate the coordinates within the noise field
        real_y = (_yhue[i] + shift_y) * scale;       // based on the precalculated positions

        real_z = millis() * 19;                  // increment z linear

        _noise = inoise16(real_x, real_y, real_z) >> 8;           // get the noise data and scale it down

        _hue = _noise * 3;                        // map led color based on noise data
        _sat = 255;
        _val = _noise;

        LEFT[i] = CHSV( _hue, _sat, _val);

        shift_x = 0;                             // no movement along x and y
        shift_y = 0;

        real_x = (_xhue[i] + shift_x) * scale;       // calculate the coordinates within the noise field
        real_y = (_yhue[i] + shift_y) * scale;       // based on the precalculated positions

        real_z = millis() * 23;                  // increment z linear

        _noise = inoise16(real_x, real_y, real_z) >> 8;           // get the noise data and scale it down

        _hue = _noise * 3;                        // map led color based on noise data
        _sat = 255;
        _val = _noise;

        RIGHT[i] = CHSV( _hue, _sat, _val);
    }
}

uint8_t fadeval = 235, frameRate = 45;
void fire(){ // my own simpler 'fire' code - randomly generate fire and move it up the strip while fading
    EVERY_N_MILLISECONDS(1000/frameRate){
        for(int i = 0; i < NUM_LEDS/4; i++){
            R1[i] = R1[i+1].nscale8(fadeval); if(R1[i].g > 0) R1[i].g--;
            R2[NUM_LEDS/4-i] = R1[i];
            L1[i] = R1[i+1].nscale8(fadeval); if(L1[i].g > 0) L1[i].g--;
            L2[NUM_LEDS/4-i] = L1[i];
        }
        uint8_t _hue = 0, _sat = 255, _val = 0;
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        R1[NUM_LEDS/4-1] = CHSV( _hue, _sat, _val*_val/255);
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        R2[0] = CHSV( _hue, _sat, _val*_val/255);
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        L1[NUM_LEDS/4-1] = CHSV( _hue, _sat, _val*_val/255);
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        L2[0] = CHSV( _hue, _sat, _val*_val/255);
    }
}

void fireSparks(){ // randomly generate color and move it up the strip while fading, plus some yellow 'sparkles'
    EVERY_N_MILLISECONDS(1000/frameRate){
        for(int i = 0; i < NUM_LEDS/4; i++){
            R1[i] = R1[i+1].nscale8(fadeval); if(R1[i].g > 0) R1[i].g--;
            R2[NUM_LEDS/4-i] = R1[i];
            L1[i] = R1[i+1].nscale8(fadeval); if(L1[i].g > 0) L1[i].g--;
            L2[NUM_LEDS/4-i] = L1[i];
        }
        uint8_t _hue = 0, _sat = 255, _val = 0;
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        R1[NUM_LEDS/4-1] = CHSV( _hue, _sat, _val*_val/255);
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        R2[0] = CHSV( _hue, _sat, _val*_val/255);
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        L1[NUM_LEDS/4-1] = CHSV( _hue, _sat, _val*_val/255);
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        L2[0] = CHSV( _hue, _sat, _val*_val/255);
        EVERY_N_MILLISECONDS(1000/10){
            CRGB spark = CRGB::Yellow;
            if( random8() < 80)
                R1[NUM_LEDS/4-1-random8(NUM_LEDS/8)] = spark;
            if( random8() < 80)
                R2[random8(NUM_LEDS/8)]              = spark;
            if( random8() < 80)
                L1[NUM_LEDS/4-1-random8(NUM_LEDS/8)] = spark;
            if( random8() < 80)
                L2[random8(NUM_LEDS/8)]              = spark;
        }
    }
}

void fireRainbow(){ // same as fire, but with color cycling
    EVERY_N_MILLISECONDS(1000/frameRate){
        for(int i = 0; i < NUM_LEDS/4; i++){
            R1[i] = R1[i+1].nscale8(fadeval);
            R2[NUM_LEDS/4-i] = R1[i];
            L1[i] = R1[i+1].nscale8(fadeval);
            L2[NUM_LEDS/4-i] = L1[i];
        }
        uint8_t _hue = 0, _sat = 255, _val = 0;
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        R1[NUM_LEDS/4-1] = CHSV( _hue+gHue1, _sat, _val*_val/255);
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        R2[0] = CHSV( _hue+gHue1, _sat, _val*_val/255);
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        L1[NUM_LEDS/4-1] = CHSV( _hue+gHue2, _sat, _val*_val/255);
        _val = random(0, 255);
        _sat = 255 - (_val/255.0 * 50);
        _hue = _val/255.0 * 55;
        L2[0] = CHSV( _hue+gHue2, _sat, _val*_val/255);
    }
}

uint8_t blurval = 150;
void ripple_blur(){ // randomly drop a light somewhere and blur it using blur1d
    EVERY_N_MILLISECONDS(1000/30){
        //blur1d( ledsx(0         , NUM_LEDS/2-1), NUM_LEDS/2, blurval);
        //blur1d( leds(NUM_LEDS/2, NUM_LEDS    ), NUM_LEDS/2, blurval);
    }
    EVERY_N_MILLISECONDS(30){
        if( random8() < 15) {
            uint8_t pos = random(NUM_LEDS/2);
            LEFT [pos] = CHSV(random(0, 64)+gHue1, random(250, 255), 255);
        }
        if( random8() < 15) {
            uint8_t pos = random(NUM_LEDS/2);
            RIGHT [pos] = CHSV(random(0, 64)-gHue2, random(250, 255), 255);
        }
    }
}
