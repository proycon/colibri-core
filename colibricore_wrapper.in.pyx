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
from colibricore_classes cimport ClassEncoder as cClassEncoder, ClassDecoder as cClassDecoder, Pattern as cPattern, IndexedData as cIndexedData, IndexReference as cIndexReference, PatternMap as cPatternMap, OrderedPatternMap as cOrderedPatternMap, PatternSet as cPatternSet, PatternModelOptions as cPatternModelOptions, PatternModel as cPatternModel,IndexedPatternModel as cIndexedPatternModel, IndexedDataHandler as cIndexedDataHandler, BaseValueHandler as cBaseValueHandler, cout, t_relationmap, t_relationmap_double, t_relationmap_iterator, t_relationmap_double_iterator,IndexedCorpus as cIndexedCorpus, BEGINPATTERN as cBEGINPATTERN, ENDPATTERN as cENDPATTERN, SKIPPATTERN as cSKIPPATTERN, FLEXPATTERN as cFLEXPATTERN, UNKPATTERN as cUNKPATTERN, AlignedPatternMap as cAlignedPatternMap
from unordered_map cimport unordered_map
from libc.stdint cimport *
from libcpp.map cimport map as stdmap
from libcpp.utility cimport pair
import os.path

class Category:
    """Pattern Category"""
    NGRAM=1
    SKIPGRAM=2
    FLEXGRAM=3

cdef class ClassEncoder:
    """The class encoder allows patterns to be built from their string representation. Load in a class file and invoke the ``buildpattern()`` method"""

    cdef cClassEncoder data

    def __init__(self, str filename=None):
        if filename:
            self.data.load(filename.encode('utf-8'))

    def __len__(self):
        """Returns the total number of classes"""
        return self.data.size()

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

        cdef cPattern cpattern = self.data.buildpattern(text.encode('utf-8'), allowunknown, autoaddunknown)
        pattern = Pattern()
        pattern.bind(cpattern)
        return pattern

    def build(self, str filename): #build a class from this dataset
        """Builds a class encoder from a plain-text corpus (utf-8)"""
        if os.path.exists(filename):
            self.data.build(filename.encode('utf-8'))
        else:
            raise IOError("File " + filename + " does not exist")

    def encodefile(self, str sourcefile, str targetfile, bool allowunknown=True): #apply the encoder to a file
        """Encodes the specified sourcefile according to the classer (as targetfile)"""
        if os.path.exists(sourcefile):
            self.data.encodefile(sourcefile.encode('utf-8'), targetfile.encode('utf-8'),allowunknown)
        else:
            raise IOError("File " + sourcefile + " does not exist")

    def save(self, str filename):
        self.data.save(filename.encode('utf-8'))


cdef class ClassDecoder:
    """The Class Decoder allows Patterns to be decoded back to their string representation. An instance of ClassDecoder is passed to Pattern.tostring()"""

    cdef cClassDecoder data #it's not actually a pointer anymore..

    def __init__(self, str filename):
        self.data.load(filename.encode('utf-8'))

    def __len__(self):
        """Returns the total number of classes"""
        return self.data.size()


cdef class Pattern:
    """The Pattern class contains an ngram, skipgram or flexgram, and allows a wide variety of actions to be performed on it. It is stored in a memory-efficient fashion and facilitating fast operation and comparison. Use ClassEncoder.buildpattern to build a pattern."""

    cdef cPattern cpattern

    cdef cPattern getcpattern(self):
        return self.cpattern

    cdef bind(self, cPattern& cpattern):
        self.cpattern = cpattern

    def bindbegin(self):
        self.cpattern = cBEGINPATTERN

    def bindend(self):
        self.cpattern = cENDPATTERN

    def bindunk(self):
        self.cpattern = cUNKPATTERN

    def bindskip(self):
        self.cpattern = cSKIPPATTERN

    def bindflex(self):
        self.cpattern = cFLEXPATTERN

    def tostring(self, ClassDecoder decoder):
        """Convert a Pattern back to a str

        :param decoder: the class decoder to use
        :type decoder: ClassDecoder
        :rtype: str
        """

        return str(self.cpattern.tostring(decoder.data),'utf-8')

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
            c_pattern = cPattern(self.cpattern, item, item+1)
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

    def tolist(self):
        """Returns a list representing the raw classes in the pattern"""
        cdef vector[int] state = self.cpattern.tovector()
        return state

    def __bytes__(self):
        cdef int s = self.bytesize()
        cdef bytes b = self.cpattern.data[:s]
        return b

    def __getstate__(self):
        cdef int s = self.bytesize()
        cdef bytes b = self.cpattern.data[:s]
        return b

    def __setstate__(self, bytes byterep):
        cdef unsigned char * cdata = byterep
        self.cpattern.set(cdata, len(byterep))

    def isgap(self, int index):
        return self.cpattern.isgap(index)

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

    cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t] data
    cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t].iterator it
    cdef uint32_t value

    @include colibricore_patterndict.pxi

    def __setitem__(self, Pattern pattern, uint32_t v):
        """Set the value for a pattern in the dictionary

        :param pattern: the pattern
        :param value: its value
        """
        self[pattern] = v

    cdef bind(self, cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t]& newdata):
        self.data = newdata



cdef class SmallPatternDict_int32: #maps Patterns to uint32
    """This is a simple low-level dictionary that takes Pattern instances as keys, and integer (max 32 bit, unsigned) as value. For complete pattern models, use IndexedPatternModel or UnindexPatternModel instead. This is a Small version taht allows at most 65536 patterns."""

    cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint16_t] data
    cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint16_t].iterator it
    cdef uint32_t value

    @include colibricore_patterndict.pxi

    def __setitem__(self, Pattern pattern, uint32_t v):
        """Set the value for a pattern in the dictionary

        :param pattern: the pattern
        :param value: its value
        """
        self[pattern] = v

    cdef bind(self, cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint16_t]& newdata):
        self.data = newdata


cdef class TinyPatternDict_int32: #maps Patterns to uint32
    """This is a simple low-level dictionary that takes Pattern instances as keys, and integer (max 32 bit, unsigned) as value. For complete pattern models, use IndexedPatternModel or UnindexPatternModel instead. This is a tiny version that allow only up to 256 patterns."""

    cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint8_t] data
    cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint8_t].iterator it
    cdef uint32_t value

    @include colibricore_patterndict.pxi

    def __setitem__(self, Pattern pattern, uint32_t v):
        """Set the value for a pattern in the dictionary

        :param pattern: the pattern
        :param value: its value
        """
        self[pattern] = v

    cdef bind(self, cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint8_t]& newdata):
        self.data = newdata

cdef class PatternDict_int: #maps Patterns to uint64
    """This is a simple low-level dictionary that takes Pattern instances as keys, and integer (unsigned 64 bit) as value. For complete pattern models, use IndexedPatternModel or UnindexPatternModel instead."""

    cdef cPatternMap[uint,cBaseValueHandler[uint],uint32_t] data
    cdef cPatternMap[uint,cBaseValueHandler[uint],uint32_t].iterator it
    cdef int value

    @include colibricore_patterndict.pxi

    def __setitem__(self, Pattern pattern, int v):
        """Set the value for a pattern in the dictionary

        :param pattern: the pattern
        :param value: its value
        """
        self[pattern] = v

cdef class PatternDict_float: #maps Patterns to float
    """This is a simple low-level dictionary that takes Pattern instances as keys, and float (double) as value. For complete pattern models, use IndexedPatternModel or UnindexPatternModel instead."""

    cdef cPatternMap[float,cBaseValueHandler[float],uint32_t] data
    cdef cPatternMap[float,cBaseValueHandler[float],uint32_t].iterator it
    cdef float value

    @include colibricore_patterndict.pxi

    def __setitem__(self, Pattern pattern, float v):
        """Set the value for a pattern in the dictionary

        :param pattern: the pattern
        :param value: its value
        """
        self[pattern] = v


cdef class AlignedPatternDict_int32: #maps Patterns to Patterns to uint32 (nested dicts)
    """This is a simple low-level dictionary that takes Pattern instances as keys, and integer (unsigned 64 bit) as value. For complete pattern models, use IndexedPatternModel or UnindexPatternModel instead."""

    cdef cAlignedPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t] data
    cdef cAlignedPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t].iterator it


    def __len__(self):
        """Return the total number of patterns in the dictionary. If you want the length for a particular pattern, use childcount(pattern)"""
        return self.data.size()

    def childcount(self, Pattern pattern):
        """Returns the number of children for the specified pattern. Use children(pattern) to iterate over them."""
        return self.data[pattern.cpattern].size()

    cpdef has(self, Pattern pattern):
        return self.data.has(pattern.cpattern)

    cpdef haspair(self, Pattern pattern, Pattern pattern2):
        if not self.data.has(pattern.cpattern):
            return False
        else:
            return self.data[pattern.cpattern].has(pattern2.cpattern)

    def __contains__(self, item):
        """Test if the pattern or the combination of patterns is in the aligned dictionary::

                pattern in aligneddict

            Or:

                (pattern1,pattern2) in aligneddict

            """
        if isinstance(item, tuple):
            if len(item) != 2:
                raise ValueError("Expected instance of Pattern or two-tuple of Patterns")
            elif not isinstance(item[0], Pattern) or not isinstance(item[1], Pattern):
                raise ValueError("Expected instance of Pattern or two-tuple of Patterns")
            return self.haspair(item[0], item[1])
        elif not isinstance(item, Pattern):
            raise ValueError("Expected instance of Pattern or two-tuple of Patterns")
            return self.has(item)

    def __iter__(self):
        """Iterate over all patterns in the dictionary. If you want to iterate over pattern pairs, use pairs() instead, to iterate over the children for a specific pattern, use children()"""
        it = self.data.begin()
        cdef cPattern cpattern
        while it != self.data.end():
            cpattern = deref(it).first
            pattern = Pattern()
            pattern.bind(cpattern)
            yield pattern
            inc(it)


    def children(self, Pattern pattern):
        """Iterate over all patterns in the dictionary. If you want to iterate over pattern pairs, use pairs() instead"""
        cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t].iterator it2
        it2 = self.data[pattern.cpattern].begin()
        cdef cPattern cpattern
        while it2 != self.data[pattern.cpattern].end():
            cpattern = deref(it2).first
            pattern = Pattern()
            pattern.bind(cpattern)
            yield pattern
            inc(it2)


    def items(self):
        """Iterate over all pattern pairs and their values in the dictionary. Yields (pattern1,pattern2,value) tuples"""
        cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t].iterator it2
        cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t].iterator it2nd
        cdef int value
        it = self.data.begin()
        cdef cPattern cpattern
        while it != self.data.end():
            cpattern = deref(it).first
            pattern = Pattern()
            pattern.bind(cpattern)
            it2 = self.data[pattern.cpattern].begin()
            it2end = self.data[pattern.cpattern].end()
            while it2 != it2end:
                cpattern = deref(it2).first
                pattern2 = Pattern()
                pattern2.bind(cpattern)
                value = deref(it2).second
                yield pattern, pattern2, value
                inc(it2)
            inc(it)

    cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t] * getmap(self, Pattern pattern):
        if not isinstance(pattern, Pattern):
            raise ValueError("Expected instance of Pattern")

        if not self.has(pattern):
           self.data[pattern.cpattern]

        cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t] * m = &(self.data[pattern.cpattern])
        return m




    cpdef getpair(self, Pattern pattern, Pattern pattern2):
        return self.data[pattern.cpattern][pattern2.cpattern]

    cpdef setpair(self, Pattern pattern, Pattern pattern2, uint32_t value):
        self.data[pattern.cpattern][pattern2.cpattern] = value

    def __getitem__(self, item):
        """Retrieve the item, item is a two-tuple of Pattern instances.

             aligneddict[(pattern1,pattern2)]


        :param item: A two tuple of Pattern instances
        """
        cdef cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint32_t] * mapdata

        if isinstance(item, tuple):
            if len(item) != 2:
                raise ValueError("Expected instance of Pattern or two-tuple of Patterns")
            elif not isinstance(item[0], Pattern) or not isinstance(item[1], Pattern):
                raise ValueError("Expected instance of Pattern or two-tuple of Patterns")
            return self.getpair(item[0], item[1])
        elif isinstance(item, Pattern):
            mapdata = self.getmap(item[0])
            d = PatternDict_int32()
            d.bind(deref(mapdata))
            return d
        else:
            raise ValueError("Expected instance of Pattern or two-tuple of Patterns")

    def __setitem__(self, item, value):
        if isinstance(item, tuple):
            if len(item) != 2:
                raise ValueError("Expected two-tuple of Patterns")
            elif not isinstance(item[0], Pattern) or not isinstance(item[1], Pattern):
                raise ValueError("Expected two-tuple of Patterns")
            self.setpair(item[0], item[1], value)
        else:
            raise ValueError("Expected two-tuple of Patterns")


    def read(self, str filename):
        if os.path.exists(filename):
            self.data.read(filename.encode('utf-8'))
        else:
            raise IOError

    def write(self, str filename):
        self.data.write(filename.encode('utf-8'))


cdef class IndexedPatternModel:
    """Indexed Pattern Model. Implemented using a hash map (dictionary)"""

    cdef cIndexedPatternModel[cPatternMap[cIndexedData,cIndexedDataHandler,uint64_t]] data
    cdef cPatternModel[cIndexedData,cIndexedDataHandler,cPatternMap[cIndexedData,cIndexedDataHandler,uint64_t]].iterator it

    @include colibricore_patternmodel.pxi
    @include colibricore_indexedpatternmodel.pxi

cdef class OrderedIndexedPatternModel:
    """Indexed Pattern Model. Implemented using an ordered map"""

    cdef cIndexedPatternModel[cOrderedPatternMap[cIndexedData,cIndexedDataHandler,uint64_t]] data
    cdef cPatternModel[cIndexedData,cIndexedDataHandler,cOrderedPatternMap[cIndexedData,cIndexedDataHandler,uint64_t]].iterator it

    @include colibricore_patternmodel.pxi
    @include colibricore_indexedpatternmodel.pxi

cdef class UnindexedPatternModel:
    """Unindexed Pattern Model, less flexible and powerful than its indexed counterpart, but smaller memory footprint"""
    cdef cPatternModel[uint32_t,cBaseValueHandler[uint32_t],cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint64_t]] data
    cdef cPatternModel[uint32_t,cBaseValueHandler[uint32_t],cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint64_t]].iterator it

    @include colibricore_patternmodel.pxi
    @include colibricore_unindexedpatternmodel.pxi


cdef class OrderedUnindexedPatternModel:
    """Unindexed Pattern Model, implemented using an ordered map, less flexible and powerful than its indexed counterpart, but smaller memory footprint"""
    cdef cPatternModel[uint32_t,cBaseValueHandler[uint32_t],cOrderedPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint64_t]] data
    cdef cPatternModel[uint32_t,cBaseValueHandler[uint32_t],cOrderedPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint64_t]].iterator it

    @include colibricore_patternmodel.pxi
    @include colibricore_unindexedpatternmodel.pxi

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

cdef class IndexedCorpus:
    """An indexed version of a corpus, reads an entire corpus (colibri.dat file) in memory"""
    cdef cIndexedCorpus data

    def __init__(self, str filename):
        """:param filename: The name of the colibri.dat file to load"""
        self.data.load(filename.encode('utf-8'))

    def __len__(self):
        """Return the total number of tokens in the corpus"""
        return self.data.size()

    cdef has(self,tuple item):
        cdef int sentence = item[0]
        cdef int token = item[1]
        cdef cIndexReference ref = cIndexReference(sentence,token)
        return self.data.has(ref)


    cdef get(self,tuple item):
        cdef int sentence = item[0]
        cdef int token = item[1]
        cdef cIndexReference ref = cIndexReference(sentence,token)
        cdef cPattern cpattern
        cpattern = self.data[ref]
        pattern = Pattern()
        pattern.bind(cpattern)
        return pattern

    cdef getslice(self,tuple start, tuple stop):
        cdef int startsentence = start[0]
        cdef int starttoken = start[1]
        cdef cIndexReference startref = cIndexReference(startsentence,starttoken)
        cdef int stopsentence = stop[0]
        cdef int stoptoken = stop[1]
        cdef cIndexReference stopref = cIndexReference(stopsentence,stoptoken)
        cdef cPattern cpattern = self.data.getpattern(startref, stopref.token - startref.token)
        pattern = Pattern()
        pattern.bind(cpattern)
        return pattern

    def __contains__(self, tuple indexreference):
        """Test if the indexreference, a (sentence,tokenoffset) tuple is in the corpus. This is a much slower lookup than using a pattern model!!"""
        return self.has(indexreference)

    def __iter__(self):
        """Iterate over all indexes in the corpus, generator over (sentence, tokenoffset) tuples"""
        it = self.data.begin()
        cdef cPattern cpattern
        cdef cIndexReference ref
        while it != self.data.end():
            ref = deref(it).first
            yield (ref.sentence, ref.token)
            inc(it)

    def __getitem__(self, item):
        """Retrieve the token Pattern given a (sentence, tokenoffset) tuple """
        if isinstance(item, slice):
            start = item.start
            stop = item.stop
            if not isinstance(start, tuple):
                raise ValueError("Expected tuple for start of slice")
            if not isinstance(stop, tuple):
                raise ValueError("Expected tuple for end of slice")
            if start.sentence != stop.sentence:
                raise ValueError("Slices only supported within the same sentence")
            return self.getslice(start, stop)
        else:
            if not isinstance(item, tuple):
                raise ValueError("Expected tuple")
            return self.get(item)


    def items(self):
        """Iterate over all indexes and their unigram patterns. Yields ((sentence,tokenoffset), unigrampattern) tuples"""
        it = self.data.begin()
        cdef cPattern cpattern
        cdef cIndexReference ref
        while it != self.data.end():
            cpattern = deref(it).second
            ref = deref(it).first
            pattern = Pattern()
            pattern.bind(cpattern)
            yield ( (ref.sentence, ref.token), pattern )
            inc(it)

    def findmatches(self, Pattern pattern, int maxmatches=0):
        """Generator over the indexes in the corpus where this pattern is found. Note that this is much slower than using the reverse index on an IndexedPatternModel!!!

        :param pattern: The pattern to find
        :type pattern: Pattern
        :param maxmatches: The maximum number of patterns to return (0 = unlimited)
        :type maxmatches: int
        :rtype: generator over (sentence, tokenoffset) tuples
        """

        cdef vector[cIndexReference] matches
        cdef vector[cIndexReference].iterator it
        cdef cIndexReference ref
        matches = self.data.findmatches(pattern.cpattern, maxmatches)
        it = matches.begin()
        while it != matches.end():
            ref = deref(it)
            yield (ref.sentence, ref.token)
            inc(it)

    def sentencelength(self,int sentence):
        return self.data.sentencelength(sentence)



BEGINPATTERN = Pattern()
BEGINPATTERN.bindbegin()
ENDPATTERN = Pattern()
ENDPATTERN.bindend()
UNKPATTERN = Pattern()
UNKPATTERN.bindunk()
SKIPPATTERN = Pattern()
SKIPPATTERN.bindskip()
FLEXPATTERN = Pattern()
FLEXPATTERN.bindflex()
