
// Access the CPU Clock counter
// taken from https://sub.nanona.fi/esp8266/timing-and-ticks.html
// Note: Overflows every 53.6870912 seconds!
ICACHE_RAM_ATTR static inline uint32_t getClockTicks(void)
{
    uint32_t r;
    asm volatile("rsr %0, ccount" : "=r"(r));
    return r;
}
