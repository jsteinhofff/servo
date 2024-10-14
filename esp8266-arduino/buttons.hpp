



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

