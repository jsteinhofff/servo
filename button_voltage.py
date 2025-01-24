
button_count = 4

resistor_ladder_ohms = [
    2000.0,
    5000.0,
    10000.0,
    20000.0
]

lowside_resistor_ohms = 1000.0
topside_resistor_ohms = 100000.0
base_emmiter_drop_volts = 0.65
input_volts = 3.3
offset = -0.06

# some short names for the formulas:

ras = resistor_ladder_ohms
rb = topside_resistor_ohms
rl = lowside_resistor_ohms
ui = input_volts
ut = base_emmiter_drop_volts


for i in range(button_count):
    denominator = ui - ut + (ras[i] * ui) / rb
    divisor = 1 + ras[i] / rl + ras[i] / rb

    value = denominator / divisor + offset

    value_millivolts = int(1000.0 * value)

    lower = value_millivolts - 50
    upper = value_millivolts + 50

    print(f"{i} => {value} => [{lower} - {upper}]")
