from libcpp.string cimport string
from libcpp.set cimport set as cppset
from libcpp.vector cimport vector
from libcpp cimport bool
from unordered_map cimport unordered_map
from libcpp.utility cimport pair
from libc.stdint cimport *

cdef extern from "<iostream>" namespace "std":
    cdef cppclass ostream:
        pass

    extern ostream cout

cdef extern from "pattern.h":
    cdef cppclass Pattern:
        Pattern() nogil
        Pattern(Pattern&, int,int) nogil
        string tostring(ClassDecoder&) nogil
        int n() nogil
        int bytesize() nogil
        int skipcount() nogil
        int category() nogil
        int hash() nogil
        bint operator==(Pattern&) nogil
        bint operator<(Pattern&) nogil
        bint operator>(Pattern&) nogil
        Pattern operator+(Pattern&) nogil
        int ngrams(vector[Pattern] container,int n)
        int parts(vector[Pattern] container)
        int subngrams(vector[Pattern] container,int minn=0,int maxn=9)

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
        Pattern buildpattern(string , bool allowunknown, bool autoaddunknown)

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

    cdef cppclass IndexedDataHandler:
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
        int types() nogil
        int tokens() nogil
        int type() nogil
        int version() nogil
        int maxlength() nogil
        int minlength() nogil
        int occurrencecount(Pattern&) nogil
        int coveragecount(Pattern&) nogil
        float coverage(Pattern&) nogil
        float frequency(Pattern&) nogil

        int totaloccurrencesingroup(int category, int n)
        int totalpatternsingroup(int category, int n)
        int totaltokensingroup(int category, int n)
        int totalwordtypesingroup(int category, int n)

        void insert(Pattern&, ValueType& value) nogil
        bool has(Pattern&) nogil
        int size() nogil
        ValueType& operator[](Pattern&) nogil
        iterator erase(Pattern&) nogil
        int prune(int threshold, int n) nogil
        iterator find(Pattern&) nogil
        void load(string, PatternModelOptions) nogil
        void write(string) nogil
        void printmodel(ostream*, ClassDecoder&) nogil
        void printpattern(ostream*, ClassDecoder&, Pattern&) nogil
        void report(ostream*) nogil
        void histogram(ostream*) nogil
        void outputrelations(Pattern&,ClassDecoder&, ostream*)
        

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

