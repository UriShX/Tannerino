//include SPI lib for using mcp4131 digipot
#include <SPI.h>

//include MIDI library
#include <MIDI.h>

//include illutron synth from modified source, and AVR_pgmspace for it. original is https://github.com/dzlonline/the_synth
#include "synth.h"
#include <avr/pgmspace.h>

//array containing frequency equivalent for MIDI notes. array is stored as volatile since it is too slow to read from progmem
//#include "MIDI_freq.h" //replaced with function

//EEPROM expanded library
//#include <EEPROMex.h>
//#include <EEPROMVar.h>

//LCD using LiquidCrystal library. from https://playground.arduino.cc/Main/LiquidCrystal. changed name to differentiate from original
#include <LiquidCrystalSPI.h>

//include 16bit ADC library from Kerry D. Wong. get fork from https://github.com/Electronza/AD770X
#include <AD770X.h>

//definition for different baud rate for MIDI library: instead of standard 31250, so can work with hairless midi
struct HairlessMIDI : public midi::DefaultSettings
{
  static const long BaudRate = 115200;
};

//create custom instance of MIDI library with above definition
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, HairlessMIDI);
//MIDI_CREATE_DEFAULT_INSTANCE();

AD770X ad7705(65535, 8); //callout to SPI 16bit ADC, with pin 8 as CS

synth edgar; //declare a synth

// select the pins used on the LCD panel - pin 9 is CS
LiquidCrystalSPI lcd(9);

const byte potPin = A4;      //pin for analog potentiometer
const byte buttonPin = 2;    //pin for play/stop button
//const int modPot = A5;    //pin for modulation
const byte digiPotPin = 10; //pin for SPI digipot mcp4131
const byte digiPotAddress = 0x00; //8-bit word for digipot register: (AD3=0 AD2=0 AD1=0 AD0=0)=address (C1=0 C0=0)->write D9=0->always=0 D8=0->if=1 than potVal=255
//int lastRead = 0;           //debug

byte noteVal = 72;             //pot readout converted to MIDI (middle C)
int pitchA4 = 440;            //intialize pitch of A4
boolean noteOn = false;       //note On/Off flag
boolean buttonVal = false;    //button state
boolean lastButtonVal = true; //storeable value for button state debounce
//int noteCompare;              //storeable value for note changes
byte vel = 110;                //initial MIDI velocity
byte velCompare = 110;
byte channel = 1;              //MIDI channel select
byte synthVoice = 0;           //defines which synth voice is activated
byte range = 24;               //+- range for pitch bend, used to synchronize synth with MIDI
double lFreq = 130.8128;                  //float for arduino synth min frequency
double hFreq = 2093.0045;                 //float for arduino synth max frequency

void setup() {
  SPI.begin();
  lcd.begin(16, 2);              // start the library
  lcd.setCursor(0, 0);
  lcd.print("Loading..."); // print a simple message
  delay(500);

  //DIDR0 = 0x3F;   //disable the digital input buffers on the analog pins to save a bit of power and reduce noise.
  edgar.begin(CHB); //start the synth with output on Pin 3
  lcd.clear(); lcd.print("synth OK"); delay(500);
  //*********************************************************************
  //  Setup all voice parameters in MIDI range
  //  voice[0-3],wave[0-6],pitch[0-127],envelope[0-4],length[0-127],mod[0-127:64=no mod]
  //*********************************************************************
  edgar.setupVoice(synthVoice, SINE, noteVal, ENVELOPE4, 90, 64);//0,SINE,60,ENVELOPE0,90,64
  lcd.clear(); lcd.print("synth w/SINE"); delay(500);
  pinMode(potPin, INPUT);
  lcd.clear(); lcd.print("potPin fine"); delay(500);
  pinMode(buttonPin, INPUT);
  lcd.clear(); lcd.print("buttonPin fine"); delay(500);
  pinMode(digiPotPin, OUTPUT);
  MIDI.begin();
  lcd.clear(); lcd.print("MIDI initialized"); delay(500);
  //  Serial.begin(115200);
  ad7705.reset(); //to get a known baseline
  lcd.clear(); lcd.print("reset ADC fine"); delay(500);
  //overloaded constructor: AD770X::init(byte channel, byte clkDivider, byte polarity, byte gain, byte updRate)
  //initialized as ad7705, only ch.1, clock divder=1, bipolar mode, low gain, update rate of 500Hz
  ad7705.init(AD770X::CHN_AIN1, AD770X::CLK_DIV_1, AD770X::BIPOLAR, AD770X::GAIN_1, AD770X::UPDATE_RATE_500);
  //ad7705.init(AD770X::CHN_AIN1);
  lcd.clear(); lcd.print("ADC callout fine"); delay(500);
  digitalPotWrite(vel);                                         //init: send volume (velocity) to digiPot (see function at bottom)
  double nP = pow(2.0,((float(noteVal)+float(range)-9)/12));    //power function for high freq calculation
  double nM = pow(2.0,((float(noteVal)-float(range)-9)/12));    //power function for low freq calculation
  lFreq = (float(pitchA4)/32)*nM;                               //get min frequency for arduino synth
  hFreq = (float(pitchA4)/32)*nP;                               //get max frequency for arduino synth
  //Serial.print(lFreq);Serial.print(" ");Serial.println(hFreq);  //debug
}

void loop() {
  unsigned int pitchPotVal = ad7705.readADResult(AD770X::CHN_AIN1);    //for use with SPI 16bit ADC
  int volPotVal = analogRead(potPin);                                  //vol pot
  byte vel = map(volPotVal, 0, 1023, 0, 127);                           //map volume to MIDI velocity
  int readVal = map(pitchPotVal, 0, 65535, -8192, 8191);               //map readings of pitch pot to MIDI protocol pitch bend values
  float synthFreq = map(pitchPotVal, 0, 65535, lFreq, hFreq);          //map readings of pitch pot to arduino synth from min frequency to max frequency
  //delay(25);
  //Serial.print(readVal);Serial.print(" ");Serial.println(synthFreq);   //debug
  //lcd.clear();lcd.print(vel);delay(25);
  buttonVal = digitalRead(buttonPin);                                  //read button pin
  delay(15);                                                           //delay for debounce purposes, need to keep less than 25 overall for audio
  //lcd.clear();lcd.print(pitchPotVal);delay(50);

  if (buttonVal == lastButtonVal) {                                    //debounce
    if (buttonVal) {                                                   //if the button is pressed than:
      if (!noteOn) {                                                   //if it is the first note after silence than:
        MIDI.sendPitchBend(readVal, channel);                          //first send pitch bend for MIDI note played
        MIDI.sendNoteOn(noteVal, vel, channel);                        //send the center note to MIDI with velocity
        //Serial.print("noteOn: ");Serial.println(noteVal);              //debug
        //        MIDI.sendControlChange(68, 127, channel);                      //and send legato on message
        //Serial.print("Legato :");Serial.println("68, 127");            //debug
        //        noteCompare = noteVal;                                         //store the note played for future comparison
        noteOn = true;                                                 //and switch the note flag On
      }                                                                //after that:
      MIDI.sendPitchBend(readVal, channel);                            //send pitch bend data for the MIDI note played
      MIDI.sendNoteOn(noteVal, vel, channel);                          //send the center note to MIDI with velocity
      edgar.setFrequency(synthVoice, synthFreq);                       //send pitch to arduino synth
      if (velCompare!=vel){
        digitalPotWrite(vel);                                            //send volume (velocity) to digiPot (see function at bottom)
        velCompare = vel;
      }
      edgar.trigger(synthVoice);                                       //play the arduino synth
      //      if (noteCompare != noteVal) {                                    //check to see if the last note read is different than the last. if so:
      //        MIDI.sendControlChange(68, 127, channel);                      //repeat legato on message, just to be on the safe side
      //        //Serial.print("Legato :");Serial.println("68, 127");            //debug
      //        noteCompare = noteVal;                                         //and store the last note played again for future comparison
      //      }
    } else {                                                            //if the button is NOT pressed than:
      switch (noteOn) {                                                //then, determine if in the last time the loop ran a note was played, so will not keep midi busy
        case 0:                                                        //if not:
          break;                                                       //do nothing and skip forward
        case 1:                                                        //if there was a note played however:
          MIDI.sendNoteOff(noteVal, vel, channel);                     //send a note off message to MIDI
          //Serial.print("noteoff: ");Serial.println(noteVal);           //debug
          MIDI.sendControlChange(68, 0, channel);                      //and switch legato mode off, just in case
          //Serial.print("Legato :");Serial.println("68, 0");            //debug
          break;
      }                                                                //after switch iterated:
      noteOn = false;                                                  //turn note flag off
    }
  }                                                                    //after loop:
  lastButtonVal = buttonVal;                                           //store the button state for debounce
}

void digitalPotWrite(byte value)
{
  SPI.beginTransaction(SPISettings(10000000 , MSBFIRST, SPI_MODE0));
  digitalWrite(digiPotPin, LOW);
  SPI.transfer(digiPotAddress);
  SPI.transfer(value);
  digitalWrite(digiPotPin, HIGH);
  SPI.endTransaction();
}

