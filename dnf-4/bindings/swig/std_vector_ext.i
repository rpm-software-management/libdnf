%include <std_vector.i>
%include <std_string.i>


%template(VectorString) std::vector<std::string>;

%pythoncode %{

def VectorString__str__(self):
    return str(list(self))
VectorString.__str__ = VectorString__str__

def VectorString__eq__(self, other):
    return list(self) == list(other)
VectorString.__eq__ = VectorString__eq__

def VectorString__ne__(self, other):
    return list(self) != list(other)
VectorString.__ne__ = VectorString__ne__

def VectorString__lt__(self, other):
    return list(self) < list(other)
VectorString.__lt__ = VectorString__lt__

def VectorString__le__(self, other):
    return list(self) <= list(other)
VectorString.__le__ = VectorString__le__

def VectorString__gt__(self, other):
    return list(self) > list(other)
VectorString.__gt__ = VectorString__gt__

def VectorString__ge__(self, other):
    return list(self) >= list(other)
VectorString.__ge__ = VectorString__ge__

def VectorString__iadd__(self, value):
    self.extend(value)
    return self
VectorString.__iadd__ = VectorString__iadd__

def VectorString__imul__(self, value):
    data = list(self)
    data *= value
    self.clear()
    self.extend(data)
    return self
VectorString.__imul__ = VectorString__imul__

def VectorString__mul__(self, value):
    result = self.copy()
    result *= value
    return result
VectorString.__mul__ = VectorString__mul__

def VectorString__rmul__(self, value):
    return self * value
VectorString.__rmul__ = VectorString__rmul__

def VectorString__add__(self, value):
    result = self.copy()
    result.extend(value)
    return result
VectorString.__add__ = VectorString__add__

def VectorString__append(self, item):
    self.push_back(item)
VectorString.append = VectorString__append

def VectorString__copy(self):
    return VectorString(list(self))
VectorString.copy = VectorString__copy

def VectorString__count(self, item):
    return list(self).count(item)
VectorString.count = VectorString__count

def VectorString__extend(self, iterable):
    for i in iterable:
        self.push_back(i)
VectorString.extend = VectorString__extend

def VectorString__index(self, *args, **kwargs):
    data = list(self)
    return data.index(*args, **kwargs)
VectorString.index = VectorString__index

def VectorString__insert(self, *args, **kwargs):
    data = list(self)
    data.insert(*args, **kwargs)
    self.clear()
    self.extend(data)
VectorString.insert = VectorString__insert

def VectorString__remove(self, *args, **kwargs):
    data = list(self)
    data.remove(*args, **kwargs)
    self.clear()
    self.extend(data)
VectorString.remove = VectorString__remove

def VectorString__sort(self, *args, **kwargs):
    data = list(self)
    data.sort()
    self.clear()
    self.extend(data)
VectorString.sort = VectorString__sort

def VectorString__reverse(self, *args, **kwargs):
    data = list(self)
    data.reverse()
    self.clear()
    self.extend(data)
VectorString.reverse = VectorString__reverse
%}
