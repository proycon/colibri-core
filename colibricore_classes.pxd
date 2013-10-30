from libcpp.string cimport string
from libcpp.set cimport set as stdset
from libcpp.map cimport map as stdmap
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
        Pattern(Pattern&) nogil
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
        int ngrams(vector[Pattern] container,int n) nogil
        int parts(vector[Pattern]& container) nogil
        int gaps(vector[pair[int,int]]& container) nogil
        int subngrams(vector[Pattern] container,int minn=0,int maxn=9) nogil
        bool contains(Pattern&) nogil
        Pattern toflexgram() nogil

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
        bool DOSKIPGRAMS
        bool DOSKIPGRAMS_EXHAUSTIVE
        int MINSKIPTYPES
        bool DOREVERSEINDEX
        bool DEBUG
        bool QUIET

    cdef cppclass IndexedDataHandler:
        int count(IndexedData &)
        int add(IndexedData *, IndexReference&)
        str tostring(IndexedData&)

    ctypedef PatternMap[uint32_t,BaseValueHandler[uint32_t],uint64_t] t_relationmap
    ctypedef PatternMap[double,BaseValueHandler[double],uint64_t] t_relationmap_double

    ctypedef PatternMap[uint32_t,BaseValueHandler[uint32_t],uint64_t].iterator t_relationmap_iterator
    ctypedef PatternMap[double,BaseValueHandler[double],uint64_t].iterator t_relationmap_double_iterator

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

        t_relationmap getsubchildren(Pattern & pattern)
        t_relationmap getsubparents(Pattern & pattern)
        t_relationmap getleftneighbours(Pattern & pattern)
        t_relationmap getrightneighbours(Pattern & pattern)
        t_relationmap getskipcontent(Pattern & pattern)


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

