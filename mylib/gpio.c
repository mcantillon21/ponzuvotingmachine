#include "gpio.h"

volatile unsigned int * const FSEL0 = (unsigned int *) 0x20200000;
volatile unsigned int * const FSEL1 = (unsigned int *) 0x20200004;
volatile unsigned int * const FSEL2 = (unsigned int *) 0x20200008;
volatile unsigned int * const FSEL3 = (unsigned int *) 0x2020000C;
volatile unsigned int * const FSEL4 = (unsigned int *) 0x20200010;
volatile unsigned int * const FSEL5 = (unsigned int *) 0x20200014;
volatile unsigned int * const SET0 = (unsigned int *) 0x2020001C;
volatile unsigned int * const SET1 = (unsigned int *) 0x20200020;
volatile unsigned int * const CLR0 = (unsigned int *) 0x20200028;
volatile unsigned int * const CLR1 = (unsigned int *) 0x2020002C;
volatile unsigned int * const LEV0 = (unsigned int *) 0x20200034;
volatile unsigned int * const LEV1 = (unsigned int *) 0x20200038;
volatile unsigned int * const FSELS[6] = {FSEL0, FSEL1, FSEL2, FSEL3, FSEL4, FSEL5};
volatile unsigned int * const SETS[2] = {SET0, SET1};
volatile unsigned int * const CLRS[2] = {CLR0, CLR1};
volatile unsigned int * const LEVS[2] = {LEV0, LEV1};

void gpio_init(void) {
    // no initialization required for this peripheral
}

void gpio_set_function(unsigned int pin, unsigned int function) {
    if (function >= 4 || pin > GPIO_PIN_LAST) return;
    unsigned int current_fsel = *(FSELS[pin / 10]); 
	unsigned int where_pin = pin % 10;
	unsigned int reset_mask = ~(0b111 << (where_pin * 3));
	current_fsel = current_fsel & reset_mask;
	unsigned int function_mask = function << (where_pin * 3);
	current_fsel = current_fsel | function_mask;
	*(FSELS[pin / 10]) = current_fsel;
}

unsigned int gpio_get_function(unsigned int pin) {
    if (pin > GPIO_PIN_LAST) return 0;
	unsigned int pinNumber = pin / 10;	
	unsigned int function = *(FSELS[pinNumber]);
	unsigned int where_pin = pin % 10;
	return (function >> (where_pin * 3)) & 0b111;
}

void gpio_set_input(unsigned int pin) {
	gpio_set_function(pin, GPIO_FUNC_INPUT);
}

void gpio_set_output(unsigned int pin) {
	gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

void gpio_write(unsigned int pin, unsigned int value) {
    if (pin > GPIO_PIN_LAST) return;
	unsigned int where_pin = pin % 32;
	if (value == 1) {
		*(SETS[pin / 32]) = 1 << where_pin;
	} else {
		*(CLRS[pin / 32]) = 1 << where_pin;
	}
}

unsigned int gpio_read(unsigned int pin) {
    if (pin > GPIO_PIN_LAST) return 0;
	unsigned int where_pin = pin % 32;
	return ((*(LEVS[pin / 32]) >> where_pin) & 1);
}
