#include <arduinoFFT.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
//  https://projecthub.arduino.cc/shajeeb/32-band-audio-spectrum-visualizer-analyzer-924af5
//
// legs = leds
// pattern: wave, bottom to top
// belley
// patter: 
// tail:
// pattern:
// eyes:
// pattern: roll , wave
/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
*/

// sin
#define yres 8
#define SAMPLES 64            
#define NUM_LEDS 300
#define NUM_LEDS_BELLY 50 
#define NUM_LEDS_TAIL 50 
#define NUM_LEDS_CLOUD 100 
#define LED_PIN 0 //D3   
#define LED_PIN_BELLY 14 //D5   
#define LED_PIN_TAIL 2   //D4
#define LED_PIN_CLOUD  12   //D6
#define xres 32      
#define yres 8
#define BRIGHTNESS 100
int random_led = 0;
CRGB leds[NUM_LEDS];
CRGB leds_belly[NUM_LEDS_BELLY];
CRGB leds_tail[NUM_LEDS_TAIL];
CRGB leds_cloud[NUM_LEDS_CLOUD];
typedef void (*PatternFunction)(CRGB *, CRGB); // Function pointer typedef for easier management
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
const int buttonPin = 5;  // GPIO 2 of ESP8266
int buttonState = HIGH;  // Variable to store the button state (HIGH by default due to pull-up resistor)
int lastButtonState = HIGH;  // Variable to store the last button state
int button_push_counter = 0;
unsigned long lastDebounceTime = 0;  // Last time_now the button was toggled
unsigned long debounceDelay = 50;  // Debounce delay in milliseconds
// Function declarations
void pattern1(CRGB *leds, CRGB baseColor);
void pattern2(CRGB *leds, CRGB baseColor);
void pattern3(CRGB *leds, CRGB baseColor);
void pattern4(CRGB *leds, CRGB baseColor);
void pattern5(CRGB *leds, CRGB baseColor);

PatternFunction patternFunctions[] = {
  pattern1, pattern2, pattern3, pattern4, pattern5
};

int MY_ARRAY[]={0, 128, 192, 224, 240, 248, 252, 254, 255};
int MY_MODE_1[]={0, 128, 192, 224, 240, 248, 252, 254, 255};
int MY_MODE_2[]={0, 128, 64, 32, 16, 8, 4, 2, 1};
int MY_MODE_3[]={0, 128, 192, 160, 144, 136, 132, 130, 129};
int MY_MODE_4[]={0, 128, 192, 160, 208, 232, 244, 250, 253};
int MY_MODE_5[]={0, 1, 3, 7, 15, 31, 63, 127, 255};
const int SAMPLING_FREQUENCY_MSEC= 5;
double vReal[SAMPLES];
double vImag[SAMPLES];
char data_avgs[xres];
int yvalue;
int displaycolumn , displayvalue;
int peaks[xres];
arduinoFFT FFT = arduinoFFT();
unsigned long time_now = 0;
void setup() {
    Serial.begin(9600);   // Initialize serial communication at 9600 baud rate
    Serial.println("Starting");
    pinMode(buttonPin, INPUT_PULLUP);  // Set the button as an input with an internal pull-up resistor
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
    FastLED.addLeds<NEOPIXEL, LED_PIN_BELLY>(leds_belly, NUM_LEDS_BELLY);
    //FastLED.addLeds<NEOPIXEL, LED_PIN_TAIL>(leds_tail, NUM_LEDS_TAIL);
    //FastLED.addLeds<NEOPIXEL, LED_PIN_CLOUD>(leds_cloud, NUM_LEDS_CLOUD);
    //FastLED.setBrightness(BRIGHTNESS);
    //FastLED.setMaxRefreshRate(0);
    /*delay(500);
    for( int j = 0; j < NUM_LEDS; j++){
        leds[j] = CRGB::Red  ;
    }
    FastLED.show();
    delay(2000);
    for( int j = 0; j < NUM_LEDS; j++){
        leds[j] = CRGB::Black  ;
    }
    FastLED.show();
    delay(50);*/            
}
int getMajorPeakIndex(double* vReal, uint16_t samples) {
    double maxVal = 0;
    int index = 0;
    
    for (uint16_t i = 2; i < samples / 2; i++) {
        if (vReal[i] > maxVal) {
            maxVal = vReal[i];
            index = i;
        }
    }
    return index;
}

void run_fft(){
   for(int i=0; i<SAMPLES; i++)
    {

      int value = analogRead(A0);               
      vReal[i]= value/8;                      
      vImag[i] = 0;
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
    //Serial.println("");
}

// Adds a random sparkle (glitter) effect to the LEDs
void addGlitter1(CRGB *leds, CRGB color, uint8_t chanceOfGlitter, int num_leds) {
  if (random8() < chanceOfGlitter) {
    leds[random16(num_leds)] += color;
  }
}
// Glitter pattern: Call this in your main loop
void glitterPattern1(CRGB *leds, CRGB baseColor, uint8_t chanceOfGlitter, int num_leds ) {
  fill_solid(leds, num_leds, baseColor);
  addGlitter1(leds, CRGB::White, 100 ,num_leds);
  //FastLED.show();
}
int curr_led = 0;
unsigned long startTime, elapsedTime;
bool guitar_state = true;
bool prev_guitar_state = false;

void loop() {
  //leds[5] = CRGB::Red;
  EVERY_N_MILLISECONDS( SAMPLING_FREQUENCY_MSEC){
      run_fft();
  }
  if (guitar_state !=  prev_guitar_state){
    prev_guitar_state = guitar_state;
    startTime = millis();
  }
  if (guitar_state) {
     for( int j = 0 ; j < 50; j++){
            light_led(leds_belly, 50,j,CHSV(50,200,200));
          }
     for( int j = 0 ; j < 300; j++){
            light_led(leds, 300,j,CHSV(80,200,200));
          }
      //light_all_gradually(leds_belly, 300, 0, 5000, true, 80);
      //light_all_gradually(leds_belly, 300, 5000, 10000, false,150);
      //light_all_gradually_bright( leds_belly, 300, 10000, 20000, false,150);
      //light_all_gradually_bright( leds_belly, 300, 20000, 30000, true ,150);
      //wave_brightness(3000,4000);
  }
  else {
      EVERY_N_MILLISECONDS( 10){
          //pattern_sound(leds_cloud,CRGB::Blue, sizeof(leds_cloud));//D4
          //button_push();
      }
  }

  EVERY_N_MILLISECONDS( 50) {
    FastLED.show();
  }
}

void light_led(CRGB *_leds,  int num_leds, int led_id,CHSV color  ){
    if (led_id < num_leds){
        _leds[led_id] = color;
        //Serial.println(led_id);
    }else{
      Serial.print("Error id");Serial.println(led_id);
    }
}

void light_all_gradually( CRGB *_leds, int num_leds,int start, int end, bool up , unsigned char hue){
    int i = 0;
    CHSV color = CHSV(hue,255,255);
    int pattern_duration_msec = (end-start);
    time_now = millis()-startTime;
    if (time_now > start && time_now < end )
    {
        //time_now = millis()%pattern_duration_msec;
        i = map(time_now -start, 0, pattern_duration_msec, 0 , num_leds-1);
        //Serial.print(time_now);Serial.print(",");Serial.println(i);
        if (up)
          light_led(_leds, num_leds,i,color);
        else
          light_led(_leds, num_leds,(num_leds-1)-i,color);
    }
}

void light_all_gradually_bright( CRGB *_leds, int num_leds,int start, int end, bool up , unsigned char hue){
    int i = 0;
    CHSV color = CHSV(hue,255,255);
    int pattern_duration_msec = (end-start);
    time_now = millis()-startTime;
    unsigned char bright;
    if (time_now > start && time_now < end )
    {
        //time_now = millis()%pattern_duration_msec;
        bright = map(time_now - start, 0, pattern_duration_msec, 0 , 255);
        if (!up){
          bright = 255 - bright;
        }
        color  = CHSV(hue, 255, bright);
        Serial.print(time_now-start);Serial.print(",");Serial.println(bright);
        {
          for( int j = 0 ; j < num_leds; j++){
            light_led(_leds, num_leds,j,color);
          }
        }

        //else
          //light_led(_leds, num_leds,(num_leds-1)-i,color);
    }
}

void wave_brightness( int start, int end){
    if (time_now > start && time_now < end){
        int i = 0;
        CHSV color = CHSV(80,255,i);
        light_led(leds, sizeof(leds),i,color);
        light_led(leds_belly, sizeof(leds_belly),i, color);
        light_led(leds_tail, sizeof(leds_tail),i, color);
        light_led(leds_cloud, sizeof(leds_cloud),i, color);
        FastLED.show();
    }
}

void wave( int start, int end){
    if (time_now > start && time_now < end) {
        int i = 0;
        CHSV color = CHSV(80, 255, i);
        light_led(leds, sizeof(leds), i, color);
        light_led(leds_belly, sizeof(leds_belly), i, color);
        light_led(leds_tail, sizeof(leds_tail), i, color);
        light_led(leds_cloud, sizeof(leds_cloud), i, color);
        FastLED.show();
    }
}


void button_push() {
  int reading = digitalRead(buttonPin);  // Read the state of the button
  // If the state has changed, update the last debounce time_now
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // If the button state has been stable for longer than the debounce delay, proceed
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button state has changed
    if (reading != buttonState) {
      buttonState = reading;
      
      // If the new button state is LOW (i.e., it was just released), increment the button_push_counter
      if (buttonState == LOW) {
        button_push_counter++;
        Serial.println(button_push_counter);
      }
    }
  }
  
  lastButtonState = reading;  // Update the last button state to the current reading
}



void pattern_sound_scroll(CRGB *_leds, CRGB baseColor, int num_leds) {
  int  max_vol  = 0;
  //for(int i=2; i<xres; i++)
  ///int i = 2;
  //{
  //  max_vol = max(max_vol,data_avgs[i]);
    //Serial.println((int)data_avgs[i]);
  //}
  int i = 2;
  max_vol = data_avgs[i] + data_avgs[i+1] + data_avgs[i+2]; 
  Serial.println((int)max_vol);
  handleEvent(_leds,max_vol,150, CRGB::Red, num_leds);
}

void pattern_sound(CRGB *_leds, CRGB baseColor, int num_leds) {
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
     // -- send to display according measured value  
    //FastLED.show();
}

CHSV getLEDColor(int displayValue) {
    int hue = map(displayValue, 0, 255, 160, 0); // Maps displayValue from a blue to red hue.
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
      color = getLEDColorRGB(MY_ARRAY[i]);
      //color = getLEDColor(_displayvalue);
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


// Pattern 1: Simple fill
void pattern1(CRGB *leds, CRGB baseColor, int num_leds) {
  fill_solid(leds, num_leds, baseColor);
}

// Pattern 2: Alternating colors
void pattern2(CRGB *leds, CRGB baseColor, int num_leds) {
  for (int i = 0; i < num_leds; i++) {
    leds[i] = (i % 2 == 0) ? baseColor : CRGB::Black;
  }
}

// Pattern 3: Gradient
void pattern3(CRGB *leds, CRGB baseColor, int num_leds) {
  fill_gradient_RGB(leds, num_leds, CRGB::Black, baseColor);
}

// Pattern 4: Middle outwards pattern
void pattern4(CRGB *leds, CRGB baseColor, int num_leds) {
  int middle = num_leds / 2;
  for (int i = 0; i < middle; i++) {
    leds[middle - i - 1] = baseColor;
    leds[middle + i] = baseColor;
    //delay(10);
    //FastLED.show();
  }
}

// Pattern 5: Random speckles
void pattern5(CRGB *leds, CRGB baseColor, int num_leds) {
  for (int i = 0; i < num_leds; i++) {
    leds[random(num_leds)] = baseColor;
    delay(10);
    //FastLED.show();
  }
}



void rainbow(CRGB *_leds, CRGB baseColor, int num_leds)
{
  // FastLED's built-in rainbow generator
  fill_rainbow( _leds, num_leds, gHue, 7);
}

void rainbowWithGlitter(CRGB *_leds, CRGB baseColor, int num_leds)
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow(_leds, baseColor, num_leds);
  addGlitter(_leds, 80, CRGB::White, num_leds);
}

void addGlitter( CRGB *_leds, fract8 chanceOfGlitter, CRGB color, int num_leds)
{
  random_led = random16(num_leds);
  if( random8() < chanceOfGlitter) {
    _leds[ random_led ] = color;
  }
}

void shiftAndAdd(CRGB *_leds, CRGB newColor, int num_leds) {
  // Shift all colors forward
  for (int i = num_leds - 1; i > 0; i--) {
    _leds[i] = _leds[i - 1];
  }
  // Add new color to the front
  _leds[0] = newColor;
}

void handleEvent(CRGB *_leds, int volume, int threshold, CRGB color, int num_leds) {
  if (volume > threshold) {
    shiftAndAdd(_leds, color,  num_leds);
  } else {
    shiftAndAdd(_leds, CRGB::Black,  num_leds);  // Add black (or turn off) the first LED if volume is below threshold
  }
}
