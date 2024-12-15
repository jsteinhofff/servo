

int encoder_increments = 0;


// increment of angle for the 16 possible bit codes
const int encoder_table[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

// little bit optimized version:
// https://godbolt.org/z/e38TPn6an

ICACHE_RAM_ATTR inline void encoder_step()
{
    // the old value of the sensor ports
    static unsigned char ab = 0;

    ab = ab << 2;

    uint32_t inputs = PIN_IN;
    ab |= EXTRACT_BIT(inputs, ENCODER_A) << 0;
    ab |= EXTRACT_BIT(inputs, ENCODER_B) << 1;

    encoder_increments += encoder_table[(ab & 0xf)];
}

