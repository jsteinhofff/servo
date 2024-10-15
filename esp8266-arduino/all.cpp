#include <ESP8266WiFi.h>


int debug = 1;
int interrupt_counter = 0;
uint32_t interrupt_ticks = 0;
uint32_t interrupt_spacing = 0;

#define ENCODER_A 4
#define ENCODER_B 5
#define BUTTON_RESET 12
#define MOTOR_CONTROL_1 13
#define MOTOR_CONTROL_2 14


#include "utils.hpp"
#include "buttons.hpp"
#include "encoder.hpp"
#include "motor.hpp"





ICACHE_RAM_ATTR void interrupt_handler() {
    uint32_t begin_ticks = get_clock_ticks();

    interrupt_counter += 1;
    encoder_step();

    float encoder_increments_per_rotation = 7.0;
    float gear_ratio = 1.0 / 50.0;
    float pi = 3.1415193;

    float current_position = (float) encoder_increments / encoder_increments_per_rotation / gear_ratio * 2 * pi;
    motor_step(get_clock_ticks(), current_position);

    digitalWrite(MOTOR_CONTROL_1, motor_out_a ? HIGH : LOW);
    digitalWrite(MOTOR_CONTROL_2, motor_out_b ? HIGH : LOW);

    uint32_t end_ticks = get_clock_ticks();
    // difference is correct, even if end_ticks has overflown and begin_ticks didnt
    interrupt_ticks = end_ticks - begin_ticks;
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
    
    timer1_attachInterrupt(interrupt_handler);

    // 80 MHz / 256 = 312.5Khz (1 tick = 3.2us)
    timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);
    uint32_t timer_ticks = 1000;
    timer1_write(timer_ticks);
    interrupt_spacing = timer_ticks * 256;

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
    Serial.printf("Interrupt ticks: %u corresponds to %d percent load\n", interrupt_ticks, interrupt_ticks * 100 / interrupt_spacing);
    if (state == STATE_INITIAL)
    {
        int button = detect_button();
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
        motor_target_rad = 100.0;
        state = STATE_INITIAL;
    }
    else if (state == STATE_2)
    {
        
        motor_target_rad = 200.0;
        state = STATE_INITIAL;
    }
    else if (state == STATE_3)
    {
        Serial.print("Interrupt counter: ");
        Serial.println(interrupt_counter);
        Serial.print("Encoder angle: ");
        Serial.println(encoder_increments);
        interrupt_counter = 0;
        motor_target_rad = 0.0;
        state = STATE_INITIAL;
    }
    yield;
}

/*



*/