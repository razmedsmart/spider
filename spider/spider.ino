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
#define debug
#define FASTLED_ESP8266_D1_PIN_ORDER
#define Power_Per_LED 60
#define TOTAL_NUM_LEDS 1200
#define MAX_POWER_MILLIAMPS = Power_Per_LED * TOTAL_NUM_LEDS

#define NUM_LEDS 100  // Replace with the number of LEDs in your strip
#define BRIGHTNESS          250
#define yres 12
#define SAMPLES 128            
#define NUM_LEDS 150
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


// FFPT part
#define SAMPLING_FREQUENCY 1000 //Hz, must be less than 10000 due to ADC
#define xres 32      
#define yres 8
int MY_ARRAY[]={0, 128, 192, 224, 240, 248, 252, 254, 255};
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
const char* apSSID = "MyESP32AP";  // Change this to the SSID you want for your AP
const char* apPassword = "1111111111";  // Change this to the password for your AP
AsyncWebServer server(80);
unsigned char newBrightness = 255;
unsigned char currentBrightness = BRIGHTNESS;
unsigned int led_pos = 0;
TaskHandle_t Core0Task;
TaskHandle_t Core1Task;

// Define the core numbers for each task
const int coreLoop = 0; // Core 0 for loop()
const int coreFFT = 1;  // Core 1 for run_fft()


//https://github.com/ohnoitsalobo/sound-reactive-esp32/blob/master/src/FFT.ino
#define noise 1500
#define MAX 50000

#define samples SAMPLES// must ALWAYS be a power of 2 // VERY IMPORTANT
#define samplingFrequency 25000 // samples per second, not to be confused with Nyquist frequency which will be half of this
double spectrum[3][samples/2];
arduinoFFT LFFT = arduinoFFT(vReal, vImag, samples, samplingFrequency);

void fftSetup(){
    sampling_period_us = round(1000000*(1.0/samplingFrequency));
    
    double exponent = 0.66;  //// this number will have to change for the best output on different numbers of LEDs and different numbers of samples.
    for (uint16_t i = 2; i < samples/2; i++){
        spectrum[0][i] = pow((i-2)/(samples/2.0-2), exponent) * NUM_LEDS; // **
        spectrum[1][i] = 0; // left  channel values
        spectrum[2][i] = 0; // right channel values
    }
    // ** the purpose of this line is to set up the 'logarithmic' spacing 
    // of the LED lights for different frequencies. The human perception of audio
    // frequency doesn't ascend linearly, so 
    // 120Hz vs 130Hz is 'musically' (and proportionally) a bigger change than
    // 1020Hz vs 1030Hz which is itself a bigger change than 
    // 10020Hz vs 10030 Hz
    // For this reason, I tried to set the exponent such that the lowest frequencies
    // each get their 'own' LED spot, while the higher frequencies 'share' LED spots
     // right now it is optimized for 72 LEDs, 512 samples ~ 0.66
     // with more LEDs you'll want a higher exponent, less LEDs a lower exponent, but it's a logarithmic relationship not linear.
     // let me know if you figure out a way to calculate it automatically, I figured it manually by trying different numbers.
     
    for (uint16_t i = 0; i < samples; i++){
        vReal[i] = 0; //vReal[1][i] = 0;
        vImag[i] = 0; //vImag[1][i] = 0;
    }
}

//// remember that this code is running independently of your 
  // main LED code, on the other core of the ESP32.
void fftLoop(){
#ifdef debug
    Serial.println("Starting fftLoop");
#endif

    //// audio signal capture happens here
    microseconds = micros();
    for(int i=0; i<samples; i++){
        vReal[i] = analogRead(34);
        vImag[i] = 0;
#ifdef STEREO
        //vReal[1][i] = analogRead(RightPin);
        vImag[1][i] = 0;
#endif
        while(micros() - microseconds < sampling_period_us){  }
        microseconds += sampling_period_us;
    }

    //// FFT magic happens here
    LFFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    LFFT.Compute(FFT_FORWARD);
    LFFT.ComplexToMagnitude();
    //// audio frequencies are output here into the 'spectrum' array which is then read in the LED code
    PrintVector(vReal, (samples >> 1), 1);

#ifdef STEREO
    //// FFT magic happens here
    RFFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    RFFT.Compute(FFT_FORWARD);
    RFFT.ComplexToMagnitude();
    //// audio frequencies are output here into the 'spectrum' array which is then read in the LED code
    PrintVector(vReal[1], (samples >> 1), 2);
#endif

#ifdef debug
    Serial.println("Ending fftLoop");
#endif
}

void PrintVector(double *vData, uint16_t bufferSize, int leftRight) {
    for (uint16_t i = 2; i < bufferSize; i++){
        if(vData[i] > noise){
            spectrum[leftRight][i] = vData[i]-noise;
            if(spectrum[leftRight][i] > MAX)
                spectrum[leftRight][i] = MAX;
        }else{
            spectrum[leftRight][i] = 0;
        }
        yield();
    }
}


void audio_spectrum(){ // using arduinoFFT to calculate frequencies and mapping them to light spectrum
    uint8_t fadeval = 90;
    nscale8(leds, NUM_LEDS, fadeval); // smaller = faster fade
    CRGB tempRGB1, tempRGB2;
    uint8_t pos = 0, h = 0, s = 0, v = 0;
    double temp1 = 0, temp2 = 0;
    for(int i = 2; i < samples/2; i++){
        pos = spectrum[0][i];
        h = pos/(NUM_LEDS/2.0)*224;
        temp1 = spectrum[1][i]/MAX;
        s = 255 - (temp1*30.0);
        v = temp1*255.0;
        tempRGB1 = CHSV(h, s, 255);
        tempRGB1 = CHSV(h, s, 255);
        if(tempRGB1 > leds[pos]){
            leds[pos] = tempRGB1;
        }
        Serial.printf("%d %d %d %d\n", pos, h, s);
        leds[pos] = tempRGB1;
        FastLED.show();
        

        uint8_t p = NUM_LEDS/2-1-pos;
#ifdef STEREO
        temp2 = spectrum[2][i]/MAX;
        s = 255 - (temp2*30.0);
        v = temp2*255.0;
        tempRGB2 = CHSV(h, s, v);
        if(tempRGB2 > LEFT[pos]){
            LEFT[p] = tempRGB2;
        }
#else
        //LEFT[p] = RIGHT[pos];
#endif
        yield();
    }
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
    //Serial.print("Task 1 loop on core ");
    //Serial.println(xPortGetCoreID());
    delay(100);
    switch (currentState) {
      case TEST:
        EVERY_N_MILLISECONDS(30){  
          FastLED.show();
          led_test();
        };
        if (millis() - stateChangeTime >= 2000) {
          stateChangeTime = millis();
          //fadeOutLeds(2000);
          changeState(MUSIC);
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
        wave(leds, sizeof(leds)/3, false, 5,0, 10);
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
            changeState(PRIDE);
          }
      break;
      case PRIDE:
        pride();
        if (millis() - stateChangeTime >= 10000) {
            //FastLED.clear();
            FastLED.show();
            changeState(SHOW_DOWN);
          }
      break;
      case MUSIC:
        //audio_spectrum();
        //    FastLED.show();
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

  FastLED.clear();
  //FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,MAX_POWER_MILLIAMPS);
  FastLED.show(0);
  WiFi.softAP(apSSID, apPassword);
  IPAddress apIP = WiFi.softAPIP();
  Serial.println("AP IP address: " + apIP.toString());
  server.on("/", HTTP_GET, handleRoot);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.on("/setbrightness", HTTP_POST, handleSetBrightness);
  server.on("/state", HTTP_GET, handleState);
  server.on("/setState", HTTP_POST, handleSetState);
  server.begin();
  setup_tasks();
}

void led_test(){
  for (int i = 0; i < NUM_LEDS; i++) 
      leds[i] = CHSV(0+i*5, 255, 255); // Set hue to 0 (red), saturation to 255 (max), and value to 255 (max)
  for (int i = 0; i < NUM_LEDS_BELLY; i++) 
      leds_belly[i] = CHSV(0+i*5, 255, 255); // Set hue to 0 (red), saturation to 255 (max), and value to 255 (max)
  for (int i = 0; i < NUM_LEDS_TAIL; i++) 
      leds_tail[i] = CHSV(0+i*5, 255, 255); // Set hue to 0 (red), saturation to 255 (max), and value to 255 (max)
  for (int i = 0; i < NUM_LEDS_CLOUD; i++) 
      leds_cloud[i] = CHSV(0+i*5, 255, 255); // Set hue to 0 (red), saturation to 255 (max), and value to 255 (max)

}


void changeState(State newState) {
  Serial.print("Changing state  "); printStateEnum(currentState); Serial.print("->"); printStateEnum(newState);Serial.println();
  currentState = newState;
  stateChangeTime = millis();
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = "<html><body style='font-size: 18px;'>";
  html += "<h1>LED Control</h1>";
  html += "<p style='font-size: 18px;'>Current Brightness: " + String(currentBrightness) + "</p>";
  html += "<form action='/setbrightness' method='POST'>";
  html += "<input style='font-size: 18px;' type='range' name='brightness' min='0' max='255' value='" + String(currentBrightness) + "'>";
  html += "<input style='font-size: 18px;' type='submit' value='Set Brightness'>";
  html += "<p style='font-size: 18px;'>Current Brightness: " + String(currentBrightness) + "</p>";
  html += "<form action='/setbrightness' method='POST'>";
  html += "<input style='font-size: 18px;' type='range' name='state' min='0' max='255' value='" + String(currentState) + "'>";
  html += "<input style='font-size: 18px;' type='submit' value='Set state'>";
  html += "</form>";
  html += "</body></html>";
  request->send(200, "text/html", html);
    request->redirect("/");
}

void handleBrightness(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", String(currentBrightness));
}

void handleSetBrightness(AsyncWebServerRequest *request) {
  if (request->hasParam("brightness", true)) {
    int newBrightness = request->getParam("brightness", true)->value().toInt();
    if (newBrightness >= 0 && newBrightness <= 255) {
      currentBrightness = newBrightness;
      FastLED.setBrightness(currentBrightness);
      request->send(303); // 303 is the HTTP status code for "See Other" (redirect)
      request->redirect("/");
    } else {
      request->send(400, "text/plain", "Invalid brightness value");
    }
  } else {
    request->send(400, "text/plain", "Missing brightness parameter");
  }
}

void handleState(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", String(currentState));
}

void handleSetState(AsyncWebServerRequest *request) {
  if (request->hasParam("state", true)) {
    int newState = request->getParam("state", true)->value().toInt();
    if (newState > 0){      
      request->send(303); // 303 is the HTTP status code for "See Other" (redirect)
      request->redirect("/");
      //led_test_state= true;      
    } else {
      request->send(400, "text/plain", "Invalid state value");
    }
  } else {
    request->send(400, "text/plain", "Missing state parameter");
  }
}


void run_fft(){
   for(int i=0; i<SAMPLES; i++)
    {
      microseconds = micros(); 
      int value = analogRead(34);               
      vReal[i]= value/16;                      
      vImag[i] = 0;
      while(micros() - microseconds < sampling_period_us){  }
      microseconds += sampling_period_us;

     //while(micros() < (microseconds + sampling_period_us)){
      //}
    }
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
     // The frequency range for a standard six-string guitar is roughly 82 Hz (E2) to 1319 Hz (E6)
    //double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY_MSEC*1000);
    //int peakIndex = getMajorPeakIndex(vReal, SAMPLES);
    //if (peak >= 80 && peak <= 1400) {
        //Serial.println("Guitar sound detected!");
    //}
    int step = (SAMPLES/2)/xres; //=1
    int c=0;
    for(int i=0; i<(SAMPLES/2); i+=step)  // 32
    {
      data_avgs[c] = 0;
      for (int k=0 ; k< step ; k++) {
          data_avgs[c] = data_avgs[c] + vReal[i+k];
      }
      data_avgs[c] = data_avgs[c]/step; 
      c++;
      //Serial.print((int)data_avgs[c]);Serial.print(",");
      //Serial.print((int)vReal[i]);Serial.print(",");
    }
    //Serial.println(".");
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

void show_grow( CRGB *_leds, int num_leds, bool up , unsigned char hue, unsigned char bright, int _delay_msec){
  // Set the rest of the LEDs to CHSV(0, 255, 255) (Hue 0, Saturation 255, Value 255)
  for ( auto i = 1; i < num_leds; i++) {
    auto led_id = i;
    if (!up){
      led_id = num_leds -i ;
    }
    _leds[led_id] = CHSV(hue, 255, bright);
    FastLED.show();//[0].showLeds(255);
    delay(_delay_msec);
  }
}

void wave( CRGB *_leds, int num_leds, bool up , unsigned char hue, unsigned char bright, int _delay_msec){
  // Set the rest of the LEDs to CHSV(0, 255, 255) (Hue 0, Saturation 255, Value 255)
    for ( auto i=0; i < num_leds; i++)
    {
        auto sinh = sin16( (i * 65536)/(num_leds)*45 + (millis() +1)/1001);
        auto hue =  map(sinh, -32767, 32767, 0, 100);
        CHSV color = CHSV(hue, 255, 255);
        //generate wave for changing led pixels
        uint16_t sinBeat = beatsin16(10, 0, num_leds, 0, 0);
//generate wave for colors
        uint8_t colBeat  = beatsin8(20, 0, 255, 0, 0);
        fadeToBlackBy(_leds, num_leds, 10);

        _leds[i] = CHSV(colBeat, 255, 255);
        /*light_led(leds_belly, sizeof(leds_belly), i, color);
        light_led(leds_tail, sizeof(leds_tail), i, color);
        light_led(leds_cloud, sizeof(leds_cloud), i, color);*/
        FastLED.show();
    }
}
 
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void pride() 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 64);
  }
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

void music(CRGB *_leds, CRGB baseColor, int num_leds) {
 // ++ send to display according measured value 
    int displaycolumn , displayvalue;
    int max_bands = 10;//was xres
    int start_band = 2;
    for(int i=start_band; i < start_band+10 && i < xres; i++)
    {
      data_avgs[i] = constrain(data_avgs[i],0,80);            // set max & min values for buckets
      data_avgs[i] = map(data_avgs[i],  0, 80, 0, yres);        // remap averaged values to yres
      yvalue=data_avgs[i];

      peaks[i] = peaks[i]-1;    // decay by one light
      if (yvalue > peaks[i])  
          peaks[i] = yvalue ;
      yvalue = peaks[i];    
      displayvalue=MY_ARRAY[yvalue];
      displaycolumn= i;
      //setColumn(_leds, displaycolumn, displayvalue, num_leds);              //  for left to right
      setColumn(_leds, displaycolumn, yvalue, num_leds);              //  for left to right
     }
    FastLED.show();
}
