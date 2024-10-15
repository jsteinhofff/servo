



void collect_analog_values(int *values)
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

int get_median_of_5(int arr[])
{
    int a = arr[0], b = arr[1], c = arr[2], d = arr[3], e = arr[4];
    int m1 = max(min(a, b), min(c, d));
    int m2 = min(max(a, b), max(c, d));
    return max(min(m1, m2), min(max(m1, m2), e));
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

    enum
    {
        button_count = 4
    };

    int button_ranges[button_count][2] = {
        {200, 400},
        {520, 620},
        {720, 820},
        {850, 950}};

    int button = 0;

    for (int i = 0; i < button_count; i++)
    {
        int lower = button_ranges[i][0];
        int upper = button_ranges[i][1];
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

