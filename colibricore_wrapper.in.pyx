#embedsignature=True
#*****************************
# Colibri Core
#   by Maarten van Gompel
#   Centre for Language Studies
#   Radboud University Nijmegen
#
#   http://proycon.github.io/colibri-core
#
#   Licensed under GPLv3
#****************************/
from libcpp.string cimport string
from libcpp cimport bool
from libcpp.vector cimport vector
from cython.operator cimport dereference as deref, preincrement as inc
from cython import address
from colibricore_classes cimport ClassEncoder as cClassEncoder, ClassDecoder as cClassDecoder, Pattern as cPattern, IndexedData as cIndexedData, IndexReference as cIndexReference, PatternMap as cPatternMap, PatternSet as cPatternSet, PatternModelOptions as cPatternModelOptions, PatternModel as cPatternModel,IndexedPatternModel as cIndexedPatternModel, IndexedDataHandler as cIndexedDataHandler, BaseValueHandler as cBaseValueHandler, cout, t_relationmap, t_relationmap_double, t_relationmap_iterator, t_relationmap_double_iterator
from unordered_map cimport unordered_map
from libc.stdint cimport *
from libcpp.map cimport map as stdmap
from libcpp.utility cimport pair

class Category:
    """Pattern Category"""
    NGRAM=1
    SKIPGRAM=2
    FLEXGRAM=3

cdef class ClassEncoder:
    """The class encoder allows patterns to be built from their string representation. Load in a class file and invoke the ``buildpattern()`` method"""

    cdef cClassEncoder *thisptr

    def __cinit__(self):
        self.thisptr = NULL

    def __cinit__(self, str filename):
        self.thisptr = new cClassEncoder(filename.encode('utf-8'))

    def __dealloc__(self):
        del self.thisptr

    def __len__(self):
        """Returns the total number of classes"""
        return self.thisptr.size()

    def buildpattern(self, str text, bool allowunknown=True, bool autoaddunknown=False):
        """Builds a pattern: converts a string representation into a Pattern

        :param text: The actual text of the pattern
        :type text: str
        :param allowunknown: Encode unknown classes as 'unknown', a single class for all, rather than failing with an exception if a word type is unseen (bool, default=False)
        :type allowunknown: bool
        :param autoaddunknown: Automatically add unknown classes to the model (bool, default=False)
        :type autoaddunknown: bool
        :rtype: Pattern
        """

        c_pattern = self.thisptr.buildpattern(text.encode('utf-8'), allowunknown, autoaddunknown)
        pattern = Pattern()
        pattern.bind(c_pattern)
        return pattern


cdef class ClassDecoder:
    """The Class Decoder allows Patterns to be decoded back to their string representation. An instance of ClassDecoder is passed to Pattern.tostring()"""

    cdef cClassDecoder *thisptr

    def __cinit__(self):
        self.thisptr = NULL

    def __cinit__(self, str filename):
        self.thisptr = new cClassDecoder(filename.encode('utf-8'))

    def __dealloc__(self):
        del self.thisptr

    def __len__(self):
        """Returns the total number of classes"""
        return self.thisptr.size()


cdef class Pattern:
    """The Pattern class contains an ngram, skipgram or flexgram, and allows a wide variety of actions to be performed on it. It is stored in a memory-efficient fashion and facilitating fast operation and comparison. Use ClassEncoder.buildpattern to build a pattern."""

    cdef cPattern cpattern

    cdef cPattern getcpattern(self):
        return self.cpattern

    cdef bind(self, cPattern cpattern):
        self.cpattern = cpattern

    def tostring(self, ClassDecoder decoder):
        """Convert a Pattern back to a str

        :param decoder: the class decoder to use
        :type decoder: ClassDecoder
        :rtype: str
        """

        return str(self.cpattern.tostring(deref(decoder.thisptr)),'utf-8')

    def __contains__(self, Pattern pattern):
        """Check if the specified pattern occurs within this larger pattern.
        
        :param pattern: the subpattern to look for
        :type pattern: Pattern
        :rtype: bool

        Example::

            subpattern in pattern
        """
        cdef bool r
        r = self.cpattern.contains(pattern.cpattern)
        return r

    def __len__(self):
        """Returns the length of the pattern in words/tokens::

            len(pattern)
        """
        return self.cpattern.n()

    def __copy__(self):
        """Produces a copy of the pattern (deep)::

            pattern2 = copy(pattern)
        """
        cdef cPattern c_pattern
        c_pattern = cPattern(self.cpattern) #copy constructor
        newpattern = Pattern()
        newpattern.bind(c_pattern)
        return newpattern

    def __deepcopy__(self):
        """Produces a copy of the pattern (deep)::

            pattern2 = copy(pattern)
        """
        cdef cPattern c_pattern
        c_pattern = cPattern(self.cpattern) #copy constructor
        newpattern = Pattern()
        newpattern.bind(c_pattern)
        return newpattern

    def concat(self, Pattern pattern):
        cdef cPattern newcpattern = self.cpattern + pattern.cpattern
        newpattern = Pattern()
        newpattern.bind(newcpattern)
        return newpattern

    def __add__(self, Pattern other):
        """Concatenate two patterns together, forming a larger one::

                pattern3 = pattern1 + pattern2
        """
        return self.concat(other)

    def __getitem__(self, item):
        """Retrieves a word/token from the pattern::

            unigram = pattern[index]

        Or retrieves a subpattern::

            subpattern = pattern[begin:end]


        :param item: an index or slice
        :rtype: a Pattern instance
        """

        cdef cPattern c_pattern
        if isinstance(item, slice):
            c_pattern = cPattern(self.cpattern, item.start, item.stop-item.start)
            newpattern = Pattern()
            newpattern.bind(c_pattern)
            return newpattern
        else:
            c_pattern = cPattern(self.cpattern, item.start, item.start+1)
            newpattern = Pattern()
            newpattern.bind(c_pattern)
            return newpattern

    def __iter__(self):
        """Iterates over all words/tokens in this pattern"""
        for i in range(0, len(self)):
            yield self[i]

    def bytesize(self):
        """Returns the number of bytes used to encode this pattern in memory"""
        return self.cpattern.bytesize()

    def skipcount(self):
        """Returns the number of gaps in this pattern"""
        return self.cpattern.skipcount()

    def category(self):
        """Returns the category of this pattern
        :rtype: Category.NGRAM (1), Category.SKIPGRAM (2), or Category.FLEXGRAM (3)
        """
        return self.cpattern.category()

    def __hash__(self):
        """Returns the hashed value for this pattern"""
        return self.cpattern.hash()

    def __richcmp__(Pattern self, Pattern other, int op):
        """Allows comparisons of two patterns using the usual operators, <, > , <=, <=, =="""
        if op == 2: # ==
            return self.cpattern == other.cpattern
        elif op == 0: #<
            return self.cpattern < other.cpattern
        elif op == 4: #>
            return self.cpattern > other.cpattern
        elif op == 3: #!=
            return not( self.cpattern == other.cpattern)
        elif op == 1: #<=
            return (self.cpattern == other.cpattern) or (self.cpattern < other.cpattern)
        elif op == 5: #>=
            return (self.cpattern == other.cpattern) or (self.cpattern > other.cpattern)



    cdef Pattern add(Pattern self, Pattern other):
        cdef cPattern newcpattern = self.cpattern + other.cpattern
        newpattern = Pattern()
        newpattern.bind(newcpattern)
        return newpattern

    def ngrams(self,int n):
        """Generator iterating over all ngrams of a particular size that are enclosed within this pattern. Despite the name, this may also return skipgrams!

        :param n: The desired size to obtain
        :type n: int
        :rtype: generator over Pattern instances
        """
        cdef vector[cPattern] result
        self.cpattern.ngrams(result, n)
        cdef cPattern cngram
        cdef vector[cPattern].iterator it = result.begin()
        while it != result.end():
            cngram  = deref(it)
            ngram = Pattern()
            ngram.bind(cngram)
            yield ngram
            inc(it)

    def parts(self):
        """Generating iterating over the consecutive non-gappy parts in a skipgram of flexgram

        :rtype: generator over Pattern instances
        """
        cdef vector[cPattern] result
        self.cpattern.parts(result)
        cdef cPattern cngram
        cdef vector[cPattern].iterator it = result.begin()
        while it != result.end():
            cngram  = deref(it)
            ngram = Pattern()
            ngram.bind(cngram)
            yield ngram
            inc(it)

    def gaps(self):
        """Generator iterating over the gaps in a skipgram or flexgram, return a tuple (begin,length) for each. For flexgrams, the minimum length (1) is always returned.

        :rtype: generator over (begin, length) tuples
        """
        cdef vector[pair[int,int]] result
        self.cpattern.gaps(result)
        cdef vector[pair[int,int]].iterator it = result.begin()
        cdef pair[int,int] p
        while it != result.end():
            p  = deref(it)
            yield (p.first, p.second)
            inc(it)

    def toflexgram(self):
        """Converts a skipgram to a flexgram

        :rtype: Pattern
        """
        cdef cPattern newcpattern = self.cpattern.toflexgram()
        newpattern = Pattern()
        newpattern.bind(newcpattern)
        return newpattern

    def subngrams(self,int minn=1,int maxn=9):
        """Generator iterating over all ngrams of all sizes that are enclosed within this pattern. Despite the name, this may also return skipgrams!
        :param minn: minimum length (default 1)
        :type minn: int
        :param maxn: maximum length (default unlimited)
        :type maxn: int
        :rtype: generator over Pattern instances
        """
        minn = max(1,minn)
        maxn = min(maxn, len(self) -1 )
        for n in range(minn,maxn+1):
            for pattern in self.ngrams(n):
                yield pattern




cdef class IndexedData:
    """IndexedData is essentially a set of indexes in the form of (sentence,token) tuples, sentence is generally 1-indexed, token is always 0-indexed. It is used by Indexed Pattern Models to keep track of exact occurrences of all the patterns. Use len() to if you're merely interested in the number of occurrences, rather than their exact wherabouts."""

    cdef cIndexedData data

    cdef bind(self, cIndexedData cdata):
        self.data = cdata


    def __contains__(self, item):
        """Tests if the specified (sentence,token) tuple is in the set"""
        if not isinstance(item, tuple) or len(item) != 2:
            raise ValueError("Item should be a 2-tuple (sentence,token)")
        cdef cIndexReference ref = cIndexReference(item[0], item[1])
        return self.data.has(ref)

    def __iter__(self):
        """Iterate over all (sentence,token) tuples in the set"""
        cdef cIndexReference ref
        cdef cIndexedData.iterator it = self.data.begin()
        while it != self.data.end():
            ref  = deref(it)
            yield (ref.sentence, ref.token)
            inc(it)

    def __len__(self):
        """Returns the number of occurrences, i.e. the length of the set"""
        return self.data.size()

    def __int__(self):
        return self.data.size()


cdef class PatternSet:
    """This is a simple low-level set that contains Pattern instances"""
    cdef cPatternSet[uint] data
    @include colibricore_patternset.pxi


cdef class PatternDict_int32: #maps Patterns to uint32
    """This is a simple low-level dictionary that takes Pattern instances as keys, and integer (max 32 bit, unsigned) as value. For complete pattern models, use IndexedPatternModel or UnindexPatternModel instead."""

    cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint] data
    cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint].iterator it
    cdef uint32_t value

    @include colibricore_patterndict.pxi

    def __setitem__(self, pattern, uint32_t v):
        """Set the value for a pattern in the dictionary

        :param pattern: the pattern
        :param value: its value
        """
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        self[pattern] = v


cdef class PatternDict_int: #maps Patterns to uint32
    """This is a simple low-level dictionary that takes Pattern instances as keys, and integer (unsigned 64 bit) as value. For complete pattern models, use IndexedPatternModel or UnindexPatternModel instead."""

    cdef cPatternMap[uint,cBaseValueHandler[uint],uint] data
    cdef cPatternMap[uint,cBaseValueHandler[uint],uint].iterator it
    cdef int value

    @include colibricore_patterndict.pxi

    def __setitem__(self, pattern, int v):
        """Set the value for a pattern in the dictionary

        :param pattern: the pattern
        :param value: its value
        """
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        self[pattern] = v

cdef class PatternDict_float: #maps Patterns to uint32
    """This is a simple low-level dictionary that takes Pattern instances as keys, and float (double) as value. For complete pattern models, use IndexedPatternModel or UnindexPatternModel instead."""

    cdef cPatternMap[float,cBaseValueHandler[float],uint] data
    cdef cPatternMap[float,cBaseValueHandler[float],uint].iterator it
    cdef float value

    @include colibricore_patterndict.pxi
    
    def __setitem__(self, pattern, float v):
        """Set the value for a pattern in the dictionary

        :param pattern: the pattern
        :param value: its value
        """
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        self[pattern] = v


cdef class IndexedPatternModel:
    """Indexed Pattern Model"""

    cdef cIndexedPatternModel[cPatternMap[cIndexedData,cIndexedDataHandler,uint64_t]] data

    @include colibricore_patternmodel.pxi

    cdef getdata(self, Pattern pattern):
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        cdef cIndexedData cvalue
        if pattern in self:
            cvalue = self.data[pattern.cpattern]
            value = IndexedData()
            value.bind(cvalue)
            return value
        else:
            raise KeyError


    def __iter__(self):
        """Iterate over all patterns in this model"""
        cdef cPatternModel[cIndexedData,cIndexedDataHandler,cPatternMap[cIndexedData,cIndexedDataHandler,uint64_t]].iterator it = self.data.begin()
        cdef cPattern cpattern
        while it != self.data.end():
            cpattern = deref(it).first
            pattern = Pattern()
            pattern.bind(cpattern)
            yield pattern
            inc(it)

    def items(self):
        """Iterate over all patterns and their index data (IndexedData instances) in this model"""
        cdef cPatternModel[cIndexedData,cIndexedDataHandler,cPatternMap[cIndexedData,cIndexedDataHandler,uint64_t]].iterator it = self.data.begin()
        cdef cPattern cpattern
        cdef cIndexedData cvalue
        while it != self.data.end():
            cpattern = deref(it).first
            cvalue = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            value = IndexedData()
            value.bind(cvalue)
            yield (pattern,value)
            inc(it)


    cpdef outputrelations(self, Pattern pattern, ClassDecoder decoder):
        """Compute and output (to stdout) all relations for the specified pattern:
        
        :param pattern: The pattern to output relations for
        :type pattern: Pattern
        :param decoder: The class decoder
        :type decoder: ClassDecoder
        """
        self.data.outputrelations(pattern.cpattern,deref(decoder.thisptr),&cout)


    def getsubchildren(self, Pattern pattern):
        """Get subsumption children for the specified pattern
        :param pattern: The pattern
        :type pattern: Pattern
        :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrences for this particularrelationship
        """
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getsubchildren(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()

        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield (pattern,value)
            inc(it)

    def getsubparents(self, Pattern pattern):
        """Get subsumption parents for the specified pattern
        :param pattern: The pattern
        :type pattern: Pattern
        :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrences for this particularrelationship
        """
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getsubparents(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()
        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield (pattern,value)
            inc(it)

    def getleftneighbours(self, Pattern pattern):
        """Get left neighbours for the specified pattern
        :param pattern: The pattern
        :type pattern: Pattern
        :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrences for this particularrelationship
        """
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getleftneighbours(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()
        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield (pattern,value)
            inc(it)

    def getrightneighbours(self, Pattern pattern):
        """Get right neighbours for the specified pattern
        :param pattern: The pattern
        :type pattern: Pattern
        :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrences for this particularrelationship
        """
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getrightneighbours(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()
        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield (pattern,value)
            inc(it)

    def getskipcontent(self, Pattern pattern):
        """Get skip content for the specified pattern
        :param pattern: The pattern
        :type pattern: Pattern
        :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrence for this particularrelationship
        """
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getskipcontent(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()
        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield (pattern,value)
            inc(it)

cdef class UnindexedPatternModel:
    """Unindexed Pattern Model, less flexible and powerful than its indexed counterpart, but smaller memory footprint"""
    cdef cPatternModel[uint32_t,cBaseValueHandler[uint32_t],cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint64_t]] data
    cdef cPatternModel[uint32_t,cBaseValueHandler[uint32_t],cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint64_t]].iterator it

    @include colibricore_patternmodel.pxi

    cpdef getdata(self, Pattern pattern):
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")
        cdef cIndexedData cvalue
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





cdef class PatternModelOptions:
    """Options for Pattern model, you can get and set the following attributes:

    * MINTOKENS - The token threshold, patterns with an occurrence below this will be pruned
    * MAXLENGTH - Maximum pattern length
    * DOSKIPGRAMS - Compute skipgrams?
    * DOSKIPGRAMS_EXHAUSTIVE - Compute skipgrams exhaustively?
    * MINSKIPTYPES - Minimum amount of different skip content types
    * DOREVERSEINDEX - Build reverse index? (default: True)
    * DEBUG
    * QUIET (default: False)

    These can also be passed at keyword arguments to the constructor, in a case insensitive fashion::

        options = PatternModelOptions(mintokens=3)
    """
    cdef cPatternModelOptions coptions

    def __init__(self, **kwargs):
        for kwarg, value in kwargs.items():
            setattr(self,kwarg.upper(), value)

    def __setattr__(self,key, value):
        if key == 'MINTOKENS':
            self.coptions.MINTOKENS = value
        elif key == 'MAXLENGTH':
            self.coptions.MAXLENGTH = value
        elif key == 'DOSKIPGRAMS':
            self.coptions.DOSKIPGRAMS = value
        elif key == 'DOSKIPGRAMS_EXHAUSTIVE':
            self.coptions.DOSKIPGRAMS_EXHAUSTIVE = value
        elif key == 'MINSKIPTYPES':
            self.coptions.MINSKIPTYPES = value
        elif key == 'DOREVERSEINDEX':
            self.coptions.DOREVERSEINDEX = value
        elif key == 'DEBUG':
            self.coptions.DEBUG = value
        elif key == 'QUIET':
            self.coptions.QUIET = value
        else:
            raise KeyError

    def __getattr__(self,key):
        if key == 'MINTOKENS':
            return self.coptions.MINTOKENS
        elif key == 'MAXLENGTH':
            return self.coptions.MAXLENGTH
        elif key == 'DOSKIPGRAMS':
            return self.coptions.DOSKIPGRAMS
        elif key == 'DOSKIPGRAMS_EXHAUSTIVE':
            return self.coptions.DOSKIPGRAMS_EXHAUSTIVE
        elif key == 'MINSKIPTYPES':
            return self.coptions.MINSKIPTYPES
        elif key == 'DOREVERSEINDEX':
            return self.coptions.DOREVERSEINDEX
        elif key == 'DEBUG':
            return self.coptions.DEBUG
        elif key == 'QUIET':
            return self.coptions.QUIET
        else:
            raise KeyError
