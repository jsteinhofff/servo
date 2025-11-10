

# TODO



* Testaufbau für Motor mit Höhe 30 cm, Lineal
* 3V Motor Versuch wiederholen, Video?
* 6V Motor Versuch, Video?
* Encoder Lesen mit Timer Interrupt?
* Dauer der Encoder Berechnung erfassen / Interrupt last?
    https://sub.nanona.fi/esp8266/timing-and-ticks.html

* NONOS Version von ESP ausprobieren?



# NOTES


2024-10-14

Thoughts about the interrupt frequency:

* Encoder signals up to 10k Hz
* PWM with 20k Hz , with 10% duty cycle resolution => 200k Hz update
  => 5 us per update
* we have 80 MHz clock cycle => 12.5 ns per clock cycle
  => only 400 clocks per update

This seems to be too challenging. In this time we have to:
* Read encoder state (2 x digital input)
* Encoder calculations
* PID update (worst case SW floatingpoint calculation?)
* PWM update (digitalWrite)

Can either go with
* lower PWM frequency (might become audible)
    => scales linear, can easily adjust it
* lower PWM duty cycle resolution (might be ok, eg. 25%)
    => to check how it behaves

Observation: interrupt handler with just encoder handling
takes 216 ticks

Adding one line

    counter += digitalRead(ENCODER_A);

Increases interrupt handler runtime to 259 ticks. ~ 40 ticks for digitalRead!!

Now with TIM_DIV256 and timer value 10 we have roughly 30k Hz interrupt frequency
and interrupt load is 8 %.


2024-10-15

With full encoder, PID controller and PWM for motor, it is roughly 1400 ticks per Interrupt.
This can be run with 10 timer ticks, with TIM_DIV256 this means 30 kHz interrupt frequency, of which we have 50% already.
For 10-step PWM resolution this leads to ~3 kHz beeping when the motor is holding position.
Lots of float computation in there, have to have a look at it.


2024-10-16

Since all fixed point approaches failed me:
* the ones from github gist https://gist.github.com/oldrev/a18c856b77634d0043372393940df224
    dont give meaningful results
    also seem to need comparable instructions

* https://github.com/tomcombriat/FixMath not clear how to use and somehow gives wrong results (all zero)

* stdfix.h, gives compile error, seems not supported on Xelera CPU

=> thinking about getting rid of float.

Looking at the problem from birds-eye-view:

* Input are encoder ticks
  * we have 28 per motor rotation and a gearing with ~29 ratio => 812 ticks per rotation
  * aim is to drive the subject up/down 200 mm, so with 15 mm diameter of the winch we can expect 4.x rotations, lets go with 10
  * 8120 ticks we should be able to handle, it can also go negative, so lets say +-10.000 ticks

* Output is PWM, at most resolution of 10% duty cycle

So actually the PID only is supposed to output a number between -10 .. +10 based on a delta of values in the range of +-10.000. Hardly sounds like float would be needed for this.

What to do?
1. convert target position to ticks
2. currently only using P part of PID, so lets have a pure P one with int



Original, with float based calculation:
    interrupt ticks: 1476 from 2560 corresponding to 57.656250 % load
Calculation implemented with ints:
    interrupt ticks: 549 from 2560 corresponding to 21.445312 % load
Optimized by replacing digitalReady by direct register access
    interrupt ticks: 455 from 1280 corresponding to 35.546875 % load
Optimized by also replacing digitalWrite by direct register access
    interrupt ticks: 243 from 1280 corresponding to 18.984375 % load



2024-11-10
----------

* Most of motor control simulated in python, there were some issues with
  representation as int32, need to carefully order mathematical operations
  to stay in a meaningful range (>100 and < 100mio)
  Calculation in int32 has some implications to accuracy. Value is strictly
  lower than ideal value, but when applied at the end of the winch the precision
  still looks ok.

* Concern about precision of the overall approach:
  - first the ramps are calculated in time domain with floating point
  - then the ramp-times are converted to interrupt-tick counts (still quite
    precise since ticks are very fast)
  - then based on integer acceleration value, desired position in encoder
    granularity is calcluated with each interrupt and PID controller is
    driving to that position



2025-01-06
----------

Observations with N20 Motor:
- It is capable of driving 100g weight up and down with 15 mm diameter
  winch quite easily. At 6V the current is
  * ~0.18 A when accelerating upwards ( a_winch = 100.0 / 60.0 * (2 * pi) )
  * ~0.1 A when holding position on top
  * 0.02 when driving down

- For 200g the movement is not so smooth and the acceleration had to be halfed.
  There is still some hickup in the drive, after 45 drives an overload situation
  happened. (>10mm follow error). As long as it worked, the current
  was about 0.2 .. 0.3 A while driving up and about 0.35 A for holding position.
  The motor is getting notably warm.

=> in general the N20 motor seems to be a bit underwhelming for 200g weight.
   it can pull it up, but needs max current and also holding position needs
   lots of energy (58 mAh when holding for 10 min),
   exceeding the target energy of 33 mAh per operation.

First impressions of JGA25-370 12V 200RPM operated with 6v:
- this motor is very strong, it can easily pull 200g
- the gear is quite stiff, even without voltage, the 200g weight is far from
  driving the winch by gravity
- the construction is much stronger, the winch mounted to the motor axis
  feels sturdy enough to hold the 200g weight without ball gear
- current when driving without load: 0.07 A
- current when blocked: 0.5 A
- current when pulling up 200g: 0.12 A

Power considerations:

1 month of operation with a single battery charge desired.
Assuming 3500 mAh cells with a capacity of 2000 mAh in practice with sufficient
voltage. Two tea preparations per day, 30 days in a month leaves us with
33 mAh per operation. We assume a holding time of 10 min in average.

2025-01-21
==========

Updated solution for button handling (without sample-and-hold feature) seems
to work. Lets get real!

- Sample buttons once started, print detected button
- Determine button based on closest voltage calculated from resistor-ladder
- beep?
- waiting time in seconds


PID controller in C++:
https://gist.github.com/bradley219/5373998


Fix-Point version:
https://gist.github.com/oldrev/a18c856b77634d0043372393940df224


Another time
============

* Understand binary numbers overflow behaviour
  https://www.youtube.com/watch?v=8C4tGkG5EaE

* Faster reading of digital ports
  https://forum.arduino.cc/t/digitalwritefast-digitalreadfast-pinmodefast-etc/47037

  directly read the register PIN_IN : https://github.com/esp8266/esp8266-wiki/wiki/gpio-registers


* Understand esp8266 boot modes
  https://riktronics.wordpress.com/2017/10/02/esp8266-error-messages-and-exceptions-explained/



Thanks to
=========


* M.Klett mk@mkesc.co.uk (interfacing shaft encoders)
* Juha Railoma (https://sub.nanona.fi/esp8266/index.html)
* Arduino project

