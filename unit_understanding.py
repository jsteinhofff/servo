
import math

from tracked_number import TrackedNumber, TrackedInt32

"""

Relevant units:

* Easy to think about values of rotation speed and acceleration are at the
  winch `winch_*` or `*_winch`
* The actual motor control loop operates at the motor-axis where also the encoder
  is attached. `mot_*` or `*_mot`
* For our application relevant is the linear movement up and down achieved by
  a string winded up on the winch `x` in [mm]

"""



##
## physical constants
## ---
ticks_per_second = 80000000
timer_ticks = 5
interrupt_spacing = timer_ticks * 256
interrupt_spacing_s = interrupt_spacing / ticks_per_second

encoder_increments_per_rotation = 7.0 * 4
gear_ratio = 29.125
winch_diameter = 15.0

##
## configuration values
## --

# maximum acceleration, assuming we aim to go from 0 to 100 rpm in 1 s
# a = 100 rpm / 1s = 100.0 / 60.0 rotations / 1s / 1s = 100.0 / 60.0 * (2 * pi) [rad / s^2]
# Note, this applies at the winch!
a_winch = 100.0 / 60.0 * (2 * math.pi) # rad / s^2

# at the motor
a_motor = a_winch * gear_ratio

# maximum velocity, assuming 100 rpm
v_max_winch = 100.0 / 60.0 * (2 * math.pi)  # [rad/s]

winch_target_mm = 200.0


##
## derived
## --

rad_to_enc = encoder_increments_per_rotation / (2 * math.pi)
acceleration_rad_per_intr = a_motor * (interrupt_spacing_s ** 2)
velocity_rad_per_intr = a_motor * interrupt_spacing_s
acceleration_enc_per_intr = a_motor * rad_to_enc * (interrupt_spacing_s ** 2)
TrackedInt32("acceleration_enc_per_intr", acceleration_enc_per_intr)
acceleration_enc_per_intr_gibi = TrackedInt32("a_enc_per_intr_gibi", round(a_motor * rad_to_enc * (interrupt_spacing_s ** 2) * 1024 * 1024 * 1024))

winch_rotations = winch_target_mm / winch_diameter / math.pi
winch_radians = winch_rotations * 2.0 * math.pi





def calculate_ramp_times(acceleration, position_offset, start_velocity):
    # Implementation of doi:10.1088/1757-899X/294/1/012055 Chapter #2
    # m is the ratio of acceleration to deceleration
    m = 1.0
    s = position_offset
    a = acceleration

    v_s = start_velocity

    mp1 = m + 1.0
    x = v_s * mp1 / a
    T = -v_s * mp1 / a + math.sqrt(x * x + 2.0 * s * mp1 / a)

    t_a = T / mp1
    t_d = T - t_a
    v = v_s + a * t_a
    t_v = 0.0

    if v <= v_max_winch:
        # triangular speed profile
        # we are done
        pass
    else:
        # trapezoidal speed profile
        v = v_max_winch
        t_a = (v - v_s) / a
        T_ad = t_a * mp1
        t_d = T_ad - t_a
        s_ad = v_s * T_ad + a * T_ad * T_ad / (2.0 * mp1)
        t_v = (s - s_ad) / v
        T = T_ad + t_v
    
    return t_a, t_v, t_d



def mot_rot_to_x(rots):
    """Convert from motor rotations to linear motion in mm"""
    return rots / gear_ratio * 2 * math.pi * winch_diameter / 2.0


def mot_rad_to_x(rads):
    """Convert from motor radians to linear motion in mm"""
    return rads / gear_ratio * winch_diameter / 2.0

def winch_rot_to_x(rots):
    """Convert from winch rotations to linear motion in mm"""
    return rots * 2 * math.pi * winch_diameter / 2.0

def winch_rad_to_x(rads):
    """Convert from winch radians to linear motion in mm"""
    return rads * winch_diameter / 2.0


def calculate_x_from_t(a, v0, x0, duration):
    """
    Calculate stepwise position from acceleration a and constant velocity v0
    and return one list for the points in time and one list for the positions.
    This function does not care about units, so the parameters have to fit together.
    """
    max_steps = int(duration / interrupt_spacing_s)

    ts = []
    xs = []
    for i in range(max_steps):
        t = i * interrupt_spacing_s

        # it should be trivial, but since i lost some time with it ...
        # one can play around with the nice properties of constant acceleration in
        # https://www.geogebra.org/calculator/bxwrh825
        x = x0 + v0 * t + 0.5 * a * t**2

        ts.append(t)
        xs.append(x)
    
    return ts, xs

def calculate_x_from_intr(a, v0, x0, duration):
    max_steps = int(duration / interrupt_spacing_s)
    ts = []
    xs = []
    for i in range(max_steps):
        t = i * interrupt_spacing_s
        x = x0 + v0 * i + 0.5 * a * (i ** 2)
        ts.append(t)
        xs.append(x)

    return ts, xs

def calculate_enc_by_intr(a, v0, enc0, duration):
    max_steps = int(duration / interrupt_spacing_s)

    ts = []
    encs = []
    for i in range(max_steps):
        t = i * interrupt_spacing_s

        # with the current interrupt configuration, we have 62500 interrupts per second
        # for a ramp of 1s this means we have to work with a square of this number which is 3`906`250`000
        # which is scary close to 2^32 and it would be good to have some head room
        # in case the interrupt gets configured differently or in case the ramp duration is increased.
        # => at least a divisor of 16 seems appropriate bringing us down to 244`140`625 (for 1s)
        # => more than 8s ramp possible with Int32 (=31 bits available)

        i_squeeze = TrackedInt32("i_squeeze", i // 16)
        i_square_squeeze = TrackedInt32("i_square_kibi", i_squeeze.value * i)
        i_square_mibi = TrackedInt32("i_square_mibi", i_square_squeeze.value // 1024 // (1024 / 16))
        e = TrackedInt32("e", i_square_mibi.value * a)
        f = TrackedInt32("f", e.value // 2048)
        g = TrackedInt32("g", v0 * i_squeeze.value // 1024 // (1024 / 16))
        h = TrackedInt32("h", g.value + enc0)
        enc = f + h
        enc.set_alias("enc")
        ts.append(t)
        encs.append(enc.value)

    return ts, encs


def main():
    import matplotlib.pyplot as plt
    from mpl_toolkits.axes_grid1.inset_locator import inset_axes
    from mpl_toolkits.axes_grid1.inset_locator import mark_inset

    t_a, t_v, t_d = calculate_ramp_times(a_winch, winch_radians, 0.0)

    v_const_winch = a_winch * t_a
    v_const_motor_intr = a_motor * t_a * interrupt_spacing_s
    velocity_enc_per_intr_mibi = TrackedInt32("velocity_enc_per_intr_mibi", round(v_const_motor_intr * rad_to_enc * 1024 * 1024))

    print(f"velocity_enc_per_intr_mibi: {velocity_enc_per_intr_mibi}")

    result_t_a, result_x_a = calculate_x_from_t(a_winch, v0=0.0, x0=0.0, duration=t_a)
    result_t_v, result_x_v = calculate_x_from_t(a=0.0, v0=v_const_winch, x0=result_x_a[-1], duration=t_v)
    result_t_d, result_x_d = calculate_x_from_t(-a_winch, v0=v_const_winch, x0=result_x_v[-1], duration=t_d)
    # concatenate all results
    result_t = result_t_a + [t + t_a for t in result_t_v] + [t + t_a + t_v for t in result_t_d]
    result_x = [winch_rad_to_x(x) for x in result_x_a + result_x_v + result_x_d]

    
    result_t2_a, result_x2_a = calculate_x_from_intr(a=acceleration_rad_per_intr, v0=0.0, x0=0.0, duration=t_a)
    result_t2_v, result_x2_v = calculate_x_from_intr(a=0.0, v0=v_const_motor_intr, x0=result_x2_a[-1], duration=t_v)
    result_t2_d, result_x2_d = calculate_x_from_intr(a=-acceleration_rad_per_intr, v0=v_const_motor_intr, x0=result_x2_v[-1], duration=t_d)
    # concatenate all results
    result_t2 = result_t2_a + [t + t_a for t in result_t2_v] + [t + t_a + t_v for t in result_t2_d]
    result_x2 = [mot_rad_to_x(x) + 0.5 for x in result_x2_a + result_x2_v + result_x2_d]

    result_t3_a, result_enc3_a = calculate_enc_by_intr(a=acceleration_enc_per_intr_gibi.value, v0=0.0, enc0=0, duration=t_a)
    result_t3_v, result_enc3_v = calculate_enc_by_intr(a=0.0, v0=velocity_enc_per_intr_mibi.value, enc0=result_enc3_a[-1], duration=t_v)
    print(result_enc3_v[-1])
    result_t3_d, result_enc3_d = calculate_enc_by_intr(a=-acceleration_enc_per_intr_gibi.value, v0=velocity_enc_per_intr_mibi.value, enc0=result_enc3_v[-1], duration=t_d)
    result_t3 = result_t3_a + [t + t_a for t in result_t3_v] + [t + t_a + t_v for t in result_t3_d]
    result_x3 = [mot_rot_to_x(enc / encoder_increments_per_rotation) for enc in result_enc3_a + result_enc3_v + result_enc3_d]


    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)
    #ax.plot(result_t, result_a, color='tab:blue', label="Acceleration [rad/s^2]")
    #ax.plot(result_t, result_v, color='tab:orange', label="Speed [rad/s]")
    ax.plot(result_t, result_x, color='tab:red', label="float")
    #ax.plot(result_t2, result_x2, color='tab:orange', label="mm by intr")
    ax.plot(result_t3, result_x3, color='tab:blue', label="int(enc/intr)")

    
    plt.legend(loc="lower center")

    # set the limits
    #ax.set_xlim([0, 1])
    #ax.set_ylim([0, 1])

    ax2 = inset_axes(ax, width="30%", height="30%", loc=2)
    ax2.axis([0, 0.25, -5, 10])
    ax2.plot(result_t, result_x, color='tab:red')
    ax2.plot(result_t3, result_x3, color='tab:blue')
    mark_inset(ax, ax2, loc1=2, loc2=4, fc="none", ec="0.5")

    ax3 = inset_axes(ax, width="30%", height="30%", loc=4)
    ax3.axis([3.35, 3.6, 190, 205])
    ax3.plot(result_t, result_x, color='tab:red')
    ax3.plot(result_t3, result_x3, color='tab:blue')
    mark_inset(ax, ax3, loc1=2, loc2=4, fc="none", ec="0.5")

    ax4 = inset_axes(ax, width="30%", height="30%", loc="upper center")
    ax4.axis([1.65, 1.90, 95, 105])
    ax4.plot(result_t, result_x, color='tab:red')
    ax4.plot(result_t3, result_x3, color='tab:blue')
    mark_inset(ax, ax4, loc1=2, loc2=4, fc="none", ec="0.5")


    
    TrackedNumber.report_states()

    # display the plot
    plt.show()





if __name__ == "__main__":
    main()
    

