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
#include <algorithm>
#include "common.h"
#include "classdecoder.h"


/**
 * @file pattern.h
 * \brief Contains the Pattern class that is ubiquitous throughout Colibri Core.
 *
 * @author Maarten van Gompel (proycon) <proycon@anaproy.nl>
 * 
 * @section LICENSE
 * Licensed under GPLv3
 *
 * @section DESCRIPTION
 * Contains the Pattern class that is ubiquitous throughout Colibri Core
 *
 */

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

/**
 * Not really used much yet, but reserved for encoding structural markup later
 * on.
 */
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

class PatternPointer;

/**
 * \brief Pattern class, represents a pattern (ngram, skipgram or flexgram).
 * Encoded in a memory-saving fashion. Allows numerous operations.
 */
class Pattern {
    protected:
     void reader_marker(unsigned char * _data, std::istream * in);
    public:
     unsigned char * data; /**< This array holds the variable-width byte representation, it is always terminated by \0 (ENDMARKER). Though public, you usually do not want to access it directly */
     
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
     Pattern(const PatternPointer& ref,int begin, int length);

     /**
      * Copy constructor for Pattern
      * @param ref Reference pattern
      */
     Pattern(const Pattern& ref);
     Pattern(const PatternPointer& ref);

     /**
      * Read Pattern from input stream (in binary form)
      * @param in The input stream
      * @param ignoreeol Ignore end of line markers and read on until the end of the file, storing corpus data in one pattern
      */
     Pattern(std::istream * in, bool ignoreeol = false, bool debug = false); 
     Pattern(std::istream * in, unsigned char * buffer, int maxbuffersize, bool ignoreeol = false, bool debug = false);


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
     const bool unknown() const;
     
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
     std::string tostring(const ClassDecoder& classdecoder) const; //pattern to string (decode)

     /**
      * alias for tostring()
      */
     std::string decode(const ClassDecoder& classdecoder) const { return tostring(classdecoder); } //alias

     /**
      * Debug function outputting the classes in this pattern to stderr
      */
     bool out() const;

     /**
      * Convert the pattern to a vector of integers, where the integers
      * correspond to the token classes.
      */
     std::vector<int> tovector() const;

     bool operator==(const Pattern & other) const;
     bool operator!=(const Pattern & other) const;

     /**
      * Assignment operator
      */
     void operator =(const Pattern & other);   


     /**
      * Patterns can be sorted, note however that the sorting is based on the
      * frequencies of the tokens and is not alphanumerical!
      */
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
     int ngrams(std::vector<PatternPointer> & container, const int n) const; 

     /**
      * Adds all patterns (not just ngrams) of all sizes that are contained within the pattern to
      * container. Does not extract skipgrams that are not directly present in
      * the pattern.
      */
     int subngrams(std::vector<Pattern> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)
     int subngrams(std::vector<PatternPointer> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)

     /**
      * Adds all pairs of all patterns (not just ngrams) of size n that are
      * contained within the pattern, with the token offset at which they were
      * found,  to container.  Does not extract skipgrams that are not directly
      * present in the pattern.
      */
     int ngrams(std::vector<std::pair<Pattern,int>> & container, const int n) const; //return multiple ngrams
     int ngrams(std::vector<std::pair<PatternPointer,int>> & container, const int n) const; //return multiple ngrams

     /**
      * Adds all pairs of all patterns (not just ngrams) that are
      * contained within the pattern, with the token offset at which they were
      * found,  to container.  Does not extract skipgrams that are not directly
      * present in the pattern.
      */
     int subngrams(std::vector<std::pair<Pattern,int>> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)
     int subngrams(std::vector<std::pair<PatternPointer,int>> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)

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


     /**
      * Given a skipgram and an ngram instantation of it (i.e, both of the same length), extract a pattern from the instance that would fill the gaps. Raise an exception if the instance can not be matched with the skipgram
      */
     Pattern extractskipcontent(Pattern & instance) const; 

     /**
      * Replace the tokens from begin (0-indexed), up to the specified length,
      * with a replacement pattern (of any length)
      */
     Pattern replace(int begin, int length, const Pattern & replacement) const;

     /**
      * Replaces a series of tokens with a skip/gap of a particular size.
      * Effectively turns a pattern into a skipgram.
      * @param gap The position and size of the skip/gap: a pair consisting of a begin index (0-indexed) and a length, i.e. the size of the skip
      */
     Pattern addskip(std::pair<int,int> gap) const;
     /**
      * Replaces multiple series of tokens with skips/gaps of particular sizes.  Effectively turns a pattern into a skipgram.
      * @param gaps The positions and sizes of the gaps: a vector of pairs, each pair consisting of a begin index (0-indexed) and a length, indicating where to place the gap
      * @return A skipgram
      */
     Pattern addskips(std::vector<std::pair<int,int> > & gaps) const;
     /**
      * Replaces multiple series of tokens with skips/gaps of undefined variable size.  Effectively turns a pattern into a flexgram.
      * @param gaps The positions and sizes of the gaps: a vector of pairs, each pair consisting of a begin index (0-indexed) and a length, indicating where to place the gap
      * @return A flexgram
      */
     Pattern addflexgaps(std::vector<std::pair<int,int> > & gaps) const;

     /**
      * Returns a pattern with the tokens in reverse order
      */
     Pattern reverse() const; 

     /**
      * converts a skipgram into a flexgram (ngrams just come out unchanged)
      */
     Pattern toflexgram() const;

     /**
      * Is the word at the specified index (0 indexed) a gap?
      */
     bool isgap(int i) const; 

     
     //NOT IMPLEMENTED YET:

     Pattern addcontext(const Pattern & leftcontext, const Pattern & rightcontext) const;    
     void mask(std::vector<bool> & container) const; //returns a boolean mask of the skipgram (0 = gap(encapsulation) , 1 = skipgram coverage)

     //sets an entirely new value
     void set(const unsigned char* dataref, const int size); 
};


Pattern patternfromfile(const std::string & filename); //helper function to read pattern from file, mostly for Cython

class PatternPointer {
    public:
     unsigned char * data; /** Pointer to Pattern data */
     unsigned char bytes; //number of bytes
    
     PatternPointer(unsigned char* dataref, const int bytesize) {
         data = dataref;
         if (bytesize > 255) {
             std::cerr << "ERROR: Pattern too long for pattern pointer" << std::endl;
             throw InternalError();
         }
         bytes = bytesize;
     }

     PatternPointer(const Pattern * ref) {
         data = ref->data;
         const size_t b = ref->bytesize();
         if (b > 255) {
             std::cerr << "ERROR: Pattern too long for pattern pointer" << std::endl;
             throw InternalError();
         }
         bytes = b;
     }
     PatternPointer(const PatternPointer& ref) {
         data = ref.data;
         bytes = ref.bytes;
     }
     PatternPointer & operator =(const PatternPointer other) {
         data = other.data;
         bytes = other.bytes;
         // by convention, always return *this (for chaining)
         return *this;
     }

     PatternPointer(const PatternPointer&, int,int);
     PatternPointer(const Pattern&, int,int);

     const size_t n() const;
     const size_t bytesize() const { return bytes; }
     const size_t size() const { return n(); }
     
     const PatternCategory category() const;

     std::string tostring(const ClassDecoder& classdecoder) const; //pattern to string (decode)
     std::string decode(const ClassDecoder& classdecoder) const { return tostring(classdecoder); } //pattern to string (decode)
     bool out() const;
     
     bool operator==(const PatternPointer & other) const {
         return ((data == other.data) && (bytes == other.bytes));
     }
     bool operator!=(const PatternPointer & other) const { return !(*this == other); }


     int ngrams(std::vector<PatternPointer> & container, const int n) const; 
     int subngrams(std::vector<PatternPointer> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)
     int ngrams(std::vector<std::pair<PatternPointer,int>> & container, const int n) const; //return multiple ngrams
     int subngrams(std::vector<std::pair<PatternPointer,int>> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)
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
static const Pattern BEGINPATTERN = Pattern((const unsigned char *) tmp_bos,2);
static const Pattern ENDPATTERN = Pattern((const unsigned  char *) tmp_eos,2);
static const Pattern UNKPATTERN = Pattern((const unsigned char* ) tmp_unk,2);

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


unsigned char * inttopatterndata(unsigned char * buffer,unsigned int cls);


#endif
