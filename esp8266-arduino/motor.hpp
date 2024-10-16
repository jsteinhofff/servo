

#include <math.h>
#include <stdlib.h>

uint32_t motor_pwm_steps = 3;
int32_t motor_on_counter = 0;
int32_t motor_off_counter = 0;
int32_t motor_out_a = 0;
int32_t motor_out_b = 0;

float motor_target_rad = 0.0;
float motor_position_delta = 0.0;

float pid_kp = 0.5;
float pid_ki = 0.0;
float pid_kd = 0.0;

float pid_proportional = 0.0;

float pid_limit_pos = 1.0;
float pid_limit_neg = -1.0;

uint32 pid_last_ticks = 0;
float pid_last_error = 0.0;
float pid_integrated_error = 0.0;




ICACHE_RAM_ATTR float pid_step(uint32_t ticks, float error) {
    float proportional = pid_kp * error;

    uint32_t deltaTicks = ticks - pid_last_ticks;
    float dt = (float) deltaTicks / 80000000.0;

    float integral;
    float derivative;

    if (dt > 0.0) {
        pid_integrated_error += error * dt;
        integral = pid_ki * pid_integrated_error;

        float d = (error - pid_last_error) / dt;
        derivative = pid_kd * d;
    } else {
        integral = 0.0;
        derivative = 0.0;
    }

    pid_last_ticks = ticks;
    pid_last_error = error;

    float result = proportional + integral + derivative;
    pid_proportional = proportional;

    return fmaxf(pid_limit_neg, fminf(pid_limit_pos, result));
}

ICACHE_RAM_ATTR void motor_step(uint32_t ticks, float position) {
    if (motor_target_rad == 0.0) {
        // killswitch for target 0.0
        return;
    }

    // check whether a PWM cycle is finished
    if ((motor_on_counter <= 0) && (motor_off_counter <= 0)) {
        // compute new controller output
        float position_delta = (motor_target_rad - position);
        motor_position_delta = position_delta;
        float y = pid_step(ticks, position_delta);
        int32_t power_pwm = int32_t(y * (float) motor_pwm_steps);

        motor_on_counter = abs(power_pwm);
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


