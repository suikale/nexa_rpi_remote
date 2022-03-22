/*
    Crude remote control software for Attiny85 which controls Nexa MYCR-1000 wireless plugs. 
    Receives 2 or 5 bytes via SPI

    for hardcoded remote ID
        1: [rrrrrrrr]
        2: [00ggddss]

    for custom remote ID
        1: [00000000]
        2: [rrrrrrrr]
        3: [rrrrrrrr]
        4: [rrrrrrrr]
        5: [rrggddss] 
      where 
        r: remote id
        g: group
        d: device
        s: state

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
// maximum size for buffer
#define BUFFER_MAX_SIZE 5

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// FIFO ring buffer
int8_t buffer_size = 0;
uint8_t buffer_read = 0;
uint8_t buffer_write = -1;
uint8_t buffer[BUFFER_MAX_SIZE];

// payload consists of these bytes
const uint8_t byte[4][4] = {{0, 1, 0, 1}, {0, 1, 1, 0}, {1, 0, 0, 1}, {1, 0, 1, 0}};

// IDs for hardcoded remotes
volatile uint8_t remote_id[][13] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // reserved for custom remote ID
    {2, 0, 1, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2}, // 1st hard coded remote ID
    {2, 1, 0, 1, 0, 2, 2, 3, 0, 0, 1, 3, 2}, // 2nd hard coded remote ID
    {2, 1, 0, 1, 3, 0, 2, 3, 0, 3, 2, 2, 2}  // 2rd hard coded remote ID
};
// number of remotes
const uint8_t remote_max = sizeof(remote_id) / sizeof(remote_id[0]);

/*
    FIFO buffer operations
*/
void push(uint8_t a)
{
    if (buffer_size < BUFFER_MAX_SIZE)
    {
        buffer[++buffer_write] = a;
        buffer_size++;

        if (buffer_write == BUFFER_MAX_SIZE - 1)
        {
            buffer_write = -1;
        }
    }
}

uint8_t pop()
{
    if (buffer_size > 0)
    {
        uint8_t a = buffer[buffer_read++];
        buffer_size--;

        if (buffer_read == BUFFER_MAX_SIZE)
        {
            buffer_read = 0;
        }
        
        return a;
    }
    // TODO: maybe find a better way to differentiate errors
    return 0;
}

uint8_t is_empty()
{
    return buffer_size == 0;
}

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
    send payload: [remote id][state][group][device id]. Repeats REPEAT times
*/
void send(uint8_t state, uint8_t device, uint8_t group, uint8_t remote)
{
    if (remote > remote_max)
    {
        return;
    }

    for (uint8_t i = 0; i < REPEAT; i++)
    {
        // init bit
        send_bit(-1);

        // remote id
        for (uint8_t i = 0; i < sizeof(remote_id[0]); i++)
            send_byte(byte[remote_id[remote][i]]);

        // payload bytes
        send_byte(byte[state & 3]);
        send_byte(byte[group & 3]);
        send_byte(byte[device & 3]);

        // end
        PORTB |= (1 << ANT_PIN);
        _delay_us(SHORT_DELAY);
        PORTB &= ~(1 << ANT_PIN);

        // add some delay between repeats
        _delay_us(REPEAT_DELAY);
    }
}

void stop_timer()
{
    TIMSK ^= (1<<TOIE0); // disable timer0 interrupt
}

/*
    triggers an interrupt after timer runs out
    255 * (1/8000000) * 1024 = 0.03264 seconds
*/
void start_timer()
{   
    TCNT0 = 0xff;  // set timer value to 255
    TIMSK |= (1<<TOIE0); // enable timer0 interrupt
}


/*
    handles the values in buffer
*/
void buffer_handler()
{
    uint8_t remote, device, group, state, data;
    remote = pop();
    data = pop();

    if (remote)
    {
        /*
            using hardcoded remote ID
            [rrrrrrrr]
            [00ggddss]
        */
        group  = (data >> 4) & 3;
        device = (data >> 2) & 3;
        state  = (data >> 0) & 3;
    }
    else
    {
        /*
            using custom remote ID
            [00000000]
            [rrrrrrrr]
            [rrrrrrrr]
            [rrrrrrrr]
            [rrggddss] 
        */
        remote_id[0][0] = (data >> 6) & 3;
        remote_id[0][1] = (data >> 4) & 3;
        remote_id[0][2] = (data >> 2) & 3;
        remote_id[0][3] = (data >> 0) & 3;

        data = pop();
        remote_id[0][4] = (data >> 6) & 3;
        remote_id[0][5] = (data >> 4) & 3;
        remote_id[0][6] = (data >> 2) & 3;
        remote_id[0][7] = (data >> 0) & 3;

        data = pop();
        remote_id[0][8]  = (data >> 6) & 3;
        remote_id[0][9]  = (data >> 4) & 3;
        remote_id[0][10] = (data >> 2) & 3;
        remote_id[0][11] = (data >> 0) & 3;

        data = pop();
        remote_id[0][12] = (data >> 6) & 3;
        group            = (data >> 4) & 3;
        device           = (data >> 2) & 3;
        state            = (data >> 0) & 3;
    }

    send(state, device, group, remote);
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

    // SPI
    USICR |= (1 << USIWM0);   // enable SPI
    USICR |= (1 << USICS1);   // enable external clock
    USICR |= (1 << USIOIE);   // enable SPI interrupts

    // timer
    TCCR0A = 0; // COM0{A,B}{0,1} = 0, normal operation    
    TCCR0B = 0|(1<<CS00)|(1<<CS02); // prescaler: 1024

    sei(); // enable global interrupts
}

/*
    SPI interrupt handler
    see datasheet section 15.3.3
    https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf
*/
ISR(USI_OVF_vect)
{
    // USIDR contains received byte
    push(USIDR);

    // overflow bit
    USISR = (1 << USIOIF);

    // wait for transmission to end
    loop_until_bit_is_clear(USISR, USIOIF);

    start_timer();
}

/*
    Timer interrupt handler.
    Waits 4 cycles before unloading buffer to give SPI operations time
    
    TODO: find a reliable way to find out when the transmission is over
          if receiving more than 5 bytes at 250 KHz
*/
uint8_t counter = 0;
ISR(TIMER0_OVF_vect)
{
    counter++;
    if (counter == 4)
    {
        stop_timer();
        buffer_handler();
        counter = 0;
    }
}

int main(void)
{
    setup();

    while (1) { /* wait for interrupts */ }

    return 0;
}
