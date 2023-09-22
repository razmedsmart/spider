#include "leds.h"
CRGB* get_jleds_0();
int get_jleds_0_num();
int pattern = 0;
typedef void (*SimplePatternList[])(CRGB *_leds, int num_leds);
typedef void (*_SimplePatternList[])();
_SimplePatternList _autoPatterns = {  rainbow, rainbowWithGlitter, rainbow_scaling, fire, fireSparks, fireRainbow, noise1, noise2, noise3, blendwave, confetti, ripple_blur, sinelon, dot_beat, juggle };
SimplePatternList autoPatterns = { pride, wave2};
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t get_gCurrentPatternNumber(){
  return gCurrentPatternNumber;
}
void set_gCurrentPatternNumber(uint8_t val){
  gCurrentPatternNumber = val;
}
#define PATTERN_PERIOD_SEC 50
void run_just_dance(){
  bool faded = false;
  EVERY_N_SECONDS(10){
    pattern = (pattern +1 ) % 2;
    for (int i = 120; i >= 0; i--) {
        FastLED.setBrightness(i);
        FastLED.show();
        delay(50);
    }
    faded = true;
  }
  autoPatterns[pattern](get_jleds_0(),get_jleds_0_num());
  FastLED.show();
  if ( faded){
    for (int i = 0; i < 255 ; i++) {
        FastLED.setBrightness(i);
        FastLED.show();
        delay(10);
    }
    faded = false;
    FastLED.setBrightness(250);
  }
}