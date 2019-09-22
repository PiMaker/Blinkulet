/*
 * 
 * Lights up some Neopixel-compatible LEDs with changing hue.
 * Uses the FastLED library and some code modified from their wiki.
 * 
 * The code for sleeping/disabling ADC is taken from:
 * https://www.re-innovation.co.uk/docs/sleep-modes-on-attiny85/
 * 
 */

#include "FastLED.h"

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// Helpers to clear/set bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// Necessary for sleepiness
const int deviceType = 85;
volatile boolean f_wdt = 1;

/*
 * CONFIG - change these two if you want
 */
#define BRIGHTNESS 210
#define LED_COUNT 2

// Arrays
CRGB leds[LED_COUNT];
uint8_t gHue[LED_COUNT];

void setup() {
  // 2 LEDs on pin 0
  FastLED.addLeds<NEOPIXEL, 0>(leds, LED_COUNT);

  // turn off builtin red LED just in case
  pinMode(1, OUTPUT);
  digitalWrite(1, false);

  // start the LED's hue out of phase
  uint8_t offset = 0xFF / LED_COUNT;
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    gHue[i] = offset * i;
  }

  // disable unneeded ADC and enable sleeping
  cbi(ADCSRA, ADEN);
  setup_watchdog(3); // this 3 defines the speed, see below for values
}

void loop() {
  // I don't think this can happen, but it was on the website
  // so let's better leave it in ¯\_(ツ)_/¯
  if (f_wdt==0) return;
  f_wdt = 0;
  
  // calculate color
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    gHue[i] += 1;
    leds[i].setHSV(gHue[i], 255, BRIGHTNESS);
  }

  // update LEDs
  FastLED.show();

  // go to sleep to save energy
  system_sleep();
}


// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System actually sleeps here
  sleep_disable();                     // System continues execution here when watchdog timed out 
}
 
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
 
  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;
 
  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}
  
// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
  f_wdt=1;  // set global flag
}
