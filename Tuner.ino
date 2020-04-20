#include "arduinoFFT.h"

#define SAMPLES 64
#define SAMPLING_FREQ 2048
#define CFirstFreq 16.35
#define twelfthRoot 1.05946309436
#define noiseInput 1
#define touchInput 5

arduinoFFT fft = arduinoFFT(); 

int sharpLED = 13; 
int goodLED = 12; 
int flatLED = 11; 
unsigned int samplingPer; 
unsigned long microSec; 
unsigned long touchMillis; 
unsigned long currentMillis; 
const unsigned long period = 700; 
int chosenNoteIndex = 4; //default starting note is E
bool touched = false;
String notes[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
int counter = 0; 

double Re[SAMPLES]; 
double Im[SAMPLES]; 

void setup() {
  // put your setup code here, to run once:
  pinMode(sharpLED, OUTPUT); 
  pinMode(goodLED, OUTPUT); 
  pinMode(flatLED, OUTPUT); 
  Serial.begin(115200);
  samplingPer = round(1000000*(1.0/SAMPLING_FREQ)); 
}

void loop() { //G#4 - D#5
  int noiseVal = analogRead(noiseInput); 
  currentMillis = millis(); 
  for(int i = 0; i < SAMPLES; i++){
    microSec = micros(); 
    Re[i] = analogRead(1); //reads value from A1
    Im[i] = 0; 
    while(micros() < microSec+samplingPer){}
  }
  
  fft.Windowing(Re, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD); 
  fft.Compute(Re, Im, SAMPLES, FFT_FORWARD); 
  fft.ComplexToMagnitude(Re, Im, SAMPLES); 

  double trueFreq = .975*(fft.MajorPeak(Re, SAMPLES, SAMPLING_FREQ)); //peak frequency read
  int octave = int(log(trueFreq/CFirstFreq)/log(2)); 
  double CFreq = CFirstFreq*pow(2, octave); //lower bound of octave frequency (C)
  int halfSteps = int(log(trueFreq/CFreq)/log(twelfthRoot)); 
  double freq = CFreq*pow(twelfthRoot, halfSteps); 

  int noteIndex = halfSteps < 0 ? 12-halfSteps : halfSteps; 
  int touchVal = digitalRead(touchInput); 
  if(touchVal == 1) {
    if(!touched){
      touched = true; 
      touchMillis = currentMillis; 
    }
    else if(touched && currentMillis-touchMillis < period && currentMillis-touchMillis>100){
      chosenNoteIndex = chosenNoteIndex == 0 ? 11 : chosenNoteIndex-1; 
      touched = false; 
      touchMillis = currentMillis; 
      Serial.println(notes[chosenNoteIndex]); 
      delay(50); //MAKE SURE DOESNT CAUSE ISSUES
    }
    
  }
  if(touched && currentMillis-touchMillis > period){
    touched = false;  
    chosenNoteIndex = chosenNoteIndex == 11 ? 0: chosenNoteIndex+1; 
    Serial.println(notes[chosenNoteIndex]); 
    touchMillis = currentMillis;
  }
  
  if(noiseVal>500) {
  
    //Serial.println(noiseVal); 
    int result = compareNote(trueFreq, chosenNoteIndex); 
    Serial.print(result); 
    digitalWrite(sharpLED, LOW); 
    digitalWrite(goodLED, LOW); 
    digitalWrite(flatLED, LOW); 
    digitalWrite(result, HIGH); 
    Serial.print(" ");
    Serial.println(trueFreq); 
  }

}

int compareNote(double measuredFreq, int noteIndex){//11 = flat, 12 = good, 13 = sharp
  int halfSteps = noteIndex; 
  int octave = (int)(log(measuredFreq/CFirstFreq)/log(2)); 
  double CFreq = CFirstFreq*pow(2, octave); //lower bound of octave frequency (C)
  double flatBoundary = (CFreq*pow(twelfthRoot, halfSteps) + CFreq*pow(twelfthRoot, halfSteps-0.6))/2; 
  double sharpBoundary = (CFreq*pow(twelfthRoot, halfSteps) + CFreq*pow(twelfthRoot, halfSteps+0.6))/2;
  return measuredFreq < flatBoundary ? 11 : (measuredFreq > sharpBoundary ? 13 : 12); 
}
