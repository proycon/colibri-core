from libcpp.string cimport string
from libcpp.set cimport set as cppset
from libcpp.vector cimport vector
from libcpp cimport bool
from unordered_map cimport unordered_map
from libcpp.utility cimport pair
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

    cdef cppclass BaseValueHandler[T]:
        int count(T &)
        int add(T *, IndexReference&)
        str tostring(T&)

    cdef cppclass PatternMap[ValueType,ValueHandler,ReadWriteSizeType]:
        cppclass iterator:
            pair[Pattern,ValueType] & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        PatternMap() nogil
        void insert(Pattern&, ValueType& value) nogil
        void has(Pattern&) nogil
        int size() nogil
        ValueType& operator[](Pattern&) nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil


    cdef cppclass OrderedPatternMap[ValueType,ValueHandler,ReadWriteSizeType]:
        cppclass iterator:
            pair[Pattern,ValueType] & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        OrderedPatternMap() nogil
        void insert(Pattern&, ValueType& value) nogil
        void has(Pattern&) nogil
        int size() nogil
        ValueType& operator[](Pattern&) nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil

    cdef cppclass PatternSet[ReadWriteSizeType]:
        cppclass iterator:
            Pattern& operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        PatternSet() nogil
        void insert(Pattern&) nogil
        void has(Pattern&) nogil
        int size() nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil

    cdef cppclass OrderedPatternSet[ReadWriteSizeType]:
        cppclass iterator:
            Pattern& operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        PatternSet() nogil
        void insert(Pattern&) nogil
        void has(Pattern&) nogil
        int size() nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil


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

    cdef cppclass PatternModelOptions:
        int MINTOKENS
        int MAXLENGTH
        bool DOFIXEDSKIPGRAMS
        int MINSKIPTYPES
        bool DOREVERSEINDEX
        bool DEBUG

    cdef cppclass IndexedValueHandler:
        int count(IndexedData &)
        int add(IndexedData *, IndexReference&)
        str tostring(IndexedData&)

    cdef cppclass PatternModel[ValueType,ValueHandler,MapType]:
        cppclass iterator:
            pair[Pattern,ValueType] & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        iterator begin() nogil
        iterator end() nogil
        PatternModel() nogil
        void insert(Pattern&, ValueType& value) nogil
        bool has(Pattern&) nogil
        int size() nogil
        ValueType& operator[](Pattern&) nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil
        void load(string, PatternModelOptions) nogil
        void write(string) nogil

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

