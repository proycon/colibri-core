cpdef getdata(self, Pattern pattern):
    if pattern in self:
        return self.data[pattern.cpattern]
    else:
        raise KeyError

def items(self):
    """Iterate over all patterns and their occurrence count in this model"""
    it = self.data.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while it != self.data.end():
        cpattern = deref(it).first
        value = deref(it).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(it)

cpdef add(self, Pattern pattern, int count=1):
    """Add a pattern to the unindexed model

    :param pattern: The pattern to add
    :type pattern: Pattern
    :param count: The number of occurrences
    :type count: int
    """
    self.data[pattern.cpattern] = self.data[pattern.cpattern] + count
