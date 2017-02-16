cdef object corpus

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
    return self.data.type()

def version(self):
    """Return the version of the model type"""
    return self.data.version()

def occurrencecount(self, Pattern pattern):
    """Returns the number of times the specified pattern occurs in the training data

    :param pattern: A pattern
    :type pattern: Pattern
    :rtype: int
    """
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.occurrencecount(pattern.cpattern)

def coveragecount(self, Pattern pattern):
    """Returns the number of tokens all instances of the specified pattern cover in the training data

    :param pattern: A pattern
    :type pattern: Pattern
    :rtype: int
    """
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.coveragecount(pattern.cpattern)

def coverage(self, Pattern pattern):
    """Returns the number of tokens all instances of the specified pattern cover in the training data, as a fraction of the total amount of tokens

    :param pattern: A pattern
    :type pattern: Pattern
    :rtype: float
    """
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.coverage(pattern.cpattern)

def frequency(self, Pattern pattern):
    """Returns the frequency of the pattern within its category (ngram/skipgram/flexgram) and exact size class. For a bigram it will thus return the bigram frequency.

    :param pattern: A pattern
    :type pattern: Pattern
    :rtype: float
    """
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.frequency(pattern.cpattern)


def totaloccurrencesingroup(self, int category=0, int n=0):
    """Returns the total number of occurrences in the specified group, within the specified category and/or size class, you can set either to zero (default) to consider all. Example, category=Category.SKIPGRAM and n=0 would consider give the total occurrence count over all skipgrams.

    :param category: The category constraint (Category.NGRAM, Category.SKIPGRAM, Category.FLEXGRAM or 0 for no-constraint, default)
    :type category: int
    :param n: The size constraint (0= no constraint, default)
    :type n: int
    :rtype: int
    """
    return self.data.totaloccurrencesingroup(category,n)

def totalpatternsingroup(self, int category=0, int n=0):
    """Returns the total number of distinct patterns in the specified group, within the specified category and/or size class, you can set either to zero (default) to consider all. Example, category=Category.SKIPGRAM and n=0 would consider give the total number of distrinct skipgrams.

    :param category: The category constraint (Category.NGRAM, Category.SKIPGRAM, Category.FLEXGRAM or 0 for no-constraint, default)
    :type category: int
    :param n: The size constraint (0= no constraint, default)
    :type n: int
    :rtype: int
    """
    return self.data.totalpatternsingroup(category,n)

def totaltokensingroup(self, int category=0, int n=0):
    """Returns the total number of covered tokens in the specified group, within the specified category and/or size class, you can set either to zero (default) to consider all. Example, category=Category.SKIPGRAM and n=0 would consider give the total number of covered tokens over all skipgrams.

    :param category: The category constraint (Category.NGRAM, Category.SKIPGRAM, Category.FLEXGRAM or 0 for no-constraint, default)
    :type category: int
    :param n: The size constraint (0= no constraint, default)
    :type n: int
    :rtype: int
    """
    return self.data.totaltokensingroup(category,n)

def totalwordtypesingroup(self, int category=0, int n=0):
    """Returns the total number of covered word types (unigram types) in the specified group, within the specified category and/or size class, you can set either to zero (default) to consider all. Example, category=Category.SKIPGRAM and n=0 would consider give the total number of covered word types over all skipgrams.

    :param category: The category constraint (Category.NGRAM, Category.SKIPGRAM, Category.FLEXGRAM or 0 for no-constraint, default)
    :type category: int
    :param n: The size constraint (0= no constraint, default)
    :type n: int
    :rtype: int
    """
    return self.data.totaltokensingroup(category,n)
    return self.data.totalwordtypesingroup(category,n)

cdef has(self, Pattern pattern):
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.data.has(pattern.cpattern)

def __contains__(self, pattern):
    """Tests if a pattern is in the model:

    :param pattern: A pattern
    :type pattern: Pattern
    :rtype: bool

    Example::

        pattern in patternmodel
    """
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.has(pattern)

def __getitem__(self, pattern):
    """Retrieves the value for the pattern

    :param pattern: A pattern
    :type pattern: Pattern
    :rtype: int (for Unindexed Models), IndexData (for Indexed models)

    Example (unindexed model)::

        occurrences = model[pattern]
    """
    if not isinstance(pattern, Pattern):
        raise ValueError("Expected instance of Pattern")
    return self.getdata(pattern)



def __iter__(self):
    """Iterates over all patterns in the model. Also consider using the filter() or top() methods if they suit your needs, as they will be faster than doing it manually.

    Example::

        for pattern in model:
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

def __init__(self, str filename = "",PatternModelOptions options = None, constrainmodel = None, reverseindex = None):
    """Initialise a pattern model. Either an empty one or loading from file.

    :param filename: The name of the file to load, must be a valid colibri patternmodel file
    :type filename: str
    :param options: An instance of PatternModelOptions, containing the options used for loading
    :type options: PatternModelOptions
    :param constrainmodel: A model to be used as a constraint, only patterns occuring in this constraint model will be loaded/trained
    :type constrainmodel: IndexedPatternModel, UnindexedPatternModel or None (default)
    :param reverseindex: Corpus data to use as reverse index. On indexed models, this is required for various operations, including computation of skipgrams
    :type reverseindex: IndexedCorpus or None
    """

    self.corpus = None
    if reverseindex:
        self.loadreverseindex(reverseindex)

    if filename:
        if not options:
            options = PatternModelOptions()
        if filename and not os.path.exists(filename):
            raise FileNotFoundError(filename)
        self.load(filename,options, constrainmodel)

def load(self, str filename, PatternModelOptions options=None, constrainmodel = None):
    """Load a patternmodel from file.

    :param filename: The name of the file to load, must be a valid colibri patternmodel file
    :type filename: str
    :param options: An instance of PatternModelOptions, containing the options used for loading
    :type options: PatternModelOptions
    """
    if not options:
        options = PatternModelOptions()

    if filename and not os.path.exists(filename):
        raise FileNotFoundError(filename)

    if isinstance(constrainmodel, IndexedPatternModel):
        self.loadconstrainedbyindexedmodel(filename,options, constrainmodel)
    elif isinstance(constrainmodel, UnindexedPatternModel):
        self.loadconstrainedbyunindexedmodel(filename,options, constrainmodel)
    elif isinstance(constrainmodel, UnindexedPatternModel):
        self.loadconstrainedbysetmodel(filename,options, constrainmodel)
    else:
        self.data.load(encode(filename), options.coptions, NULL)

def loadreverseindex(self, IndexedCorpus reverseindex):
    self.data.reverseindex = reverseindex.data
    self.corpus = reverseindex #so python doesn't garbage collect the python object


cpdef loadconstrainedbyindexedmodel(self, str filename, PatternModelOptions options, IndexedPatternModel constrainmodel):
    self.data.load(encode(filename),options.coptions,  constrainmodel.getinterface())

cpdef loadconstrainedbyunindexedmodel(self, str filename, PatternModelOptions options, UnindexedPatternModel constrainmodel):
    self.data.load(encode(filename),options.coptions,  constrainmodel.getinterface())

cpdef loadconstrainedbysetmodel(self, str filename, PatternModelOptions options, PatternSetModel constrainmodel):
    self.data.load(encode(filename),options.coptions,  constrainmodel.getinterface())

def read(self, str filename, PatternModelOptions options=None):
    """Alias for load"""
    self.load(filename, options)

cpdef write(self, str filename):
    """Write a patternmodel to file

    :param filename: The name of the file to write to
    :type filename: str
    """
    self.data.write(encode(filename))

cpdef printmodel(self,ClassDecoder decoder):
    """Print the entire pattern model to stdout, a detailed overview

    :param decoder: The class decoder
    :type decoder: ClassDecoder
    """
    self.data.printmodel(&cout, decoder.data )

cpdef train(self, str filename, PatternModelOptions options, constrainmodel = None):
    """Train the patternmodel on the specified corpus data (a *.colibri.dat file)

    :param filename: The name of the file to load, must be a valid colibri.dat file. Can be set to an empty string if a corpus was pre-loaded already.
    :type filename: str
    :param options: An instance of PatternModelOptions, containing the options used for loading
    :type options: PatternModelOptions
    :param constrainmodel: A patternmodel or patternsetmodel to constrain training (default None)
    :type constrainmodel: None, UnindexedPatternModel, IndexedPatternModel, PatternSetModel
    """

    if self.data.reverseindex != NULL:
        filename = ""
    if isinstance(self, IndexedPatternModel) and self.data.reverseindex == NULL and options.DOSKIPGRAMS:
        raise ValueError("No reversindex was specified but you are requesting to train skipgrams, set reverseindex to an IndexedCorpus instance upon model construction")


    if constrainmodel:
        assert len(constrainmodel) >= 0
        if isinstance(constrainmodel, IndexedPatternModel):
            self.trainconstrainedbyindexedmodel(filename, options, constrainmodel)
        elif isinstance(constrainmodel, UnindexedPatternModel):
            self.trainconstrainedbyunindexedmodel(filename, options, constrainmodel)
        elif isinstance(constrainmodel, PatternSetModel):
            self.trainconstrainedbypatternsetmodel(filename, options, constrainmodel)
        elif isinstance(constrainmodel, PatternAlignmentModel_float):
            self.trainconstrainedbyalignmodel(filename, options, constrainmodel)
        else:
            raise ValueError("Invalid type for constrainmodel") #TODO: build patternmodel on the fly from an iterable of patterns or lower level patternstorage
    elif filename:
        self.data.train(<string> encode(filename),options.coptions, NULL, NULL)
    elif self.data.reverseindex == NULL:
        raise ValueError("No filename or reverseindex specified!")
    else:
        self.data.train(<istream*> NULL ,options.coptions, NULL, NULL)

cpdef train_filtered(self, str filename, PatternModelOptions options, PatternSet filterset):
    """Train the patternmodel on the specified corpus data (a *.colibri.dat file)

    :param filename: The name of the file to load, must be a valid colibri.dat file. Can be set to an empty string if a corpus was pre-loaded already.
    :type filename: str
    :param options: An instance of PatternModelOptions, containing the options used for loading
    :type options: PatternModelOptions
    :param filterset: An instance of PatternSet. A limited set of skipgrams/flexgrams to use as a filter, patterns will only be included if they are an instance of a skipgram in this list (i.e. disjunctive). Ngrams can also be included as filters, if a pattern subsumes one of the ngrams in the filter, it counts as a match (or if it matches it exactly).
    """
    cdef cPatternSet[uint] * cfilterset = address(filterset.data)

    if filename:
        self.data.train(<string> encode(filename),options.coptions, NULL, cfilterset)
    elif self.data.reverseindex == NULL:
        raise ValueError("No filename or reverseindex specified!")
    else:
        self.data.train(<istream*> NULL ,options.coptions, NULL, cfilterset)


cdef cPatternModelInterface* getinterface(self):
    return self.data.getinterface()

cpdef trainconstrainedbyindexedmodel(self, str filename, PatternModelOptions options, IndexedPatternModel constrainmodel):
    if filename:
        self.data.train(<string> encode(filename),options.coptions,  constrainmodel.getinterface(), NULL)
    elif self.data.reverseindex == NULL:
        raise ValueError("No filename or reverseindex specified!")
    else:
        self.data.train(<istream*> NULL,options.coptions,  constrainmodel.getinterface(), NULL)

cpdef trainconstrainedbyunindexedmodel(self, str filename, PatternModelOptions options, UnindexedPatternModel constrainmodel):
    if filename:
        self.data.train(<string> encode(filename),options.coptions,  constrainmodel.getinterface(), NULL)
    elif self.data.reverseindex == NULL:
        raise ValueError("No filename or reverseindex specified!")
    else:
        self.data.train(<istream*> NULL,options.coptions,  constrainmodel.getinterface(), NULL)

cpdef trainconstrainedbypatternsetmodel(self, str filename, PatternModelOptions options, PatternSetModel constrainmodel):
    if filename:
        self.data.train(<string> encode(filename),options.coptions,  constrainmodel.getinterface(), NULL)
    elif self.data.reverseindex == NULL:
        raise ValueError("No filename or reverseindex specified!")
    else:
        self.data.train(<istream*> NULL,options.coptions,  constrainmodel.getinterface(), NULL)

cpdef trainconstrainedbyalignmodel(self, str filename, PatternModelOptions options, PatternAlignmentModel_float constrainmodel):
    if filename:
        self.data.train(<string> encode(filename),options.coptions,  constrainmodel.getinterface(), NULL)
    elif self.data.reverseindex == NULL:
        raise ValueError("No filename or reverseindex specified!")
    else:
        self.data.train(<istream*>  NULL,options.coptions,  constrainmodel.getinterface(), NULL)

cpdef report(self):
    """Print a detailed statistical report to stdout"""
    self.data.report(&cout)

cpdef printhistogram(self):
    """Print a histogram to stdout"""
    self.data.histogram(&cout)



cpdef prune(self, int threshold, int n=0):
    """Prune all patterns occurring below the threshold.

    :param threshold: the threshold value (minimum number of occurrences)
    :type threshold: int
    :param n: prune only patterns of the specified size, use 0 (default) for no size limitation
    :type n: int
    """
    self.data.prune(threshold, n)


def reverseindex(self):
    """Returns the reverseindex associated with the model, this will be an instance of IndexedCorpus. Use getreverseindex( (sentence, token) ) instead if you want to query the reverse index."""
    return self.corpus


def getreverseindex(self, indexreference):
    """Generator over all patterns occurring at the specified index reference

    :param indexreference: a (sentence, tokenoffset) tuple
    """

    if not isinstance(indexreference, tuple) or not len(indexreference) == 2:
        raise ValueError("Expected tuple")
    if self.data.reverseindex == NULL:
        raise ValueError("No reverse index loaded")

    cdef int sentence = indexreference[0]
    cdef int token = indexreference[1]
    cdef cIndexReference ref = cIndexReference(sentence, token)
    cdef unordered_set[cPatternPointer] results = self.data.getreverseindex(ref)
    cdef unordered_set[cPatternPointer].iterator resit = results.begin()
    cdef cPattern cpattern
    while resit != results.end():
        cpattern = deref(resit).pattern()
        pattern = Pattern()
        pattern.bind(cpattern)
        yield pattern
        inc(resit)

def getreverseindex_bysentence(self, int sentence):
    """Generator over all patterns occurring in the specified sentence

    :param sentence: a sentence number
    """

    if self.data.reverseindex == NULL:
        raise ValueError("No reverse index loaded")

    cdef vector[pair[cIndexReference,cPatternPointer]] results = self.data.getreverseindex_bysentence(sentence)
    cdef vector[pair[cIndexReference,cPatternPointer]].iterator resit = results.begin()
    cdef pair[cIndexReference,cPatternPointer] p
    cdef cPattern cpattern
    while resit != results.end():
        p = deref(resit)
        pattern = Pattern()
        cpattern = p.second.pattern()
        pattern.bind(cpattern)
        yield (p.first.sentence, p.first.token), pattern
        inc(resit)


def histogram(self, unsigned int threshold=0, unsigned int cap=0, int category = 0, int size = 0):
    """Generator over a histogram of occurrence count data, produces (occurrencecount, frequency) tuples. A minimum threshold may be configured, or a cap on total number of occurrences may be specified (to get only the top occurrences). The histogram can be constrained by category and/or pattern size (if set to >0 values)"""
    cdef stdmap[unsigned int,unsigned int] hist
    cdef stdmap[unsigned int,unsigned int].iterator it
    self.data.histogram(hist,threshold,cap, category, size)
    it = hist.begin()
    while it != hist.end():
        yield deref(it).first, deref(it).second
        inc(it)


def top(self, int amount, int category = 0, int size = 0):
    """Generator over the top [amount] most occurring patterns (of specified category and size if set to values above 0). This is faster than iterating manually! Will return (pattern, occurrencecount) tuples (unsorted). Note that this may return less than the specified amount of patterns if there are multiple patterns with the same occurrence count in its tail. """

    cdef unsigned int smallest = self.data.topthreshold(amount, category, size)
    return self.filter(smallest, category, size)



def filter(self, unsigned int threshold, int category = 0, int size = 0):
    """Generator over patterns occurring over the set occurrence threshold (and of specified category and size if set to values above 0). This is faster than iterating and filtering manually! Will return (pattern, occurrencecount) tuples (unsorted)"""
    cdef unsigned int count
    it = self.data.begin()
    cdef cPattern cpattern
    while it != self.data.end():
        cpattern = deref(it).first
        inc(it)
        if ((category > 0) and (cpattern.category() != category)) or (size > 0) and (size != cpattern.n()):
            continue
        count = self.data.occurrencecount(cpattern)
        if count >= threshold:
            pattern = Pattern()
            pattern.bind(cpattern)
            yield pattern, count

def getinstance(self, tuple pos, Pattern pattern):
    """Gets a specific instance of a pattern (skipgram or flexgram), at the specified position. Raises a KeyError when not found."""
    if self.data.reverseindex == NULL:
        raise ValueError("No reverse index loaded")
    return self.corpus.getinstance(pos, pattern)

