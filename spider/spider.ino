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
#define LED_PIN 0 //D3   
#define LED_PIN_BELLY 2 //D4   
#define LED_PIN_TAIL 14   //D5
#define LED_PIN_EYES  12   //D6
#define xres 32      
#define yres 8
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
CRGB leds_belly[NUM_LEDS];
CRGB leds_tail[NUM_LEDS];
CRGB leds_eyes[NUM_LEDS];
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
    FastLED.addLeds<NEOPIXEL, LED_PIN_BELLY>(leds_belly, NUM_LEDS);
    FastLED.addLeds<NEOPIXEL, LED_PIN_TAIL>(leds_tail, NUM_LEDS);
    FastLED.addLeds<NEOPIXEL, LED_PIN_EYES>(leds_eyes, NUM_LEDS);
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
void glitterPattern1(CRGB *leds, CRGB baseColor, uint8_t chanceOfGlitter ) {
  fill_solid(leds, NUM_LEDS, baseColor);
  addGlitter1(leds, CRGB::White, chanceOfGlitter);
  FastLED.show();
}
void loop() {
    run_fft();   
    pattern_sound(leds,CRGB::Blue);//D3
    //pattern5(leds,CRGB::Blue);//D3
    //rainbowWithGlitter(leds_belly,CRGB::Blue);//D4
    //pattern_sound(leds_belly,CRGB::Blue);//D4
    //pattern2(leds_tail,CRGB::Blue);//D5
    glitterPattern1(leds_belly, CRGB::Black, 502); // Glitter effect on a black background
    button_push();
     //displayModeChange ();         
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

// Pattern 1: Simple fill
void pattern_sound(CRGB *leds, CRGB baseColor) {
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
      ledIndex = (i*(NUM_LEDS/xres)) + j; 
      if (ledIndex > (NUM_LEDS-1)){
        Serial.println(ledIndex);

      }else{
        color.hue = constrain(color.hue + color_delta ,0,254);
      leds[ledIndex] = color;
      color_delta++; 
      }
    }
    }
    
    FastLED.show();

}


// Pattern 1: Simple fill
void pattern1(CRGB *leds, CRGB baseColor) {
  fill_solid(leds, NUM_LEDS, baseColor);
}

// Pattern 2: Alternating colors
void pattern2(CRGB *leds, CRGB baseColor) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = (i % 2 == 0) ? baseColor : CRGB::Black;
  }
}

// Pattern 3: Gradient
void pattern3(CRGB *leds, CRGB baseColor) {
  fill_gradient_RGB(leds, NUM_LEDS, CRGB::Black, baseColor);
}

// Pattern 4: Middle outwards pattern
void pattern4(CRGB *leds, CRGB baseColor) {
  int middle = NUM_LEDS / 2;
  for (int i = 0; i < middle; i++) {
    leds[middle - i - 1] = baseColor;
    leds[middle + i] = baseColor;
    //delay(10);
    FastLED.show();
  }
}

// Pattern 5: Random speckles
void pattern5(CRGB *leds, CRGB baseColor) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[random(NUM_LEDS)] = baseColor;
    delay(10);
    FastLED.show();
  }
}

// Adds a random sparkle (glitter) effect to the LEDs
void addGlitter1(CRGB *leds, CRGB color, uint8_t chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += color;
  }
}


void rainbow(CRGB *_leds, CRGB baseColor) 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( _leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter(CRGB *_leds, CRGB baseColor) 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow(_leds, baseColor);
  addGlitter(_leds, 80, CRGB::White);
}

void addGlitter( CRGB *_leds, fract8 chanceOfGlitter, CRGB color) 
{
  random_led = random16(NUM_LEDS);
  if( random8() < chanceOfGlitter) {
    _leds[ random_led ] = color;
  }
}
