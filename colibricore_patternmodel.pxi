def __len__(self):
    return self.data.size()

def types(self):
    return self.data.types()

def tokens(self):
    return self.data.tokens()

def minlength(self):
    return self.data.minlength()

def maxlength(self):
    return self.data.maxlength()

def type(self):
    return self.data.type()

def version(self):
    return self.data.version()

def occurrencecount(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.occurrencecount(pattern.cpattern)

def coveragecount(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.coveragecount(pattern.cpattern)

def coverage(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.coverage(pattern.cpattern)

def frequency(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.coverage(pattern.cpattern)


def totaloccurrencesingroup(self, int category=0, int n=0):
    return self.data.totaloccurrencesingroup(category,n)

def totalpatternsingroup(self, int category=0, int n=0):
    return self.data.totalpatternsingroup(category,n)

def totaltokensingroup(self, int category=0, int n=0):
    return self.data.totaltokensingroup(category,n)

def totalwordtypesingroup(self, int category=0, int n=0):
    return self.data.totalwordtypesingroup(category,n)

cdef has(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.has(pattern.cpattern)

def __contains__(self, pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.has(pattern)

def __getitem__(self, pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.getdata(pattern)



def __iter__(self):
    it = self.data.begin()
    cdef cPattern cpattern
    while it != self.data.end():
        cpattern = deref(it).first
        pattern = Pattern()
        pattern.bind(cpattern)
        yield pattern
        inc(it)

def __init__(self, str filename = "",PatternModelOptions options = None):
    if filename:
        if not options:
            options = PatternModelOptions()
        self.load(filename,options)

def load(self, str filename, PatternModelOptions options=None):
    if not options:
        options = PatternModelOptions()
    self.data.load(filename.encode('utf-8'), options.coptions)

cpdef write(self, str filename):
    self.data.write(filename.encode('utf-8'))

cpdef printmodel(self,ClassDecoder decoder):
    self.data.printmodel(&cout, deref(decoder.thisptr) )

cpdef train(self, str filename, PatternModelOptions options):
    self.data.train(filename.encode('utf-8'),options.coptions)

cpdef report(self):
    self.data.report(&cout)

cpdef histogram(self):
    self.data.report(&cout)

cpdef prune(self, int threshold, int n=0):
    self.data.prune(threshold, n)
