#include <ESP8266WiFi.h>


int debug = 1;
bool run_tests = true;
bool welcome_shown = false;


#include "utils.hpp"
#include "buttons.hpp"
#include "encoder.hpp"
#include "motor.hpp"


volatile int32_t interrupt_counter = 0;
volatile uint32_t interrupt_ticks = 0;
volatile uint32_t interrupt_spacing = 0;
float interrupt_spacing_s = 0.0;
volatile uint32_t ramp_interrupt_counter = 0;
volatile uint32_t acceleration_end_interrupt_count = 0;
volatile uint32_t constant_velocity_end_interrupt_count = 0;
volatile uint32_t deceleration_end_interrupt_count = 0;

float ramp_time_accelerate = 0.0;
float ramp_time_constant = 0.0;
float ramp_time_decelerate = 0.0;
volatile int32_t ramp_constant_velocity = 0;
volatile int32_t ramp_acceleration_enc_per_intr_gibi = 0;
volatile int32_t ramp_velocity_enc_per_intr_mibi = 0;
volatile int32_t ramp_start_position = 0;
volatile int32_t ramp_acceleration_end_position = 0;
volatile int32_t ramp_constant_velocity_end_position = 0;
volatile int32_t ramp_max_motor_following_error_enc = 0;
volatile bool ramp_done = false;
volatile bool ramp_overload = false;

/*

* Most usable values of rotation speed and acceleration are at the
  winch, we mark them by  `winch_*` or `*_winch` prefix and suffix.
* The actual motor control loop operates at the motor-axis where also the encoder
  is attached. Values refering to the motor axis are marked by `mot_*` or `*_mot`.
* For our application relevant is the linear movement up and down achieved by
  a string winded up on the winch `x` in [mm], the relevant values are marked
  by the "_mm" suffix.

*/

float winch_diameter = 15.0;


// derived values
float rad_to_enc_mot = 0.0;
float mm_to_rot_winch = 0.0;
float mm_to_rad_winch = 0.0;






inline int enc_by_intr(int i, int a_gibi, int v0_mibi, int enc0) {
    // Calculate the desired encoder pulses based on the interrupt counter.
    // 
    // The actual formula is simply enc = 0.5 * i**2 * a + i * v0 + enc0
    // but we do it in whole numbers and have to scale things around
    // to prevent overflows on one hand and prevent loosing too much precision on the other hand.
    //
    // with 62500 interrupts per second, for a ramp of 1s this means we have to
    // work with a square of this number which is 3`906`250`000 which is scary
    // close to 2^32 and it would be good to have some head room in case the
    // interrupt gets configured differently or in case the ramp duration is increased.
    // => at least a divisor of 16 seems appropriate bringing us down to 244`140`625 (for 1s)
    // => more than 8s ramp possible with Int32 (=31 bits available)
    int i_squeeze = i / 16;
    int i_square_squeeze = i_squeeze * i;
    int i_square_mibi = i_square_squeeze / 1024 / (1024 / 16);
    int e = i_square_mibi * a_gibi;
    int f = e / 2048;
    int g = v0_mibi * i_squeeze / 1024  / (1024 / 16);
    int h = g + enc0;
    return f + h;
}


ICACHE_RAM_ATTR void interrupt_handler() {
    uint32_t begin_ticks = get_clock_ticks();

    interrupt_counter++;
    ramp_interrupt_counter++;
    encoder_step();

    if (!ramp_done && !ramp_overload) {
    if (ramp_interrupt_counter < acceleration_end_interrupt_count) {
        // accelerating
        motor_target_ticks = enc_by_intr(ramp_interrupt_counter, ramp_acceleration_enc_per_intr_gibi, 0, ramp_start_position);
    } else if (ramp_interrupt_counter < constant_velocity_end_interrupt_count) {
        // constant velocity
        int32_t constant_counter = ramp_interrupt_counter - acceleration_end_interrupt_count;
        motor_target_ticks = enc_by_intr(constant_counter, 0, ramp_velocity_enc_per_intr_mibi, ramp_acceleration_end_position);
    } else if (ramp_interrupt_counter < deceleration_end_interrupt_count) {
        // decelerating
        int32_t deceleration_counter = ramp_interrupt_counter - constant_velocity_end_interrupt_count;
        motor_target_ticks = enc_by_intr(deceleration_counter, -ramp_acceleration_enc_per_intr_gibi, ramp_velocity_enc_per_intr_mibi, ramp_constant_velocity_end_position);
    } else {
            // drive is done, disable counters, such that they dont go crazy when they overflow
            ramp_done = true;
        }
    }

    // we keep driving the motor to hold position as long as we didnt detect overload
    if (!ramp_overload) {
    motor_step(encoder_increments);

        if (abs(motor_delta_ticks) > ramp_max_motor_following_error_enc) {
            ramp_overload = true;
            motor_overload();
        }

    uint32_t pins_out = PIN_OUT;
    SET_BIT_IF(pins_out, MOTOR_CONTROL_1, motor_out_a)
    SET_BIT_IF(pins_out, MOTOR_CONTROL_2, motor_out_b)
    PIN_OUT = pins_out;
    }

    uint32_t end_ticks = get_clock_ticks();
    // difference is correct, even if end_ticks has overflown and begin_ticks didnt
    interrupt_ticks = end_ticks - begin_ticks;
}


void setup_drive(float winch_target_mm) {
    // this function only is called once at the beginning of a drive, so we can/should do as many
    // operations as possible, using floating point where meaningful. Everything
    // we can already prepare for the interrupt handler, eg. by having necessary values ready as integers
    // we should do here.
    float winch_rotations = winch_target_mm / winch_diameter / pi;
    float winch_radians = winch_rotations * 2.0 * pi;
    int direction = sign(winch_radians);
    Serial.printf("winch_radians: %f with abs: %f\n", winch_radians, fabs(winch_radians));

    ramp_time_accelerate = 0.0;
    ramp_time_constant = 0.0;
    ramp_time_decelerate = 0.0;

    // the ramp times are calculated based on what is supposed to happen at the winch
    calculate_ramp_times(fabs(winch_radians), 0.0, v_max_winch, a_winch, &ramp_time_accelerate, &ramp_time_constant, &ramp_time_decelerate);
    
    // again convert to the unit we can use in the interrupt handler (interrupt counts)
    acceleration_end_interrupt_count = ramp_time_accelerate / interrupt_spacing_s;
    constant_velocity_end_interrupt_count = ramp_time_constant / interrupt_spacing_s + acceleration_end_interrupt_count;
    deceleration_end_interrupt_count = ramp_time_decelerate / interrupt_spacing_s + constant_velocity_end_interrupt_count;


    // prepare the velocity and acceleration in the unit we can most efficiently use in the interrupt handler
    float v_const_motor_intr = a_motor * ramp_time_accelerate * interrupt_spacing_s;
    ramp_velocity_enc_per_intr_mibi = direction * v_const_motor_intr * rad_to_enc_mot * 1024.0 * 1024.0;

    float acceleration_enc_per_intr = a_motor * rad_to_enc_mot * interrupt_spacing_s * interrupt_spacing_s;
    ramp_acceleration_enc_per_intr_gibi = direction * acceleration_enc_per_intr * (1024.0 * 1024.0 * 1024.0);

    ramp_interrupt_counter = 0;
    ramp_start_position = encoder_increments;
    ramp_acceleration_end_position = enc_by_intr(acceleration_end_interrupt_count,
                                                    ramp_acceleration_enc_per_intr_gibi, // a
                                                    0, // v0
                                                    ramp_start_position);

    ramp_constant_velocity_end_position = enc_by_intr(constant_velocity_end_interrupt_count - acceleration_end_interrupt_count,
                                                        0, // a
                                                        ramp_velocity_enc_per_intr_mibi, // v0
                                                        ramp_acceleration_end_position);

    // allow a maximum of 10 mm delta from current target position to motor position
    ramp_max_motor_following_error_enc = 10.0 * mm_to_rad_winch * gear_ratio * rad_to_enc_mot;

    ramp_done = false;
    ramp_overload = false;
}


void long_term_test() {
    float destination = 150.0; // mm
    for (int i = 0; i < 100; i++) {
        setup_drive(destination * (((i + 1) % 2) * 2 - 1));

        Serial.printf("RAMP START %d\n", i);

        if (debug) {
            Serial.printf("Motor ticks per rotation %f, gear ration %f\n", encoder_increments_per_rotation, gear_ratio);
            Serial.printf("Times: Acceleration %f s, Constant %f s, Deceleration: %f s\n", ramp_time_accelerate, ramp_time_constant, ramp_time_decelerate);
            Serial.printf("Ramp acceleration %d [enc incs/interrupts^2/2^30]\n", ramp_acceleration_enc_per_intr_gibi);
            Serial.printf("Constant velocity %d [enc incs/interrupts/2^20]\n", ramp_velocity_enc_per_intr_mibi);
            Serial.printf("Interrupt interval %f s\n", interrupt_spacing_s);
            Serial.printf("Max motor following error %d [enc incs]\n", ramp_max_motor_following_error_enc);

            for (int i = 0; i < 100; i++) {
                Serial.printf("Pos %d , delta: %d , motor A: %d, motor B: %d\n", encoder_increments, motor_delta_ticks, motor_out_a, motor_out_b);
                delay(100);
            }
        } else {
            while (true) {
                if (ramp_overload) {
                    Serial.printf("Overload in drive %d\n", i);
                    i = 999999;
                    break;
                }

                if (ramp_done) {
                    break;
                }
                delay(100);
            }
        }
    }
}


void print_encoder_pos() {
    for (int i = 0; i < 1000; i++) {
        Serial.printf("Pos %d\n", encoder_increments);
        delay(100);
    }
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

    motor_setup(2);

    rad_to_enc_mot = encoder_increments_per_rotation / (2 * pi);
    mm_to_rot_winch = 1.0 / winch_diameter / pi;
    mm_to_rad_winch = mm_to_rot_winch * 2.0 * pi;

    if (run_tests) {
        //
    }
    
    timer1_attachInterrupt(interrupt_handler);

    // 80 MHz / 256 = 312.5Khz (1 tick = 3.2us)
    timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);

    // Arm timer with an initial value of 5 ticks, which corresponds to 16 us
    // or 62,5 kHz. Thanks to TIM_LOOP this value will be reused over and over.
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
    STATE_4 = 5
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
            else if (button == 4) {
                state = STATE_4;
            } else {
                // hmm
            }
            digitalWrite(BUTTON_RESET, HIGH);
            delay(200);
            digitalWrite(BUTTON_RESET, LOW);
        }
    }
    else if (state == STATE_1)
    {
        //long_term_test();

        state = STATE_INITIAL;
    }
    else if (state == STATE_2)
    {
        //print_encoder_pos();

        state = STATE_INITIAL;
    }
    else if (state == STATE_3)
    {
        state = STATE_INITIAL;
    }
    else if (state == STATE_4)
    {
        state = STATE_INITIAL;
    }
    else
    {
        //
    }
    yield;
}

/*



*/