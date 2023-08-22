#include <arduinoFFT.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>

//
// legs = leds
// pattern: wave, bottom to top
// belley
// patter: 
// tail:
// pattern:
// eyes:
// pattern: roll , wave
/*#define SAMPLES 64            
#define NUM_LEDS 300 
#define LED_PIN 5   
#define xres 32      */
#define yres 8       
#define SAMPLES 64            
#define NUM_LEDS 300 
#define NUM_LEDS_BELLY 50 
#define NUM_LEDS_TAIL 50 
#define NUM_LEDS_CLOUD 100 
#define LED_PIN 0 //D3   
#define LED_PIN_BELLY 2 //D4   
#define LED_PIN_TAIL 14   //D5
#define LED_PIN_CLOUD  12   //D6
#define xres 32      
#define yres 8
#define BRIGHTNESS 100
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
unsigned long lastDebounceTime = 0;  // Last time the button was toggled
unsigned long debounceDelay = 50;  // Debounce delay in milliseconds
int counter = 0;

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

double vReal[SAMPLES];
double vImag[SAMPLES];
char data_avgs[xres];

int yvalue;
int displaycolumn , displayvalue;
int peaks[xres];
//const int buttonPin = 5;
int state = HIGH;             
int previousState = LOW;   
int displaymode = 1;
unsigned long led_offeset = 0;
arduinoFFT FFT = arduinoFFT(); 

void setup() {
    //ADCSRA = 0b11100101;      
    //ADMUX = 0b00000000;       
    Serial.begin(9600);   // Initialize serial communication at 9600 baud rate
    Serial.println("Starting");
    pinMode(buttonPin, INPUT_PULLUP);  // Set the button as an input with an internal pull-up resistor

    //pinMode(buttonPin, INPUT);
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
    FastLED.addLeds<NEOPIXEL, LED_PIN_BELLY>(leds_belly, NUM_LEDS_BELLY);
    FastLED.addLeds<NEOPIXEL, LED_PIN_TAIL>(leds_tail, NUM_LEDS_TAIL);
    FastLED.addLeds<NEOPIXEL, LED_PIN_CLOUD>(leds_cloud, NUM_LEDS_CLOUD);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.setMaxRefreshRate(0);
    for( int j = 0; j < NUM_LEDS; j++){
        leds[j] = CRGB::Black;
    }
    FastLED.show();
    delay(50);            
}
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;

void run_fft(){
   for(int i=0; i<SAMPLES; i++)
    {
      //while(!(ADCSRA & 0x10));        
      //ADCSRA = 0b11110101 ;               
      //int value = ADC - 512 ;  
      int value = analogRead(A0);               
      vReal[i]= value/8;                      
      vImag[i] = 0; 
      //Serial.println(value);
                   
    }

    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
    
    int step = (SAMPLES/2)/xres; 
    int c=0;
    for(int i=0; i<(SAMPLES/2); i+=step)  
    {
      data_avgs[c] = 0;
      for (int k=0 ; k< step ; k++) {
          data_avgs[c] = data_avgs[c] + vReal[i+k];
      }
      data_avgs[c] = data_avgs[c]/step; 
      c++;
    }

}

// Glitter pattern: Call this in your main loop
void glitterPattern1(CRGB *leds, CRGB baseColor, uint8_t chanceOfGlitter, int num_leds ) {
  fill_solid(leds, num_leds, baseColor);
  addGlitter1(leds, CRGB::White, 100 ,num_leds);
  FastLED.show();
}
int curr_led = 0;
unsigned long startTime, elapsedTime;

void loop() {
  startTime = micros();  // Get the start time in microseconds
  run_fft();   
  EVERY_N_MILLISECONDS( 10){
    pattern_sound_scroll(leds,CRGB::Blue, 300);//D3
  }
  //pattern5(leds,CRGB::Blue);//D3
  //rainbowWithGlitter(leds_belly,CRGB::Blue);//D4
  //pattern_sound(leds_cloud,CRGB::Blue, sizeof(leds_cloud));//D4
  //pattern2(leds_tail,CRGB::Blue);//D5
  //glitterPattern1(leds_belly, CRGB::Black, 502); // Glitter effect on a black background
  //button_push();
  //displayModeChange (); 
  EVERY_N_MILLISECONDS( 10){
  //leds[curr_led] = CRGB::Black;
  //curr_led = (curr_led+1)%(num_leds-1);
  //leds[curr_led] = CRGB::Red;
  }
  //if (curr_led == 0) 
  //    Serial.println(curr_led);

    
  EVERY_N_MILLISECONDS( 50) {
      FastLED.show();
      //elapsedTime = micros() - startTime;  // Calculate the elapsed time
      //Serial.print("Time taken for FastLED.show(): ");
      //Serial.print(elapsedTime);
      //Serial.println(" microseconds");
  }       
} 

void button_push() {
  int reading = digitalRead(buttonPin);  // Read the state of the button

  // If the state has changed, update the last debounce time
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // If the button state has been stable for longer than the debounce delay, proceed
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the button state has changed
    if (reading != buttonState) {
      buttonState = reading;
      
      // If the new button state is LOW (i.e., it was just released), increment the counter
      if (buttonState == LOW) {
        counter++;
        Serial.println(counter);
      }
    }
  }
  
  lastButtonState = reading;  // Update the last button state to the current reading
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

void displayModeChange() {
  int reading = digitalRead(buttonPin);
  if (reading == HIGH && previousState == LOW && millis() - lastDebounceTime > debounceDelay)
  {
   switch (displaymode) {
    case 1:    
      displaymode = 2;
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_2[i];
      }
      break;
    case 2:    
      displaymode = 3;
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_3[i];
      }
      break;
    case 3:    
      displaymode = 4;
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_4[i];
      }
      break;
    case 4:    
      displaymode = 5;
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_5[i];
      }
      break;
    case 5:    
      displaymode = 1;      
      for (int i=0 ; i<=8 ; i++ ) {
        MY_ARRAY[i]=MY_MODE_1[i];
      }
      break;
  }

    lastDebounceTime = millis();
  }
  previousState = reading;
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
// Pattern 1: Simple fill
void pattern_sound(CRGB *_leds, CRGB baseColor, int num_leds) {
  for(int i=0; i<xres; i++)
  {
    data_avgs[i] = constrain(data_avgs[i],0,80);            
    data_avgs[i] = map(data_avgs[i], 0, 80, 0, yres);        
    yvalue=data_avgs[i];

    peaks[i] = peaks[i]-1;    
    if (yvalue > peaks[i]) 
        peaks[i] = yvalue ;
    yvalue = peaks[i];  
    displayvalue=MY_ARRAY[yvalue];
    int ledIndex = i;//(xres - 1) -i;
    //int ledIndex = 31 - i;
    char color_delta = 0;
    CHSV color = getLEDColor(displayvalue);  
    for( int j = 0; j < 9; j++){
      ledIndex = (i*(num_leds/xres)) + j;
      if (ledIndex > (num_leds-1)){
        //Serial.println(ledIndex);

      }else{
        color.hue = constrain(color.hue + color_delta ,0,254);
      _leds[ledIndex] = color;
      color_delta++; 
      }
    }
    }
    
    FastLED.show();

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
    FastLED.show();
  }
}

// Pattern 5: Random speckles
void pattern5(CRGB *leds, CRGB baseColor, int num_leds) {
  for (int i = 0; i < num_leds; i++) {
    leds[random(num_leds)] = baseColor;
    delay(10);
    FastLED.show();
  }
}

// Adds a random sparkle (glitter) effect to the LEDs
void addGlitter1(CRGB *leds, CRGB color, uint8_t chanceOfGlitter, int num_leds) {
  if (random8() < chanceOfGlitter) {
    leds[random16(num_leds)] += color;
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
