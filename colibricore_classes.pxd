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
        int ngrams(vector[Pattern]& container,int n)
        int parts(vector[Pattern]& container) nogil
        int gaps(vector[pair[int,int]]& container) nogil
        int subngrams(vector[Pattern]& container,int minn=0,int maxn=9)
        bool contains(Pattern&) nogil
        Pattern toflexgram() nogil
        vector[int] tovector() nogil
        void set(unsigned char *,int ) nogil
        bool isgap(int) nogil
        bool isskipgram() nogil
        bool isflexgram() nogil
        bool unknown() nogil
        Pattern reverse() nogil

    Pattern patternfromfile(const string&)



cdef extern from "datatypes.h":
    cdef cppclass IndexReference:
        IndexReference()
        IndexReference(int,int)
        uint32_t sentence
        uint16_t token

    cdef cppclass BaseValueHandler[T]:
        unsigned int count(T &)
        int add(T *, IndexReference&)
        str tostring(T&)

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

    cdef cppclass PatternFeatureVector[T]:
        vector[T] data
        Pattern pattern
        cppclass iterator:
            T& operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        T get(int) nogil
        void clear() nogil
        void push_back(T) nogil


    cdef cppclass PatternFeatureVectorMap[T]:
        PatternFeatureVectorMap() nogil
        cppclass iterator:
            PatternFeatureVector[T] * operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        bool has(Pattern&) nogil
        iterator find(Pattern&) nogil
        void insert(PatternFeatureVector&)
        PatternFeatureVector[T] * getdata(Pattern&) nogil


cdef extern from "patternstore.h":
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

    cdef cppclass HashOrderedPatternMap[ValueType,ValueHandler,ReadWriteSizeType]:
        cppclass iterator:
            pair[Pattern,ValueType] & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        HashOrderedPatternMap() nogil
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

    cdef cppclass HashOrderedPatternSet[ReadWriteSizeType]:
        cppclass iterator:
            Pattern& operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        HashOrderedPatternSet() nogil
        void insert(Pattern&) nogil
        bool has(Pattern&) nogil
        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil
        void read(string filename) nogil
        void write(string filename) nogil

    cdef cppclass IndexPattern:
        IndexReference ref
        Pattern pattern()

    cdef cppclass IndexedCorpus:
        cppclass iterator:
            IndexPattern & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil

        size_t size() nogil
        iterator begin() nogil
        iterator end() nogil
        IndexedCorpus() nogil
        void load(string, bool) nogil
        bool has(IndexReference&) nogil
        Pattern getpattern(IndexReference&,int)  except +KeyError
        vector[IndexReference] findpattern(Pattern&,int) nogil
        int operator[](IndexReference&) nogil except +KeyError
        int sentencelength(int) nogil
        int sentences() nogil
        Pattern getsentence(int) nogil except +KeyError

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
        string decodefiletostring(string,  int begin = 0, int end = 0, bool quiet = False) nogil

cdef extern from "classencoder.h":
    cdef cppclass ClassEncoder:
        ClassEncoder(int minlength=0, int maxlength=0) nogil except +
        ClassEncoder(string, int minlength=0, int maxlength=0) nogil except +
        void load(string, int minlength, int maxlength) nogil
        int size() nogil
        void processcorpus(string filename, unordered_map[string,int]) nogil
        void buildclasses(unordered_map[string,int]) nogil
        void build(string filename) nogil #build a class from this dataset
        void encodefile(string, string, bool allowunknown, bool autoaddunknown, bool append) nogil
        void save(string)
        Pattern buildpattern(string, bool allowunknown, bool autoaddunknown) nogil

cdef extern from "patternmodel.h":
    cdef cppclass PatternModelOptions:
        int MINTOKENS
        int MINTOKENS_UNIGRAMS
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
        unsigned int count(IndexedData &)
        int add(IndexedData *, IndexReference&)
        str tostring(IndexedData&)


    cdef cppclass PatternModelInterface:
        int getmodeltype()
        int getmodelversion()
        bool has(const Pattern &)
        size_t size()
        unsigned int occurrencecount(Pattern & pattern)
        double frequency(Pattern &)
        int maxlength()
        int minlength()
        unsigned int types()
        unsigned int tokens()


    ctypedef PatternMap[unsigned int,BaseValueHandler[uint],unsigned long] t_relationmap
    ctypedef PatternMap[double,BaseValueHandler[double],unsigned long] t_relationmap_double
    ctypedef PatternMap[unsigned int,BaseValueHandler[uint],unsigned long].iterator t_relationmap_iterator
    ctypedef PatternMap[double,BaseValueHandler[double],unsigned long].iterator t_relationmap_double_iterator


    cdef cppclass PatternSetModel:
        cppclass iterator:
            Pattern & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        iterator begin() nogil
        iterator end() nogil

        bool has(Pattern&) nogil
        unsigned int size() nogil

        unsigned int occurrencecount(Pattern&) nogil #always returns 0
        float frequency(Pattern&) nogil

        PatternModelInterface * getinterface() nogil

        void insert(Pattern&) nogil

        void load(string, PatternModelOptions) nogil except +IOError
        void write(string) nogil except +IOError

    cdef cppclass PatternModel[ValueType,ValueHandler,MapType]:
        cppclass iterator:
            pair[Pattern,ValueType] & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        iterator begin() nogil
        iterator end() nogil
        PatternModel() nogil
        unsigned int types() nogil
        unsigned int tokens() nogil
        int type() nogil
        int version() nogil
        int maxlength() nogil
        int minlength() nogil
        unsigned int occurrencecount(Pattern&) nogil
        unsigned int coveragecount(Pattern&) nogil
        float coverage(Pattern&) nogil
        float frequency(Pattern&) nogil
        void add(Pattern&, ValueType*, IndexReference&)

        IndexedCorpus * reverseindex
        bool externalreverseindex

        PatternModelInterface * getinterface() nogil
        void train(string filename, PatternModelOptions options, PatternModelInterface *)

        unsigned int totaloccurrencesingroup(int category, int n)
        unsigned int totalpatternsingroup(int category, int n)
        unsigned int totaltokensingroup(int category, int n)
        unsigned int totalwordtypesingroup(int category, int n)

        void insert(Pattern&, ValueType& value) nogil
        bool has(Pattern&) nogil
        unsigned int size() nogil
        ValueType& operator[](Pattern&) nogil
        bool erase(Pattern&) nogil
        unsigned int prune(int threshold, int n) nogil
        iterator find(Pattern&) nogil
        void load(string, PatternModelOptions, PatternModelInterface*) nogil except +IOError
        void write(string) nogil except +IOError
        void printmodel(ostream*, ClassDecoder&) nogil
        void printpattern(ostream*, ClassDecoder&, Pattern&) nogil
        void report(ostream*) nogil
        void histogram(stdmap[unsigned int,unsigned int] & hist, unsigned int threshold, unsigned int cap,int,unsigned int)
        void histogram(ostream*) nogil
        unsigned int topthreshold(int,int,int) nogil
        void outputrelations(Pattern&,ClassDecoder&, ostream*)

        t_relationmap getsubchildren(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError
        t_relationmap getsubparents(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError
        t_relationmap getleftneighbours(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size, unsigned int cutoff) except +KeyError
        t_relationmap getrightneighbours(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size, unsigned int cutoff) except +KeyError
        t_relationmap getskipcontent(Pattern & pattern) except +KeyError
        t_relationmap gettemplates(Pattern & pattern, unsigned int occurrencethreshold) except +KeyError
        t_relationmap getinstances(Pattern & pattern, unsigned int occurrencethreshold) except +KeyError
        t_relationmap getcooc(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError
        t_relationmap getleftcooc(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError
        t_relationmap getrightcooc(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError

        vector[Pattern] getreverseindex(IndexReference&)
        vector[pair[IndexReference,Pattern]] getreverseindex_bysentence(int)

    cdef cppclass IndexedPatternModel[MapType]:
        cppclass iterator:
            pair[Pattern,IndexedData] & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        iterator begin() nogil
        iterator end() nogil
        PatternModel() nogil
        unsigned int types() nogil
        unsigned int tokens() nogil
        int type() nogil
        int version() nogil
        int maxlength() nogil
        int minlength() nogil
        unsigned int occurrencecount(Pattern&) nogil
        unsigned int coveragecount(Pattern&) nogil
        float coverage(Pattern&) nogil
        float frequency(Pattern&) nogil

        PatternModelInterface * getinterface() nogil
        void train(string filename, PatternModelOptions options, PatternModelInterface *)

        IndexedCorpus * reverseindex
        bool externalreverseindex

        unsigned int totaloccurrencesingroup(int category, int n)
        unsigned int totalpatternsingroup(int category, int n)
        unsigned int totaltokensingroup(int category, int n)
        unsigned int totalwordtypesingroup(int category, int n)

        void insert(Pattern&, IndexedData& value) nogil
        bool has(Pattern&) nogil
        unsigned int size() nogil
        IndexedData& operator[](Pattern&) nogil
        bool erase(Pattern&) nogil
        unsigned int prune(int threshold, int n) nogil
        iterator find(Pattern&) nogil
        void load(string, PatternModelOptions, PatternModelInterface* ) nogil except +IOError
        void write(string) nogil except +IOError
        void printmodel(ostream*, ClassDecoder&) nogil
        void printpattern(ostream*, ClassDecoder&, Pattern&) nogil
        void report(ostream*) nogil
        void histogram(ostream*) nogil
        void histogram(stdmap[unsigned int,unsigned int] & hist, unsigned int threshold , unsigned int cap,int,unsigned int )
        unsigned int topthreshold(int,int,int) nogil
        void outputrelations(Pattern&,ClassDecoder&, ostream*)

        void add(Pattern&, IndexedData*, IndexReference&)

        vector[Pattern] getreverseindex(IndexReference&)
        vector[pair[IndexReference,Pattern]] getreverseindex_bysentence(int)

        t_relationmap getsubchildren(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError
        t_relationmap getsubparents(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError
        t_relationmap getleftneighbours(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size, unsigned int cutoff) except +KeyError
        t_relationmap getrightneighbours(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size, unsigned int cutoff) except +KeyError
        t_relationmap getskipcontent(Pattern & pattern) except +KeyError
        t_relationmap gettemplates(Pattern & pattern, unsigned int occurrencethreshold) except +KeyError
        t_relationmap getinstances(Pattern & pattern, unsigned int occurrencethreshold) except +KeyError
        t_relationmap getcooc(Pattern & pattern,  unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError
        t_relationmap getleftcooc(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError
        t_relationmap getrightcooc(Pattern & pattern, unsigned int occurrencethreshold, int category, unsigned int size) except +KeyError

cdef extern from "alignmodel.h":
    cdef cppclass PatternAlignmentModel[FeatureType]:
        cppclass iterator:
            pair[Pattern,PatternFeatureVectorMap[FeatureType]] & operator*() nogil
            iterator operator++() nogil
            bint operator==(iterator) nogil
            bint operator!=(iterator) nogil
        iterator begin() nogil
        iterator end() nogil
        PatternAlignmentModel() nogil
        unsigned int types() nogil
        unsigned int tokens() nogil
        int type() nogil
        int version() nogil
        int maxlength() nogil
        int minlength() nogil
        void add(Pattern&, Pattern&, vector[FeatureType]&)

        PatternModelInterface * getinterface() nogil

        void insert(Pattern&, PatternFeatureVectorMap[FeatureType]& value) nogil
        bool has(Pattern&) nogil
        bool has(Pattern&, Pattern&) nogil

        unsigned int size() nogil

        PatternFeatureVectorMap[double]& operator[](Pattern&) nogil
        PatternFeatureVectorMap[double]* getdata(Pattern&, bool makeifnew=False) nogil
        PatternFeatureVector[double]* getdata(Pattern&, Pattern&, bool makeifnew=False) nogil

        iterator erase(Pattern&) nogil
        iterator find(Pattern&) nogil

        void load(string, PatternModelOptions) nogil except +IOError
        void write(string) nogil except +IOError

