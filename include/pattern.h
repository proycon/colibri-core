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

std::set<unsigned char> ALLMARKERS;
for (int i = 130; i < 256; i++) ALLMARKERS.insert((unsigned char) i);



class PatternKeyError: public std::exception {
  virtual const char* what() const throw()  {
     return "Pattern is merely a key, get it first from a PatternStore";
  }
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
     Pattern(const Pattern& ref);
     Pattern(std::istream * in); //read from file
     ~Pattern();

     void write(std::ostream * out) const; //write binary output

     const unsigned int n() const; //return the size of the pattern in tokens (will return 0 if variable width gaps are present!)
     const unsigned int size() const; //return the size of the pattern (in bytes)
     const unsigned char category() const;
     const StructureType type() const;
     const bool isskipgram() const { return category() > NGRAM; }


     virtual std::string str(ClassDecoder& classdecoder) const; //pattern to string (decode)
     virtual bool out() const;

     bool operator==(const Pattern * other) const;
     bool operator!=(const Pattern * other) const;
     Pattern & operator =(Pattern other);   

     Pattern operator +(const Pattern&) const;



     Pattern slice(const int begin,const int length) const; 
     Pattern addcontext(const Pattern & leftcontext, const Pattern & rightcontext) const;
     Pattern gettoken(const int index) const; 
     int gettoken(const int index) const; // but returns class as int  (was: getclass)
    

     bool contains(const Pattern & pattern) const;

     int ngrams(vector<const Pattern> & container, int n) const; //return multiple ngrams
     Pattern ngram(int n, int index) const; //return a single ngram
     int parts(vector<const Pattern> & container) const; //only for skipgrams
     int parts(std::vector<std::pair<int,int> > & container) const; //inverse of getgaps
     int gaps(std::vector<std::pair<int,int> > & container) const;

     void mask(std::vector<bool> & container) const; //returns a boolean mask of the skipgram (0 = gap(encapsulation) , 1 = skipgram coverage)
};



class PatternStore {
    public:
        PatternStore();

        virtual bool has(const Pattern &) const =0;

        virtual const Pattern * get(const Pattern &) =0; //get the pattern in the store, or NULL if it does not exist
        
        virtual size_t size() const =0; 

        virtual void save(const string & filename);
};



template<class T>
typedef unordered_map<const Pattern, T> t_patternmap;

typedef unordered_set<const Pattern> t_patternset;



class PatternSet: public PatternStore {
    protected:
        t_patternset data;
    public:

        PatternSet(bool freeondestroy=true): PatternStore(freeondestroy) {};
        ~PatternSet();

        void insert(const Pattern pattern) {
            data.insert(pattern);
        }

        bool has(const Pattern & pattern) const { return data.count(pattern); }



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

        const_iterator find(const Pattern pattern) {
            return const_iterator(data.find(pattern));
        }
        
        const Pattern * get(const Pattern pattern) {
            const_iterator iter = find(pattern);
            if (iter != end()) {
                return *iter;
            } else {
                return NULL;
            } 
        }


};

/*
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
*/

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
