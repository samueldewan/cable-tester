//
//  main.c
//  Cable Tester
//
//  Created by Samuel Dewan on 2017-06-26.
//  Copyright © 2017 Samuel Dewan. All rights reserved.
//

// This program is meant to be used on an ATTiny85 with the default fuses (E:FF, H:DF, L:62)

#include <avr/io.h>
#include <avr/interrupt.h>

#include "pindefinitions.h"

// MARK: Macros

// *** Configuration (change this part) ***
#define NUM_PINS 3  // The number of pins to test (1 to 5)

#define STEP_TIME   500
#define HOLD_TIME   1500

// Comment out if wired common cathode (grouds together), Uncomment if wired common anode (positives together)
#define COMMON_ANODE
// ****************************************

// Macros to abstract over how the LEDs are wired
#ifdef COMMON_ANODE
#define LED_ON(port, num)  ((port) &= ~(1 << num))
#define LED_OFF(port, num) ((port) |= (1 << num))
#else
#define LED_ON(port, num)  ((port) |= (1 << num))
#define LED_OFF(port, num) ((port) &= ~(1 << num))
#endif

// MARK: Function prototypes
void main_loop (void);

// MARK: Variable Definitions
typedef enum {STEP, HOLD} mode;

volatile uint32_t millis;
uint32_t last_step;
uint8_t current_pin;
mode current_mode;

volatile uint8_t *test_ports[5] = {&TEST_ONE_PORT, &TEST_TWO_PORT, &TEST_THREE_PORT, &TEST_FOUR_PORT, &TEST_FIVE_PORT};
volatile uint8_t test_nums[5] = {TEST_ONE_NUM, TEST_TWO_NUM, TEST_THREE_NUM, TEST_FOUR_NUM, TEST_FIVE_NUM};

// MARK: Funciton definitions
void init_IO(void) {
    // Set test pins as outputs
	TEST_ONE_DDR   |= (1 << TEST_ONE_NUM);
    TEST_TWO_DDR   |= (1 << TEST_TWO_NUM);
    TEST_THREE_DDR |= (1 << TEST_THREE_NUM);
    TEST_FOUR_DDR  |= (1 << TEST_FOUR_NUM);
    TEST_FIVE_DDR  |= (1 << TEST_FIVE_NUM);
}

void init_timers(void) {
    //Timer 0 (clock)
    TCCR0A |= (1 << WGM01); 				//Set the Timer Mode to CTC
    OCR0A = 0x7D; 							//Set the value to count to 125 (1 millisec at 1Mhz where prescaler = 8)
    TIMSK |= (1 << OCIE0A);                 //Set the ISR COMPA vector
    TCCR0B |= (1 << CS01);                  //set prescaler to 8 and start timer 0
}

int main(void) {
    cli(); // Disable interupts
    init_IO();
    init_timers();
    sei(); // Enable interupts
    
    current_mode = STEP;
    
    // Start with all but the first LED off
    LED_ON(*test_ports[0], test_nums[0]);
    for (int i = 1; i < NUM_PINS; i++) {
        LED_OFF(*test_ports[i], test_nums[i]);
    }
    
    for (;;) {
        main_loop();
	}
	return 0; // never reached
}

void main_loop (void) {
    if ((current_mode == STEP) && ((millis - last_step) > STEP_TIME)) {
        // While in step mode, after each step interval turn off the current LED and turn on the next one
        last_step = millis;
        LED_OFF(*test_ports[current_pin], test_nums[current_pin]);
        current_pin++;
        if (current_pin == NUM_PINS) {
            current_pin = 0;
            current_mode = HOLD;    // Switch to hold mode if all pins have been cycled through
        }
        LED_ON(*test_ports[current_pin], test_nums[current_pin]);
    } else if ((current_mode == HOLD)  && ((millis - last_step) > HOLD_TIME)) {
        // Once the hold time has passed, switch back to step mode, starting with the first LED on
        last_step = millis;
        LED_OFF(*test_ports[current_pin], test_nums[current_pin]);
        current_mode = STEP;
        current_pin = 0;
        LED_ON(*test_ports[current_pin], test_nums[current_pin]);
    } else if (current_mode == HOLD) {
        // While in hold mode, before HOLD_TIME has ellapsed, switch LEDs on each itteration of the loop
        LED_OFF(*test_ports[current_pin], test_nums[current_pin]);
        current_pin++;
        if (current_pin == NUM_PINS) {
            current_pin = 0;
        }
        LED_ON(*test_ports[current_pin], test_nums[current_pin]);
    }
}


//Interrupt service routines
ISR (TIMER0_COMPA_vect) { 					//timer0 overflow interrupt (called every millisecond)
    millis++; 								//Increment clock every millisecond
}
