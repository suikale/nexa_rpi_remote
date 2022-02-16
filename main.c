#include <stdio.h>
#include <pigpio.h>

#define ANT 0

// accepted bytes
int byte[4][4] = {{0, 1, 0, 1}, {0, 1, 1, 0}, {1, 0, 0, 1}, {1, 0, 1, 0}};

void send_bit(int bit)
{
    gpioWrite(ANT, 1);
    // sleep 250µs
    gpioWrite(ANT, 0);
    if (bit == 0)
    {
        // sleep 250
    }
    else if (bit == 1)
    {
        // sleep 1250
    }
    else
    {
        // sleep 2500
    }
}

void send_byte(int* byte)
{
    for (int i = 0; i < 4; i++)
    {
        send_bit(byte[i]);
    }
}

int setup()
{
    int r = gpioInitialise();
    if (r < 0)
    {
        return r;
    }
    r = gpioSetMode(ANT, PI_OUTPUT);
    if (r < 0)
    {
        return r;
    }
    return 0;
}

int main(int argc, char** argv)
{
    int s = setup();
    if (s < 0)
    {
        printf("init failed");
        return s;
    }

    for (int i = 0; i < 4; i++)
    {
        send_byte(byte[i]);
    }

    printf("jee");
    return 0;
}
