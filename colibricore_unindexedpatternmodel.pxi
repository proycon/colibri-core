cpdef getdata(self, Pattern pattern):
    if pattern in self:
        return self.data[pattern.cpattern]
    else:
        raise KeyError

def items(self):
    """Iterate over all patterns and their occurrence count in this model"""
    it = self.data.begin()
    cdef cPattern cpattern
    cdef int value
    while it != self.data.end():
        cpattern = deref(it).first
        value = deref(it).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(it)
