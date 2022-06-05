// Copy/paste your code from assign5/ps2.c as starting point
#include "gpio.h"
#include "gpio_extra.h"
#include "malloc.h"
#include "ps2.h"
#include "gpio_interrupts.h"
#include "interrupts.h"
#include "ringbuffer.h"
#include "uart.h"
#include "timer.h"
#include "printf.h"


// Since the ps2.h API only uses this struct as a pointer, its
// definition can be implementation-specific and deferred until
// here.
struct ps2_device {
    unsigned int clock; // GPIO pin number of the clock line
    unsigned int data;  // GPIO pin number of the data line
    unsigned int buf; // Buffer to store input of scanner code 
    unsigned int ticker; // Used to keep track of the number of bits added to buf
    unsigned int last_press_time; // Stores the previous time clock down occured
    rb_t* ring_buffer;
    rb_t* time_ring_buffer;
};

#define TIME_OUT_DELAY 1000

// Get i-th LSB
#define bitfl(num, i) ((num >> i) & 1)

// Interprets the code stored in ps2_device_t buffer, and adds to the RBQ
// if the code in the buffer is valid. Returns whether the code is valid.
void interpret_code(ps2_device_t *dev)
{
    unsigned int valid = 1;

    // Check that first bit is 0.
    if (bitfl(dev->buf, 10) != 0) valid = 0;

    // Read input for the message bits
    unsigned char input = 0;
    unsigned char parity = 0;

    for (int i = 2; i < 10; i++) {
        input <<= 1;
        unsigned char bit = bitfl(dev->buf, i);
        parity += bit;
        input += bit; 
    }

    // Check that the amount of total bits (including parity) is odd.
    parity += bitfl(dev->buf, 1);
    if (parity % 2 == 0) valid = 0;
    
    // Check that last bit is 1
    if (bitfl(dev->buf, 0) != 1) valid = 0;

    if (valid) {
        rb_enqueue(dev->ring_buffer, input);
        // rb_enqueue(dev->time_ring_buffer, );
    } 
}

// Handles each falling clock edge called through an interrupt.
void handle_falling(unsigned int pc, void *aux_data) {
    ps2_device_t* dev = (ps2_device_t*) aux_data;

    // If last clock down was before 
    unsigned int curr_time = timer_get_ticks();
    if (curr_time > dev->last_press_time + TIME_OUT_DELAY) dev->ticker = 0;
    dev->last_press_time = curr_time;

    // Read bit and add to buffer
    int curr_bit = gpio_read(dev->data);
    dev->buf <<= 1;
    dev->buf |= curr_bit;

    // Interpret code if we have collected 11 bits in buf
    dev->ticker++;
    if (dev->ticker == 11) {
        interpret_code(dev);
    }
    dev->ticker%=11;
    
    gpio_check_and_clear_event(((ps2_device_t *) aux_data)->clock);
}

ps2_device_t *ps2_new(unsigned int clock_gpio, unsigned int data_gpio)
{
    ps2_device_t *dev = malloc(sizeof(*dev));
    dev->ring_buffer = rb_new();
    dev->last_press_time = timer_get_ticks();
    dev->ticker = 0;
    dev->buf = 0;
    dev->data = data_gpio;
    dev->clock = clock_gpio;

    // Configure GPIO 
    gpio_init();
    gpio_interrupts_init();
    gpio_set_input(clock_gpio); // configure button
    gpio_set_pullup(clock_gpio);
    gpio_set_input(data_gpio);
    gpio_set_pullup(data_gpio);

    // Configure interrupts handler
    gpio_interrupts_enable();
    gpio_enable_event_detection(clock_gpio, GPIO_DETECT_FALLING_EDGE);
    gpio_interrupts_register_handler(clock_gpio, handle_falling, dev);

    return dev;
}

// Read a single PS2 scan code. Always returns a correctly received scan code:
// if an error occurs (e.g., start bit not detected, parity is wrong), the
// function should read another scan code.
unsigned char ps2_read(ps2_device_t *dev)
{
    while (rb_empty(dev->ring_buffer)) { }
    int elem = 0;
    rb_dequeue(dev->ring_buffer, &elem);
    return (unsigned char) elem;
} 