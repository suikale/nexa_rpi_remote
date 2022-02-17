/*
    Crude remote control software
*/

#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>

// GPIO pin connected to antenna
#define ANT_PIN     2
// delays is µs
#define SHORT_DELAY 250
#define LONG_DELAY  1250
#define INIT_DELAY  2500
#define REPEAT_DELAY 10000

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
    gpioWrite(ANT_PIN, 1);
    usleep(SHORT_DELAY);
    gpioWrite(ANT_PIN, 0);
    if (bit == 0)
    {
        usleep(SHORT_DELAY);
    }
    else if (bit == 1)
    {
        usleep(LONG_DELAY);
    }
    else
    {
        usleep(INIT_DELAY);
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
        gpioWrite(ANT_PIN, 1);
        usleep(SHORT_DELAY);
        gpioWrite(ANT_PIN, 0);

        // add some delay between repeats
        usleep(REPEAT_DELAY);
    }
}

int setup(void)
{
    if (gpioInitialise() < 0 || gpioSetMode(ANT_PIN, PI_OUTPUT) < 0)
    {
        return -1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (setup() < 0)
    {
        printf("init failed");
        return -1;
    }

    send(0, 0, 0, 6);

    return 0;
}
