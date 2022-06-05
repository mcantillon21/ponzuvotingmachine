#include "keyboard.h"
#include "ps2.h"
#include "ps2_keys.h"
#include "printf.h"

ps2_return ps2_read_time(ps2_device_t *dev);


static ps2_device_t *dev;
static keyboard_modifiers_t modifiers;
static bool keys_active[256];

// For testing
keyboard_modifiers_t get_modifier(void) {
    return modifiers;
}

void keyboard_init(unsigned int clock_gpio, unsigned int data_gpio)
{
    dev = ps2_new(clock_gpio, data_gpio);
    modifiers = 0;
}

ps2_return keyboard_read_scancode(void)
{ 
    return ps2_read_time(dev);
}

key_action_t keyboard_read_sequence(void)
{
    // 1 BYTE SEQUENCE
    ps2_return first_out = ps2_read_time(dev);
    unsigned char first = first_out.elem;
    unsigned int time = first_out.time;
    // Case: Sequence is a Normal Press 
    if (first != 0xE0 && first != 0xF0) {
        key_action_t action = {
            0,
            first,
            time
        };
        return action;
    } 

    // 2 BYTE SEQUENCES
    ps2_return second_out = ps2_read_time(dev);
    unsigned char second = second_out.elem;
    if (second != 0xF0) {
        // Case: Sequence is a Special Key Press 
        if (first == 0xE0) {
            key_action_t action = {
                0,
                second,
                time
            };
            return action;
        }
        // Case: Sequence is a Normal Key Release
        else {
            key_action_t action = {
                1,
                second,
                time
            };
            return action;
        }
    }

    // 3 BYTE SEQUENCE
    // Case: Sequence is a Special Key Release 
    ps2_return third_out = ps2_read_time(dev);
    unsigned char third = third_out.elem;
    key_action_t action = {
        1,
        third,
        time
    };
    return action;
}

// helper function:
// Given the keyboard modifier code,
// and the key action, change modifier
void handle_modifier(int code, int what) {
    if (what == KEY_PRESS) {
        modifiers |= code;
    }
    else {
        modifiers &= ~code;
    }
}

key_event_t keyboard_read_event(void)
{
    key_action_t action;
    ps2_key_t code;
    unsigned int time;

    // Handle events in the case that it is a modifier key event
    // The loop breaks when the first non-modifier key event occurs
    while (1) {        
        action = keyboard_read_sequence();
        code = ps2_keys[action.keycode];
        time = action.time;

        // If current event is a modifier event, make changes to modifier
        // otherwise, exit loop and return the event
        bool exit = false;
        switch (code.ch) {
            case PS2_KEY_CTRL:
                handle_modifier(KEYBOARD_MOD_CTRL, action.what);
                break;
            case PS2_KEY_ALT:
                handle_modifier(KEYBOARD_MOD_ALT, action.what);
                break;
            case PS2_KEY_SHIFT:
                handle_modifier(KEYBOARD_MOD_SHIFT, action.what);
                break;
            case PS2_KEY_CAPS_LOCK:
                if (action.what == KEY_PRESS && keys_active[action.keycode]) {
                    modifiers ^= KEYBOARD_MOD_CAPS_LOCK;
                    keys_active[action.keycode] = true;
                } else if (action.what == KEY_RELEASE) {
                    keys_active[action.keycode] = false;
                }
                break;
            default:
                exit = true; 
        }

        if (exit) break;
    }

    // Return the first non-modifier event
    key_event_t event = {
        action, code, modifiers, time
    };
    return event;
}

#define is_lower_alpha(x) (x >= 97 && x <= 122)

key_out_t keyboard_read_next(void)
{
    key_event_t kd_event;
    // Wait until receive a keydown event 
    while (1) {
        kd_event = keyboard_read_event();
        if (kd_event.action.what == KEY_PRESS && !keys_active[kd_event.action.keycode]) {
            keys_active[kd_event.action.keycode] = true;
            break;
        }
        else if (kd_event.action.what == KEY_RELEASE) {
            keys_active[kd_event.action.keycode] = false; 
        }
    }

    key_out_t key_out;
    key_out.time = kd_event.time;

    // If Shift pressed, return upper
    if (kd_event.modifiers & KEYBOARD_MOD_SHIFT) {
        key_out.elem = kd_event.key.other_ch;
    } 
    // If Caps enabled, return upper for only alpha numeric
    else if (kd_event.modifiers & KEYBOARD_MOD_CAPS_LOCK && is_lower_alpha(kd_event.key.ch)) {
        key_out.elem = kd_event.key.other_ch;
    }
    // return lower
    else {
        key_out.elem = kd_event.key.ch;
    }
    return key_out;
}
