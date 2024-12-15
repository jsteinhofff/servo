

from bitstring import Bits

def add_bits(a, b):
    if len(a) != len(b):
        raise RuntimeError("Both bit strings must have same length")
    
    result = ""
    carry = False

    for i in range(len(a)):
        j = len(a) - i - 1
        value = int(a[j]) + int(b[j]) + int(carry)
        bit = value % 2
        carry = value > 1
        result = str(bit) + result
    
    return Bits(bin=result)

def two_complement(a):
    return add_bits(~a, Bits(uint=1, length=len(a)))


def subtract_bits(a, b):
    return add_bits(a, two_complement(b))


#a = Bits(bin='0011')
#b = Bits(bin='1111')
#c = Bits(bin="1100")

for i in range(32):
    for j in range(i, i + 15):
        a = Bits(uint=i % 16, length=4)
        b = Bits(uint=j % 16, length=4)
        c = subtract_bits(b, a)
        actual_difference = j - i
        print(f"{j} - {i} = {actual_difference}  |  {c.uint}")

        if actual_difference != c.uint:
            raise RuntimeError("Error!")


from fixedint import Int32


import matplotlib.pyplot as plt

values_real = []
values_int32 = []
values_try = []

for i in range(65000):
    values_real.append(i**2 / (1024.0 * 1024.0))
    values_int32.append((Int32(i) // Int32(1024) * Int32(i)) // Int32(1024))
    values_try.append(i // 16 * i // (1024 / 16) // 1024)



plt.plot(range(65000), values_real)
plt.plot(range(65000), values_int32)
plt.plot(range(65000), values_try)
plt.show()
