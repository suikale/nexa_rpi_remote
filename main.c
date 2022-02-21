/*
    Crude remote control software. Currently receives 8 bits via SPI
    [rrggddss] = [remote][group][device][state]

    Pinout
    RPI | Attiny 
    15  | 1   | Reset 
    17  | 8   | 3.3v
    19  | 5   | MOSI
    21  | 6   | MISO
    23  | 7   | SCLK
    25  | 4   | GND
*/

// avr clock, 8 MHz
#define F_CPU 8000000UL
// spi and antenna pins
#define MOSI    PB0
#define MISO    PB1
#define SCK     PB2
#define ANT_PIN PB3
// delays in µs
#define SHORT_DELAY 250
#define LONG_DELAY    5 * SHORT_DELAY
#define INIT_DELAY   10 * SHORT_DELAY
#define REPEAT_DELAY 40 * SHORT_DELAY
// repeat signal this many times
#define REPEAT 6

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

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
    send payload based on state, device id, group. repeats n times
    TODO: different remotes 
*/
void send(uint8_t state, uint8_t id, uint8_t group, uint8_t remote)
{
    // 1001 [11 bytes for remote id] 1001
    const uint8_t initial_bytes[13] = {2, 0, 1, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2};

    for (uint8_t i = 0; i < REPEAT; i++)
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

/*
    setup i/o
*/
void setup()
{
    // MISO, ANT_PIN as output
    DDRB |= (1 << MISO);
    DDRB |= (1 << ANT_PIN);
    // MOSI, SCK as input
    DDRB &= ~(1 << MOSI);
    DDRB &= ~(1 << SCK);

    USICR |= (1 << USIWM0);   // enable SPI
    USICR |= (1 << USICS1);   // enable external clock
    USICR |= (1 << USIOIE);   // enable interrupts

    sei(); // set interrupts on
}

/*
    interrupt handler
    see datasheet section 15.3.3
    https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf
*/
ISR(USI_OVF_vect)
{
    // USIDR contains received byte
    uint8_t get = USIDR;

    uint8_t state  = (uint8_t)(get & 3);
    uint8_t device = (uint8_t)((get >> 2) & 3);
    uint8_t group  = (uint8_t)((get >> 4) & 3);
    uint8_t remote = (uint8_t)((get >> 6) & 3);

    send(state, device, group, remote);

    // overflow bit
    USISR = (1 << USIOIF);

    loop_until_bit_is_clear(USISR, USIOIF);
}

int main(void)
{
    setup();

    while (1) { /* wait for interrupts */ }

    return 0;
}
