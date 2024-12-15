

import math



# maximum acceleration, assuming we aim to go from 0 to 100 rpm in 1 s
# a = 100 rpm / 1s = 100.0 / 60.0 rotations / 1s / 1s = 100.0 / 60.0 * (2 * pi) [rad / s^2]
a_m = 100.0 / 60.0 * (2 * math.pi)  # [rad/s^2]

# maximum velocity, assuming 100 rpm
v_M = 100.0 / 60.0 * (2 * math.pi)  # [rad/s]



def calculate_ramp_times(start_position, start_velocity, target_position):
    """
    Implementation of doi:10.1088/1757-899X/294/1/012055 Chapter #2
    """
    # m is the ratio of acceleration to deceleration
    m = 1.0

    s = target_position - start_position
    a = a_m

    v_s = start_velocity

    T = -v_s * (m + 1) / a + math.sqrt((v_s * (m + 1) / a)**2 + 2 * s * (m + 1) / a)

    t_a = T / (m + 1)
    t_d = T - t_a
    v = v_s + a * t_a

    if v <= v_M:
        # triangular speed profile
        # we are done, only need the time values
        t_v = 0.0
        pass
    else:
        # trapezoidal speed profile
        v = v_M
        t_a = (v - v_s) / a
        T_ad = t_a * (m + 1)
        t_d = T_ad - t_a
        s_ad = v_s * T_ad + a * T_ad**2 / (2 * (m + 1))
        t_v = (s - s_ad) / v
        T = T_ad + t_v
        pass

    return (t_a, t_v, t_d)




def test_scenario(target_position, steps_per_second):
    t_a, t_v, t_d = calculate_ramp_times(0.0, 0.0, target_position)

    print(f"Times: a: {t_a}, v: {t_v}, d: {t_d} [s]")

    result_t = list()  # time
    result_a = list()  # acceleration
    result_v = list()  # speed
    result_x = list()  # position

    t = 0.0
    a = 0.0
    v = 0.0
    x = 0.0

    dt = 1.0 / steps_per_second

    for i in range(int(t_a * steps_per_second)):
        t = dt * i
        a = a_m
        v += a * dt
        x += v * dt
        result_t.append(t)
        result_a.append(a)
        result_v.append(v)
        result_x.append(x)

    for i in range(int(t_v * steps_per_second)):
        t = t_a + dt * i
        a = 0.0
        v += a * dt
        x += v * dt
        result_t.append(t)
        result_a.append(a)
        result_v.append(v)
        result_x.append(x)

    for i in range(int(t_d * steps_per_second)):
        t = t_a + t_v + dt * i
        a = -a_m
        v += a * dt
        x += v * dt
        result_t.append(t)
        result_a.append(a)
        result_v.append(v)
        result_x.append(x)
    
    
    return result_t, result_a, result_v, result_x




def main():
    import matplotlib.pyplot as plt

    print(f"Maximum acceleration: {a_m} [rad/s^2]")
    print(f"Maximum speed: {v_M} [rad/s]")

    result_t, result_a, result_v, result_x = test_scenario(100.0, 10000)


    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)
    ax.plot(result_t, result_a, color='tab:blue', label="Acceleration [rad/s^2]")
    ax.plot(result_t, result_v, color='tab:orange', label="Speed [rad/s]")
    ax.plot(result_t, result_x, color='tab:red', label="Position [rad]")


    # set the limits
    #ax.set_xlim([0, 1])
    #ax.set_ylim([0, 1])

    #ax.set_title('line plot with data points')

    plt.legend(loc='lower right')

    # display the plot
    plt.show()





if __name__ == "__main__":
    main()



