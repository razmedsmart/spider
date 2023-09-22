
//https://github.com/ohnoitsalobo/sound-reactive-esp32/blob/master/src/FFT.ino
#define noise 1500
#define MAX 50000
int scale_vreal = 8;
int max_value_yres = 80;
#define samples SAMPLES// must ALWAYS be a power of 2 // VERY IMPORTANT
#define samplingFrequency 25000 // samples per second, not to be confused with Nyquist frequency which will be half of this
double spectrum[3][samples/2];
arduinoFFT LFFT = arduinoFFT(vReal, vImag, samples, samplingFrequency);

void set_scale_vreal(int val){
  scale_vreal = val;
}
int get_scale_vreal(){
  return scale_vreal;
}

void set_max_value_yres(int val){
  max_value_yres = val;
}
int get_max_value_yres(){
  return max_value_yres;
}
void fftSetup(){
    sampling_period_us = round(1000000*(1.0/samplingFrequency));
    
    double exponent = 0.66;  //// this number will have to change for the best output on different numbers of LEDs and different numbers of samples.
    for (uint16_t i = 2; i < samples/2; i++){
        spectrum[0][i] = pow((i-2)/(samples/2.0-2), exponent) * NUM_LEDS; // **
        spectrum[1][i] = 0; // left  channel values
        spectrum[2][i] = 0; // right channel values
    }     
    for (uint16_t i = 0; i < samples; i++){
        vReal[i] = 0; //vReal[1][i] = 0;
        vImag[i] = 0; //vImag[1][i] = 0;
    }
}

void run_fft(){
   int scale_vreal = 8;
   for(int i=0; i<SAMPLES; i++)
    {
      microseconds = micros(); 
      int value = analogRead(34);               
      vReal[i]= value/scale_vreal;                      
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

void music(CRGB *_leds, CRGB baseColor, int num_leds) {
 // ++ send to display according measured value 
    int displaycolumn , displayvalue;
    int max_bands = 10;//was xres
    int start_band = 0;
    for(int i=start_band; i < start_band+8 && i < xres; i++)
    {
      
      data_avgs[i] = constrain(data_avgs[i],0,max_value_yres);            // set max & min values for buckets
      data_avgs[i] = map(data_avgs[i],  0, max_value_yres, 0, yres);        // remap averaged values to yres
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
    apply_to_all_led_from_leds();
    FastLED.show();
}

