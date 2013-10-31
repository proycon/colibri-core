from libcpp.string cimport string
from libcpp cimport bool
from libcpp.vector cimport vector
from cython.operator cimport dereference as deref, preincrement as inc
from cython import address
from colibricore_classes cimport ClassEncoder as cClassEncoder, ClassDecoder as cClassDecoder, Pattern as cPattern, IndexedData as cIndexedData, IndexReference as cIndexReference, PatternMap as cPatternMap, PatternModelOptions as cPatternModelOptions, PatternModel as cPatternModel, IndexedDataHandler as cIndexedDataHandler, BaseValueHandler as cBaseValueHandler, cout, t_relationmap, t_relationmap_double, t_relationmap_iterator, t_relationmap_double_iterator
from unordered_map cimport unordered_map
from libc.stdint cimport *
from libcpp.map cimport map as stdmap
from libcpp.utility cimport pair

class Category:
    NGRAM=1
    SKIPGRAM=2
    FLEXGRAM=3

cdef class ClassEncoder:
    cdef cClassEncoder *thisptr

    def __cinit__(self):
        self.thisptr = NULL

    def __cinit__(self, str filename):
        self.thisptr = new cClassEncoder(filename.encode('utf-8'))

    def __dealloc__(self):
        del self.thisptr

    def __len__(self):
        return self.thisptr.size()

    def buildpattern(self, str text, bool allowunknown=True, bool autoaddunknown=False):
        c_pattern = self.thisptr.buildpattern(text.encode('utf-8'), allowunknown, autoaddunknown)
        pattern = Pattern()
        pattern.bind(c_pattern)
        return pattern


cdef class ClassDecoder:
    cdef cClassDecoder *thisptr

    def __cinit__(self):
        self.thisptr = NULL

    def __cinit__(self, str filename):
        self.thisptr = new cClassDecoder(filename.encode('utf-8'))

    def __dealloc__(self):
        del self.thisptr

    def __len__(self):
        return self.thisptr.size()


cdef class Pattern:

    cdef cPattern cpattern

    cdef cPattern getcpattern(self):
        return self.cpattern

    cdef bind(self, cPattern cpattern):
        self.cpattern = cpattern

    def tostring(self, ClassDecoder decoder):
        return str(self.cpattern.tostring(deref(decoder.thisptr)),'utf-8')

    def __contains__(self, Pattern pattern):
        cdef bool r
        r = self.cpattern.contains(pattern.cpattern)
        return r

    def __len__(self):
        return self.cpattern.n()

    def __copy__(self):
        cdef cPattern c_pattern
        c_pattern = cPattern(self.cpattern) #copy constructor
        newpattern = Pattern()
        newpattern.bind(c_pattern)
        return newpattern

    def __deepcopy__(self):
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
        return self.concat(other)

    def __getitem__(self, item):
        cdef cPattern c_pattern
        if isinstance(item, slice):
            c_pattern = cPattern(self.cpattern, item.start, item.stop)
            newpattern = Pattern()
            newpattern.bind(c_pattern)
            return newpattern
        else:
            c_pattern = cPattern(self.cpattern, item.start, item.start+1)
            newpattern = Pattern()
            newpattern.bind(c_pattern)
            return newpattern

    def __iter__(self):
        for i in range(0, len(self)):
            yield self[i]

    def bytesize(self):
        return self.cpattern.bytesize()

    def skipcount(self):
        return self.cpattern.skipcount()

    def category(self):
        return self.cpattern.category()

    def __hash__(self):
        return self.cpattern.hash()

    def __richcmp__(Pattern self, Pattern other, int op):
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

    def ngrams(self,int n=0):
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
        cdef vector[pair[int,int]] result
        self.cpattern.gaps(result)
        cdef vector[pair[int,int]].iterator it = result.begin()
        cdef pair[int,int] p
        while it != result.end():
            p  = deref(it)
            yield (p.first, p.second)
            inc(it)

    def toflexgram(self):
        cdef cPattern newcpattern = self.cpattern.toflexgram()
        newpattern = Pattern()
        newpattern.bind(newcpattern)
        return newpattern

    def subngrams(self,int minn=0,int maxn=9):
        minn = max(1,minn)
        maxn = min(maxn, len(self) -1 )
        for n in range(minn,maxn+1):
            for pattern in self.ngrams(n):
                yield pattern

cdef class IndexedData:

    cdef cIndexedData data

    cdef bind(self, cIndexedData cdata):
        self.data = cdata


    def __contains__(self, item):
        if not isinstance(item, tuple) or len(item) != 2:
            raise ValueError("Item should be a 2-tuple (sentence,token)")
        cdef cIndexReference ref = cIndexReference(item[0], item[1])
        return self.data.has(ref)

    def __iter__(self):
        cdef cIndexReference ref
        cdef cIndexedData.iterator it = self.data.begin()
        while it != self.data.end():
            ref  = deref(it)
            yield tuple(ref.sentence, ref.token)
            inc(it)

    def __len__(self):
        return self.data.size()


cdef class IndexedPatternModel:
    cdef cPatternModel[cIndexedData,cIndexedDataHandler,cPatternMap[cIndexedData,cIndexedDataHandler,uint64_t]] data

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
        return self.data.occurrencecount(pattern.cpattern)

    def coveragecount(self, Pattern pattern):
        return self.data.coveragecount(pattern.cpattern)

    def coverage(self, Pattern pattern):
        return self.data.coverage(pattern.cpattern)

    def frequency(self, Pattern pattern):
        return self.data.coverage(pattern.cpattern)


    def totaloccurrencesingroup(self, int category=0, int n=0):
        return self.data.totaloccurrencesingroup(category,n)

    def totalpatternsingroup(self, int category=0, int n=0):
        return self.data.totalpatternsingroup(category,n)

    def totaltokensingroup(self, int category=0, int n=0):
        return self.data.totaltokensingroup(category,n)

    def totalwordtypesingroup(self, int category=0, int n=0):
        return self.data.totalwordtypesingroup(category,n)

    cpdef has(self, Pattern pattern):
        return self.data.has(pattern.cpattern)

    def __contains__(self, pattern):
        assert isinstance(pattern, Pattern)
        return self.has(pattern)

    def __getitem__(self, pattern):
        assert isinstance(pattern, Pattern)
        return self.getdata(pattern)

    cdef getdata(self, Pattern pattern):
        assert isinstance(pattern, Pattern)
        cdef cIndexedData cvalue
        if pattern in self:
            cvalue = self.data[pattern.cpattern]
            value = IndexedData()
            value.bind(cvalue)
            return value
        else:
            raise KeyError


    def __iter__(self):
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
            yield tuple(pattern,value)
            inc(it)

    cpdef load(self, str filename, threshold=2, doskipgrams=False, maxlength=99, minskiptypes=2):
        cdef cPatternModelOptions options
        options.MINTOKENS = threshold
        options.DOSKIPGRAMS = doskipgrams
        options.MAXLENGTH = maxlength
        options.MINSKIPTYPES = minskiptypes
        options.DOREVERSEINDEX = True
        self.data.load(filename, options)

    cpdef write(self, str filename):
        self.write(filename)

    cpdef printmodel(self,ClassDecoder decoder):
        self.data.printmodel(&cout, deref(decoder.thisptr) )

    cpdef report(self):
        self.data.report(&cout)

    cpdef histogram(self):
        self.data.report(&cout)

    cpdef outputrelations(self, Pattern pattern, ClassDecoder decoder):
        self.data.outputrelations(pattern.cpattern,deref(decoder.thisptr),&cout)

    cpdef prune(self, int threshold, int n=0):
        self.data.prune(threshold, n)

cdef class UnindexedPatternModel:
    cdef cPatternModel[uint32_t,cBaseValueHandler[uint32_t],cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint64_t]] data

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
        return self.data.occurrencecount(pattern.cpattern)

    def coveragecount(self, Pattern pattern):
        return self.data.coveragecount(pattern.cpattern)

    def coverage(self, Pattern pattern):
        return self.data.coverage(pattern.cpattern)

    def frequency(self, Pattern pattern):
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
        return self.data.has(pattern.cpattern)

    def __contains__(self, pattern):
        assert isinstance(pattern, Pattern)
        return self.has(pattern)

    def __getitem__(self, pattern):
        assert isinstance(pattern, Pattern)
        return self.getdata(pattern)

    cpdef getdata(self, Pattern pattern):
        assert isinstance(pattern, Pattern)
        cdef cIndexedData cvalue
        if pattern in self:
            return self.data[pattern.cpattern]
        else:
            raise KeyError


    def __iter__(self):
        cdef cPatternModel[uint32_t,cBaseValueHandler[uint32_t],cPatternMap[uint32_t,cBaseValueHandler[uint32_t],uint64_t]].iterator it = self.data.begin()
        cdef cPattern cpattern
        cdef int value
        while it != self.data.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield tuple(pattern,value)
            inc(it)

    def __init__(self, str filename = "",PatternModelOptions options = None):
        if filename:
            if not options:
                options = PatternModelOptions()
            self.load(filename,options)



    def load(self, str filename, PatternModelOptions options=None):
        if not options:
            options = PatternModelOptions()
        self.data.load(filename, options.coptions)

    cpdef write(self, str filename):
        self.write(filename)

    cpdef printmodel(self,ClassDecoder decoder):
        self.data.printmodel(&cout, deref(decoder.thisptr) )

    cpdef train(self, str filename, PatternModelOptions options):
        self.data.train(filename,options.coptions)

    cpdef report(self):
        self.data.report(&cout)

    cpdef histogram(self):
        self.data.report(&cout)

    cpdef outputrelations(self, Pattern pattern, ClassDecoder decoder):
        self.data.outputrelations(pattern.cpattern,deref(decoder.thisptr),&cout)

    cpdef prune(self, int threshold, int n=0):
        self.data.prune(threshold, n)

    def getsubchildren(self, Pattern pattern):
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getsubchildren(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()

        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield tuple(pattern,value)
            inc(it)

    def getsubparents(self, Pattern pattern):
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getsubparents(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()
        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield tuple(pattern,value)
            inc(it)

    def getleftneighbours(self, Pattern pattern):
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getleftneighbours(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()
        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield tuple(pattern,value)
            inc(it)

    def getrightneighbours(self, Pattern pattern):
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getrightneighbours(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()
        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield tuple(pattern,value)
            inc(it)

    def getskipcontent(self, Pattern pattern):
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long]  relations = self.data.getskipcontent(pattern.cpattern)
        cdef cPatternMap[unsigned int,cBaseValueHandler[uint],unsigned long].iterator it = relations.begin()
        cdef cPattern cpattern
        cdef int value
        while it != relations.end():
            cpattern = deref(it).first
            value = deref(it).second
            pattern = Pattern()
            pattern.bind(cpattern)
            yield tuple(pattern,value)
            inc(it)

#    def reverseindex(self, int index):
#        #cdef vector[const pycolibri_classes.EncAnyGram *] v = self.thisptr.reverse_index[index]
#        cdef vector[const pycolibri_classes.EncAnyGram*] v = self.thisptr.get_reverse_index(index)
#        cdef vector[const pycolibri_classes.EncAnyGram *].iterator it = v.begin()
#        while it != v.end():
#            anygram  = <pycolibri_classes.EncAnyGram*> deref(it)
#            pattern = Pattern()
#            pattern.bind(anygram)
#            yield pattern
#            inc(it)


cdef class PatternModelOptions:
    cdef cPatternModelOptions coptions

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
