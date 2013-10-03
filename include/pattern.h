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

enum PatternCategory {
    KEY = 0, NGRAM = 1, FIXEDSKIPGRAM = 2, DYNAMICSKIPGRAM = 3
};

enum StructureType { 
    STRUCT_PATTERN = 0, //undefined pattern (if n==1 -> token)
    STRUCT_SENTENCE = 1,
    STRUCT_PARAGRAPH = 2,
    STRUCT_HEADER = 3, //like paragraph
    STRUCT_TEXT = 4, //aka document
    STRUCT_QUOTE = 5,
    STRUCT_DIV = 6,
};
enum Markers { // <128 size
    ENDMARKER = 0, //marks the end of each pattern (necessary!)

    TEXTMARKER = 255, //aka begin document 
    SENTENCEMARKER = 254,
    PARAGRAPHMARKER = 253,
    BEGINDIVMARKER = 252,
    ENDDIVMARKER = 251,
    HEADERMARKER = 250,
    
    DYNAMICGAP = 129,
    FIXEDGAP = 128,
};





void readanddiscardpattern(std::istream * in);
int reader_passmarker(const unsigned char c, std::istream * in); 




class Pattern {
    protected:
     void reader_marker(unsigned char * _data, std::istream * in);
    public:
     unsigned char * data;

     Pattern();
     Pattern(const unsigned char* dataref, const int size); //low-level constructor
     Pattern(const Pattern& ref, int begin, int length); //slice constructor
     Pattern(const Pattern& ref); //copy constructor
     Pattern(std::istream * in); //read from binary file constructor
     ~Pattern();

     void write(std::ostream * out) const; //write binary output

     const size_t n() const; //return the size of the pattern in tokens (will return 0 if variable width gaps are present!)
     const size_t bytesize() const; //return the size of the pattern (in bytes)
     const size_t size() const { return n(); } // alias
     const unsigned int skipcount() const; //return the number of skips
     const PatternCategory category() const;
     const StructureType type() const;
     const bool isskipgram() const { return category() > NGRAM; }
     
     Pattern operator [](int index) { return Pattern(*this, index,1); } //return single token, not byte!! use with n(), not with size()

     const size_t hash(bool stripmarkers = false) const;

     std::string tostring(ClassDecoder& classdecoder) const; //pattern to string (decode)
     std::string decode(ClassDecoder& classdecoder) const { return tostring(classdecoder); } //alias
     bool out() const;

     bool operator==(const Pattern & other) const;
     bool operator!=(const Pattern & other) const;
     Pattern & operator =(Pattern other);   

     //patterns are sortable
     bool operator<(const Pattern & other) const;
     bool operator>(const Pattern & other) const;

     Pattern operator +(const Pattern&) const;

     int find(const Pattern & pattern) const; //returns the index, -1 if not found 
     bool contains(const Pattern & pattern) const;

     int ngrams(std::vector<Pattern> & container, const int n) const; //return multiple ngrams
     int subngrams(std::vector<Pattern> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)

     int ngrams(std::vector<std::pair<Pattern,int>> & container, const int n) const; //return multiple ngrams
     int subngrams(std::vector<std::pair<Pattern,int>> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)

     int parts(std::vector<Pattern> & container) const; 
     int parts(std::vector<std::pair<int,int> > & container) const;

     int gaps(std::vector<std::pair<int,int> > & container) const;

     Pattern extractskipcontent(Pattern & instance) const; //given a pattern and an instance, extract a pattern from the instance that would fill the gaps

     Pattern replace(int begin, int length, const Pattern & replacement) const;
     Pattern addfixedskips(std::vector<std::pair<int,int> > & gaps) const;
     Pattern adddynamicskips(std::vector<std::pair<int,int> > & gaps) const;

     //CHANGES from old colibri ngram:
     //
     //no slice, ngram, gettoken method, use slice constructor
     //no ngram method, use slice constructor
     
     //NOT IMPLEMENTED YET:

     Pattern addcontext(const Pattern & leftcontext, const Pattern & rightcontext) const;    
     void mask(std::vector<bool> & container) const; //returns a boolean mask of the skipgram (0 = gap(encapsulation) , 1 = skipgram coverage)
};

const unsigned char tmp_fixedgap = FIXEDGAP;
const unsigned char tmp_dynamicgap = DYNAMICGAP;
const Pattern FIXEDGAPPATTERN = Pattern(&tmp_fixedgap,1);
const Pattern DYNAMICGAPPATTERN = Pattern(&tmp_dynamicgap,1);

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

/************* ValueHandler for reading/serialising basic types ********************/

template<class ValueType>
class AbstractValueHandler {
    virtual ValueType read(std::istream * in)=0;
    virtual void write(std::ostream * out, ValueType & value)=0;
    virtual std::string tostring(ValueType & value)=0;
    virtual int count(ValueType & value) const =0;
    virtual int add(const Pattern & pattern, ValueType * value, IndexReference & ref)=0;
};

template<class ValueType>
class BaseValueHandler: public AbstractValueHandler<ValueType> {
    const static bool indexed = false;
    ValueType read(std::istream * in) {
        ValueType v;
        in->read( (char*) &v, sizeof(ValueType)); 
        return v;
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
};

/************* Base abstract container for pattern storage  ********************/

template<class ContainerType,class ReadWriteSizeType = uint64_t>
class PatternStore {
    public:
        PatternStore();

        virtual void insert(const Pattern & pattern)=0; //might be a noop in some implementations that require a value

        virtual bool has(const Pattern &) const =0;
        virtual bool erase(const Pattern &) =0;
        
        virtual size_t size() const =0; 
        
        virtual const Pattern * getpointer(const Pattern &) =0; //get the pattern in the store, or NULL if it does not exist
        
        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find()=0;
        
        virtual void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = begin(); iter != end(); iter++) {
                Pattern p = iter->first;
                p.write(out);
            }
        }

        virtual void read(std::istream * in) {
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            for (int i = 0; i < s; i++) {
                Pattern p = Pattern(in);
                insert(p);
            }
        }
};


/************* Abstract datatype for all kinds of maps ********************/

template<class ContainerType, class ValueType, class ValueHandler=BaseValueHandler<ValueType>,class ReadWriteSizeType = uint64_t>
class PatternMapStore: public PatternStore<ContainerType,ReadWriteSizeType> { 
     protected:
        ValueHandler valuehandler;
     public:
        PatternMapStore(): PatternStore<ContainerType,ReadWriteSizeType>() {};
        ~PatternMapStore();

        virtual void insert(const Pattern & pattern, ValueType & value)=0;

        virtual bool has(const Pattern &) const =0;
        virtual bool erase(const Pattern &) const =0;
        
        virtual size_t size() const =0; 
        
        virtual const Pattern * getpointer(const Pattern &) =0; //get the pattern in the store, or NULL if it does not exist
        
        virtual ValueType operator [](const Pattern & pattern)=0;
        virtual ValueType operator [](const Pattern pattern)=0;
        
        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find()=0;

        virtual void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = begin(); iter != end(); iter++) {
                Pattern p = iter->first;
                p.write(out);
                valuehandler.write(out, iter->second);
            }
        }

        virtual void read(std::istream * in) {
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            for (ReadWriteSizeType i = 0; i < s; i++) {
                Pattern p = Pattern(in);
                ValueType value = valuehandler.read(in);
                insert(p,value);
            }
        }

};


/************* Specific STL-compatible containers ********************/

typedef std::unordered_set<Pattern> t_patternset;

template<class ReadWriteSizeType = uint64_t>
class PatternSet: public PatternStore<t_patternset,ReadWriteSizeType> {
    protected:
        t_patternset data;
    public:

        PatternSet(): PatternStore<t_patternset,ReadWriteSizeType>() {};
        ~PatternSet();

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

};

typedef std::set<Pattern> t_orderedpatternset;


template<class ReadWriteSizeType = uint64_t>
class OrderedPatternSet: public PatternStore<t_orderedpatternset,ReadWriteSizeType> {
    protected:
        t_orderedpatternset data;
    public:

        OrderedPatternSet(): PatternStore<t_orderedpatternset,ReadWriteSizeType>() {};
        ~OrderedPatternSet();

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

        const Pattern * getpointer(const Pattern & pattern) { //get the pattern in the store, or NULL if it does not exist
            iterator iter = find(pattern);
            if (iter == end()) {
                return NULL;
            } else {
                return &(*iter);
            }
        }

};


template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class ReadWriteSizeType = uint64_t>
class PatternMap: public PatternMapStore<std::unordered_map<const Pattern,ValueType>,ValueType,ValueHandler,ReadWriteSizeType> {
    protected:
        std::unordered_map<const Pattern, ValueType> data;
    public:
        PatternMap(): PatternMapStore<std::unordered_map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        ~PatternMap();

        void insert(const Pattern & pattern, ValueType & value) { 
            data[pattern] = value;
        }

        void insert(const Pattern & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType
        
        bool has(const Pattern & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); } 

        ValueType operator [](const Pattern & pattern) { return data[&pattern]; } 
        ValueType operator [](const Pattern pattern) { return data[pattern]; }
        
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
                return &(*iter);
            }
        }
};


template<class ValueType,class ValueHandler = BaseValueHandler<ValueType>,class ReadWriteSizeType = uint64_t>
class OrderedPatternMap: public PatternMapStore<std::map<const Pattern,ValueType>,ValueType,ValueHandler,ReadWriteSizeType> {
    protected:
        std::map<const Pattern, ValueType> data;
    public:
        OrderedPatternMap(): PatternMapStore<std::map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        ~OrderedPatternMap();

        void insert(const Pattern & pattern, ValueType & value) { 
            data[pattern] = value;
        }

        void insert(const Pattern & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType

        bool has(const Pattern & pattern) const { return data.count(pattern); }

        bool erase(const Pattern & pattern) { return data.erase(pattern); }

        size_t size() const { return data.size(); } 

        ValueType operator [](const Pattern & pattern) { return data[&pattern]; } 
        ValueType operator [](const Pattern pattern) { return data[pattern]; }
        
        typedef typename std::map<const Pattern,ValueType>::iterator iterator;
        typedef typename std::map<const Pattern,ValueType>::const_iterator const_iterator;
        
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        const Pattern * getpointer(const Pattern & pattern) { //get the pattern in the store, or NULL if it does not exist
            iterator iter = find(pattern);
            if (iter == end()) {
                return NULL;
            } else {
                return &(*iter);
            }
        }
};





template<class MapType,class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class ReadWriteSizeType = uint32_t>
class PatternGraph: public PatternMapStore<MapType,ValueType,ValueHandler, ReadWriteSizeType> {
    public:
        PatternGraph(): PatternMapStore<MapType,ValueType,ValueHandler, ReadWriteSizeType>() {};
        ~PatternGraph();

        bool has(const Pattern & pattern, const Pattern & pattern2) const { if (has(pattern) > 0) { (*this)[pattern].has(pattern2); } else return 0; }
        
        ValueType getvalue(const Pattern & pattern, const Pattern & pattern2) { if (has(pattern,pattern2)) { return (*this)[pattern][pattern2]; } else return ValueType(); }


        void insert(const Pattern pattern, const Pattern pattern2, const ValueType value) {
            (*this)[pattern][pattern2] = value;
        }

        
        virtual void write(std::ostream * out) {
            uint64_t s = (uint64_t) this->size();
            out->write( (char*) &s, sizeof(uint64_t));
            for (typename PatternMapStore<MapType,ValueType,ValueHandler, ReadWriteSizeType>::iterator iter = this->begin(); iter != this->end(); iter++) {
                Pattern p = iter->first;
                p.write(out);
                ReadWriteSizeType s2 = (ReadWriteSizeType) iter->second.size();
                out->write( (char*) &s2, sizeof(ReadWriteSizeType));
                for (typename MapType::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
                    Pattern p2 = iter->first;
                    p2.write(out);
                    this->valuehandler.write(out, iter->second);
                }
            }
        }

        virtual void read(std::istream * in) {
            uint64_t s; //read size:
            in->read( (char*) &s, sizeof(uint64_t));
            for (uint64_t i = 0; i < s; i++) {
                Pattern p = Pattern(in);
                ReadWriteSizeType s2; //read size:
                in->read( (char*) &s2, sizeof(ReadWriteSizeType));
                for (ReadWriteSizeType i = 0; i < s2; i++) {
                    Pattern p2 = Pattern(in);
                    ValueType value = this->valuehandler.read(in);
                    insert(p,p2,value);
                }
            }
        }
};




//TODO: Implement a real Trie, conserving more memory


#endif
