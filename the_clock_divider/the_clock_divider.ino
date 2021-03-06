#include "Arduino.h"
#include <uClock.h>

#define INTERNAL_CLOCK_ANALOG_SYNC_RATIO 4
#define EXTERNAL_CLOCK_ANALOG_SYNC_RATIO 2
#define EVEN_ODD_PIN 12
#define GATE_PIN 11

// MIDI clock, start and stop byte definitions - based on MIDI 1.0 Standards.
#define MIDI_CLOCK 0xF8
#define MIDI_START 0xFA
#define MIDI_STOP  0xFC

// Number of outputs
const uint8_t pinCount = 5;
const static uint8_t outJack[] = {2, 3, 4, 5,  6};
// Even divisions
const static uint8_t evendiv[] = {1, 2, 4, 8, 16};
// Odd divisions
const static uint8_t oddsdiv[] = {1, 2, 3, 7, 11};

// controls, even: select the division; gate: switch from trigger out to gate out
bool even = true;
bool gate = true;

// The callback function wich will be called by Clock each Pulse of 96PPQN clock resolution.
void ClockOut96PPQN(uint32_t * tick) 
{
  // Send MIDI_CLOCK to external gears
  Serial.write(MIDI_CLOCK);
}

// the clock for analog outs! <3
void clockOutput32PPQN(uint32_t* tick)
{
  
  trigger(tick);
}

// The callback function wich will be called when clock starts by using Clock.start() method.
void onClockStart() 
{
  Serial.write(MIDI_START);
}

// The callback function wich will be called when clock stops by using Clock.stop() method.
void onClockStop() 
{
  Serial.write(MIDI_STOP);
}


void trigger(uint32_t* tick)
{
  // traverse all outputs and related divisions
  for (uint8_t index = 0; index < pinCount; index++)
  {
    // send voltage state to digital out on base of count calculations
    sendDigitalOut(index, count(tick, index));
  }
}

// here happens the magic
bool count(uint32_t* tick, uint8_t index)
{
  uint8_t ratio = uClock.getMode() == uClock.INTERNAL_CLOCK ? INTERNAL_CLOCK_ANALOG_SYNC_RATIO : EXTERNAL_CLOCK_ANALOG_SYNC_RATIO;
  // NOTES
  // don't know, can be nice! return ((*tick % (ratio + index) ) == 0)
  // a mess return ((*tick % (ratio * (index) + 1)) ) == 0) {

  // QUANDO SONO SLAVE LA MODAITà GATE NON FUNZIONA BENE
  // è POSSIBILE CHE IO DEBBA CAMBIARE IL MODO IN CUI GESTISCO 
  // IL GATE, ANCHE IL MODO IN FACCIO LO SLAVE MI PUZZA
  // SE ASCOLTO 16PPQN VENGO CLOCCATO ALLO STESSO MODO DI SLAVE
  // PERò DEVO ASSOLUTAMENTE RIPENSARE LA LOGICA DEL GATE!!!!

  if (even == true && gate == true )
  {
    return (*tick % (ratio * evendiv[index]) ) < evendiv[index] * 2.0;
  }
  else if (even == true )
  {
    return (*tick % (ratio * evendiv[index]) ) == 0;
  }
  else if (even == false && gate == true )
  {
    return (*tick % (ratio * oddsdiv[index]) ) < oddsdiv[index] * 2.0;
  }
  else
  {
    return (*tick % (ratio * oddsdiv[index]) ) == 0;
  }
}

void sendDigitalOut(uint8_t pin, bool state)
{
  byte pinState = state ? HIGH : LOW;
  digitalWrite(outJack[pin], pinState);
}

void setup() 
{
  // Initialize serial communication at 31250 bits per second, the default MIDI serial speed communication:
  Serial.begin(31250);

  // Inits the clock
  uClock.init();

  // Set the callback functions
  uClock.setClock96PPQNOutput(ClockOut96PPQN);
  uClock.setClock32PPQNOutput(clockOutput32PPQN);

  // Set the callback function for MIDI Start and Stop messages.
  uClock.setOnClockStartOutput(onClockStart);  
  uClock.setOnClockStopOutput(onClockStop);

  // Starts the clock, tick-tac-tick-tac...
  uClock.start();

}

bool prevClock = LOW;
uint32_t clockThresh = 512;
int32_t noClockVolt = 731;
int32_t noClockThresh = 5;

void loop() 
{
  even = digitalRead(EVEN_ODD_PIN) == HIGH;
  gate = digitalRead(GATE_PIN) == HIGH;

  int32_t clockRead = analogRead(A5);
  if (abs(noClockVolt - clockRead) > noClockThresh)
  {
    // we have a clock input!
    if (uClock.getMode() != uClock.EXTERNAL_CLOCK)
    {
      uClock.setMode(uClock.EXTERNAL_CLOCK);
    }
    
    if (clockRead >= clockThresh && prevClock == LOW)
    {
      uClock.clockMe();
      prevClock = HIGH;
    }
    else if (clockRead < clockThresh && prevClock == HIGH)
    {
      uClock.clockMe();
      prevClock = LOW;
    }
  }
  else
  {
    // switch to internal clock if necessary
    if (uClock.getMode() != uClock.INTERNAL_CLOCK)
    {
      uClock.setMode(uClock.INTERNAL_CLOCK);
    }
  
    uClock.setTempo((analogRead(A1) / 1024.0 * 270.0) + 30.0);
  }
  delay(5);
}
