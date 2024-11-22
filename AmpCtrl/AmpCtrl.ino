
#include <Bounce.h>
#include <Encoder.h>

/** Interface for MIDI-controllable controls
*   such as rotary encoders, sliders etc.
*/
class ICustomControl
{
public:
  /// called when MIDI value received from USB
  virtual void OnMIDIValue(uint8_t value)=0;
  /// returns the MIDI value stored in the control
  virtual uint8_t GetValue()const;
  /// update the value from hardware
  virtual bool Update()=0;
};

/** class to manage an encoder and its display LED.
*   - pin1,pin2: encoder pins.
*   - iLedPin: the pin for the brightness LED.
*   - iResetPin: the pin for the reset button (resets to default value set by defaultPos).
*/
class EncoderWithLed : public ICustomControl
{
  public:
  static const int kCountRatio=4;
  static constexpr float kPWMRatio=.8;
  EncoderWithLed(uint8_t pin1,uint8_t pin2,uint8_t iLedPin,uint8_t iResetPin,int defaultPos=0):
   defaultPosition(defaultPos),
   encoder(pin1,pin2),
   pushButton(iResetPin,20), // 20 ms bounce
   ledPin(iLedPin),
   currentPosition(defaultPos),
   encoderDelta(0)
  {
      pinMode(ledPin,OUTPUT);
      analogWriteFrequency(ledPin,90000); ///< freq to be outside of hearable range
      pinMode(iResetPin,INPUT_PULLUP);
      encoder.write(0);
  }

  // MIDI CC callback
  virtual void OnMIDIValue(uint8_t value)
  {
      // update current position and reset encoder
      currentPosition=value;
      encoder.write(0);
      encoderDelta=0;
  }
  
  virtual uint8_t GetValue()const
  {
      return currentPosition;
  }

  // local update
  virtual bool Update()
  {
    int oldPosition=currentPosition;

    // check encoder for param change + check boundaries
    int delta=encoder.readAndReset();
    encoderDelta+=delta;
    if(abs(encoderDelta)>=kCountRatio)
    {
      int actualDelta=encoderDelta/kCountRatio;
      encoderDelta-=actualDelta*kCountRatio;
      if(actualDelta!=0)
      {
        // compute how fast the encoder has been turned.
        // fast turn? increase delta (5 times actual delta)
        int elapsedMs=elapsed;
        elapsed=0;
        if(elapsedMs/abs(actualDelta)<100)
          actualDelta*=5;

        currentPosition+=actualDelta;

        // bounds checking
        if(currentPosition>127)
          currentPosition=127;
        if(currentPosition<0)
          currentPosition=0;  
      }
    }

    // check button for reset
    if(pushButton.update())
    {
      if(pushButton.fallingEdge())// button pushed -> reset to default
      {
        currentPosition=defaultPosition;
        encoder.write(0);
      }
    }

    // update led - adjust brightness via PWM ratio according to position
    analogWrite(ledPin,int(currentPosition*kPWMRatio));/// PWM 0 to 255
    return currentPosition!=oldPosition;
  }
  int defaultPosition;
private:
  Encoder encoder; ///< the encoder
  elapsedMillis elapsed; ///< timer to check rotation speed
  Bounce  pushButton; ///< the reset to default button
  uint8_t ledPin; //< the LED to show value
  int currentPosition; ///< the current MIDI position of the encoder
  int encoderDelta; ///< how much has the encoder moved since last update?
};

/** Utility class to manage the Amp channels: clean/crunch/lead.
*   - switch1Pin: the pin for the switch to toggle between clean and crunch.
*   - switch2Pin: the pin for the switch to toggle between clean and lead.
*   - led1/2/3: leds for channels 1/2/3
*/
class AmpChannelManager
{
  public:
    AmpChannelManager(uint8_t switch1Pin,uint8_t switch2Pin,uint8_t led1,uint8_t led2,uint8_t led3):
    crunchButton(switch1Pin,30),
    leadButton(switch2Pin,30),
    currentProgram(0),
    led1Pin(led1),
    led2Pin(led2),
    led3Pin(led3)
    {
      pinMode(switch1Pin,INPUT_PULLUP);
      pinMode(switch2Pin,INPUT_PULLUP);
      pinMode(led1Pin,OUTPUT);
      pinMode(led2Pin,OUTPUT);
      pinMode(led3Pin,OUTPUT);
      digitalWrite(led1Pin,LOW);
      digitalWrite(led2Pin,HIGH);
      digitalWrite(led3Pin,LOW);
    }

    bool Update()
    {
      int oldProg=currentProgram;
       // check buttons updates
      if(crunchButton.update())
      {
        if(crunchButton.fallingEdge())// button pushed -> toggle betwwen prog 0 and 1
        {
          if(currentProgram!=1)
            currentProgram=1;
          else
            currentProgram=0;
        }
      }
      else if(leadButton.update())
      {
        if(leadButton.fallingEdge())// button pushed -> toggle betwwen prog 0 and 2
        {
           if(currentProgram!=2)
            currentProgram=2;
          else
            currentProgram=0;
        }
      }

      // update LEDs state
      switch(currentProgram)
      {
        case 0: // Clean
        {
            digitalWrite(led1Pin,LOW);
            digitalWrite(led2Pin,HIGH);
            digitalWrite(led3Pin,LOW);
            break;
        }
        case 1: // Crunch
        {
            digitalWrite(led1Pin,HIGH);
            digitalWrite(led2Pin,LOW);
            digitalWrite(led3Pin,LOW);
            break;
        }
        case 2: // Lead
        {
            digitalWrite(led1Pin,LOW);
            digitalWrite(led2Pin,LOW);
            digitalWrite(led3Pin,HIGH);
            break;
        }
      }

      // has it changed?
      return currentProgram!=oldProg;
    }

    uint8_t GetValue()const
    {
      return currentProgram;
    }

    private:
    Bounce crunchButton;
    Bounce leadButton;
    uint8_t currentProgram;
    uint8_t led1Pin;
    uint8_t led2Pin;
    uint8_t led3Pin;
};

/** Utility class to manage selected bank (using up and down switch).
*   switch1Pin: pin for the up switch.
*   switch2Pin: pin for the down switch.
*/
class BankSelector
{
  public:
    BankSelector(uint8_t switch1Pin,uint8_t switch2Pin):
    upButton(switch1Pin,30),
    downButton(switch2Pin,30),
    currentBank(1)
    {
      pinMode(switch1Pin,INPUT_PULLUP);
      pinMode(switch2Pin,INPUT_PULLUP);
    }

    bool Update()
    {
      int oldBank=currentBank;
       // check buttons updates
      if(upButton.update())
      {
        if(upButton.fallingEdge())// button pushed -> increase bank #
        {
          if(currentBank<127)
            currentBank++;
        }
      }
      else if(downButton.update())
      {
        if(downButton.fallingEdge())// button pushed -> decrease bank #
        {
          if(currentBank>1) // only using user banks (skip 0 which contain factory presets)
              currentBank--;
        }
      }
        return currentBank!=oldBank;
    }

    uint8_t GetValue()const
    {
      return currentBank;
    }

    private:
    Bounce upButton;
    Bounce downButton;
    uint8_t currentBank;
};

// Setup MIDI channel here
const int kMIDIOutChannel=1;
const int kMIDIInChannel=1;

// Setting up the control pins, as connected to the controller
EncoderWithLed inKnob(22,23,0,1,63);
EncoderWithLed driveKnob(20,21,2,3);
EncoderWithLed bassKnob(18,19,4,25,63); /// TBD 0 or other
EncoderWithLed midKnob(16,17,6,5,63);
EncoderWithLed trebleKnob(14,15,8,7,63);
EncoderWithLed toneKnob(40,41,10,9,42); // default ~.3
EncoderWithLed outKnob(38,39,12,11,63);

EncoderWithLed* encoders[7]={&inKnob,&driveKnob,&bassKnob,&midKnob,&trebleKnob,&toneKnob,&outKnob};
AmpChannelManager ampSelect(31,32,24,28,33);
BankSelector bankSelect(30,29);

// elapsed time / used to reset the test LED that shows MIDI activity
elapsedMillis elapsed;

// On MIDI CC input callback
void onCC(byte channel, byte control, byte value)
{
  // toggle built-in LED for MIDI activity and reset elapsed time.
  digitalToggle(LED_BUILTIN);
  elapsed=0;

  // MIDI channel and control # ok? set encoder value
  if(channel==kMIDIInChannel && control>0 && control<=7)
  {
    encoders[control-1]->OnMIDIValue(value);
  }
}

// setting up
void setup() {
  usbMIDI.setHandleControlChange(onCC);
  pinMode(LED_BUILTIN,OUTPUT);
}

// main loop
void loop() {
  // get MIDI in (calls MIDI CC callback if any event arrived)
  usbMIDI.read();

  bool midiSent=false;

  // update encoders from hardware
  for(int i=0;i<7;i++)
  {
    if(encoders[i]->Update())
    {
      // if changed send MIDI
      usbMIDI.sendControlChange(i+1,encoders[i]->GetValue(),kMIDIOutChannel);
      midiSent=true;
    }
  }

  // update bank selector
  if(bankSelect.Update())
  {
      if changed, send MIDI
      usbMIDI.sendControlChange(0,bankSelect.GetValue(),kMIDIOutChannel);
      usbMIDI.sendProgramChange(ampSelect.GetValue(),kMIDIOutChannel);
      midiSent=true;
  }

  // update amp channel selector
  if(ampSelect.Update())
  {
    // if changed, send MIDI
    usbMIDI.sendProgramChange(ampSelect.GetValue(),kMIDIOutChannel);
    midiSent=true;
  }

  // update MIDI I/O led
  if(midiSent)
  {
    digitalToggle(LED_BUILTIN);
    elapsed=0;
  }

  // make sure the light ends up being shut down after 500 ms without any MIDI activity
  if(elapsed>500)
  {
    digitalWrite(LED_BUILTIN,LOW);
    elapsed=0;
  }
}
