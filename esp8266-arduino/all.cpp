#include <ESP8266WiFi.h>


int debug = 1;
int counter = 0;
uint32_t interruptTicks = 0;
uint32_t interruptSpacing = 0;

#define ENCODER_A 4
#define ENCODER_B 5
#define BUTTON_RESET 12
#define MOTOR_CONTROL_1 13
#define MOTOR_CONTROL_2 14


#include "utils.hpp"
#include "buttons.hpp"
#include "encoder.hpp"






ICACHE_RAM_ATTR void interruptHandler() {
    uint32_t beginTicks = getClockTicks();
    counter += 1;

    updateEncoder();
    uint32_t endTicks = getClockTicks();

    // works, even if endTicks has overflown and beginTicks didnt
    interruptTicks = endTicks - beginTicks;
}


void setup()
{

    WiFi.mode(WIFI_OFF);

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(A0, INPUT);

    Serial.begin(74880);
    Serial.setTimeout(2000);

    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(1000);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
    delay(1000);

    pinMode(ENCODER_A, INPUT_PULLUP);
    pinMode(ENCODER_B, INPUT_PULLUP);
    
    timer1_attachInterrupt(interruptHandler);

    // 80 MHz / 256 = 312.5Khz (1 tick = 3.2us)
    timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);
    uint32_t timerTicks = 10;
    timer1_write(timerTicks);
    interruptSpacing = timerTicks * 256;

    pinMode(BUTTON_RESET, OUTPUT);
    digitalWrite(BUTTON_RESET, LOW);

    pinMode(MOTOR_CONTROL_1, OUTPUT);
    digitalWrite(MOTOR_CONTROL_1, LOW);

    pinMode(MOTOR_CONTROL_2, OUTPUT);
    digitalWrite(MOTOR_CONTROL_2, LOW);

    Serial.println("Started.");
}

enum State
{
    STATE_INITIAL = 1,
    STATE_1 = 2,
    STATE_2 = 3,
    STATE_3 = 4,
};

enum State state = STATE_INITIAL;

void loop()
{
    Serial.printf("Interrupt ticks: %u corresponds to %d percent load\n", interruptTicks, interruptTicks * 100 / interruptSpacing);
    if (state == STATE_INITIAL)
    {
        int button = detectButton();
        if (button == 0)
        {
            delay(100);
        }
        else
        {
            Serial.println(__TIMESTAMP__);
            if (button == 1)
            {
                state = STATE_1;
            }
            else if (button == 2)
            {
                state = STATE_2;
            }
            else if (button == 3)
            {
                state = STATE_3;
            }
            digitalWrite(BUTTON_RESET, HIGH);
            delay(200);
            digitalWrite(BUTTON_RESET, LOW);
        }
    }
    else if (state == STATE_1)
    {
        angle = 0;
        digitalWrite(MOTOR_CONTROL_1, HIGH);
        digitalWrite(MOTOR_CONTROL_2, LOW);
        delay(1000);
        digitalWrite(MOTOR_CONTROL_1, LOW);
        digitalWrite(MOTOR_CONTROL_2, LOW);
        state = STATE_INITIAL;
    }
    else if (state == STATE_2)
    {
        digitalWrite(MOTOR_CONTROL_1, LOW);
        digitalWrite(MOTOR_CONTROL_2, HIGH);
        delay(1000);
        digitalWrite(MOTOR_CONTROL_1, LOW);
        digitalWrite(MOTOR_CONTROL_2, LOW);
        state = STATE_INITIAL;
    }
    else if (state == STATE_3)
    {
        Serial.print("Counter: ");
        Serial.println(counter);
        Serial.print("Angle: ");
        Serial.println(angle);
        counter = 0;
        state = STATE_INITIAL;
    }
    yield;
}

/*



*/