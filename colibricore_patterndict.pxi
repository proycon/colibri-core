def __len__(self):
    """Return the total number of patterns in the dictionary"""
    return self.data.size()

cpdef has(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.has(pattern.cpattern)

def __contains__(self, pattern):
    """Test if the pattern is in the dictionary"""
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.has(pattern)

def __getitem__(self, pattern):
    """Retrieve the value for a pattern in the dictionary"""
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.getdata(pattern)

def __setitem__(self, pattern, int value):
    """Set the value for a pattern in the dictionary"""
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    self[pattern] = value

def __iter__(self):
    """Iterate over all patterns in the dictionary"""
    cdef cPatternMap[uint32_t, cBaseValueHandler[uint32_t],uint64_t].iterator it = self.data.begin()
    cdef cPattern cpattern
    while it != self.data.end():
        cpattern = deref(it).first
        pattern = Pattern()
        pattern.bind(cpattern)
        yield pattern
        inc(it)

def items(self):
    """Iterate over all patterns and their values in the dictionary"""
    cdef cPatternMap[uint32_t, cBaseValueHandler[uint32_t],uint64_t].iterator it = self.data.begin()
    cdef cPattern cpattern
    cdef int value
    while it != self.data.end():
        cpattern = deref(it).first
        value = deref(it).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(it)
