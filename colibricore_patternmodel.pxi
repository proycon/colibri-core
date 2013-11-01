def __len__(self):
    """Returns the total number of distinct patterns in the model"""
    return self.data.size()

def types(self):
    """Returns the total number of distinct word types in the training data"""
    return self.data.types()

def tokens(self):
    """Returns the total number of tokens in the training data"""
    return self.data.tokens()

def minlength(self):
    """Returns the minimum pattern length in the model"""
    return self.data.minlength()

def maxlength(self):
    """Returns the maximum pattern length in the model"""
    return self.data.maxlength()

def type(self):
    """Returns the model type (10 = UNINDEXED, 20 = INDEXED)"""
    return self.data.type()

def version(self):
    """Return the version of the model type"""
    return self.data.version()

def occurrencecount(self, Pattern pattern):
    """Returns the number of times the specified pattern occurs in the training data"""
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.occurrencecount(pattern.cpattern)

def coveragecount(self, Pattern pattern):
    """Returns the number of tokens all instances of the specified pattern cover in the training data"""
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.coveragecount(pattern.cpattern)

def coverage(self, Pattern pattern):
    """Returns the number of tokens all instances of the specified pattern cover in the training data, as a fraction of the total amount of tokens"""
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.coverage(pattern.cpattern)

def frequency(self, Pattern pattern):
    """Returns the frequency of the pattern within its category (ngram/skipgram/flexgram) and exact size class. For a bigram it will thus return the bigram frequency."""
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.frequency(pattern.cpattern)


def totaloccurrencesingroup(self, int category=0, int n=0):
    """Returns the total number of occurrences in the specified group, within the specified category and/or size class, you can set either to zero (default) to consider all. Example, category=Category.SKIPGRAM and n=0 would consider give the total occurrence count over all skipgrams.""" 
    return self.data.totaloccurrencesingroup(category,n)

def totalpatternsingroup(self, int category=0, int n=0):
    """Returns the total number of distinct patterns in the specified group, within the specified category and/or size class, you can set either to zero (default) to consider all. Example, category=Category.SKIPGRAM and n=0 would consider give the total number of distrinct skipgrams.""" 
    return self.data.totalpatternsingroup(category,n)

def totaltokensingroup(self, int category=0, int n=0):
    """Returns the total number of covered tokens in the specified group, within the specified category and/or size class, you can set either to zero (default) to consider all. Example, category=Category.SKIPGRAM and n=0 would consider give the total number of covered tokens over all skipgrams.""" 
    return self.data.totaltokensingroup(category,n)

def totalwordtypesingroup(self, int category=0, int n=0):
    """Returns the total number of covered word types (unigram types) in the specified group, within the specified category and/or size class, you can set either to zero (default) to consider all. Example, category=Category.SKIPGRAM and n=0 would consider give the total number of covered word types over all skipgrams.""" 
    return self.data.totaltokensingroup(category,n)
    return self.data.totalwordtypesingroup(category,n)

cdef has(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.has(pattern.cpattern)

def __contains__(self, pattern):
    """Tests if a pattern is in the model"""
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.has(pattern)

def __getitem__(self, pattern):
    """Retrieves the value for the pattern"""
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.getdata(pattern)



def __iter__(self):
    """Iterates over all patterns in the model"""
    it = self.data.begin()
    cdef cPattern cpattern
    while it != self.data.end():
        cpattern = deref(it).first
        pattern = Pattern()
        pattern.bind(cpattern)
        yield pattern
        inc(it)

def __init__(self, str filename = "",PatternModelOptions options = None):
    """Initialise a pattern model. Either an empty one or loading from file"""
    if filename:
        if not options:
            options = PatternModelOptions()
        self.load(filename,options)

def load(self, str filename, PatternModelOptions options=None):
    """Load a patternmodel from file"""
    if not options:
        options = PatternModelOptions()
    self.data.load(filename.encode('utf-8'), options.coptions)

cpdef write(self, str filename):
    """Write a patternmodel to file"""
    self.data.write(filename.encode('utf-8'))

cpdef printmodel(self,ClassDecoder decoder):
    """Print the entire pattern model to stdout, a detailed overview"""
    self.data.printmodel(&cout, deref(decoder.thisptr) )

cpdef train(self, str filename, PatternModelOptions options):
    """Train the patternmodel on the specified corpus data (a *.colibri.dat file)"""
    self.data.train(filename.encode('utf-8'),options.coptions)

cpdef report(self):
    """Print a detailed statistical report to stdout"""
    self.data.report(&cout)

cpdef histogram(self):
    """Print a histogram to stdout"""
    self.data.report(&cout)

cpdef prune(self, int threshold, int n=0):
    """Prune all patterns occurring below the threshold.
    
    :param n: prune only patterns of the specified size, use 0 (default) for no size limitation
    :type n: int
    """
    self.data.prune(threshold, n)
