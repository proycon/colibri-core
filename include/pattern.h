#ifndef COLIBRIPATTERN_H
#define COLIBRIPATTERN_H

/*****************************
* Colibri Core
*   by Maarten van Gompel
*   Centre for Language Studies
*   Radboud University Nijmegen
*
*   http://proycon.github.io/colibri-core
*   
*   Licensed under GPLv3
*****************************/

#include <string>
#include <iostream>
#include <ostream>
#include <istream>
#include <unordered_map>
#include <vector>
#include <set>
#include <map>
#include <array>
#include <unordered_set>
#include <iomanip> // contains setprecision()
#include <exception>
#include "common.h"
#include "classdecoder.h"


const int MAINPATTERNBUFFERSIZE = 40960;

/**
 * Pattern categories
 */
enum PatternCategory {
    UNKNOWNPATTERN = 0, /**< not used */
    NGRAM = 1, /**< For ngrams, i.e. patterns without gaps */
    SKIPGRAM = 2, /**< For skipgrams, i.e. patterns with fixed-width gaps */
    FLEXGRAM = 3, /**< For flexgrams, i.e. patterns with dynamic-width gaps */
};

//Not really used much yet, but reserved for encoding structural markup
enum StructureType { 
    STRUCT_PATTERN = 0, //undefined pattern (if n==1 -> token)
    STRUCT_SENTENCE = 1,
    STRUCT_PARAGRAPH = 2,
    STRUCT_HEADER = 3, //like paragraph
    STRUCT_TEXT = 4, //aka document
    STRUCT_QUOTE = 5,
    STRUCT_DIV = 6,
};


//Markers (en lieu of the size marker with byte value <128)
enum Markers { // <128 size
    ENDMARKER = 0, //marks the end of each pattern (necessary!)

    TEXTMARKER = 255, //aka begin document 
    SENTENCEMARKER = 254,
    PARAGRAPHMARKER = 253,
    BEGINDIVMARKER = 252,
    ENDDIVMARKER = 251,
    HEADERMARKER = 250,
    
    FLEXMARKER = 129,
    SKIPMARKER = 128,
};





void readanddiscardpattern(std::istream * in);
int reader_passmarker(const unsigned char c, std::istream * in); 



/**
 * Pattern class
 * Represents a pattern (ngram, skipgram or flexgram), encoded in a
 * memory-saving fashion. Allows numerous operations.
 */
class Pattern {
    protected:
     void reader_marker(unsigned char * _data, std::istream * in);
    public:
     unsigned char * data; /**< This array holds the variable-width byte representation, it is always terminated by \0 (ENDMARKER) */

     /**
      * Default/empty Pattern constructor. Creates an empty pattern. Still consumes one
      * byte (the end-marker)
      */
     Pattern() { data = new unsigned char[1]; data[0] = ENDMARKER; }

     /**
      * Low-level pattern constructor from character array. The size is in
      * bytes and never includes the end-marker. 
      * @param dataref Reference data, must be properly class-encoded
      * @param size The size (without \0 end marker!) to copy from dataref
      */
     Pattern(const unsigned char* dataref, const int size); 

     /**
      * Slice constructor for Pattern
      * @param ref Reference pattern
      * @param begin Index of the first token to copy (0-indexed)
      * @param length Number of tokens to copy
      */
     Pattern(const Pattern& ref, int begin, int length); 

     /**
      * Copy constructor for Pattern
      * @param ref Reference pattern
      */
     Pattern(const Pattern& ref);

     /**
      * Read Pattern from input stream (in binary form)
      * @param in The input stream
      */
     Pattern(std::istream * in); 

     ~Pattern();


     /**
      * Pattern constructor consisting of only a fixed-size gap
      * @param size The size of the gap
      */
     Pattern(int size) {
         //pattern consisting only of fixed skips
         data = new unsigned char[size+1];
         for (int i = 0; i < size; i++) data[i] = SKIPMARKER;
         data[size] = ENDMARKER;
     }

     /**
      * Write Pattern to output stream (in binary form)
      * @param out The output stream
      */
     void write(std::ostream * out) const; 

     /**
      * return the size of the pattern in tokens (will count flex gaps gaps as size 1)
      */
     const size_t n() const;


     /**
      * return the size of the pattern (in bytes), this does not include the
      * final \0 end-marker.
      */
     const size_t bytesize() const; 
     
     /**
      * return the size of the pattern in tokens (will count flex gaps gaps as size 1)
      */
     const size_t size() const { return n(); } 


     /**
      * return the number of skips in this pattern
      */
     const unsigned int skipcount() const;

     /**
      * Returns the category of this pattern (value from enum PatternCategory)
      */
     const PatternCategory category() const;


     const StructureType type() const;
     const bool isskipgram() const { return category() == SKIPGRAM; }
     const bool isflexgram() const { return category() == FLEXGRAM; }
     
     /**
      * Return a single token (not a byte!). index < size().
      */
     Pattern operator [](int index) const { return Pattern(*this, index,1); } 

     /**
      * Compute a hash value for this pattern
      */
     const size_t hash(bool stripmarkers = false) const;

     /**
      * Converts this pattern back into its string representation, using a
      * classdecoder
      */
     std::string tostring(ClassDecoder& classdecoder) const; //pattern to string (decode)

     /**
      * alias for tostring()
      */
     std::string decode(ClassDecoder& classdecoder) const { return tostring(classdecoder); } //alias

     /**
      * Debug function outputting the classes in this pattern to stderr
      */
     bool out() const;
     std::vector<int> tovector() const;

     bool operator==(const Pattern & other) const;
     bool operator!=(const Pattern & other) const;

     Pattern & operator =(const Pattern other);   


     //patterns are sortable
     bool operator<(const Pattern & other) const;
     bool operator>(const Pattern & other) const;

     Pattern operator +(const Pattern&) const;

     /**
      * Finds the specified subpattern in the this pattern. Returns the index
      * at which it is found, or -1 if it is not found at all.
      */
     int find(const Pattern & subpattern) const; 

     /**
      * Test whether the pattern contains the specified subpattern.
      */
     bool contains(const Pattern & subpattern) const; //does the pattern contain the smaller pattern?
   
     /**
      * Tests whether the pattern is an instantiation of the specified skipgram
      */
     bool instanceof(const Pattern & skipgram) const; 
    

     /**
      * Adds all patterns (not just ngrams) of size n that are contained within the pattern to
      * container. Does not extract skipgrams that are not directly present in
      * the pattern.
      */
     int ngrams(std::vector<Pattern> & container, const int n) const; 

     /**
      * Adds all patterns (not just ngrams) of all sizes that are contained within the pattern to
      * container. Does not extract skipgrams that are not directly present in
      * the pattern.
      */
     int subngrams(std::vector<Pattern> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)

     /**
      * Adds all pairs of all patterns (not just ngrams) of size n that are
      * contained within the pattern, with the token offset at which they were
      * found,  to container.  Does not extract skipgrams that are not directly
      * present in the pattern.
      */
     int ngrams(std::vector<std::pair<Pattern,int>> & container, const int n) const; //return multiple ngrams

     /**
      * Adds all pairs of all patterns (not just ngrams) that are
      * contained within the pattern, with the token offset at which they were
      * found,  to container.  Does not extract skipgrams that are not directly
      * present in the pattern.
      */
     int subngrams(std::vector<std::pair<Pattern,int>> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)

     /**
      * Finds all the parts of a skipgram, parts are the portions that are not skips and adds them to container... Thus 'to be {*} not {*} be' has three parts 
      */
     int parts(std::vector<Pattern> & container) const; 
     
     /**
      * Finds all the parts of a skipgram, parts are the portions that are not skips and adds them to container as begin,length pairs... Thus 'to be {*} not {*} be' has three parts 
      */
     int parts(std::vector<std::pair<int,int> > & container) const; 

     /**
      * Finds all the gaps of a skipgram or flexgram., parts are the portions that are not skips and adds them to container as begin,length pairs... Thus 'to be {*} not {*} be' has three parts. The gap-length of a flexgram will always be its minimum length one.
      */
     int gaps(std::vector<std::pair<int,int> > & container) const; 

     Pattern extractskipcontent(Pattern & instance) const; //given a pattern and an instance, extract a pattern from the instance that would fill the gaps

     Pattern replace(int begin, int length, const Pattern & replacement) const;
     Pattern addskips(std::vector<std::pair<int,int> > & gaps) const;
     Pattern addflexgaps(std::vector<std::pair<int,int> > & gaps) const;

     /**
      * converts a skipgram into a flexgram, ngrams just come out unchanged
      */
     Pattern toflexgram() const;

     bool isgap(int i) const; //is the word at this position a gap?

     //CHANGES from old colibri ngram:
     //
     //no slice, ngram, gettoken method, use slice constructor
     //no ngram method, use slice constructor
     
     //NOT IMPLEMENTED YET:

     Pattern addcontext(const Pattern & leftcontext, const Pattern & rightcontext) const;    
     void mask(std::vector<bool> & container) const; //returns a boolean mask of the skipgram (0 = gap(encapsulation) , 1 = skipgram coverage)

     //sets an entirely new value
     void set(const unsigned char* dataref, const int size); 
};

const unsigned char tmp_skipmarker = SKIPMARKER;
const unsigned char tmp_flexmarker = FLEXMARKER;
//const uint16_t tmp_unk = 0x0102; //0x01 0x02
//const uint16_t tmp_bos = 0x0103; //0x01 0x03
//const uint16_t tmp_eos = 0x0104; //0x01 0x04
static const unsigned char * tmp_unk = (const unsigned char *) "\1\2";
static const unsigned char * tmp_bos = (const unsigned char *) "\1\3";
static const unsigned char * tmp_eos = (const unsigned char *) "\1\4";
const Pattern SKIPPATTERN = Pattern(&tmp_skipmarker,1);
const Pattern FLEXPATTERN = Pattern(&tmp_flexmarker,1);
const Pattern BEGINPATTERN = Pattern((const unsigned char *) &tmp_bos,2);
const Pattern ENDPATTERN = Pattern((const unsigned  char *) &tmp_eos,2);
const Pattern UNKPATTERN = Pattern((const unsigned char* ) &tmp_unk,2);

namespace std {

    template <>
    struct hash<Pattern> {
     public: 
          size_t operator()(Pattern pattern) const throw() {                            
              return pattern.hash();              
          }
    };

    template <>
    struct hash<const Pattern> {
     public: 
          size_t operator()(const Pattern pattern) const throw() {                            
              return pattern.hash();              
          }
    };


    template <>
    struct hash<const Pattern *> {
     public: 
          size_t operator()(const Pattern * pattern) const throw() {                            
              return pattern->hash();              
          }
    };

}


/******************** IndexReference **************************/
class IndexReference {
    /* Reference to a position in the corpus */
   public:
    uint32_t sentence;
    uint16_t token;
    IndexReference() { sentence=0; token = 0; } 
    IndexReference(uint32_t sentence, uint16_t token ) { this->sentence = sentence; this->token = token; }  
    IndexReference(std::istream * in) {
        in->read( (char*) &sentence, sizeof(uint32_t)); 
        in->read( (char*) &token, sizeof(uint16_t)); 
    }
    IndexReference(const IndexReference& other) { //copy constructor
        sentence = other.sentence;
        token = other.token;
    };     
    void write(std::ostream * out) const {
        out->write( (char*) &sentence, sizeof(uint32_t)); 
        out->write( (char*) &token, sizeof(uint16_t)); 
    }
    bool operator< (const IndexReference& other) const {
        if (sentence < other.sentence) {
            return true;
        } else if (sentence == other.sentence) {
            return (token < other.token);
        } else {
            return false;
        }
    }
    bool operator> (const IndexReference& other) const {
        return other < *this;
    }
    bool operator==(const IndexReference &other) const { return ( (sentence == other.sentence) && (token == other.token)); };
    bool operator!=(const IndexReference &other) const { return ( (sentence != other.sentence) || (token != other.token)); };
    IndexReference operator+(const int other) const { return IndexReference(sentence, token+ other); };
    
    std::string tostring() const {
        return std::to_string(sentence) + ":" + std::to_string(token);
    }
};


/***********************************************************************************/

//Class for reading an entire (class encoded) corpus into memory, providing a
//reverse index by IndexReference
//TODO: implement
class IndexedCorpus {
    protected:
        std::map<IndexReference,Pattern> data; //tokens
    public:
        IndexedCorpus() {};
        IndexedCorpus(std::istream *in);
        IndexedCorpus(std::string filename);
        
        void load(std::istream *in);
        void load(std::string filename);
        typedef std::map<IndexReference,Pattern>::iterator iterator;
        typedef std::map<IndexReference,Pattern>::const_iterator const_iterator;

        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const IndexReference ref) { return data.find(ref); }
        const_iterator find(const IndexReference ref) const { return data.find(ref); }
        
        bool has(const IndexReference ref) const { return data.count(ref); }

        size_t size() const { return data.size(); } 

        Pattern operator [](const IndexReference ref) { return data[ref]; } 

        Pattern getpattern(IndexReference begin, int length);
         
        std::vector<IndexReference> findmatches(const Pattern & pattern, int maxmatches=0); //by far not as efficient as a pattern model obviously

        int sentencelength(int sentence); 
};

/************* ValueHandler for reading/serialising basic types ********************/

/*
 * Value handler deal are interfaces to the values in Pattern Maps. They are
 * used to abstract from the actual value data type and provide some common
 * methods required for all values, as well at methods for serialisation
 * from/to binary file. All are derived from the abstract class
 * AbstractValueHandler
*/

template<class ValueType>
class AbstractValueHandler {
   public:
    virtual std::string id() { return "AbstractValueHandler"; }
    virtual void read(std::istream * in, ValueType & value)=0; //read value from input stream (binary)
    virtual void write(std::ostream * out, ValueType & value)=0; //write value to output stream (binary)
    virtual std::string tostring(ValueType & value)=0; //convert value to string)
    virtual int count(ValueType & value) const =0; //what count does this value represent?
    virtual void add(ValueType * value, const IndexReference & ref ) const=0; //add the indexreference to the value, will be called whenever a token is found during pattern building
    virtual void convertto(ValueType & source, ValueType & target ) const { if (&source != &target) target = source; }; //this doesn't really convert as source and target are same type, but it is required!
};

// This templated class can be used for all numeric base types (such as int, uint16_t,
// float, etc)
template<class ValueType>
class BaseValueHandler: public AbstractValueHandler<ValueType> {
   public:
    virtual std::string id() { return "BaseValueHandler"; }
    const static bool indexed = false;
    void read(std::istream * in, ValueType & v) {
        in->read( (char*) &v, sizeof(ValueType)); 
    }
    void write(std::ostream * out, ValueType & value) {
        out->write( (char*) &value, sizeof(ValueType));
    }
    virtual std::string tostring(ValueType & value) {
        return tostring(value);
    }
    int count(ValueType & value) const {
        return (int) value;
    }
    void add(ValueType * value, const IndexReference & ref ) const {
        *value = *value + 1;
    }
};

/************* Base abstract container for pattern storage  ********************/

//This is an abstract class, all Pattern storage containers are derived from
//this. ContainerType is the low-level container type used (an STL container such as set/map). ReadWriteSizeType influences only the maximum number of items that can be stored (2**64) in the container, as this will be represented in the very beginning of the binary file. No reason to change this unless the container is very deeply nested in others and contains only few items.
template<class ContainerType,class ReadWriteSizeType = uint64_t>
class PatternStore {
    public:
        PatternStore<ContainerType,ReadWriteSizeType>() {};
        ~PatternStore<ContainerType,ReadWriteSizeType>() {};
    

        virtual void insert(const Pattern & pattern)=0; //might be a noop in some implementations that require a value

        virtual bool has(const Pattern &) const =0;
        virtual bool erase(const Pattern &) =0;
        
        virtual size_t size() const =0; 
        
        
        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find(const Pattern & pattern)=0;
        
        virtual void write(std::ostream * out)=0;
        //virtual void read(std::istream * in, int MINTOKENS)=0;

};


/************* Abstract datatype for all kinds of maps ********************/


template<class ContainerType, class ValueType, class ValueHandler,class ReadWriteSizeType = uint32_t>
class PatternMapStore: public PatternStore<ContainerType,ReadWriteSizeType> { 
     protected:
        ValueHandler valuehandler;
     public:
        PatternMapStore<ContainerType,ValueType,ValueHandler,ReadWriteSizeType>(): PatternStore<ContainerType,ReadWriteSizeType>() {};
        ~PatternMapStore<ContainerType,ValueType,ValueHandler,ReadWriteSizeType>() {};

        virtual void insert(const Pattern & pattern, ValueType & value)=0;

        virtual bool has(const Pattern &) const =0;
        virtual bool erase(const Pattern &) =0;
        
        virtual size_t size() const =0; 
        
        
        virtual ValueType & operator [](const Pattern & pattern)=0;
        
        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find(const Pattern & pattern)=0;

        virtual void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = this->begin(); iter != this->end(); iter++) {
                Pattern p = iter->first;
                p.write(out);
                this->valuehandler.write(out, iter->second);
            }
        }
        
        virtual void write(std::string filename) {
            std::ofstream * out = new std::ofstream(filename.c_str());
            this->write(out);
            out->close();
            delete out;
        }


        template<class ReadValueType=ValueType, class ReadValueHandler=ValueHandler>
        void read(std::istream * in, int MINTOKENS=0) {
            ReadValueHandler readvaluehandler = ReadValueHandler();
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            //std::cerr << "Reading " << (int) s << " patterns" << std::endl;
            for (ReadWriteSizeType i = 0; i < s; i++) {
                Pattern p = Pattern(in);
                ReadValueType readvalue;
                //std::cerr << "Read pattern: " << std::endl;
                readvaluehandler.read(in, readvalue);
                if (readvaluehandler.count(readvalue) >= MINTOKENS) {
                        ValueType convertedvalue;
                        readvaluehandler.convertto(readvalue, convertedvalue); 
                        this->insert(p,convertedvalue);
                }
            }
        }

        void read(std::string filename,int MINTOKENS=0) { //no templates for this one, easier on python/cython
            std::ifstream * in = new std::ifstream(filename.c_str());
            this->read<ValueType,ValueHandler>(in);
            in->close();
            delete in;
        }

};


/************* Specific STL-like containers for patterns ********************/


/*
 * These are the containers you can directly use from your software
 *
 */

typedef std::unordered_set<Pattern> t_patternset;

template<class ReadWriteSizeType = uint32_t>
class PatternSet: public PatternStore<t_patternset,ReadWriteSizeType> {
    protected:
        t_patternset data;
    public:

        PatternSet<ReadWriteSizeType>(): PatternStore<t_patternset,ReadWriteSizeType>() {};
        ~PatternSet<ReadWriteSizeType>() {};

        void insert(const Pattern & pattern) {
            data.insert(pattern);
        }

        bool has(const Pattern & pattern) const { return data.count(pattern); }
        size_t size() const { return data.size(); } 


        typedef t_patternset::iterator iterator;
        typedef t_patternset::const_iterator const_iterator;
        
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }


        void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = begin(); iter != end(); iter++) {
                Pattern p = *iter;
                p.write(out);
            }
        }

        void read(std::istream * in) {
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            for (unsigned int i = 0; i < s; i++) {
                Pattern p = Pattern(in);
                insert(p);
            }
        }

};


typedef std::set<Pattern> t_orderedpatternset;


template<class ReadWriteSizeType = uint64_t>
class OrderedPatternSet: public PatternStore<t_orderedpatternset,ReadWriteSizeType> {
    protected:
        t_orderedpatternset data;
    public:

        OrderedPatternSet<ReadWriteSizeType>(): PatternStore<t_orderedpatternset,ReadWriteSizeType>() {};
        ~OrderedPatternSet<ReadWriteSizeType>();

        void insert(const Pattern pattern) {
            data.insert(pattern);
        }

        bool has(const Pattern & pattern) const { return data.count(pattern); }
        size_t size() const { return data.size(); } 

        typedef t_orderedpatternset::iterator iterator;
        typedef t_orderedpatternset::const_iterator const_iterator;
        
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }

        
        void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = begin(); iter != end(); iter++) {
                Pattern p = *iter;
                p.write(out);
            }
        }

        void read(std::istream * in) {
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            for (unsigned int i = 0; i < s; i++) {
                Pattern p = Pattern(in);
                insert(p);
            }
        }

};


template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class ReadWriteSizeType = uint64_t>
class PatternMap: public PatternMapStore<std::unordered_map<const Pattern,ValueType>,ValueType,ValueHandler,ReadWriteSizeType> {
    protected:
        std::unordered_map<const Pattern, ValueType> data;
    public:
        //PatternMap(): PatternMapStore<std::unordered_map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        PatternMap<ValueType,ValueHandler,ReadWriteSizeType>() {};

        void insert(const Pattern & pattern, ValueType & value) { 
            data[pattern] = value;
        }

        void insert(const Pattern & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType, usually 0
        
        bool has(const Pattern & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); } 

        ValueType& operator [](const Pattern & pattern) { return data[pattern]; } 
        
        typedef typename std::unordered_map<const Pattern,ValueType>::iterator iterator;
        typedef typename std::unordered_map<const Pattern,ValueType>::const_iterator const_iterator;
        
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }
        
        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }

};


template<class ValueType,class ValueHandler = BaseValueHandler<ValueType>,class ReadWriteSizeType = uint64_t>
class OrderedPatternMap: public PatternMapStore<std::map<const Pattern,ValueType>,ValueType,ValueHandler,ReadWriteSizeType> {
    protected:
        std::map<const Pattern, ValueType> data;
    public:
        OrderedPatternMap<ValueType,ValueHandler,ReadWriteSizeType>(): PatternMapStore<std::map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        ~OrderedPatternMap<ValueType,ValueHandler,ReadWriteSizeType>() {};

        void insert(const Pattern & pattern, ValueType & value) { 
            data[pattern] = value;
        }

        void insert(const Pattern & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType

        bool has(const Pattern & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); } 

        ValueType& operator [](const Pattern & pattern) { return data[pattern]; } 
        
        typedef typename std::map<const Pattern,ValueType>::iterator iterator;
        typedef typename std::map<const Pattern,ValueType>::const_iterator const_iterator;
        
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }
        

};


template<class T,size_t N,int countindex = 0>
class ArrayValueHandler: public AbstractValueHandler<T> {
   public:
    const static bool indexed = false;
    virtual std::string id() { return "ArrayValueHandler"; }
    void read(std::istream * in, std::array<T,N> & a) {
        for (int i = 0; i < N; i++) {
            T v;
            in->read( (char*) &v, sizeof(T)); 
            a[i] = v;
        }
    }
    void write(std::ostream * out, std::array<T,N> & a) {
        for (int i = 0; i < N; i++) {
            T v = a[i];
            out->write( (char*) &v, sizeof(T)); 
        }
    }
    std::string tostring(std::array<T,N> & a) {
        std::string s;
        for (int i = 0; i < N; i++) {
            T v = a[i];
            if (!s.empty()) s += " ";
            s += " " + tostring(a[i]);
        }
        return s;
    }
    int count(std::array<T,N> & a) const {
        return (int) a[countindex];
    }
    void add(std::array<T,N> * value, const IndexReference & ref ) const {
        (*value)[countindex] += 1;
    }
};


//using a PatternStore as value in a PatternMap requires a special valuehandler:
template<class PatternStoreType>
class PatternStoreValueHandler: public AbstractValueHandler<PatternStoreType> {
  public:
    const static bool indexed = false;
    virtual std::string id() { return "PatternStoreValueHandler"; }
    void read(std::istream * in,  PatternStoreType & value) {
        value.read(in);
    }
    void write(std::ostream * out,  PatternStoreType & value) {
        value.write(out);
    }
    virtual std::string tostring(  PatternStoreType & value) {
        std::cerr << "PatternStoreValueHandler::tostring() is not supported" << std::endl;
        throw InternalError();
    }
    int count( PatternStoreType & value) const {
        return value.size();
    }
    void add( PatternStoreType * value, const IndexReference & ref ) const {
        std::cerr << "PatternStoreValueHandler::add() is not supported" << std::endl;
        throw InternalError();
    }
};

template<class ValueType,class ValueHandler=BaseValueHandler<ValueType>, class NestedSizeType = uint16_t >
class AlignedPatternMap: public PatternMap< PatternMap<ValueType,ValueHandler,NestedSizeType>,PatternStoreValueHandler<PatternMap<ValueType,ValueHandler,NestedSizeType>>, uint64_t > {
    public:
        typedef PatternMap<ValueType,ValueHandler,NestedSizeType> valuetype;
        typedef typename PatternMap< PatternMap<ValueType,ValueHandler,NestedSizeType>,PatternStoreValueHandler<PatternMap<ValueType,ValueHandler,NestedSizeType>>, uint64_t >::iterator iterator;
        typedef typename PatternMap< PatternMap<ValueType,ValueHandler,NestedSizeType>,PatternStoreValueHandler<PatternMap<ValueType,ValueHandler,NestedSizeType>>, uint64_t >::const_iterator const_iterator;

};


//TODO: Implement a real Trie, conserving more memory


#endif
