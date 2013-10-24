#ifndef COLIBRIPATTERN_H
#define COLIBRIPATTERN_H

#include <string>
#include <iostream>
#include <ostream>
#include <istream>
#include <unordered_map>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <iomanip> // contains setprecision()
#include <exception>
#include "common.h"
#include "classdecoder.h"


const int MAINPATTERNBUFFERSIZE = 40960;

//Pattern categories
enum PatternCategory {
    UNKNOWNPATTERN = 0, NGRAM = 1, SKIPGRAM = 2, FLEXGRAM = 3
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



/*
 * This is the main pattern class
 * */
class Pattern {
    protected:
     void reader_marker(unsigned char * _data, std::istream * in);
    public:
     unsigned char * data; //holds the variable-width byte representation, terminated by \0 (ENDMARKER)

     Pattern() { data = new unsigned char[1]; data[0] = ENDMARKER; } //empty constructor (still consumes 1 byte though)
     Pattern(const unsigned char* dataref, const int size); //low-level constructor
     Pattern(const Pattern& ref, int begin, int length); //slice constructor
     Pattern(const Pattern& ref); //copy constructor
     Pattern(std::istream * in); //read from binary file constructor
     ~Pattern();

     Pattern(int size) {
         //pattern consisting only of fixed skips
         data = new unsigned char[size+1];
         for (int i = 0; i < size; i++) data[i] = SKIPMARKER;
         data[size] = ENDMARKER;
     }

     void write(std::ostream * out) const; //write binary output

     const size_t n() const; //return the size of the pattern in tokens (will count dynamic gaps as size ))
     const size_t bytesize() const; //return the size of the pattern (in bytes)
     const size_t size() const { return n(); } // alias
     const unsigned int skipcount() const; //return the number of skips
     const PatternCategory category() const;
     const StructureType type() const;
     const bool isskipgram() const { return category() == SKIPGRAM; }
     const bool isflexgram() const { return category() == FLEXGRAM; }
     
     Pattern operator [](int index) { return Pattern(*this, index,1); } //return single token, not byte!! use with n(), not with size()

     const size_t hash(bool stripmarkers = false) const;

     std::string tostring(ClassDecoder& classdecoder) const; //pattern to string (decode)
     std::string decode(ClassDecoder& classdecoder) const { return tostring(classdecoder); } //alias
     bool out() const;

     bool operator==(const Pattern & other) const;
     bool operator!=(const Pattern & other) const;

     Pattern & operator =(const Pattern other);   


     //patterns are sortable
     bool operator<(const Pattern & other) const;
     bool operator>(const Pattern & other) const;

     Pattern operator +(const Pattern&) const;

     int find(const Pattern & pattern) const; //returns the index, -1 if not found 
     bool contains(const Pattern & pattern) const; //does the pattern contain the smaller pattern?
     bool instanceof(const Pattern & skipgram) const; //Is this a full instantiation of the skipgram?
    
     

     int ngrams(std::vector<Pattern> & container, const int n) const; //return multiple ngrams
     int subngrams(std::vector<Pattern> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)

     int ngrams(std::vector<std::pair<Pattern,int>> & container, const int n) const; //return multiple ngrams
     int subngrams(std::vector<std::pair<Pattern,int>> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)

     int parts(std::vector<Pattern> & container) const; //find all the parts of a skipgram, parts are the portions that are not skips... Thus 'to be {*} not {*} be' has three parts 
     int parts(std::vector<std::pair<int,int> > & container) const; //same as above, but return (begin,length) pairs instead of patterns

     int gaps(std::vector<std::pair<int,int> > & container) const; //returns the positions (begin, length) of the gaps in a skipgram

     Pattern extractskipcontent(Pattern & instance) const; //given a pattern and an instance, extract a pattern from the instance that would fill the gaps

     Pattern replace(int begin, int length, const Pattern & replacement) const;
     Pattern addfixedskips(std::vector<std::pair<int,int> > & gaps) const;
     Pattern adddynamicskips(std::vector<std::pair<int,int> > & gaps) const;

     Pattern todynamic() const; //converts a fixed skipgram into a dynamic one, ngrams just come out unchanged

     //CHANGES from old colibri ngram:
     //
     //no slice, ngram, gettoken method, use slice constructor
     //no ngram method, use slice constructor
     
     //NOT IMPLEMENTED YET:

     Pattern addcontext(const Pattern & leftcontext, const Pattern & rightcontext) const;    
     void mask(std::vector<bool> & container) const; //returns a boolean mask of the skipgram (0 = gap(encapsulation) , 1 = skipgram coverage)
};

const unsigned char tmp_skipmarker = SKIPMARKER;
const unsigned char tmp_flexmarker = FLEXMARKER;
const Pattern SKIPPATTERN = Pattern(&tmp_skipmarker,1);
const Pattern FLEXPATTERN = Pattern(&tmp_flexmarker,1);

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

//Class for reading an entire (class encoded) corpus into memory, providing an
//index by IndexReference
//TODO: implement
class IndexedCorpus {
    protected:
        std::map<IndexReference,Pattern> data; //tokens
    public:
        IndexedCorpus(std::istream *in);
        IndexedCorpus(std::string filename);
        
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

        Pattern getpattern(IndexReference begin, int length); //TODO: implement
        
        std::vector<IndexReference> findmatches(const Pattern & pattern); //not as efficient as a pattern model obviously, TODO: implement
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
        
        virtual const Pattern * getpointer(const Pattern &) =0; //get the pattern in the store, or NULL if it does not exist
        
        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find(const Pattern & pattern)=0;
        
        virtual void write(std::ostream * out)=0;
        //virtual void read(std::istream * in, int MINTOKENS)=0;

};


/************* Abstract datatype for all kinds of maps ********************/


template<class ContainerType, class ValueType, class ValueHandler=BaseValueHandler<ValueType>,class ReadWriteSizeType = uint32_t>
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
        
        virtual const Pattern * getpointer(const Pattern &) =0; //get the pattern in the store, or NULL if it does not exist
        
        virtual ValueType & operator [](const Pattern & pattern)=0;
        
        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find(const Pattern & pattern)=0;

        virtual void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = begin(); iter != end(); iter++) {
                Pattern p = iter->first;
                p.write(out);
                valuehandler.write(out, iter->second);
            }
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
                        insert(p,convertedvalue);
                }
            }
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

        const Pattern * getpointer(const Pattern & pattern) { //get the pattern in the store, or NULL if it does not exist
            iterator iter = find(pattern);
            if (iter == end()) {
                return NULL;
            } else {
                return &(*iter);
            }
        }

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

//using PatternSet as value in a PatternMap requires a special valuehandler:
class PatternSetValueHandler: public AbstractValueHandler<PatternSet<>> {
    const static bool indexed = false;
    void read(std::istream * in, PatternSet<> & value) {
        value.read(in);
    }
    void write(std::ostream * out, PatternSet<> & value) {
        value.write(out);
    }
    virtual std::string tostring(PatternSet<> & value) {
        return ""; //NOT IMPLEMENTED! TODO
    }
    int count(PatternSet<> & value) const {
        return value.size();
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

        const Pattern * getpointer(const Pattern & pattern) { //get the pattern in the store, or NULL if it does not exist
            iterator iter = find(pattern);
            if (iter == end()) {
                return NULL;
            } else {
                return &(*iter);
            }
        }
        
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

        const Pattern * getpointer(const Pattern & pattern) { //get the pattern in the store, or NULL if it does not exist
            iterator iter = find(pattern);
            if (iter == end()) {
                return NULL;
            } else {
                return &(iter->first);
            }
        }
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

        ValueType operator [](const Pattern & pattern) { return data[pattern]; } 
        
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

        const Pattern * getpointer(const Pattern & pattern) { //get the pattern in the store, or NULL if it does not exist
            iterator iter = find(pattern);
            if (iter == end()) {
                return NULL;
            } else {
                return &(*iter);
            }
        }
};


//TODO: Implement a real Trie, conserving more memory


#endif
