#ifndef PATTERN_H
#define NGRAM_H

#include <string>
#include <iostream>
#include <ostream>
#include <istream>
#include "classdecoder.h"
#include <unordered_map>
#include <vector>
#include <set>
#include <unordered_set>
#include <iomanip> // contains setprecision()
#include <exception>
#include "common.h"


const int MAINPATTERNBUFFERSIZE = 4096;
unsigned char mainpatternbuffer[MAINPATTERNBUFFERSIZE];

enum PatternCategory {
    KEY = 0, NGRAM = 1, FIXEDSKIPGRAM = 2, DYNAMICSKIPGRAM = 3;
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

enum Markers {
    ENDMARKER = 0,
    TEXTMARKER = 255, //aka begin document 
    SENTENCEMARKER = 254,
    PARAGRAPHMARKER = 253,
    BEGINDIVMARKER = 252,
    ENDDIVMARKER = 251,
    HEADERMARKER = 250,
    
    IDMARKER = 140,
    ANNOTATORMARKER = 141,
    ANNOTATORAUTOMARKER = 142,
    ANNOTATORMANUALMARKER = 143,

    FACTOR9 = 139,
    FACTOR8 = 138,
    FACTOR7 = 137,
    FACTOR6 = 136,
    FACTOR5 = 135,
    FACTOR4 = 134,
    FACTOR3 = 133,
    FACTOR2 = 132,
    FACTOR1 = 131,

    DYNAMICGAP = 129,
    FIXEDGAP = 128,
};

std::set<unsigned char> ALLMARKERS;
for (int i = 130; i < 256; i++) ALLMARKERS.insert((unsigned char) i);


enum FactorType {
    FT_CONTENT = 0,
    FT_ID = 1, 
    FT_TOKENANNOTATION = 2,
    FT_SPANANNOTATION = 3,    
};

class FactorDef {
   private:
    FactorType type;
    std::string name; //corresponds with for instance FoLiA tagname
    std::string set;
    ClassDecoder * classdecoder;
   public:
    FactorType gettype() { return type; }
    std::string getname() { return name; }
    std::string getset() { return set; }
    FactorDef(FactorType type, std::string name, std::set set, ClassDecoder * classdecoder = NULL) { this->type = type; this->name = name; this->set = set; this->classdecoder = classdecoder; }
};


class PatternKeyError: public std::exception {
  virtual const char* what() const throw()  {
     return "Pattern is merely a key, get it first from a PatternStore";
  }
};

void readanddiscardpattern(std::istream * in);
int reader_passmarker(const unsigned char c, std::istream * in); 

class Pattern {
    protected:
     //uint8_t props; //holds category in first three bits, copy in third bit, remaining fourth are reserved 
     //void setprops(PatternCategory category); //category can be inferred from data
     
     int reader_passmarker(int i) const; //auxiliary function to aid in parsing binary data
     void reader_marker(unsigned char * _data, std::istream * in);
    public:
     unsigned char * data;

     Pattern();
     Pattern(const unsigned char* dataref, const int size, const bool copy=true, const set<int> * factors = NULL);
     Pattern(const Pattern& ref, int begin, int length, const bool copy=true, const set<int> * factors = NULL); //slice constructor
     Pattern(const Pattern& ref);
     Pattern(std::istream * in); //read from file
     ~Pattern();

     void write(std::ostream * out) const; //write binary output

     const unsigned int n() const; //return the size of the pattern in tokens (will return 0 if variable width gaps are present!)
     const unsigned int size() const; //return the size of the pattern (in bytes)
     const std::set<char> factors() const; //return set of factors present (contain numbers 1...9)
     const unsigned char category() const;
     const StructureType type() const;
     const bool isskipgram() const { return category() > NGRAM; }
     const bool iskey() const { return (data[0] == 0); }



     virtual std::string str(ClassDecoder& classdecoder) const; //pattern to string (decode)
     virtual std::string str(vector<ClassDecoder*> classdecoder, std::string factordelimiter="|") const; //pattern to string, with all factors
     virtual bool out() const;

     bool operator==(const Pattern * other) const;
     bool operator!=(const Pattern * other) const;
     Pattern & operator =(Pattern other);   

     Pattern operator +(const Pattern&) const;


     virtual size_t hash(bool stripmarkers=true) const;

     Pattern slice(const int begin,const int length, const bool copy=true, const int factors=1) const; 
     
     Pattern addcontext(const Pattern & leftcontext, const Pattern & rightcontext) const;
     Pattern gettoken(int index, const bool copy=true, const int factors=1) const; 
     int gettoken(const int index) const; // but returns class as int  (was: getclass)
    

     bool contains(const Pattern & pattern) const;

     int ngrams(vector<const Pattern> & container, int n, const bool copy=true, const int factors=1) const; //return multiple ngrams
     Pattern ngram(int n, int index, const bool copy=true,const int factors=1) const; //return a single ngram
     int parts(vector<const Pattern> & container, const bool copy=true, const int factors=1) const; //only for skipgrams
     int parts(std::vector<std::pair<int,int> > & container) const; //inverse of getgaps
     int gaps(std::vector<std::pair<int,int> > & container) const;

     void mask(std::vector<bool> & container) const; //returns a boolean mask of the skipgram (0 = gap(encapsulation) , 1 = skipgram coverage)

     Pattern refactor(vector<int> factors) const;
};



class PatternStore {
    protected:
        map<int, FactorDef> factors;
    public:
        PatternStore();

        virtual bool has(const Pattern *) const =0;
        virtual bool hasfactor(const Pattern *, int factor=0) const =0;

        virtual void addfactor(FactorDef factordef);

        virtual const Pattern * get(const Pattern *) =0; //get the pattern in the store, or NULL if it does not exist
        virtual std::vector<const Pattern *> getbyfactor(const Pattern *, int factor=0) =0; //get the pattern in the store, or NULL if it does not exist
        
        virtual size_t size() const =0; 

        virtual void save(const string & filename);
};



template<class T>
typedef unordered_map<const Pattern, T> t_patternmap;

typedef unordered_set<const Pattern> t_patternset;

typedef unordered_set<const Pattern> t_patternset_factorindex;
typedef unordered_map<const Pattern, T>  t_patternmap_factorindex;


class PatternSet: public PatternStore {
    protected:
        t_patternset data;
        map<int,t_patternset_factorindex> factorindex;
    public:

        PatternSet(bool freeondestroy=true): PatternStore(freeondestroy) {};
        ~PatternSet();

        void insert(const Pattern * pattern) { data.insert(pattern); }
        void insert(const Pattern pattern) {
            Pattern * p = new Pattern(pattern);
            data.insert(pattern);
        }

        bool has(const Pattern * pattern) const { return data.count(pattern); }



        size_t size() const { return data.size; } 


        class const_iterator {
            public:
                typedef const_iterator self_type;
                typedef Pattern value_type;
                typedef Pattern& reference;
                typedef Pattern* pointer;

                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;
                iterator(patternset::iterator refiter) : iter(refiter) { }
                patterniterator operator++() { patterniterator i = *this; iter++; return i; }
                patterniterator operator++(int junk) { iter++; return *this; }
                reference operator*() { return *iter; }
                pointer operator->() { return iter; }
                PatternKey key() { return PatternKey(iter->hash()); }
                bool operator==(const patterniterator& other) { return iter == other.iter; }
                bool operator!=(const patterniterator& other) { return iter != other.iter; }
            private:
                patternset::const_iterator iter;
        };


        const_iterator begin() {
            return const_iterator(data.begin());
        }
 
        const_iterator end() {
            return const_iterator(data.end());
        }

        const_iterator find(const Pattern * pattern) {
            return const_iterator(data.find(pattern));
        }
        
        const Pattern * get(const Pattern * pattern) {
            const_iterator iter = find(pattern);
            if (iter != end()) {
                return *iter;
            } else {
                return NULL;
            } 
        }


};


template<class T>
class PatternMap<T>: public PatternStore {
    protected:
        t_patternmap data;
        map<int,t_patternmap_factorindex> factorindex;
    public:
        PatternMap(bool freeondestroy=true): PatternStore(freeondestroy) {};
        ~PatternMap();

        void insert(const Pattern * pattern, T value) { data.insert(pattern, value); }
        void insert(const Pattern pattern, T value) { 
            Pattern * p = new Pattern(pattern);
            data.insert(pattern, value);
        }

        bool has(const Pattern * pattern, int factor=0) const { return data.count(pattern); }

        size_t size() const { return data.size(); } 

        T operator [](const Pattern pattern) { return data[&pattern] }; 
        T operator [](const Pattern * pattern) { return data[pattern] };


        //abstract over patternmap::iterator
        class iterator {
            public:
                typedef iterator self_type;
                typedef std::pair<const Pattern *,T> value_type;
                typedef std::pair<const Pattern *,T>& reference;
                typedef std::pair<const Pattern *,T>* pointer;

                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;
                iterator(patternmap::iterator refiter) : iter(refiter) { }
                iterator operator++() { iterator i = *this; iter++; return i; }
                iterator operator++(int junk) { iter++; return *this; }
                reference operator*() { return *iter; }
                pointer operator->() { return iter; }
                PatternKey key() { return iter->first.hash(); }
                bool operator==(const iterator& other) { return iter == other.iter; }
                bool operator!=(const iterator& other) { return iter != other.iter; }
            private:
                patternmap::iterator iter;
        };


        iterator begin() {
            return iterator(data.begin());
        }
 
        iterator end() {
            return iterator(data.end());
        }

        iterator find(const Pattern * pattern) {
            return iterator(data.find(pattern));
        }
        
        const Pattern * get(const Pattern * pattern) {
            iterator iter = find(pattern);
            if (iter != end()) {
                return iter->first;
            } else {
                return NULL;
            } 
        }
}


namespace std {

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


#endif
