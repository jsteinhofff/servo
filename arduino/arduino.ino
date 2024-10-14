#include <ESP8266WiFi.h>

int debug = 1;
int counter = 0;
int angle = 0;

#define ENCODER_A 4
#define ENCODER_B 5
#define BUTTON_RESET 12
#define MOTOR_CONTROL_1 13
#define MOTOR_CONTROL_2 14

static inline int32_t asm_ccount(void)
{

    int32_t r;

    asm volatile("rsr %0, ccount" : "=r"(r));
    return r;
}

ICACHE_RAM_ATTR void encoderChange()
{
    counter += 1;

    // the old value of the sensor ports
    static unsigned char ab = 0;

    // increment of angle for the 16 possible bit codes
    const int table[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

    unsigned short port = 0;
    port |= digitalRead(ENCODER_A) << 0;
    port |= digitalRead(ENCODER_B) << 1;

    ab = ab << 2;
    ab |= (port & 0x3);
    angle += table[(ab & 0xf)];
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
    attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderChange, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_B), encoderChange, CHANGE);

    pinMode(BUTTON_RESET, OUTPUT);
    digitalWrite(BUTTON_RESET, LOW);

    pinMode(MOTOR_CONTROL_1, OUTPUT);
    digitalWrite(MOTOR_CONTROL_1, LOW);

    pinMode(MOTOR_CONTROL_2, OUTPUT);
    digitalWrite(MOTOR_CONTROL_2, LOW);

    Serial.println("Started.");
}

void collectAnalogValues(int *values)
{
    for (int i = 0; i < 5; i++)
    {
        values[i] = analogRead(A0);
        delay(10);
    }
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}

int min(int a, int b)
{
    return (a < b) ? a : b;
}

int getMedianOf5(int arr[])
{
    int a = arr[0], b = arr[1], c = arr[2], d = arr[3], e = arr[4];
    int m1 = max(min(a, b), min(c, d));
    int m2 = min(max(a, b), max(c, d));
    return max(min(m1, m2), min(max(m1, m2), e));
}

int detectButton()
{
    static int values[5] = {0};
    collectAnalogValues(&values[0]);

    int median = getMedianOf5(values);

    if (median < 10)
    {
        return 0;
    }

    enum
    {
        buttonCount = 4
    };

    int buttonRanges[buttonCount][2] = {
        {200, 400},
        {520, 620},
        {720, 820},
        {850, 950}};

    int button = 0;

    for (int i = 0; i < buttonCount; i++)
    {
        int lower = buttonRanges[i][0];
        int upper = buttonRanges[i][1];
        if ((lower <= median) && (median <= upper))
        {
            button = i + 1;
            break;
        }
    }

    if (debug)
    {
        for (int i = 0; i < 5; i++)
        {
            Serial.print(values[i]);
            Serial.print("; ");
        }
        Serial.print("=> ");
        Serial.println(median);
    }

    if (debug)
    {
        Serial.print("Button: ");
        Serial.println(button);
    }

    return button;
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
            Serial.println("v3");
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