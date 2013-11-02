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

def __iter__(self):
    """Iterate over all patterns in the dictionary"""
    it = self.data.begin()
    cdef cPattern cpattern
    while it != self.data.end():
        cpattern = deref(it)
        pattern = Pattern()
        pattern.bind(cpattern)
        yield pattern
        inc(it)

def add(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    self.data.insert(pattern.cpattern)
