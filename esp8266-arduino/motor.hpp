

#include <math.h>
#include <stdlib.h>

// The way we are interpreting the encoder signals we get 4 increments per magnetic pulse
float encoder_increments_per_rotation = 7.0 * 4;
float gear_ratio = 29.125; // from datasheet it is 30, but when i wind it 10x i observe 8155 increments

uint32_t motor_pwm_steps = 3;
int32_t motor_on_counter = 0;
int32_t motor_off_counter = 0;
int32_t motor_out_a = 0;
int32_t motor_out_b = 0;

int32_t motor_target_ticks = 0;
int32_t motor_delta_ticks = 0;


ICACHE_RAM_ATTR void motor_step(int32_t current_position) {
    if (motor_target_ticks == 0) {
        // killswitch for target 0.0
        return;
    }

    // check whether a PWM cycle is finished
    if ((motor_on_counter <= 0) && (motor_off_counter <= 0)) {
        // compute new controller output
        int32_t position_delta = motor_target_ticks - current_position;
        motor_delta_ticks = position_delta;
        int32_t power_pwm = position_delta / 2;

        motor_on_counter = min(abs(power_pwm), motor_pwm_steps);
        motor_off_counter = motor_pwm_steps - motor_on_counter;

        if (power_pwm > 0) {
            motor_out_a = 1;
            motor_out_b = 0;
        } else if (power_pwm < 0) {
            motor_out_a = 0;
            motor_out_b = 1;
        } else {
            motor_out_a = 0;
            motor_out_b = 0;
        }
    } else {
        // PWM still running
        if (motor_on_counter > 0) {
            // keep voltage high
            motor_on_counter--;
        } else {
            motor_out_a = 0;
            motor_out_b = 0;
            motor_off_counter--;
        }
    }
}

// maximum acceleration, assuming we aim to go from 0 to 100 rpm in 1 s
// a = 100 rpm / 1s = 100.0 / 60.0 rotations / 1s / 1s = 100.0 / 60.0 * (2 * pi) [rad / s^2]
// Note, this applies at the winch!
float a_winch = 100.0 / 60.0 * (2 * pi);  // [rad / s^2]

// acceleration at the motor
float a_motor = a_winch * gear_ratio;

// maximum velocity, assuming 100 rpm
float v_max_winch = 100.0 / 60.0 * (2 * pi);  // [rad/s]


void calculate_ramp_times(float position_offset, float start_velocity, float max_velocity, float max_acceleration, float* t_a, float* t_v, float* t_d) {
    // Implementation of doi:10.1088/1757-899X/294/1/012055 Chapter #2
    // m is the ratio of acceleration to deceleration
    float m = 1.0;
    float s = position_offset;
    float a = max_acceleration;
    float v_s = start_velocity;

    float mp1 = m + 1.0;
    float x = v_s * mp1 / a;
    float T = -v_s * mp1 / a + sqrt(x * x + 2.0 * s * mp1 / a);

    *t_a = T / mp1;
    *t_d = T - *t_a;
    float v = v_s + a * *t_a;
    *t_v = 0.0;

    if (v <= max_velocity) {
        // triangular speed profile
        // we are done
    } else {
        // trapezoidal speed profile
        v = max_velocity;
        *t_a = (v - v_s) / a;
        float T_ad = *t_a * mp1;
        *t_d = T_ad - *t_a;
        float s_ad = v_s * T_ad + a * T_ad * T_ad / (2.0 * mp1);
        *t_v = (s - s_ad) / v;
        T = T_ad + *t_v;
    }
}


