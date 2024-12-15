#include <ESP8266WiFi.h>


int debug = 1;
bool run_tests = true;
bool welcome_shown = false;
volatile int interrupt_counter = 0;
volatile uint32_t interrupt_ticks = 0;
volatile uint32_t interrupt_spacing = 0;
float interrupt_spacing_s = 0.0;
volatile uint32_t ramp_interrupt_counter = 0;
volatile uint32_t acceleration_end_interrupt_count = 0;
volatile uint32_t constant_velocity_end_interrupt_count = 0;
volatile uint32_t deceleration_end_interrupt_count = 0;

volatile int32_t ramp_constant_velocity = 0;
volatile int32_t ramp_acceleration = 0;
volatile int32_t ramp_start_position = 0;
volatile int32_t ramp_acceleration_end_position = 0;
volatile int32_t ramp_constant_velocity_end_position = 0;


// The way we are interpreting the encoder signals we get 4 increments per magnetic pulse
float encoder_increments_per_rotation = 7.0 * 4;
float gear_ratio = 29.125; // from datasheet it is 30, but when i wind it 10x i observe 8155 increments
float winch_diameter = 15.0;


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


#include "utils.hpp"
#include "buttons.hpp"
#include "encoder.hpp"
#include "motor.hpp"



ICACHE_RAM_ATTR void interrupt_handler() {
    uint32_t begin_ticks = get_clock_ticks();

    interrupt_counter++;
    ramp_interrupt_counter++;
    encoder_step();

    uint32_t ramp_interrupt_counter_mibi = ramp_interrupt_counter / 1024;
    
    if (ramp_interrupt_counter < acceleration_end_interrupt_count) {
        // accelerating

        // know how much velocity shall be increased per interrupt in some meaning unit * 1024
        // ramp calculation was done with assuming maximum velocity 100 rpm ~ 10 rad/s
        // in the interrupt we would
        //      update the velocity and
        //      then update the target position
        //      then update the PID controller
        motor_target_ticks = ramp_acceleration * ramp_interrupt_counter_mibi * ramp_interrupt_counter_mibi / 1024 / 1024;
        ramp_acceleration_end_position = motor_target_ticks;


    } else if (ramp_interrupt_counter < constant_velocity_end_interrupt_count) {
        // constant velocity

        int32_t constant_counter = ramp_interrupt_counter - acceleration_end_interrupt_count;
        motor_target_ticks = ramp_acceleration_end_position + ramp_constant_velocity * constant_counter;
        ramp_constant_velocity_end_position = motor_target_ticks;
    } else if (ramp_interrupt_counter < deceleration_end_interrupt_count) {
        // decelerating
        int32_t deceleration_counter = ramp_interrupt_counter - constant_velocity_end_interrupt_count;
        motor_target_ticks = ramp_constant_velocity_end_position + ramp_acceleration * deceleration_counter * deceleration_counter;
    }

    motor_step(get_clock_ticks(), encoder_increments);

    uint32_t pins_out = PIN_OUT;
    SET_BIT_IF(pins_out, MOTOR_CONTROL_1, motor_out_a)
    SET_BIT_IF(pins_out, MOTOR_CONTROL_2, motor_out_b)
    PIN_OUT = pins_out;

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

    digitalWrite(LED_BUILTIN, LOW); // turn the LED on (inverted logic)
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED off
    delay(1000);

    pinMode(ENCODER_A, INPUT_PULLUP);
    pinMode(ENCODER_B, INPUT_PULLUP);

    if (run_tests) {
        //
    }
    
    timer1_attachInterrupt(interrupt_handler);

    // 80 MHz / 256 = 312.5Khz (1 tick = 3.2us)
    timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);
    uint32_t timer_ticks = 5;
    timer1_write(timer_ticks);
    interrupt_spacing = timer_ticks * 256;
    interrupt_spacing_s = (float) interrupt_spacing / ticks_per_second;
    // for 10 ticks counter with encoder, PID and motor (with float based calculation)
    //   interrupt ticks: 1476 from 2560 corresponding to 57.656250 % load
    // for 10 ticks with all calculation implemented with int
    //   interrupt ticks: 549 from 2560 corresponding to 21.445312 % load
    // for 5 ticks it goes up to 45% now, can see to cases depending on the PWM case we are
    //   interrupt ticks: 549 corresponding to 42.890625 % load
    //   interrupt ticks: 576 corresponding to 45.000000 % load
    // ... some optimizations
    

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

    if (interrupt_load > 20.0) {
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
        float winch_target_mm = 200.0;
        float winch_rotations = winch_target_mm / winch_diameter / pi;
        float winch_radians = winch_rotations * 2.0 * pi;
        //motor_target_ticks = winch_rotations * gear_ratio * encoder_increments_per_rotation;

        float ramp_time_accelerate = 0.0;
        float ramp_time_constant = 0.0;
        float ramp_time_decelerate = 0.0;

        calculate_ramp_times(winch_radians, 0.0, &ramp_time_accelerate, &ramp_time_constant, &ramp_time_decelerate);

        // acceleration phase:
        // know how much velocity shall be increased per interrupt in some meaning unit * 1024
        // ramp calculation was done with assuming maximum velocity 100 rpm ~ 10 rad/s
        // in the interrupt we would
        //      update the velocity and
        //      then update the target position
        //      then update the PID controller
        
        uint32_t now_ticks = get_clock_ticks();
        acceleration_end_interrupt_count = ramp_time_accelerate / interrupt_spacing_s;
        constant_velocity_end_interrupt_count = ramp_time_constant / interrupt_spacing_s + acceleration_end_interrupt_count;
        deceleration_end_interrupt_count = ramp_time_decelerate / interrupt_spacing_s + constant_velocity_end_interrupt_count;

        ramp_constant_velocity = 0; // how about [encoder increments / 1024 interrupts]
        // a_m is [rad/s^2] so
        //     / (2 pi) * encoder_increments_per_rotation to get to encoder increments
        //     * interrupt_spacing_s^2 to get to interrupts^2
        float interrupt_spacing_mibis = interrupt_spacing_s * 1024.0;
        float acceleration_enc_per_intr = sign(winch_radians) * a_m / (2 * pi) * encoder_increments_per_rotation * interrupt_spacing_mibis * interrupt_spacing_mibis * 1024 * 1024; // [1/1024^2 * enc increments / interrupt times mibi^2]
        ramp_acceleration = (int) acceleration_enc_per_intr;
        ramp_interrupt_counter = 0;
        ramp_start_position = 0;
        ramp_acceleration_end_position = 0;

        Serial.printf("RAMP START\n");
        Serial.printf("Times: Acceleration %f s, Constant %f s, Deceleration: %f s\n", ramp_time_accelerate, ramp_time_constant, ramp_time_decelerate);
        Serial.printf("Ramp acceleration %f [enc incs/interrupts^2]\n", acceleration_enc_per_intr);
        Serial.printf("Interrupt interval %f s\n", interrupt_spacing_s);

        for (int i = 0; i < 100; i++) {
            Serial.printf("Pos %d , delta: %d , motor A: %d, motor B: %d\n", encoder_increments, motor_delta_ticks, motor_out_a, motor_out_b);
            delay(500);
        }

        state = STATE_INITIAL;
    }
    else if (state == STATE_2)
    {
        
        motor_target_ticks = 0;
        state = STATE_INITIAL;
    }
    else if (state == STATE_3)
    {
        Serial.print("Interrupt counter: ");
        Serial.println(interrupt_counter);
        Serial.print("Encoder angle: ");
        Serial.println(encoder_increments);
        interrupt_counter = 0;
        motor_target_ticks = 0;
        state = STATE_INITIAL;
    }
    yield;
}

/*



*/