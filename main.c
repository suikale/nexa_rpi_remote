/*
    Crude remote control software
*/

// avr clock, 8 MHz
#define F_CPU 8000000UL
// GPIO pin connected to antenna
#define ANT_PIN     PB3
// delays is µs
#define SHORT_DELAY 250
#define LONG_DELAY    5 * SHORT_DELAY
#define INIT_DELAY   10 * SHORT_DELAY
#define REPEAT_DELAY 40 * SHORT_DELAY

#include <avr/io.h>
#include <util/delay.h>

// payload consists of these bytes
const uint8_t byte[4][4] = {{0, 1, 0, 1}, {0, 1, 1, 0}, {1, 0, 0, 1}, {1, 0, 1, 0}};

/*
    sends a single bit
    0: 250 µs ON,  250 µs OFF
    1: 250 µs ON, 1250 µs OFF
 init: 250 µs ON, 2500 µs OFF
*/
void send_bit(const uint8_t bit)
{
    PORTB |= (1 << ANT_PIN);  // high
    _delay_us(SHORT_DELAY);
    PORTB &= ~(1 << ANT_PIN); // low
    if (bit == 0)
    {
        _delay_us(SHORT_DELAY);
    }
    else if (bit == 1)
    {
        _delay_us(LONG_DELAY);
    }
    else
    {
        _delay_us(INIT_DELAY);
    }
}

/*
    sends a byte which is 4 bits
*/
void send_byte(const uint8_t* byte)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        send_bit(byte[i]);
    }
}

/*
    send payload based on state, id, group. repeats n times
*/
void send(uint8_t state, uint8_t id, uint8_t group, uint8_t repeat)
{
    // 1001 [11 bytes for remote id] 1001
    const uint8_t initial_bytes[13] = {2, 0, 1, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2};

    for (uint8_t i = 0; i < repeat; i++)
    {
        // init bit
        send_bit(-1);

        // remote id etc
        for (uint8_t i = 0; i < 13; i++)
            send_byte(byte[initial_bytes[i]]);

        // payload bytes
        send_byte(byte[state & 3]);
        send_byte(byte[group & 3]);
        send_byte(byte[id & 3]);

        // end
        PORTB |= (1 << ANT_PIN);
        _delay_us(SHORT_DELAY);
        PORTB &= ~(1 << ANT_PIN);

        // add some delay between repeats
        _delay_us(REPEAT_DELAY);
    }
}

int main(int argc, char** argv)
{
    // set ANT_PIN as output
    DDRB |= (1 << ANT_PIN);

    send(1, 0, 0, 6);

    return 0;
}
