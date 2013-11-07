
def __len__(self):
    """Return the total number of patterns in the dictionary"""
    return self.data.size()

cpdef has(self, Pattern pattern):
    return self.data.has(pattern.cpattern)

def __contains__(self, Pattern pattern):
    """Test if the pattern is in the dictionary"""
    return self.has(pattern)

def __iter__(self):
    """Iterate over all patterns in the dictionary"""
    it = self.data.begin()
    cdef cPattern cpattern
    while it != self.data.end():
        cpattern = deref(it).first
        pattern = Pattern()
        pattern.bind(cpattern)
        yield pattern
        inc(it)

def __getitem__(self, Pattern pattern):
    """Retrieve the value for a pattern in the dictionary
    
    :param pattern: A pattern
    :type pattern: Pattern
    """
    return self.data[pattern.cpattern]



def items(self):
    """Iterate over all patterns and their values in the dictionary"""
    it = self.data.begin()
    cdef cPattern cpattern
    while it != self.data.end():
        cpattern = deref(it).first
        value = deref(it).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(it)


def read(self, str filename):
    if os.path.exists(filename):
        self.data.read(filename)
    else:
        raise FileNotFoundError

def write(self, str filename):
    if os.path.exists(filename):
        self.data.read(filename)
    else:
        raise FileNotFoundError
