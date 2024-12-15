
import math
import random
import sys

random.seed(1)


a_pattern = [0, 0, 1, 1]
b_pattern = [0, 1, 1, 0]



a_bits = a_pattern * 10
b_bits = b_pattern * 10

pos = 4

class EncoderByPaper:
    def __init__(self):
        self.increments = 0

        self.encoder_table = [0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0]
        self.ab = 0

    def step(self, a, b):
        self.ab = self.ab << 2
        self.ab |= a << 0
        self.ab |= b << 1
        #print("  Encoder state ab: " + bin(self.ab & 0xf) + " means " + str(self.encoder_table[self.ab & 0xf]))
        self.increments += self.encoder_table[self.ab & 0xf]

class EncoderMatchPattern:
    def __init__(self):
        self.pattern = 46260
        self.pos = 8
        self.increments = 0
        #self.generate_pattern()
    
    def generate_pattern(self):
        pattern = 0
        for i, (a, b) in enumerate(zip(a_pattern * 2, b_pattern * 2)):
            pattern |= a << i * 2 + 1
            pattern |= b << i * 2 + 0
            #print(i, a, b, bin(pattern))
        self.pattern = pattern
        print(pattern)
    
    def step(self, a, b):
        forwards_pattern = (self.pattern >> (self.pos + 2)) & 0b11
        backwards_pattern = (self.pattern >> (self.pos - 2)) & 0b11

        ab = a << 1 | b
        if forwards_pattern == ab:
            self.increments += 1
            self.pos += 2
        elif backwards_pattern == ab:
            self.increments -= 1
            self.pos -= 2
        
        if self.pos <= 2:
            self.pos += 8
        elif self.pos >= 14:
            self.pos -= 8


encoder1 = EncoderByPaper()
encoder2 = EncoderMatchPattern()

increments_truth = 0

for i in range(1000):
    step = round(random.random() * 3 - 1.5)

    if (pos + step < len(a_bits)) and (pos + step >= 0):
        pos += step
        increments_truth += step

    a = a_bits[pos]
    b = b_bits[pos]

    encoder1.step(a, b)

    if encoder1.increments != increments_truth:
        print(f"Error for encoder1: {encoder1.increments} vs truth {increments_truth}")
    
    encoder2.step(a, b)

    if encoder2.increments != increments_truth:
        print(f"Error for encoder2: {encoder2.increments} vs truth {increments_truth}")

sys.exit()







# from https://picaxeforum.co.uk/threads/best-way-to-interface-with-rotary-encoder.30244/post-334305
# AB codes
# old new  INC/DEC
# 00  00    0 (same state, no change)
# 00  01   -1
# 00  10   +1
# 00  11    0 (skipped an edge)
# 01  00   +1
# 01  01    0 (same state, no change)
# 01  10    0 (skipped an edge)
# 01  11   -1
# 10  00   -1
# 10  01    0 (skipped an edge)
# 10  10    0 (same state, no change)
# 10  11   +1
# 11  00    0 (skipped an edge)
# 11  01   +1
# 11  10   -1
# 11  11    0 (same state, no change)

"""
# forwards
A = "_##__##__##__##__"
B = "__##__##__##__##_"
#    00 -> 10                   A changed   Change is Set
#     10 -> 11                              Change is Set
#      11 -> 01                 A changed
#       01 -> 00

_##__##__##__##__
__##__##__##__##_

Pos = 4
_
_

First change:
#
_

# note: need to encode a and b in single bitpattern => need to shift always by pairs
if pattern & (3 << Pos * 2 + 2) == current:
    forwards
    update pos
elif pattern & (3 << Pos * 2 - 2) == current:
    backwards
    update pos
else:
    hmm




# backwards
A = "__##__##__##__##_"
B = "_##__##__##__##__"
#    00 -> 01                               Change is Set
#     01 -> 11                  A changed   Change is Set
#      11 -> 10
#       10 -> 00                A changed


# bit fiddling approach

change = old_ab ^ new_ab;






# branching approach
if was even:
    if new_a != new_b: # now odd?
        if old_a != new_a:
            forwards
        else:
            backwards
else:
    if new_a == new_b: # now even?
        if old_b != new b:
            forwards
        else:
            backwards
old_a = new_a
old_b = new_b




"""