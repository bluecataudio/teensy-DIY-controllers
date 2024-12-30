
#include <Bounce.h>
#include <Encoder.h>

/** MIDI-controlled analog Vu-Meter handling.
*   Designed to take left and right signals as MIDI input and display the max value.
*   - vuPin: Vu meter output pin.
*/
class VuMeter
{
  public:
  VuMeter(int vuPin):
  pin(vuPin),
  midiValueL(0),
  midiValueR(0)
  {
    pinMode(pin,OUTPUT);
    analogWriteFrequency(pin,90000); ///< freq to be outside of hearable range
  }

  void SetMIDIValueL(int midiValue)
  {
    midiValueL=midiValue;
    UpdateMeter(std::max(midiValueL,midiValueR));
  }
  void SetMIDIValueR(int midiValue)
  {
    midiValueR=midiValue;
    UpdateMeter(std::max(midiValueL,midiValueR));
  }

  void UpdateMeter(int midiValue)
  {
    // Convert [0-127] midi to decibels
    float dBValue=float(midiValue)/127.0*26.0-20.0;

  // POFET Vu - using 5.6k resistor for meter / 150 for lighting
  // + capacitor 1 microF
  // PWM to dB values mapping resulting from calibration:
  // 188 -> +6
  // 123 -> +3
  // 88 -> 0
  // 62 -> -3
  // 36 -> -6
  // 17 -> -10
  // 3 -> -20 
  float pwmValue=0;
  if(dBValue<-20)
    pwmValue=0;
  else if(dBValue<-10)
  {
    pwmValue=3+(dBValue+20.0)*(17.0-3.0)/10.0;
  }
  else if(dBValue<-6)
  {
    pwmValue=17+(dBValue+10.0)*(36.0-17.0)/4.0;
  }
  else if(dBValue<-3)
  {
    pwmValue=36+(dBValue+6.0)*(62.0-36.0)/3.0;
  }
  else if(dBValue<0)
  {
    pwmValue=62+(dBValue+3.0)*(88.0-62.0)/3.0;
  }
  else if(dBValue<3)
  {
    pwmValue=88+(dBValue)*(123.0-88.0)/3.0;
  }
  else if(dBValue<=6)
  {
    pwmValue=123+(dBValue-3)*(188.0-123.0)/3.0;
  }
  analogWrite(pin,int(pwmValue+.5));
  }
  private:
  int pin;
  int midiValueL;
  int midiValueR;
};

VuMeter meters[]={VuMeter(11),VuMeter(10),VuMeter(8),VuMeter(7),VuMeter(6),VuMeter(5),VuMeter(3),VuMeter(2)};
VuMeter masterL(1);
VuMeter masterR(0);
bool ledL=false;
bool ledR=false;

static const int kMIDIChannel=16;
#define LED_PIN 16
#define LIGHT_PIN 14
#define VU_PIN 1

void onCC(byte channel, byte control, byte value)
{
  if(channel==kMIDIChannel)
  {
    if(control<=16 && control>0)
    {
      int index=(control-1)/2;
      if(control&1)
        meters[index].SetMIDIValueL(value);
      else
        meters[index].SetMIDIValueR(value);
    }
    else if(control==121)
    {
      masterL.SetMIDIValueL(value);
    }
    else if(control==122)
    {
      masterR.SetMIDIValueR(value);
    }
    else if(control==123)
    {
      // master clip LED LEFT
      ledL=(value>64);
      bool actualValue=ledL||ledR;
      if(actualValue)
        digitalWrite(LED_PIN,HIGH);
      else
        digitalWrite(LED_PIN,LOW);
    }
    else if(control==124)
    {
      // master clip LED RIGHT
      ledR=(value>64);
      bool actualValue=ledL||ledR;
      if(actualValue)
        digitalWrite(LED_PIN,HIGH);
      else
        digitalWrite(LED_PIN,LOW);
    }
    digitalToggle(LED_BUILTIN);
  }
}

void setup() {
  usbMIDI.setHandleControlChange(onCC);
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(LIGHT_PIN,OUTPUT);
  digitalWrite(LIGHT_PIN,HIGH);
  pinMode(LED_PIN,OUTPUT);
}

void loop() {
  // just read input values from MIDI
  usbMIDI.read();

  /* Test & calibration routine
  for(int i=0;i<8;i++)
  {
    meters[i].SetMIDIValueR(127*(0+20)/26);
  }*/
  /*
  int dB=0;
  meter.SetMIDIValue(127*(dB+20)/26);*/
}
