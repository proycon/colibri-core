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
        unsigned char * data
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
        vector[int] tovector() nogil
        void set(unsigned char *,int ) nogil
        bool isgap(int) nogil
        bool isskipgram() nogil
        bool isflexgram() nogil

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
        bool has(Pattern&) nogil
        ValueType& operator[](Pattern&) nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil
        void read(string filename) nogil
        void write(string filename) nogil

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
        bool has(Pattern&) nogil
        ValueType& operator[](Pattern&) nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil
        void read(string filename) nogil
        void write(string filename) nogil

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
        bool has(Pattern&) nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil
        void read(string filename) nogil
        void write(string filename) nogil

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
        bool has(Pattern&) nogil
        int size() nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil
        void read(string filename) nogil
        void write(string filename) nogil


    cdef cppclass IndexedCorpus:
        cppclass iterator:
            pair[IndexReference,Pattern] & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil

        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        IndexedCorpus() nogil
        void load(string) nogil
        bool has(IndexReference&) nogil
        Pattern getpattern(IndexReference&,int) nogil
        vector[IndexReference] findmatches(Pattern&,int) nogil
        Pattern& operator[](IndexReference&) nogil
        int sentencelength(int) nogil

    cdef cppclass AlignedPatternMap[ValueType, ValueHandler, NestedSizeType]:
        cppclass iterator:
            pair[Pattern,PatternMap[ValueType,ValueHandler,NestedSizeType]]& operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        AlignedPatternMap() nogil
        void insert(Pattern&, PatternMap[ValueType,ValueHandler,NestedSizeType]& value) nogil
        bool has(Pattern&) nogil
        PatternMap[ValueType,ValueHandler,NestedSizeType]& operator[](Pattern&) nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil
        void read(string filename) nogil
        void write(string filename) nogil

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


    extern Pattern SKIPPATTERN
    extern Pattern FLEXPATTERN
    extern Pattern BEGINPATTERN
    extern Pattern ENDPATTERN
    extern Pattern UNKPATTERN


cdef extern from "classdecoder.h":
    cdef cppclass ClassDecoder:
        ClassDecoder() nogil except +
        ClassDecoder(string) nogil except +
        void load(string) nogil
        int size() nogil

cdef extern from "classencoder.h":
    cdef cppclass ClassEncoder:
        ClassEncoder() nogil except +
        ClassEncoder(string) nogil except +
        void load(string) nogil
        int size() nogil
        void build(string filename) nogil #build a class from this dataset
        void encodefile(string, string, bool allowunknown, bool autoaddunknown=False, bool append=False) nogil
        void save(string)
        Pattern buildpattern(string, bool allowunknown, bool autoaddunknown) nogil

cdef extern from "patternmodel.h":
    cdef cppclass PatternModelOptions:
        int MINTOKENS
        int MINLENGTH
        int MAXLENGTH
        bool DOSKIPGRAMS
        bool DOSKIPGRAMS_EXHAUSTIVE
        int MINSKIPTYPES
        bool DOREVERSEINDEX
        bool DEBUG
        bool QUIET
        bool DOPATTERNPERLINE
        bool DOREMOVENGRAMS
        bool DOREMOVESKIPGRAMS
        bool DOREMOVEFLEXGRAMS
        bool DORESET

    cdef cppclass IndexedDataHandler:
        int count(IndexedData &)
        int add(IndexedData *, IndexReference&)
        str tostring(IndexedData&)


    cdef cppclass PatternModelInterface:
        int getmodeltype()
        int getmodelversion()
        bool has(const Pattern &)
        size_t size()
        int occurrencecount(Pattern & pattern)
        double frequency(Pattern &)
        int maxlength()
        int minlength()
        int types()
        int tokens()


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
        void add(Pattern&, ValueType*, IndexReference&)

        PatternModelInterface * getinterface() nogil
        void train(string filename, PatternModelOptions options, PatternModelInterface *)

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
        void load(string, PatternModelOptions) nogil except +IOError
        void write(string) nogil except +IOError
        void printmodel(ostream*, ClassDecoder&) nogil
        void printpattern(ostream*, ClassDecoder&, Pattern&) nogil
        void report(ostream*) nogil
        void histogram(ostream*) nogil
        void outputrelations(Pattern&,ClassDecoder&, ostream*)

        t_relationmap getsubchildren(Pattern & pattern) except +KeyError
        t_relationmap getsubparents(Pattern & pattern) except +KeyError
        t_relationmap getleftneighbours(Pattern & pattern) except +KeyError
        t_relationmap getrightneighbours(Pattern & pattern) except +KeyError
        t_relationmap getskipcontent(Pattern & pattern) except +KeyError
        t_relationmap gettemplates(Pattern & pattern) except +KeyError
        t_relationmap getleftcooc(Pattern & pattern) except +KeyError
        t_relationmap getrightcooc(Pattern & pattern) except +KeyError


    cdef cppclass IndexedPatternModel[MapType]:
        cppclass iterator:
            pair[Pattern,IndexedData] & operator*() nogil
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

        PatternModelInterface * getinterface() nogil
        void train(string filename, PatternModelOptions options, PatternModelInterface *)


        int totaloccurrencesingroup(int category, int n)
        int totalpatternsingroup(int category, int n)
        int totaltokensingroup(int category, int n)
        int totalwordtypesingroup(int category, int n)

        void insert(Pattern&, IndexedData& value) nogil
        bool has(Pattern&) nogil
        int size() nogil
        IndexedData& operator[](Pattern&) nogil
        iterator erase(Pattern&) nogil
        int prune(int threshold, int n) nogil
        iterator find(Pattern&) nogil
        void load(string, PatternModelOptions) nogil except +IOError
        void write(string) nogil except +IOError
        void printmodel(ostream*, ClassDecoder&) nogil
        void printpattern(ostream*, ClassDecoder&, Pattern&) nogil
        void report(ostream*) nogil
        void histogram(ostream*) nogil
        void outputrelations(Pattern&,ClassDecoder&, ostream*)

        void add(Pattern&, IndexedData*, IndexReference&)

        vector[Pattern] getreverseindex(IndexReference&)
        vector[Pattern] getreverseindex_bysentence(int)

        t_relationmap getsubchildren(Pattern & pattern) except +KeyError
        t_relationmap getsubparents(Pattern & pattern) except +KeyError
        t_relationmap getleftneighbours(Pattern & pattern) except +KeyError
        t_relationmap getrightneighbours(Pattern & pattern) except +KeyError
        t_relationmap getskipcontent(Pattern & pattern) except +KeyError
        t_relationmap gettemplates(Pattern & pattern) except +KeyError
        t_relationmap getleftcooc(Pattern & pattern) except +KeyError
        t_relationmap getrightcooc(Pattern & pattern) except +KeyError

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

