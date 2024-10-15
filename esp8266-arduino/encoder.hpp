

int encoder_increments = 0;

ICACHE_RAM_ATTR inline void encoder_step()
{
    // the old value of the sensor ports
    static unsigned char ab = 0;

    // increment of angle for the 16 possible bit codes
    const int table[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

    unsigned short port = 0;
    port |= digitalRead(ENCODER_A) << 0;
    port |= digitalRead(ENCODER_B) << 1;

    ab = ab << 2;
    ab |= (port & 0x3);
    encoder_increments += table[(ab & 0xf)];
}

