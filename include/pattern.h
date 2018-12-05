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
#include <climits>
#include "common.h"
#include "classdecoder.h"
#include "SpookyV2.h" //spooky hash
#include "algorithms.h"


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
    SKIPGRAMORFLEXGRAM = 4, /**< Only used as a parameter in querying */
};

enum PatternType {
    PATTERN = 0,
    PATTERNPOINTER = 1,
};

size_t datasize(unsigned char * data, int maxbytes = 0);
PatternCategory datacategory(const unsigned char * data, int maxbytes = 0);
bool dataout(unsigned char * data, int maxbytes = 0);
void readanddiscardpattern(std::istream * in, bool pointerformat = false);
int reader_passmarker(const unsigned char c, std::istream * in);



template<class SizeType=uint32_t, class MaskType=uint32_t>
class PatternPointer;

/**
 * \brief Pattern class, represents a pattern (ngram, skipgram or flexgram).
 * Encoded in a memory-saving fashion. Allows numerous operations.
 */
class Pattern {
    public:
     const static int patterntype = PATTERN;
     unsigned char * data; /**< This array holds the variable-width byte representation, it is always terminated by \0 (ENDMARKER). Though public, you usually do not want to access it directly */

     /**
      * Default/empty Pattern constructor. Creates an empty pattern. Special case, allocates no extra data.
      */
     Pattern() { data = NULL; }

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
     Pattern(const Pattern& ref, size_t begin, size_t length, size_t * byteoffset=NULL, bool byteoffset_shiftone = false);
     template<class SizeType=uint32_t, class MaskType=uint32_t>
     Pattern(const PatternPointer<SizeType,MaskType>& ref,size_t begin, size_t length, size_t * byteoffset=NULL, bool byteoffset_shiftone = false);

     /**
      * Copy constructor for Pattern
      * @param ref Reference pattern
      */
     Pattern(const Pattern& ref);
     template<class SizeType=uint32_t, class MaskType=uint32_t>
     Pattern(const PatternPointer<SizeType,MaskType>& ref);

     /**
      * Read Pattern from input stream (in binary form)
      * @param in The input stream
      * @param ignoreeol Ignore end of line markers and read on until the end of the file, storing corpus data in one pattern
      * @param version Version of file format (default: 2)
      * @param corpusoffset not used
      */
     Pattern(std::istream * in, bool ignoreeol = false, const unsigned char version = 2, const unsigned char * corpusstart = NULL, bool debug = false);
     //Pattern(std::istream * in, unsigned char * buffer, int maxbuffersize, bool ignoreeol = false, const unsigned char version = 2, bool debug = false);


     ~Pattern();


     /**
      * Pattern constructor consisting of only a fixed-size gap
      * @param size The size of the gap
      */
     Pattern(int size) {
         //pattern consisting only of fixed skips
         data = new unsigned char[size+1];
         for (int i = 0; i < size; i++) data[i] = ClassDecoder::skipclass;
         data[size] = ClassDecoder::delimiterclass;
     }

     /**
      * Write Pattern to output stream (in binary form)
      * @param out The output stream
      */
     void write(std::ostream * out, const unsigned char * corpusstart = NULL) const;

     /**
      * return the size of the pattern in tokens (will count flex gaps gaps as size 1)
      */
     size_t n() const;


     /**
      * return the size of the pattern (in bytes), this does not include the
      * final \0 end-marker.
      */
     size_t bytesize() const;

     /**
      * return the size of the pattern in tokens (will count flex gaps gaps as size 1)
      */
     size_t size() const { return n(); }


     /**
      * return the number of skips in this pattern
      */
     unsigned int skipcount() const;

     /**
      * Returns the category of this pattern (value from enum PatternCategory)
      */
     PatternCategory category() const;


     bool isskipgram() const { return category() == SKIPGRAM; }
     bool isflexgram() const { return category() == FLEXGRAM; }
     bool unknown() const;

     /**
      * Return a single token (not a byte!). index < size().
      */
     Pattern operator [](int index) const { return Pattern(*this, index,1); }

     /**
      * Compute a hash value for this pattern
      */
     size_t hash() const;

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
     std::vector<unsigned int> tovector() const;

     bool operator==(const Pattern & other) const;
     bool operator!=(const Pattern & other) const;

     bool operator==(const PatternPointer<> & other) const;
     bool operator!=(const PatternPointer<> & other) const;

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

     template<class SizeType, class MaskType>
     PatternPointer<SizeType,MaskType> Pattern::getpointer() const;

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
     bool instanceof(const PatternPointer<> & skipgram) const;


     /**
      * Adds all patterns (not just ngrams) of size n that are contained within the pattern to
      * container. Does not extract skipgrams that are not directly present in
      * the pattern.
      */
     int ngrams(std::vector<Pattern> & container, const int n) const;

     template<class SizeType=uint32_t, class MaskType=uint32_t>
     int ngrams(std::vector<PatternPointer<SizeType,MaskType>> & container, const int n) const;

     /**
      * Adds all patterns (not just ngrams) of all sizes that are contained within the pattern to
      * container. Does not extract skipgrams that are not directly present in
      * the pattern. Also returns the full ngram itself by default. Set maxn and minn to constrain.
      */
     int subngrams(std::vector<Pattern> & container, int minn = 1, int maxn=99) const; //return all subsumed ngrams (variable n)
     template<class SizeType=uint32_t, class MaskType=uint32_t>
     int subngrams(std::vector<PatternPointer<SizeType,MaskType>> & container, int minn = 1, int maxn=99) const; //return all subsumed ngrams (variable n)

     /**
      * Adds all pairs of all patterns (not just ngrams) of size n that are
      * contained within the pattern, with the token offset at which they were
      * found,  to container.  Does not extract skipgrams that are not directly
      * present in the pattern.
      */
     int ngrams(std::vector<std::pair<Pattern,int>> & container, const int n) const; //return multiple ngrams
     template<class SizeType=uint32_t, class MaskType=uint32_t>
     int ngrams(std::vector<std::pair<PatternPointer<SizeType,MaskType>,int>> & container, const int n) const; //return multiple ngrams

     /**
      * Adds all pairs of all patterns (not just ngrams) that are
      * contained within the pattern, with the token offset at which they were
      * found,  to container.  Does not extract skipgrams that are not directly
      * present in the pattern.
      */
     int subngrams(std::vector<std::pair<Pattern,int>> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)
     template<class SizeType=uint32_t, class MaskType=uint32_t>
     int subngrams(std::vector<std::pair<PatternPointer<SizeType,MaskType>,int>> & container, int minn = 1, int maxn=9) const; //return all subsumed ngrams (variable n)

     /**
      * Finds all the parts of a skipgram, parts are the portions that are not skips and adds them to container... Thus 'to be {*} not {*} be' has three parts
      */
     int parts(std::vector<Pattern> & container) const;
     template<class SizeType=uint32_t, class MaskType=uint32_t>
     int parts(std::vector<PatternPointer<SizeType,MaskType>> & container) const;

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
     Pattern extractskipcontent(const Pattern & instance) const;

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
     Pattern addskip(const std::pair<int,int> & gap) const;
     /**
      * Replaces multiple series of tokens with skips/gaps of particular sizes.  Effectively turns a pattern into a skipgram.
      * @param gaps The positions and sizes of the gaps: a vector of pairs, each pair consisting of a begin index (0-indexed) and a length, indicating where to place the gap
      * @return A skipgram
      */
     Pattern addskips(const std::vector<std::pair<int,int> > & gaps) const;
     /**
      * Replaces multiple series of tokens with skips/gaps of undefined variable size.  Effectively turns a pattern into a flexgram.
      * @param gaps The positions and sizes of the gaps: a vector of pairs, each pair consisting of a begin index (0-indexed) and a length, indicating where to place the gap
      * @return A flexgram
      */
     Pattern addflexgaps(const std::vector<std::pair<int,int> > & gaps) const;

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

     uint32_t getmask() const;

     //sets an entirely new value
     void set(const unsigned char* dataref, const int size);
};


Pattern patternfromfile(const std::string & filename); //helper function to read pattern from file, mostly for Cython

template<class SizeType, class MaskType> //defaults defined earlier
class PatternPointer {
    public:
     const static int patterntype = PATTERNPOINTER;
     unsigned char * data; /** Pointer to Pattern data */
     SizeType bytes; //number of bytes
     MaskType mask; //0 == NGRAM
                    //first bit high = flexgram, right-aligned, 0 = gap
                    //first bit low = skipgram, right-aligned, 0 = gap , max skipgram length 31 tokens

	 PatternPointer<SizeType,MaskType>() {
		data = NULL;
		bytes = 0;
		mask = 0;
     }

     PatternPointer<SizeType,MaskType>(unsigned char* dataref, const bsize_t bytesize) {
         data = dataref;
         if (bytesize > B32) {
             std::cerr << "ERROR: Pattern too long for pattern pointer [" << bytesize << " bytes,explicit]" << std::endl;
             throw InternalError();
         }
         bytes = bytesize;
         mask = computemask();
     }

     PatternPointer<SizeType,MaskType>(const Pattern & ref) {
         data = ref.data;
         const size_t b = ref.bytesize();
         if (b > B32) {
             std::cerr << "ERROR: Pattern too long for pattern pointer [" << b << " bytes,implicit]" << std::endl;
             throw InternalError();
         }
         bytes = b;
         mask = computemask();
     }
     PatternPointer<SizeType,MaskType>(const Pattern * ref) {
         data = ref->data;
         const size_t b = ref->bytesize();
         if (b > B32) {
             std::cerr << "ERROR: Pattern too long for pattern pointer [" << b << " bytes,implicit]" << std::endl;
             throw InternalError();
         }
         bytes = b;
         mask = computemask();
     }
     PatternPointer<SizeType,MaskType>(const PatternPointer& ref) {
         data = ref.data;
         bytes = ref.bytes;
         mask = ref.mask;
     }
     PatternPointer<SizeType,MaskType>(const PatternPointer* ref) {
		data = ref->data;
		bytes = ref->bytes;
		mask = ref->mask;
	 }
     PatternPointer<SizeType,MaskType> & operator =(const PatternPointer<SizeType,MaskType> & other) {
         data = other.data;
         bytes = other.bytes;
         mask = other.mask;
         // by convention, always return *this (for chaining)
         return *this;
     }

     PatternPointer<SizeType,MaskType>(std::istream * in, bool ignoreeol = false, const unsigned char version = 2, unsigned char * corpusstart = NULL, bool debug = false) {
        if (corpusstart == NULL) {
            std::cerr << "ERROR: Can not read PatternPointer, no corpusstart passed!" << std::endl;
            throw InternalError();
        } else {
            SizeType corpusoffset;
            in->read( (char* ) &corpusoffset, sizeof(SizeType));
            data = corpusstart + corpusoffset;
            in->read( (char* ) &bytes, sizeof(SizeType));
            in->read( (char* ) &mask, sizeof(MaskType));
            if (debug) std::cerr << "DEBUG read patternpointer @corpusoffset=" << (size_t) data << " bytes=" << bytes << " mask=" << mask << std::endl;
        }
     }

     /**
      * Write Pattern to output stream (in binary form)
      * @param out The output stream
      */
     void write(std::ostream * out, const unsigned char * corpusstart = NULL) const {
        if (corpusstart != NULL) {
            const SizeType offset = data - corpusstart;
            out->write((char*) &offset, sizeof(SizeType));
            out->write((char*) &bytes, sizeof(SizeType));
            out->write((char*) &mask, sizeof(MaskType));
        } else {
            const int s = bytesize();
            if (s > 0)  out->write( (char*) data , (int) s); //+1 to include the \0 marker
            const unsigned char null = 0;
            out->write( (char*) &null , (int) 1);  //marker
        }
     }

     //slice construtors:
     PatternPointer<SizeType,MaskType>(unsigned char * ref, SizeType begin,SizeType length, SizeType * byteoffset=NULL, bool byteoffset_shiftone = false) {
        //to be computed in bytes
        SizeType begin_b = (byteoffset != NULL) ? *byteoffset : 0;
        SizeType length_b = 0;
        bool prevhigh = false;

        SizeType i = (byteoffset != NULL) ? *byteoffset : 0;
        SizeType n = 0;
        unsigned char c;
        do {
            c = ref[i];
            if (c < 128) {
                //we have a token
                n++;
                if ((n == 1) && (byteoffset_shiftone) && (byteoffset != NULL)) *byteoffset = i+1;
                if (n - begin == length) {
                    length_b = (i + 1) - begin_b;
                    break;
                } else if ((c == ClassDecoder::delimiterclass) && (!prevhigh) ) {
                    length_b = i - begin_b;
                    break;
                }
                i++;
                if (n ==  begin) begin_b = i;
                prevhigh = false;
            } else {
                prevhigh = true;
                i++;
            }
        } while (1);
        if ((byteoffset != NULL) && (!byteoffset_shiftone)) *byteoffset = i+1;

        data = ref + begin_b;
        bytes = length_b;
        mask = computemask();
     }

     template<class SizeType2, class MaskType2>
     PatternPointer<SizeType,MaskType>(const PatternPointer<SizeType2,MaskType2>& ref, SizeType begin,SizeType length, SizeType * byteoffset=NULL, bool byteoffset_shiftone = false) {
        //to be computed in bytes
        SizeType2 begin_b = (byteoffset != NULL) ? *byteoffset : 0;
        SizeType2 length_b = 0;
        bool prevhigh = false;

        SizeType2 i = (byteoffset != NULL) ? *byteoffset : 0;
        SizeType2 n = 0;
        unsigned char c;
        do {
            if (i == ref.bytes) {
                length_b = i - begin_b;
                break;
            }
            c = ref.data[i];

            if (c < 128) {
                //we have a token
                n++;
                if ((n == 1) && (byteoffset_shiftone) && (byteoffset != NULL)) *byteoffset = i+1;
                if (n - begin == length) {
                    length_b = (i + 1) - begin_b;
                    break;
                } else if ((c == ClassDecoder::delimiterclass) && (!prevhigh) ) {
                    length_b = i - begin_b;
                    break;
                }
                i++;
                if (n == begin) begin_b = i;
                prevhigh = false;
            } else {
                prevhigh = true;
                i++;
            }
        } while (1);
        if ((byteoffset != NULL) && (!byteoffset_shiftone)) *byteoffset = i+1;

        data = ref.data + begin_b;
        bytes = length_b;
        mask = computemask();

        /*std::cerr << "Created patternpointer: b=" << bytes << " n=" << this->n() << " (begin="<<begin<<",length="<<length<<")" <<endl;
        this->out();
        std::cerr << std::endl;*/
     }

     PatternPointer<SizeType,MaskType>(const Pattern& ref, size_t begin, size_t length, size_t * byteoffset, bool byteoffset_shiftbyone) { //slice constructor
        //to be computed in bytes
        size_t begin_b = (byteoffset != NULL) ? *byteoffset : 0;
        size_t length_b = 0;
        bool prevhigh = false;

        size_t i = (byteoffset != NULL) ? *byteoffset : 0;
        size_t n = 0;
        unsigned char c;
        do {
            c = ref.data[i];
            if (c < 128) {
                //we have a token
                n++;
                if ((n == 1) && (byteoffset_shiftbyone) && (byteoffset != NULL)) *byteoffset = i+1;
                if (n - begin == length) {
                    length_b = (i + 1) - begin_b;
                    break;
                } else if ((c == ClassDecoder::delimiterclass) && (!prevhigh) ) {
                    length_b = i - begin_b;
                    break;
                }
                i++;
                if (n == begin) begin_b = i;
                prevhigh = false;
            } else {
                prevhigh = true;
                i++;
            }
        } while (1);
        if ((byteoffset != NULL) && (!byteoffset_shiftbyone)) *byteoffset = i+1;

        data = ref.data + begin_b;
        /*if (length_b >= B32) {
            std::cerr << "ERROR: Pattern too long for pattern pointer [length_b=" << length_b << ",begin=" << begin << ",length=" << length << ", reference_length_b=" << ref.bytesize() << "]  (did you set MAXLENGTH (-l)?)" << std::endl;
            std::cerr << "Reference=";
            ref.out();
            std::cerr << std::endl;
            throw InternalError();
        }*/
        bytes = length_b;
        mask = computemask();
     }

     MaskType computemask() const {
        MaskType mask = 0;
        MaskType n = 0;
        bool isflex = false;
        for (MaskType i = 0; (i < bytes) && (n < sizeof(MaskType) - 1); i++) {
            if (data[i] < 128) {
                if ((i == 0) || (data[i-1] < 128)) {
                    if (data[i] == ClassDecoder::flexclass){
                        isflex = true;
                        mask |= (1 << n);
                    } else if ((data[i] == ClassDecoder::skipclass)) {
                        mask |= (1 << n);
                    }
                }
                n++;
            }
        }
        if (isflex) mask |= (1 << 31);
        return mask;
     }

     MaskType getmask() const { return mask; }

     size_t n() const {
        return datasize(data, bytes);
     }
     SizeType bytesize() const { return bytes; }
     size_t size() const { return n(); }

     /**
      * Compute a hash value for this pattern
      */
     size_t hash() const {
        if ((data == NULL) || (data[0] == 0)) return 0;
        if (mask == 0) {
            return SpookyHash::Hash64((const void*) data , bytesize());
        } else if (isflexgram()) {
            //hashing skipgrams/flexgrams is a bit more expensive cause we need to process the mask before we can compute the hash:
            unsigned char datacopy[bytes+1];
            int size = flexcollapse(datacopy);
            datacopy[size] = ClassDecoder::delimiterclass;
            return SpookyHash::Hash64((const void*) datacopy , size+1);
        } else {
            //hashing skipgrams/flexgrams is a bit more expensive cause we need to process the mask before we can compute the hash:
            unsigned char datacopy[bytes+1];
            memcpy(datacopy, data, bytes);
            datacopy[bytes] = ClassDecoder::delimiterclass;
            unsigned int n = 0;
            for (SizeType i = 0; i < bytes; i++) {
                if (datacopy[i] < 128) {
                    if (isgap(n)) datacopy[i] = ClassDecoder::skipclass;
                    n++;
                }
            }
            return SpookyHash::Hash64((const void*) datacopy , bytes);
        }
     }

     PatternCategory category() const {
        if (mask == 0) {
            return datacategory(data, bytes);
        } else if (mask > B32 / 2) {
            return FLEXGRAM;
        } else {
            return SKIPGRAM;
        }
     }
     bool isskipgram() const { return category() == SKIPGRAM; }
     bool isflexgram() const { return category() == FLEXGRAM; }
     bool unknown() const {
        if (data == NULL) return false;
        SizeType i = 0;
        bool prevhigh = false;
        do {
            if ((bytes > 0) && (i >= bytes)) {
                return false;
            }
            if ((!prevhigh) && (data[i] == ClassDecoder::unknownclass)) {
                return true;
            }
            prevhigh = (data[i] >= 128);
            i++;
        } while (1);
     }

     std::string tostring(const ClassDecoder& classdecoder) const {
         //pattern to string (decode)
        std::string result = "";
        SizeType i = 0;
        SizeType n =0;
        SizeType length;
        unsigned int cls;
        bool flex = false;
        if (mask != 0) flex = isflexgram();
        do {
            if ((bytes > 0) && (i >= bytes)) {
                return result;
            }
            cls = bytestoint(data + i, &length);
            if ((mask != 0) && (isgap(n)))  {
                if (flex) {
                    cls = ClassDecoder::flexclass;
                } else {
                    cls = ClassDecoder::skipclass;
                }
            }
            n++;
            if (length == 0) {
                std::cerr << "ERROR: Class length==0, shouldn't happen" << std::endl;
                throw InternalError();
            }
            i += length;
            if ((bytes == 0) && (cls == ClassDecoder::delimiterclass)) {
                return result;
            } else if (classdecoder.hasclass(cls)) {
                if (!result.empty()) result += " ";
                result += classdecoder[cls];
            } else {
                if (!result.empty()) result += " ";
                result += "{?}";
            }
        } while (1);
        return result;
     }
     std::string decode(const ClassDecoder& classdecoder) const { return tostring(classdecoder); } //pattern to string (decode)
     bool out() const {
        return dataout(data, bytesize());
     }

     template<class SizeType2=SizeType, class MaskType2=MaskType>
     bool operator==(const PatternPointer<SizeType2,MaskType2> & other) const {
        if (bytes == other.bytes) {
            if ((mask != 0) && (isflexgram())) {
                if ((other.mask == 0) || (!other.isflexgram())) return false;
                unsigned char data1[bytes];
                int size1 = flexcollapse(data1);
                unsigned char data2[other.bytesize()];
                int size2 = other.flexcollapse(data2);
                if (size1 != size2) return false;
                return (memcmp(data1,data2,size1) == 0);
            } else if (mask != other.mask) {
                return false;
            } else {
                if (data == other.data) return true; //shortcut
                size_t i = 0;
                size_t n = 0;
                while (i<bytes) {
                    if ((mask != 0) && (data[i] < 128))  {
                        if (isgap(n)) {
                            if (!other.isgap(n)) return false;
                        } else if (data[i] != other.data[i]) return false;
                        n++;
                    } else if (data[i] != other.data[i]) return false;
                    i++;
                }
                return true;
            }
        }
        return false;
     }

     template<class SizeType2=SizeType, class MaskType2=MaskType>
     bool operator!=(const PatternPointer<SizeType2,MaskType2> & other) const { return !(*this == other); }

     bool operator==(const Pattern & other) const {
        if ((data == NULL) || (data[0] == 0)) {
            return (other.data == NULL || other.data[0] == 0);
        } else if ((other.data == NULL) || (other.data[0] == 0)) {
            return false;
        }
        SizeType i = 0;
        SizeType n = 0;
        if ((mask != 0) && (isflexgram())) {
            if (!other.isflexgram()) return false;
            unsigned char data1[bytesize()];
            const SizeType size1 = flexcollapse(data1);
            if (size1 != other.bytesize()) return false;
            return (memcmp(data1,other.data,size1) == 0);
        }
        while (i<bytes){
            if ((i>0) && (other.data[i-1] >= 128) && (other.data[i] == 0)) return false;
            if ((mask != 0) && (data[i] < 128))  {
                if (isgap(n)) {
                    if (other.data[i] != ClassDecoder::skipclass)  return false;
                } else if (data[i] != other.data[i]) return false;
                n++;
            } else if (data[i] != other.data[i]) return false;
            i++;
        }
        return other.data[i] == ClassDecoder::delimiterclass;
     }

     bool operator!=(const Pattern & other) const { return !(*this == other); }

     /**
      * Return a single token (not a byte!). index < size().
      */
     PatternPointer<SizeType,MaskType> operator [](SizeType index) const {
         return PatternPointer<unsigned char,unsigned char>(*this, index,1);
     }

     PatternPointer<SizeType,MaskType> toflexgram() const {
        PatternPointer copy = *this;
        if (mask != 0) copy.mask = mask | (1<<31);
        return copy;
     }

     bool isgap(int index) const {
        if ((mask == 0) || (index > 30)) return false;
        return (mask & bitmask[index]);
     }


	 /**
	  * Return a new patternpointer one token to the right, maintaining the same token length and same skip configuration (if any).
	  * Note that this will cause segmentation faults if the new PatternPointer exceeds the original data!!!
	  * It's up to the caller to check this!
	  */
	 PatternPointer<SizeType,MaskType>& operator++() {
        const size_t _n = n();
        unsigned char * cursor  = data;
        unsigned char * newdata  = NULL;
        size_t newn = 0;
        do {
            if (*cursor < 128) {
                if (newdata == NULL) {
                    newdata = cursor + 1;
                } else {
                    newn++;
                }
            }
            cursor++;
        } while (newn < _n);
        data = newdata;
        bytes = cursor-newdata;
        return *this;
     }

     template<class SizeType2, class MaskType2>
     bool operator<(const PatternPointer<SizeType2,MaskType2> & other) const {
         if (data == other.data) {
             if (bytes == other.bytes) {
                 return mask < other.mask;
             } else {
                 return bytes < other.bytes;
             }
         } else {
            return data < other.data;
        }
     }

     template<class SizeType2, class MaskType2>
     int ngrams(std::vector<PatternPointer<SizeType2,MaskType2>> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
        const SizeType _n = this->n();
        if (n > _n) return 0;
        int found = 0;
        size_t byteoffset = 0;
        for (int i = 0; i < (_n - n) + 1; i++) {
            container.push_back(PatternPointer<SizeType2,MaskType2>(*this,0,n,&byteoffset,true));
            found++;
        }
        return found;
     }

     template<class SizeType2, class MaskType2>
     int subngrams(std::vector<PatternPointer<SizeType2,MaskType2>> & container, int minn = 1, int maxn=9) const { //return all subsumed ngrams (variable n)
        const SizeType _n = n();
        if (maxn > _n) maxn = _n;
        if (minn > _n) return 0;
        int found = 0;
        for (int i = minn; i <= maxn; i++) {
            found += ngrams(container, i);
        }
        return found;
     }

     template<class SizeType2, class MaskType2>
     int ngrams(std::vector<std::pair<PatternPointer<SizeType2,MaskType2>,int>> & container, const int n) const { //return multiple ngrams
        const SizeType _n = this->n();
        if (n > _n) return 0;

        int found = 0;
        size_t byteoffset = 0;
        for (int i = 0; i < (_n - n)+1; i++) {
            container.push_back( std::pair<PatternPointer<SizeType2,MaskType2>,int>(PatternPointer<SizeType2,MaskType2>(*this,0,n,&byteoffset,true),i) );
            found++;
        }
        return found;
     }

     template<class SizeType2, class MaskType2>
     int subngrams(std::vector<std::pair<PatternPointer<SizeType2,MaskType2>,int>> & container, int minn = 1, int maxn=9) const { //return all subsumed ngrams (variable n)
        const int _n = n();
        if (maxn > _n) maxn = _n;
        if (minn > _n) return 0;
        int found = 0;
        for (int i = minn; i <= maxn; i++) {
            found += ngrams(container, i);
        }
        return found;
     }

     /**
      * Finds all the parts of a skipgram, parts are the portions that are not skips and adds them to container... Thus 'to be {*} not {*} be' has three parts
      */
     template<class SizeType2, class MaskType2>
     int parts(std::vector<PatternPointer<SizeType2,MaskType2>> & container) const {
        if (data == NULL) return 0;
        int partbegin = 0;
        int partlength = 0;

        int found = 0;
        size_t n = 0;
        for (size_t i = 0; (i<bytes) && (i<sizeof(SizeType)-1); i++) {
            const unsigned char c = data[i];
            if (c < 128) {
                if (isgap(n)) {
                    partlength = n - partbegin;
                    if (partlength > 0) {
                        container.push_back(PatternPointer<SizeType2,MaskType2>(*this,partbegin,partlength));
                        found++;
                    }
                    partbegin = n+1;
                }
                //low byte, end of token
                n++;
            }
        }
        partlength = n - partbegin;
        if (partlength > 0) {
            container.push_back(PatternPointer<SizeType2,MaskType2>(*this,partbegin,partlength));
            found++;
        }
        return found;
     }

     /**
      * Finds all the parts of a skipgram, parts are the portions that are not skips and adds them to container as begin,length pairs... Thus 'to be {*} not {*} be' has three parts
      */
     int parts(std::vector<std::pair<int,int> > & container) const {
        if (data == NULL) return 0;
        int partbegin = 0;
        int partlength = 0;

        int found = 0;
        size_t n = 0;
        for (size_t i = 0; (i<bytes) && (i<31); i++) {
            const unsigned char c = data[i];
            if (c < 128) {
                //low byte, end of token
                if ((mask & bitmask[n]) == 0) {
                    partlength = n - partbegin;
                    if (partlength > 0) {
                        container.push_back(std::pair<int,int>(partbegin,partlength));
                        found++;
                    }
                    n++;
                    partbegin = n;
                } else {
                    n++;
                }
            } else {
                //high byte
            }
        }
        partlength = n - partbegin;
        if (partlength > 0) {
            container.push_back(std::pair<int,int>(partbegin,partlength));
            found++;
        }
        return found;
     }

     /**
      * Finds all the gaps of a skipgram or flexgram., parts are the portions that are not skips and adds them to container as begin,length pairs... Thus 'to be {*} not {*} be' has three parts. The gap-length of a flexgram will always be its minimum length one.
      */
     int gaps(std::vector<std::pair<int,int> > & container) const {
        if (data == NULL) return 0;
        if (mask == 0) return 0;
        int i = 0;
        int n = 0;
        int beginskip = -1;
        int skiplength = 0;
        do {
            if (data[i] < 128) {
                if (isgap(n)) {
                    if (beginskip > 0) {
                        skiplength++;
                    } else {
                        beginskip = i;
                        skiplength = 1;
                    }
                } else {
                    if (beginskip > -1) container.push_back(std::pair<int,int>(beginskip,skiplength));
                    beginskip = -1;
                }
                n++;
            }
            i++;
        } while ((size_t) i < bytes);
        if (beginskip > -1) container.push_back(std::pair<int,int>(beginskip,skiplength));
        return container.size();
     }

     /**
      * return the number of skips in this pattern
      */
     unsigned int skipcount() const {
        if (data == NULL) return 0;
        if (mask == 0) return 0;
        unsigned int skipcount = 0;
        size_t i = 0;
        size_t n = 0;
        bool prevskip = false;
        do {
            if (data[i] < 128) {
                if (isgap(n)) {
                    if (!prevskip) skipcount++;
                    prevskip = true;
                } else {
                    prevskip = false;
                }
                n++;
            }
            i++;
        } while (i < bytes);
        return skipcount;
     }

     /**
      * Replaces a series of tokens with a skip/gap of a particular size.
      * Effectively turns a pattern into a skipgram.
      * @param gap The position and size of the skip/gap: a pair consisting of a begin index (0-indexed) and a length, i.e. the size of the skip
      */
     PatternPointer<SizeType,MaskType> addskip(const std::pair<int,int> & gap) const {
        PatternPointer<SizeType,MaskType> copy = *this;
        for (int i = gap.first; i < (gap.first + gap.second) && (i < 31); i++ ) {
            copy.mask |= bitmask[i];
        }
        return copy;
     }

     /**
      * Replaces multiple series of tokens with skips/gaps of particular sizes.  Effectively turns a pattern into a skipgram.
      * @param gaps The positions and sizes of the gaps: a vector of pairs, each pair consisting of a begin index (0-indexed) and a length, indicating where to place the gap
      * @return A skipgram
      */
     PatternPointer<SizeType,MaskType> addskips(const std::vector<std::pair<int,int> > & gaps) const {
        //Returns a patternpointer with the specified spans replaced by fixed skips
        PatternPointer<SizeType,MaskType> copy = *this;
        copy.mask = vector2mask(gaps);
        return copy;
     }

	 /**
	  * Low-level function for flexgrams, that returns a collapsed comparable representation of the flexgram in collapseddata (has to be pre-allocated). Return value is the number of bytes of the representation. In the collapsed representation adjacent flexgrams are removed.
	  */
	 int flexcollapse(unsigned char * collapseddata) const {
        //collapse data
        bool prevgap = false;
        SizeType j = 0;
        SizeType n = 0;
        for (SizeType i = 0; i < bytes; i++) {
            if (data[i] < 128) {
                if (isgap(n)) {
                    if (!prevgap) {
                        collapseddata[j++] = ClassDecoder::flexclass;
                        prevgap = true;
                    }
                } else {
                    collapseddata[j++] = data[i];
                    prevgap = false;
                }
                n++;
            } else {
                collapseddata[j++] = data[i];
            }
        }
        return j;
     }

     template<class SizeType2, class MaskType2>
     bool instanceof(const PatternPointer<SizeType2,MaskType2> & skipgram) const {
        //Is this an instantiation of the skipgram?
        //Instantiation is not necessarily full, aka: A ? B C is also an instantiation
        //of A ? ? C
        if (this->category() == FLEXGRAM) return false;
        if (skipgram.category() == NGRAM) return (*this) == skipgram;

        if (skipgram.category() == FLEXGRAM) {
            //TODO: Implement flexgram support!!!
           return false;
        } else {
            //FIXED SKIPGRAM
            const size_t _n = n();
            if (skipgram.n() != _n) return false;

            for (size_t i = 0; i < _n; i++) {
                const PatternPointer reftoken = PatternPointer(skipgram, i, 1);
                const PatternPointer token = PatternPointer(*this, i, 1);
                if ((token != reftoken) && (reftoken.category() != SKIPGRAM)) return false;
            }
            return true;
        }
     }

     operator Pattern() { return Pattern(*this); } //cast overload
     Pattern pattern() const { return Pattern(*this); } //cast overload
};


static const unsigned char * tmp_unk = (const unsigned char *) "\2";
static const unsigned char * tmp_boundarymarker = (const unsigned char *) "\1";
static const unsigned char * tmp_skipmarker = (const unsigned char *) "\3";
static const unsigned char * tmp_flexmarker = (const unsigned char *) "\4";
const Pattern BOUNDARYPATTERN = Pattern((const unsigned char *) tmp_boundarymarker,1);
const Pattern SKIPPATTERN = Pattern((const unsigned char *) tmp_skipmarker,1);
const Pattern FLEXPATTERN = Pattern((const unsigned char*) tmp_flexmarker,1);
static const Pattern UNKPATTERN = Pattern((const unsigned char* ) tmp_unk,1);

namespace std {

    template <>
    struct hash<Pattern> {
     public:
          size_t operator()(const Pattern &pattern) const throw() {
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

    template<class SizeType, class MaskType>
    struct hash<PatternPointer<SizeType,MaskType>> {
     public:
          size_t operator()(const PatternPointer<SizeType,MaskType> &pattern) const throw() {
              return pattern.hash();
          }
    };



    template<class SizeType, class MaskType>
    struct hash<const PatternPointer<SizeType,MaskType> *> {
     public:
          size_t operator()(const PatternPointer<SizeType,MaskType> * pattern) const throw() {
              return pattern->hash();
          }
    };

}

#endif
