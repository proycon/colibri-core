from libcpp.string cimport string
from libcpp.set cimport set as cppset
from libcpp.vector cimport vector
from libcpp cimport bool
from unordered_map cimport unordered_map
from libc.stdint cimport *


cdef extern from "pattern.h":
    cdef cppclass Pattern:
        Pattern()
        Pattern(Pattern&, int,int)
        string tostring(ClassDecoder&)
        int n()

    cdef cppclass IndexReference:
        IndexReference()
        IndexReference(int,int)
        uint32_t sentence
        uint16_t token

cdef extern from "classdecoder.h":
    cdef cppclass ClassDecoder:
        ClassDecoder(string) except +
        int size()

cdef extern from "classencoder.h":
    cdef cppclass ClassEncoder:
        ClassEncoder(string) except +
        int size()
        Pattern input2pattern(string , bool allowunknown, bool autoaddunknown)

cdef extern from "patternmodel.h":
    cdef cppclass IndexedData:
        cppclass iterator:
            IndexReference& operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        bool has(IndexReference& ref)

#    cdef cppclass NGramData(AnyGramData):
#        cppset[CorpusReference] refs
#        int count()
#
#    ctypedef vector[const EncAnyGram*] anygramvector
#
#    cdef cppclass IndexedPatternModel:
#        IndexedPatternModel(string, bool, bool) except +
#        bool exists(EncAnyGram*)
#        unordered_map[EncNGram,NGramData] ngrams
#        unordered_map[int, anygramvector] reverse_index
#        int types()
#        int tokens()
#        int occurrencecount(EncAnyGram*) except +
#        AnyGramData * getdata(EncAnyGram*)
#        anygramvector get_reverse_index(int i)

