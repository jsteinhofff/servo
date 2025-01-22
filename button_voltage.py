
button_count = 4

resistor_ladder_ohms = [
    2000.0,
    5000.0,
    10000.0,
    20000.0
]

lowside_resistor_ohms = 1000.0

base_emmiter_drop_volts = 0.6

input_volts = 3.3

for i in range(button_count):
    divisor = resistor_ladder_ohms[i] / lowside_resistor_ohms + 1
    denominator = input_volts - base_emmiter_drop_volts

    value = denominator / divisor

    print(f"{i} => {value}")
