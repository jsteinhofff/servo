


#define ENCODER_A 4
#define ENCODER_B 5
#define BUTTON_RESET 12
#define MOTOR_CONTROL_1 13
#define MOTOR_CONTROL_2 14


// direct access to input ports as register
// see https://github.com/esp8266/esp8266-wiki/wiki/gpio-registers
// and https://www.embedded.com/device-registers-in-c/
#define PIN_IN (*(uint32_t *)0x60000318)
#define PIN_OUT (*(uint32_t *)0x60000300)


// Access the CPU Clock counter
// taken from https://sub.nanona.fi/esp8266/timing-and-ticks.html
// Note: Overflows every 53.6870912 seconds!
ICACHE_RAM_ATTR static inline uint32_t get_clock_ticks(void)
{
    uint32_t r;
    asm volatile("rsr %0, ccount" : "=r"(r));
    return r;
}

static const uint32_t ticks_per_second = 80000000;

float pi = 3.141593;

float sign(float x) {
    return (x < 0.0) ? -1 : 1;
}

// from https://stackoverflow.com/a/3208376
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')


#define EXTRACT_BIT(value, bit) ((value >> bit) & 1)

#define SET_BIT_IF(value, bit, condition) \
    if (condition) { \
        value |= 1 << bit; \
    } else { \
        value &= ~(1 << bit); \
    }

