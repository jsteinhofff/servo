#include <ESP8266WiFi.h>


int debug = 1;
int counter = 0;

#define ENCODER_A 4
#define ENCODER_B 5
#define BUTTON_RESET 12
#define MOTOR_CONTROL_1 13
#define MOTOR_CONTROL_2 14


#include "utils.hpp"
#include "buttons.hpp"
#include "encoder.hpp"






ICACHE_RAM_ATTR void interruptHandler() {
    counter += 1;

    updateEncoder();
}


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
    attachInterrupt(digitalPinToInterrupt(ENCODER_A), interruptHandler, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_B), interruptHandler, CHANGE);

    pinMode(BUTTON_RESET, OUTPUT);
    digitalWrite(BUTTON_RESET, LOW);

    pinMode(MOTOR_CONTROL_1, OUTPUT);
    digitalWrite(MOTOR_CONTROL_1, LOW);

    pinMode(MOTOR_CONTROL_2, OUTPUT);
    digitalWrite(MOTOR_CONTROL_2, LOW);

    Serial.println("Started.");
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
    if (state == STATE_INITIAL)
    {
        int button = detectButton();
        if (button == 0)
        {
            delay(100);
        }
        else
        {
            Serial.println(__TIMESTAMP__);
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
        angle = 0;
        digitalWrite(MOTOR_CONTROL_1, HIGH);
        digitalWrite(MOTOR_CONTROL_2, LOW);
        delay(1000);
        digitalWrite(MOTOR_CONTROL_1, LOW);
        digitalWrite(MOTOR_CONTROL_2, LOW);
        state = STATE_INITIAL;
    }
    else if (state == STATE_2)
    {
        digitalWrite(MOTOR_CONTROL_1, LOW);
        digitalWrite(MOTOR_CONTROL_2, HIGH);
        delay(1000);
        digitalWrite(MOTOR_CONTROL_1, LOW);
        digitalWrite(MOTOR_CONTROL_2, LOW);
        state = STATE_INITIAL;
    }
    else if (state == STATE_3)
    {
        Serial.print("Counter: ");
        Serial.println(counter);
        Serial.print("Angle: ");
        Serial.println(angle);
        counter = 0;
        state = STATE_INITIAL;
    }
    yield;
}

/*



*/