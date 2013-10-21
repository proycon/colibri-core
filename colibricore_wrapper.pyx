from libcpp.string cimport string
from libcpp cimport bool
from libcpp.vector cimport vector
from cython.operator cimport dereference as deref, preincrement as inc
from cython import address
cimport colibricore_classes
from unordered_map cimport unordered_map

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
    cdef colibricore_classes.ClassEncoder *thisptr

    def __cinit__(self):
        self.thisptr = NULL

    def __cinit__(self, str filename):
        self.thisptr = new colibricore_classes.ClassEncoder(filename.encode('utf-8'))

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
    cdef colibricore_classes.ClassDecoder *thisptr

    def __cinit__(self):
        self.thisptr = NULL

    def __cinit__(self, str filename):
        self.thisptr = new colibricore_classes.ClassDecoder(filename.encode('utf-8'))

    def __dealloc__(self):
        del self.thisptr

    def __len__(self):
        return self.thisptr.size()


cdef class Pattern:

    cdef colibricore_classes.Pattern cpattern

    cdef bind(self, colibricore_classes.Pattern cpattern):
        self.cpattern = cpattern

    def tostring(self, ClassDecoder decoder):
        return str(self.cpattern.tostring(deref(decoder.thisptr)),'utf-8')

    def __len__(self):
        return self.thisptr.n()

#    def __getitem__(self, item):
#        if isinstance(item, slice):
#            c_pattern = colibricore_classes.Pattern(self.cpattern, item.start, item.stop)
#            newpattern = Pattern()
#            newpattern.bind(c_pattern)
#            return newpattern
#        else:
#            c_pattern = colibricore_classes.Pattern(self.cpattern, item.start, item.start+1)
#            newpattern = Pattern()
#            newpattern.bind(c_pattern)
#            return newpattern

    def __iter__(self):
        for i in range(0, len(self)):
            yield self[i]


