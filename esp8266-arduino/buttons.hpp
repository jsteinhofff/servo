



enum
{
    button_count = 4
};

int button_voltage_ranges[button_count][2] = {0};


void collect_analog_values(int *values)
{
    for (int i = 0; i < 5; i++)
    {
        // analog values are in 1024 range for 1 V
        // close enough to millivolts for our purposes
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

int get_median_of_5(int arr[])
{
    int a = arr[0], b = arr[1], c = arr[2], d = arr[3], e = arr[4];
    int m1 = max(min(a, b), min(c, d));
    int m2 = min(max(a, b), max(c, d));
    return max(min(m1, m2), min(max(m1, m2), e));
}

void button_setup() {
    // calculate expected voltage for each button from the resistors
    float resistor_ladder_ohms[button_count] = {
        2000.0,
        5000.0,
        10000.0,
        20000.0
    };

    float lowside_resistor_ohms = 1000.0;
    float topside_resistor_ohms = 100000.0;
    float base_emmiter_drop_volts = 0.65;
    float input_volts = 3.3;
    float offset = -0.1;

    // some short names for the formulas:
    float* ras = &resistor_ladder_ohms[0];
    float rb = topside_resistor_ohms;
    float rl = lowside_resistor_ohms;
    float ui = input_volts;
    float ut = base_emmiter_drop_volts;

    for (int i = 0; i < button_count; i++) {
        float denominator = ui - ut + (ras[i] * ui) / rb;
        float divisor = 1 + ras[i] / rl + ras[i] / rb;
        float value = denominator / divisor + offset;
        int value_millivolts = (int) (1000.0 * value);

        int lower = value_millivolts - 50;
        int upper = value_millivolts + 50;

        button_voltage_ranges[i][0] = lower;
        button_voltage_ranges[i][1] = upper;

        if (debug) {
            Serial.printf("Button %d voltage mid %d range %d - %d\n", i + 1, value_millivolts, lower, upper);
        }
    }
}

int detect_button()
{
    static int values[5] = {0};
    collect_analog_values(&values[0]);

    int median = get_median_of_5(values);

    if (median < 10)
    {
        return 0;
    }

    int button = 0;

    for (int i = 0; i < button_count; i++)
    {
        int lower = button_voltage_ranges[i][0];
        int upper = button_voltage_ranges[i][1];
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

