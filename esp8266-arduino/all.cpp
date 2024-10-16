#include <ESP8266WiFi.h>


int debug = 1;
bool run_tests = true;
bool welcome_shown = false;
volatile int interrupt_counter = 0;
volatile uint32_t interrupt_ticks = 0;
volatile uint32_t interrupt_spacing = 0;
volatile float current_position = 0.0;


#define ENCODER_A 4
#define ENCODER_B 5
#define BUTTON_RESET 12
#define MOTOR_CONTROL_1 13
#define MOTOR_CONTROL_2 14


#include "utils.hpp"
#include "fixpoint.hpp"
#include "buttons.hpp"
#include "encoder.hpp"
#include "motor.hpp"





ICACHE_RAM_ATTR void interrupt_handler() {
    uint32_t begin_ticks = get_clock_ticks();

    interrupt_counter += 1;
    encoder_step();

    // The way we are interpreting the encoder signals we get 4 increments per magnetic pulse
    float encoder_increments_per_rotation = 7.0 * 4;
    float gear_ratio = 29.125; // from datasheet it is 30, but when i wind it 10x i observe 8155 increments
    float pi = 3.1415193;

    current_position = (float) encoder_increments / encoder_increments_per_rotation / gear_ratio * 2.0 * pi;
    motor_step(get_clock_ticks(), current_position);

    digitalWrite(MOTOR_CONTROL_1, motor_out_a ? HIGH : LOW);
    digitalWrite(MOTOR_CONTROL_2, motor_out_b ? HIGH : LOW);

    uint32_t end_ticks = get_clock_ticks();
    // difference is correct, even if end_ticks has overflown and begin_ticks didnt
    interrupt_ticks = end_ticks - begin_ticks;
}


uint32_t setup_end_ticks = 0;

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

    if (run_tests) {
        test_fixpoint();
    }
    
    timer1_attachInterrupt(interrupt_handler);

    // 80 MHz / 256 = 312.5Khz (1 tick = 3.2us)
    timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);
    uint32_t timer_ticks = 10;
    timer1_write(timer_ticks);
    interrupt_spacing = timer_ticks * 256;
    // for 10 ticks counter with encoder, PID and motor
    //   interrupt ticks: 1476 from 2560 corresponding to 57.656250 % load

    pinMode(BUTTON_RESET, OUTPUT);
    digitalWrite(BUTTON_RESET, LOW);

    pinMode(MOTOR_CONTROL_1, OUTPUT);
    digitalWrite(MOTOR_CONTROL_1, LOW);

    pinMode(MOTOR_CONTROL_2, OUTPUT);
    digitalWrite(MOTOR_CONTROL_2, LOW);

    Serial.println("Setup complete");
    setup_end_ticks = get_clock_ticks();
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
    Serial.printf(".");

    uint32_t local_ticks = interrupt_ticks; // get a local copy such that interrupt does not change value inbetween output
    float interrupt_load = local_ticks * 100.0 / interrupt_spacing;

    if (!welcome_shown && (get_clock_ticks() > setup_end_ticks + 8000000)) {
        // Show welcome message 100 ms after end of setup (to give interrupt chance to run)
        Serial.printf("Welcome!\n");
        Serial.printf("Running software from %s\n", __TIMESTAMP__);
        Serial.printf("Interrupt ticks are %u from %u corresponding to %f %% load\n", local_ticks, interrupt_spacing, interrupt_load);
        welcome_shown = true;
    }

    if (interrupt_load > 30.0) {
        Serial.printf("HIGH INTERRUPT LOAD, interrupt ticks: %u corresponding to %f %% load\n", local_ticks, interrupt_load);
    }

    if (state == STATE_INITIAL)
    {
        int button = detect_button();
        //int button = 0;
        
        if (button == 0)
        {
            delay(100);
        }
        else
        {
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
        motor_target_rad = 50.0;

        for (int i = 0; i < 100; i++) {
            Serial.printf("Pos %f , delta: %f , prop: %f, motor A: %d, motor B: %d\n", current_position, motor_position_delta, pid_proportional, motor_out_a, motor_out_b);
            delay(500);
        }

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