import math
import matplotlib.pyplot as plt
import numpy as np


def sign(x):
    return 1 if x >= 0.0 else -1



class Motor:
    def __init__(self):
        self.last_time = 0
        self.rpm = 0
        self.max_rpm = 200
        self.holding_force = 0.05
        self.anchor_mass = 0.01
    
    def step(self, time, voltage):
        dt = time - self.last_time
        driving_force = voltage

        remaining_force = max(0, abs(driving_force) - self.holding_force) * sign(driving_force) - self.rpm * 0.02

        acceleration = remaining_force / self.anchor_mass

        self.rpm += acceleration * dt

        self.last_time = time


class Winch:
    def __init__(self):
        self.position = 0
        self.radius = 0.01 # 1 cm
        self.last_time = 0
    
    def step(self, time, rpm):
        dt = time - self.last_time
        dt_min = dt / 60.0
        self.position += dt_min * rpm * self.radius * 2 * math.pi
        self.last_time = time



class PIDController:
    def __init__(self, kp, ki, kd, max_abs):
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.limit_pos = max_abs
        self.limit_neg = -max_abs

        self.last_time = 0
        self.last_error = 0.0
        self.integrated_error = 0.0
    
    def step(self, time, error):
        proportinal = self.kp * error

        dt = time - self.last_time

        if dt > 0:
            self.integrated_error += error * dt
            integral = self.ki * self.integrated_error

            d = (error - self.last_error) / dt
            derivative = self.kd * d
        else:
            integral = 0.0
            derivative = 0.0
        
        self.last_time = time
        self.last_error = error

        result = proportinal + integral + derivative

        return max(self.limit_neg, min(self.limit_pos, result))



class Driver:
    def __init__(self):
        self.voltage = 0

        self.on_counter = 0
        self.off_counter = 0
        self.pwm_steps = 10
        self.slope_length = 20.0

        self.pid = PIDController(0.5, 0.0, 0.1, 1.0)
    
    def step(self, time, position, target_position):
        # check whether a PWM cycle is finished
        if self.on_counter <= 0 and self.off_counter <= 0:
            # compute new controller output
            pos_delta_mm = (target_position - position) * 1000.0
            y = self.pid.step(time, pos_delta_mm)
            power_pwm = int(y * self.pwm_steps)

            self.on_counter = abs(power_pwm)
            self.off_counter = self.pwm_steps - self.on_counter

            if self.on_counter > 0:
                self.voltage = sign(power_pwm) * 10
            else:
                self.voltage = 0
        else:
            # PWM still running
            if self.on_counter > 0:
                # keep voltage high
                self.on_counter -= 1
            else:
                self.voltage = 0
                self.off_counter -= 1
        



def get_target_position(t):
    target_mm = 0
    if t < 1.0:
        pass
    elif t < 4.0:
        target_mm = 100
    elif t < 7.0:
        target_mm = 120
    else:
        target_mm = 50
    
    return target_mm / 1000.0 # convert to m




x_values = list()
motor_rpm_values = list()
position_values = list()
voltage_values = list()
target_values = list()


motor = Motor()
winch = Winch()
driver = Driver()

t_per_step = 0.0001
max_steps = 100000

for step in range(max_steps):
    t = step * t_per_step
    x_values.append(t)

    target_position = get_target_position(t)
    target_values.append(target_position * 1000) # in mm

    driver.step(t, winch.position, target_position)
    voltage_values.append(driver.voltage)

    motor.step(t, driver.voltage)
    motor_rpm_values.append(motor.rpm)

    winch.step(t, motor.rpm)
    position_values.append(winch.position * 1000) # in mm



# plot the data
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
ax.plot(x_values, motor_rpm_values, color='tab:blue', label="Motor spped [RPM]")
ax.plot(x_values, position_values, color='tab:orange', label="Winch position [mm]")
ax.plot(x_values, target_values, color='tab:red', label="Target position [mm]")
ax.plot(x_values, voltage_values, color='tab:green', label="Voltage [V]")


# set the limits
#ax.set_xlim([0, 1])
#ax.set_ylim([0, 1])

#ax.set_title('line plot with data points')

plt.legend(loc='lower right')

# display the plot
plt.show()


