#include "Arduino.h"
#include <uClock.h>

#define INTERNAL_CLOCK_ANALOG_SYNC_RATIO 4
#define EXTERNAL_CLOCK_ANALOG_SYNC_RATIO 2

#define EVEN_ODD_PIN 12
#define GATE_PIN 11
#define TEMPO_PIN 1

// input clock voltage threshold (2.5v)
#define CLOCK_THRESHOLD 512

// clock input pin
#define CLOCK_PIN 5

// reference to detect a jack on clock input pin
#define NO_CLOCK_REF 725 //3.274
#define NO_CLOCK_DELTA 80

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

// the clock for analog outs! <3
void clockOutput32PPQN(uint32_t* tick)
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

  for (uint32_t i = 0; i < pinCount; i++) {
    pinMode(outJack[i], OUTPUT);
  }

  pinMode(EVEN_ODD_PIN, INPUT);
  pinMode(GATE_PIN, INPUT);

  // Inits the clock
  uClock.init();

  // Set the callback functions
//  uClock.setClock96PPQNOutput(ClockOut96PPQN);
  uClock.setClock32PPQNOutput(clockOutput32PPQN);

  // Starts the clock, tick-tac-tick-tac...
  uClock.start();

}

bool prevClock = LOW;

void loop() 
{
  even = digitalRead(EVEN_ODD_PIN) == LOW;
  gate = digitalRead(GATE_PIN) == HIGH;

  int32_t clockRead = analogRead(CLOCK_PIN);
  if (abs(NO_CLOCK_REF - clockRead) > NO_CLOCK_DELTA)
  {
    // we have a clock input!
    if (uClock.getMode() != uClock.EXTERNAL_CLOCK)
    {
      uClock.setMode(uClock.EXTERNAL_CLOCK);
    }
    
    if (clockRead >= CLOCK_THRESHOLD && prevClock == LOW)
    {
      uClock.clockMe();
      prevClock = HIGH;
    }
    else if (clockRead < CLOCK_THRESHOLD && prevClock == HIGH)
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

    uClock.setTempo((analogRead(TEMPO_PIN) / 1024.0 * 270.0) + 30.0);
  }
  delay(5);
}
