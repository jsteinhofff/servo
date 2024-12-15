
from fixedint import Int32


# taken from https://github.com/PabRod/TrackedNumberdiff/blob/main/TrackedNumberdiff/TrackedNumber.py

class NumberRange:
    def __init__(self, value, alias=""):
        self.min = value
        self.max = value
        self.value = value
        self.alias = alias
    
    def update(self, value):
        if value > self.max:
            self.max = value
        if value < self.min:
            self.min = value
        self.value = value
    
    def get_display(self, key):
        if self.alias:
            return self.alias
        else:
            return key


class TrackedNumber:

    states = {}

    def __init__(self, name, value=0.0):
        self.name = name

        plain_value = TrackedNumber.get_value(value)
        
        if self.name not in TrackedNumber.states:
            self.value = plain_value
            TrackedNumber.states[self.name] = NumberRange(plain_value)
        else:
            self.set_value(plain_value)
    
    def get_state(self):
        return TrackedNumber.states[self.name]

    @classmethod
    def get_value(cls, value):
        if isinstance(value, TrackedNumber):
            return value.value
        else:
            return value
    
    def set_value(self, value):
        self.value = value
        self.get_state().update(self.value)
    
    def set_alias(self, alias):
        self.get_state().alias = alias

    def __str__(self):
        mystate = self.get_state()
        return f"TrackedNumber('{self.name}', {self.value}, min: {mystate.min}, max: {mystate.max})"

    def __float__(self):
        """ 'Return as float' operator """
        return float(self.value)
    
    def __eq__(self, other):
        other_value = TrackedNumber.get_value(other)
        return self.value == other_value
    
    def __ne__(self, other):
        return not (self == other)
    
    def __gt__(self, other):
        other_value = TrackedNumber.get_value(other)
        return self.value > other_value

    def __ge__(self, other):
        other_value = TrackedNumber.get_value(other)
        return self.value >= other_value

    def __lt__(self, other):
        other_value = TrackedNumber.get_value(other)
        return self.value < other_value

    def __le__(self, other):
        other_value = TrackedNumber.get_value(other)
        return self.value <= other_value
    
    def __abs__(self):
        return abs(self.value)
    
    def __add__(self, other):
        return TrackedNumber(f"({self.name} + {other.name})", self.value + other.value)
    
    def __iadd__(self, other):
        if isinstance(other, TrackedNumber):
            self.set_value(self.value + other.value)
        else:
            self.set_value(self.value + other)
        return self

    def __neg__(self):
        return TrackedNumber("-" + self.name, -self.value)
    
    def __pos__(self):
        return self
    
    def __sub__(self, other):
        return TrackedNumber(f"({self.name} - {other.name})", self.value - other.value)
    
    def __mul__(self, other):
        return TrackedNumber(f"({self.name} * {other.name})", self.value * other.value)
    
    def __truediv__(self, other):
        return TrackedNumber(f"({self.name} / {other.name})", self.value / other.value)
    
    def __floordiv__(self, other):
        return TrackedNumber(f"({self.name} // {other.name})", self.value // other.value)
    
    def __pow__(self, other):
        return TrackedNumber(f"({self.name} ** {other})", self.value ** other)
    
    # __radd__, __rsub__, __rtruediv__ not implemented since we only operate with other TrackedNumber instances

    @classmethod
    def report_states(cls):
        for key, value in cls.states.items():
            if isinstance(value.value, (int, Int32)):
                print(f"{value.get_display(key):80}: min: {value.min:10d}, max: {value.max:10d}, value: {value.value}")
            elif isinstance(value.value, float):
                print(f"{value.get_display(key):80}: min: {value.min:12.3f}, max: {value.max:12.3f}, value: {value.value}")
            else:
                print(f"{value.get_display(key):80}: min: {value.min}, max: {value.max}, value: {value.value}, {type(value.value)}")


class TrackedInt(TrackedNumber):

    def __init__(self, name, value=0):
        super().__init__(name, int(value))

    def __truediv__(self, other):
        return TrackedInt(f"({self.name} / {other.name})", int(self.value / other.value)) 

    def __floordiv__(self, other):
        return TrackedInt(f"({self.name} / {other.name})", self.value // other.value)


class TrackedInt32(TrackedNumber):

    def __init__(self, name, value=0):
        super().__init__(name, Int32(value))

    def __truediv__(self, other):
        return TrackedNumber(f"({self.name} / {other.name})", self.value / other.value)

    def __floordiv__(self, other):
        return TrackedInt32(f"({self.name} / {other.name})", Int32(self.value // other.value))


