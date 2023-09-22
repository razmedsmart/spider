
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
 

void wave2(CRGB *_leds, int num_leds ) {
  int timeA = 30000;
  int timeB = 35000;
  int timeC = 65000;
  int timeEND = 70000;
  
  int timerInCycle = millis() % timeEND;
  int hueRangeMin = 0;
  int hueRangeMax = 0;
  if (timerInCycle < timeA) {
    hueRangeMin = 200;
    hueRangeMax = 245;
  } else if (timerInCycle < timeB) {
    hueRangeMin = map(timerInCycle, timeA, timeB, 200, 80);
    hueRangeMax = map(timerInCycle, timeA, timeB, 245, 170);
  } else if (timerInCycle < timeC) {
    hueRangeMin = 80;
    hueRangeMax = 170;
  } else if (timerInCycle < timeEND) {
    hueRangeMin = map(timerInCycle, timeA, timeEND, 80, 200);
    hueRangeMax = map(timerInCycle, timeA, timeEND, 170, 245);
  }

  for (uint16_t i = 0; i < num_leds; i++) {
    int speedBri2 = 2;
    int waveFraction2 = 10;
    int sinBri2 = sin16((i * 65536) / (num_leds * waveFraction2) - millis() * speedBri2 );
    int bri2 = map(sinBri2, -32767, 32767, 70, 255);
    bri2 = constrain(bri2, 80, 255);

    int speedbri3 = 1;
    int waveFraction3 = 9;
    int sinbri3 = sin16((i * 65536) / (num_leds * waveFraction3) - millis() * speedbri3);
    int bri3 = map(sinbri3, -32767, 32767, -100, 200);
    bri3 = constrain(bri3, 60, 200);

    int speedbri1 = 20;
    int numWavesBri = 1;
    int sinbri1 = sin16((i * 65536 * numWavesBri) / num_leds - millis() * speedbri1);
    int bri1 = map(sinbri1, -32767, 32767, -10, 50);
    
    int bri = max(max(bri2, bri2), bri3);
    bri = constrain(bri, 80, 120);
    bri = bri % 256;

    int speed1 = 20;
    int numWaves1 = 2;
    int sin1 = sin16((numWaves1 * i * (65536 / num_leds) + (65536 * 3 / 4) - millis() * speed1));
    int hue1 = map(sin1, -32767, 32767, 120, 230);

    int speed2 = 8;
    int numWaves2 = 2;
    int sin2 = sin16((numWaves2 * i * (65536 / num_leds) + (65536 * 3 / 4) - millis() * speed2));
    int hue2 = map(sin2, -32767, 32767, 155, 245);

    int speed3 = 10;
    int numWaves3 = 1;
    int sin3 = sin16((numWaves3 * i * (65536 / num_leds) + (65536 * 3 / 4) - millis() * speed3));
    int hue3 = map(sin3, -32767, 32767, 128, 150);

    int hue = max(max(hue1, hue2), hue3);
    hue = hue % 256;
    hue = constrain(hue, hueRangeMin, hueRangeMax);

    int sat = 255;
    _leds[i] = CHSV(hue, 255, bri2);
    //EVERY_N_MILLISECONDS(30){
    //FastLED.show();
    //}
  }
  //apply_to_all_led_from_leds();       
  //Serial.println(String(timerInCycle) + "\t" + String(hueRangeMin) + "\t" + String(hueRangeMax));
  //EVERY_N_MILLISECONDS(10){
  //FastLED.show();//};
}

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void pride(CRGB *_leds, int num_leds) 
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
  
  for( uint16_t i = 0 ; i < num_leds; i++) {
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
    
    nblend( _leds[pixelnumber], newcolor, 64);
  }
  FastLED.show();
}
