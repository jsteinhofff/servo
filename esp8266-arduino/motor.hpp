


int motor_angle = 0;

float pid_kp = 0.0;
float pid_ki = 0.0;
float pid_kd = 0.0;

float pid_limit_pos = 0.0;
float pid_limit_neg = 0.0;

float pid_last_time = 0;
float pid_last_error = 0.0;
float pid_integrated_error = 0.0;



void initMotor() {
    // nop
}

