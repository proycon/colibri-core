from libcpp.string cimport string
from libcpp cimport bool
from libcpp.vector cimport vector
from cython.operator cimport dereference as deref, preincrement as inc
from cython import address
from colibricore_classes cimport ClassEncoder as cClassEncoder, ClassDecoder as cClassDecoder, Pattern as cPattern, IndexedData as cIndexedData, IndexReference as cIndexReference, PatternModelOptions as cPatternModelOptions, PatternModel as cPatternModel, IndexedValueHandler as cIndexedValueHandler, BaseValueHandler as cBaseValueHandler
from unordered_map cimport unordered_map
from libc.stdint cimport *

#cdef class IndexedPatternModel:
#    cdef pycolibri_classes.IndexedPatternModel *thisptr
#    cdef ClassEncoder encoder
#    cdef ClassDecoder decoder
#
#    def __cinit__(self, str filename, ClassEncoder encoder, ClassDecoder decoder):
#          self.thisptr = new pycolibri_classes.IndexedPatternModel(filename.encode('utf-8'),True, False)
#          self.encoder = encoder
#          self.decoder = decoder
#          #self.thisptr.testreverseindex() #debug
#
#    def __dealloc__(self):
#        del self.thisptr
#
#    def __exists__(self, key):
#        cdef Pattern pattern = self.encoder.encode(key)
#        return self.thisptr.exists(pattern.thisptr)
#
#    def __len__(self):
#        return self.thisptr.types()
#
#    def tokens(self):
#        return self.thisptr.tokens()
#
#
#
#    def indices(self, key):
#        cdef Pattern pattern = self.encoder.encode(key)
#        cdef pycolibri_classes.NGramData * data = <pycolibri_classes.NGramData*> self.thisptr.getdata(pattern.thisptr)
#        for ref in data.refs:
#            yield (ref.sentence, ref.token)
#
#    def sentences(self, key):
#        cdef Pattern pattern = self.encoder.encode(key)
#        cdef pycolibri_classes.NGramData * data = <pycolibri_classes.NGramData*> self.thisptr.getdata(pattern.thisptr)
#        for ref in data.refs:
#            yield ref.sentence
#
#    def __getitem__(self, key):
#        cdef Pattern pattern = self.encoder.encode(key)
#        try:
#            return self.thisptr.occurrencecount(pattern.thisptr)
#        except:
#            return 0
#
#    def __iter__(self):
#        cdef unordered_map[pycolibri_classes.EncNGram, pycolibri_classes.NGramData].iterator it = self.thisptr.ngrams.begin()
#        cdef pycolibri_classes.EncAnyGram* anygram
#        while it != self.thisptr.ngrams.end():
#            anygram  = <pycolibri_classes.EncAnyGram*> address(deref(it))
#            pattern = Pattern()
#            pattern.bind(anygram)
#            yield pattern
#            inc(it)
#
#        #TODO: skipgrams
#
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
        c_pattern = self.thisptr.input2pattern(text.encode('utf-8'), allowunknown, autoaddunknown)
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

    cdef bind(self, cPattern cpattern):
        self.cpattern = cpattern

    cdef cPattern get(self):
        return self.cpattern


    def tostring(self, ClassDecoder decoder):
        return str(self.cpattern.tostring(deref(decoder.thisptr)),'utf-8')

    def __len__(self):
        return self.thisptr.n()

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
    cdef cPatternModel[cIndexedData,cIndexedValueHandler,uint64_t] data

    def __len__(self):
        return self.data.size()

    cdef has(self, Pattern pattern):
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
        cdef cPatternModel[cIndexedData,cIndexedValueHandler,uint64_t].iterator it = self.data.begin()
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
    
    cdef load(self, str filename, threshold, dofixedskipgrams, maxlength, minskiptypes):
        cdef cPatternModelOptions options
        options.MINTOKENS = threshold
        options.DOFIXEDSKIPGRAMS = dofixedskipgrams
        options.MAXLENGTH = maxlength
        options.MINSKIPTYPES = minskiptypes
        options.DOREVERSEINDEX = True
        self.data.load(filename, options)
    
    cdef write(self, str filename):
        self.write(filename)
        
        

cdef class UnindexedPatternModel:
    cdef cPatternModel[uint32_t,cBaseValueHandler[uint32_t],uint64_t] data

    def __len__(self):
        return self.data.size()

    cdef has(self, Pattern pattern):
        return self.data.has(pattern.cpattern)

    def __contains__(self, pattern):
        assert isinstance(pattern, Pattern)
        return self.has(pattern)
