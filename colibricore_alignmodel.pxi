def __len__(self):
    """Returns the total number of distinct patterns in the model"""
    return self.data.size()

def __bool__(self):
    return self.data.size() > 0

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
    return self.data.getmodeltype()

def version(self):
    """Return the version of the model type"""
    return self.data.getmodelversion()

cdef has(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.has(pattern.cpattern)

cdef hastuple(self, Pattern pattern, Pattern pattern2):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.has(pattern.cpattern, pattern2.cpattern)

def __contains__(self, pattern):
    """Tests if a pattern is in the model:

    :param pattern: A pattern or a pair of patterns
    :type pattern: Pattern or 2-tuple of patterns
    :rtype: bool

    Example::

        sourcepattern in alignmodel
        (sourcepattern, targetpattern) in alignmodel
    """
    if isinstance(pattern, tuple):
        if len(pattern) != 2 or not isinstance(pattern[0], Pattern) or not isinstance(pattern[1], Pattern):
            raise ValueError("Expected instance of Pattern or 2-tuple of Patterns")
        return self.hastuple(pattern[0], pattern[1])

    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern or 2-tuple of Patterns")

    return self.has(pattern)

def __getitem__(self, pattern):
    """Retrieves the value for the pattern

    :param pattern: A pattern
    :type pattern: Pattern
    :rtype: int (for Unindexed Models), IndexData (for Indexed models)

    Example (unindexed model)::

        occurrences = model[pattern]
    """
    if isinstance(pattern, tuple):
        if len(pattern) != 2 or not isinstance(pattern[0], Pattern) or not isinstance(pattern[1], Pattern):
            raise ValueError("Expected instance of Pattern or 2-tuple of Patterns")
        return self.getdatatuple(pattern[0], pattern[1])

    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern or 2-tuple of Patterns")

    return self.getdata(pattern)



def __iter__(self):
    """Iterates over all source patterns in the model.

    Example::

        for sourcepattern in alignmodel:
            print(pattern.tostring(classdecoder))

    """
    it = self.data.begin()
    cdef cPattern cpattern
    while it != self.data.end():
        cpattern = deref(it).first
        pattern = Pattern()
        pattern.bind(cpattern)
        yield pattern
        inc(it)

def __init__(self, str filename = "",PatternModelOptions options = None):
    """Initialise an alignment model. Either an empty one or loading from file.

    :param filename: The name of the file to load, must be a valid colibri alignmodel file
    :type filename: str
    :param options: An instance of PatternModelOptions, containing the options used for loading
    :type options: PatternModelOptions

    """
    if filename:
        self.load(filename,options)

def load(self, str filename, PatternModelOptions options=None):
    """Load an alignment model from file

    :param filename: The name of the file to load, must be a valid colibri alignmodel file
    :type filename: str
    :param options: An instance of PatternModelOptions, containing the options used for loading
    :type options: PatternModelOptions
    """
    if options is None:
        options = PatternModelOptions()
    if filename and not os.path.exists(filename):
        raise FileNotFoundError(filename)
    self.data.load(encode(filename), options.coptions)

def read(self, str filename, PatternModelOptions options=None):
    """Alias for load"""
    self.load(filename, options)

cpdef write(self, str filename):
    """Write an alignment model to file

    :param filename: The name of the file to write to
    :type filename: str
    """
    self.data.write(encode(filename))

cdef cPatternModelInterface* getinterface(self):
    return self.data.getinterface()
