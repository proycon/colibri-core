#ifndef COLIBRIPATTERNSTORE_H
#define COLIBRIPATTERNSTORE_H

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
#include "pattern.h"
#include "datatypes.h"
#include "classdecoder.h"
#include "classencoder.h"

/***********************************************************************************/

/**
 * @file patternstore.h
 * \brief Contains lower-level containers for patterns.
 *
 * @author Maarten van Gompel (proycon) <proycon@anaproy.nl>
 *
 * @section LICENSE
 * Licensed under GPLv3
 *
 * @section DESCRIPTION
 * Contains lower-level containers for patterns
 */


typedef std::pair<IndexReference,PatternPointer> IndexPattern;
/**
 * \brief Class for reading an entire (class encoded) corpus into memory.
 * It provides a reverse index by IndexReference. The reverse index stores positions and unigrams.
 */
class IndexedCorpus {
    protected:
        unsigned char * corpus;
        unsigned int corpussize; //in bytes
		PatternPointer * patternpointer; //pattern pointer covering the whole corpus
        unsigned int totaltokens;
        std::map<uint32_t,unsigned char*> sentenceindex; //sentence pointers
    private:
        IndexedCorpus(IndexedCorpus& ) {};
    public:
        IndexedCorpus() {
            corpus = NULL;
            corpussize = 0;
		    totaltokens = 0; //will be computed when queried
			patternpointer = NULL;
        };

        /*
         * Low-level constructor
         */
        IndexedCorpus(unsigned char * corpus, unsigned int corpussize) {
            this->corpus = corpus;
            this->corpussize = 0;
            totaltokens = 0; //will be computed when queried
            patternpointer = new PatternPointer(corpus,corpussize);
        }

        /*
         * Read an indexed corpus from stream. The stream must correspond to an
         * encoded corpus (*.colibri.dat)
         */
        IndexedCorpus(std::istream *in, bool debug = false);
        /*
         * Read an indexed corpus from file. The filename must correspond to an
         * encoded corpus (*.colibri.dat)
         */
        IndexedCorpus(std::string filename, bool debug = false);

        ~IndexedCorpus() {
			if (corpus != NULL) delete[] corpus;
			if (patternpointer != NULL) delete patternpointer;
            corpus = NULL;
            patternpointer = NULL;
		}


        /*
         * Read an indexed corpus from stream. The stream must correspond to an
         * encoded corpus (*.colibri.dat)
         */
        void load(std::istream *in, bool debug = false);

        /*
         * Read an indexed corpus from file. The filename must correspond to an
         * encoded corpus (*.colibri.dat)
         */
        void load(std::string filename, bool debug = false);



        /**
         * Low-level function, returns a data pointer given an IndexReference.
         * Returns NULL when the index does not exist.
         * Use getpattern() instead.
         */
        unsigned char * getpointer(const IndexReference & begin) const;

        /**
         * Returns a pattern starting at the provided position and of the
         * specified length.
         */
        PatternPointer getpattern(const IndexReference & begin, int length=1) const;


        PatternPointer getpattern() const {
            return *patternpointer;
        }
        unsigned char * beginpointer() const {
            return corpus;
        }
        unsigned int bytesize() const {
            return corpussize;
        }

        /**
         * Get the sentence (or whatever other unit your data employs)
         * specified by the given index. Sentences start at 1.
         */
        PatternPointer getsentence(int sentence) const; //returns sentence as a pattern pointer
        PatternPointer getsentence(unsigned char * sentencedata) const; //returns sentence as a pattern pointer

        /**
         * Returns all positions at which the pattern occurs. Up to a certain
         * number of maximum matches if desired. Note that this iterates over
         * the entire corpus and is by far not as efficient as a proper pattern
         * model.
         * WARNING: Skipgrams and flexgrams are limited to a maximum length 31 words (including gaps)
         * @param pattern The pattern to find (may be ngram, skipgram or flexgram)
         * @param sentence Restrict to a particular sentence (0=all sentences, default)
         * @param instantiate Instantiate skipgrams and flexgrams, i.e. return ngrams instead (default : false)
         */
        std::vector<std::pair<IndexReference,PatternPointer>> findpattern(const Pattern pattern, uint32_t sentence=0, bool instantiate=false);

        /**
         *  Middle level function, for specific sentence, call higher-level version instead.
         */
        void findpattern(std::vector<std::pair<IndexReference,PatternPointer>> & result, const Pattern & pattern,  uint32_t sentence, bool instantiate=false);

        /**
        * Low-level method to find a pattern at the predetermined position. Can also instantiate skipgrams/flexgrams by setting resultcategory to NGRAM (default!). Raises a KeyError when the pattern was not found at the specified position
        * getskipgram() and getflexgram() are higher level methods that use this one.
        */
        PatternPointer findpattern(const IndexReference & begin, const Pattern & pattern, PatternCategory resultcategory = NGRAM) const;

        /**
         * Alias for low-level findpattern() method
         */
        PatternPointer getinstance(const IndexReference & begin, const Pattern & pattern, PatternCategory resultcategory = NGRAM) const { return findpattern(begin, pattern, resultcategory); } //alias, to be consistent with Cython names

        /**
         * Returns a valid patternpointer for the flexgram at the indicated
         * position. Effectively removing a level of abstraction. If the
         * flexgram can not be found, a KeyError will be raised.
         * This is a specialised alias for the low-level findpattern() method
         */
        PatternPointer getflexgram(const IndexReference & begin, const Pattern flexgram) const;

        /**
         * Returns a valid patternpointer for the skipgram at the indicated
         * position. If the skipgram can not be found, a KeyError will be raised.
         * This is a specialised alias for the low-level findpattern() method
         */
        PatternPointer getskipgram(const IndexReference & begin, const Pattern skipgram) const;

        /**
         * Returns the length of the sentence (or whatever other unit your data
         * employs) at the given sentence index (starts at 1)
         */
        int sentencelength(int sentence) const;
        int sentencelength(unsigned char * sentencebegin) const;

        /**
         * Return the total number of sentences (or whatever other unit
         * delimites your data) in the corpus.
         */
        unsigned int sentences() const { return sentenceindex.size(); }  //returns the number of sentences (1-indexed)


		/**
		* Iterator
		*/
        class iterator {
            public:
                typedef iterator self_type;
                typedef IndexPattern value_type;
                typedef IndexPattern & reference;
                typedef IndexPattern * pointer;
                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;

                iterator(const self_type & ref) { //copy constructor
					pairpointer = new std::pair<IndexReference,PatternPointer>(*ref.pairpointer);
                    indexedcorpus = ref.indexedcorpus;
                }

                iterator(IndexedCorpus * indexedcorpus, pointer ptr) {
                    if (ptr != NULL) {
                        pairpointer = new std::pair<IndexReference,PatternPointer>(*ptr);
                    } else {
                        pairpointer = NULL;
                    }
                    this->indexedcorpus = indexedcorpus;
                }
                iterator(IndexedCorpus * indexedcorpus, IndexReference iref, PatternPointer pp) {
					pairpointer = new std::pair<IndexReference,PatternPointer>(iref, pp);
                    this->indexedcorpus = indexedcorpus;
				}
                iterator(IndexedCorpus * indexedcorpus, reference ref) {
					pairpointer = new std::pair<IndexReference,PatternPointer>(ref.first, ref.second);
                    this->indexedcorpus = indexedcorpus;
				}
                iterator() { //default constructor, required for cython
                    pairpointer = NULL;
                    indexedcorpus = NULL;
                }
   				~iterator() {
					if (pairpointer != NULL) delete pairpointer;
				}
				self_type operator++() {
					next();
					return *this;
				} //prefix

                self_type & operator=(const self_type & ref) {
					if (pairpointer != NULL) delete pairpointer;
                    if (ref.pairpointer != NULL) {
                        pairpointer = new std::pair<IndexReference,PatternPointer>(*ref.pairpointer);
                    } else {
                        pairpointer = NULL;
                    }
                    indexedcorpus = ref.indexedcorpus;
                    return *this;
                }

				void next() {
                    if ((pairpointer != NULL) && (pairpointer->second.data != NULL)) {
                        if ((indexedcorpus == NULL) || (indexedcorpus->corpus == NULL))  {
                            std::cerr << "ERROR: No indexedcorpus associated with iterator" << std::endl;
                            throw InternalError();
                        }
                        if (pairpointer->second.data + pairpointer->second.bytes >= indexedcorpus->corpus + indexedcorpus->corpussize) {
                            pairpointer->first.sentence++;
                            pairpointer->first.token = 0;
                            pairpointer->second = PatternPointer(); //null pointer
                            return;
                        } else {
                            ++(pairpointer->second);
                        }
                        if (*(pairpointer->second.data) == ClassDecoder::delimiterclass) {
                            //we never stop at delimiterclasses, iterate again:
                            pairpointer->first.sentence++;
                            pairpointer->first.token = 0;
                            if (pairpointer->second.data + pairpointer->second.bytes < indexedcorpus->corpus + indexedcorpus->corpussize) {
                                ++(pairpointer->second);
                            } else {
                                pairpointer->second = PatternPointer(); //null pointer
                            }
                        } else {
                            pairpointer->first.token++;
                        }
                        //Note: At the end of the data, the patternpointer points to NULL, checking against end() should work fine though
                    }
				}



                self_type operator++(int) { self_type tmpiter = *this; next(); return tmpiter; } //postfix

                //reference operator*() { return *pairpointer; }
                //pointer operator->()  { return pairpointer; }
                IndexReference & index() { return pairpointer->first; }
                PatternPointer & pattern() { return pairpointer->second; }
                PatternPointer & patternpointer() { return pairpointer->second; }
                IndexedCorpus * corpus() { return indexedcorpus; }

                bool operator==(self_type rhs) {
                    if ((pairpointer == NULL) || (pairpointer->second.data == NULL)) {
                        return ((rhs.pairpointer == NULL) || (rhs.pairpointer->second.data == NULL));
                    } else if ((rhs.pairpointer == NULL) || (rhs.pairpointer->second.data == NULL)) {
                        return ((pairpointer == NULL) || (pairpointer->second.data == NULL));
                    } else {
                        return pairpointer->first == rhs.index();
                    }
                }
                bool operator!=(self_type rhs) { return !(*this == rhs); }

                void debug() {
                    std::cerr << (size_t) pairpointer << std::endl;
                }

                pointer pairpointer;
                IndexedCorpus * indexedcorpus;
        };

        /*
         * Returns the begin iterator over the corpus
         */
        iterator begin() {
			IndexReference iref = IndexReference(1,0);
			PatternPointer p = getpattern(iref,1);
			return iterator(this,iref,p);
		}
        //const_iterator begin() const { return data.begin(); }

        /*
         * Returns the end iterator of the corpus
         */
        iterator end() {
			IndexReference iref = IndexReference(sentences() + 1,0);
            return iterator(this,iref, PatternPointer());
		}
        //const_iterator end() const { return data.end(); }

        /**
         * Returns an iterator starting at the given position. Correspond to
         * end() when no such position is found.
         */
        iterator find(const IndexReference & ref) {
			try {
				PatternPointer p = getpattern(ref);
				return iterator(this, ref,p);
			} catch (KeyError &e) {
				return end();
			}
        }
        /**
         * Returns a const iterator starting at the given position. Correspond to
         * end() when no such position is found.
         */
        /*const_iterator find(const IndexReference & ref) const {
            return std::lower_bound(this->begin(), this->end(), IndexPattern(ref) ); //does binary search
        }*/

        /**
         * Does the provided position occur in the corpus?
         */
        bool has(const IndexReference & ref) const {
			return (getpointer(ref) != NULL);
        }

        /**
         * Returns the number of tokens in the corpus
         */
        size_t size() {
			if (totaltokens > 0) return totaltokens;
            for (iterator iter = begin(); iter != end(); iter++) totaltokens++;
			return totaltokens;
		}

        /**
         * Is the corpus empty?
         */
        bool empty() const { return (corpussize <= 1); }


        /**
         * Returns the token at the provided position. The token is returned as
         * an integer corresponding to the class in a particular class
         * encoding. Use getpattern() if you want a Pattern instance.
         * @see getpattern
         */
        unsigned int operator [](const IndexReference & ref) {
			try {
				PatternPointer pp = getpattern(ref);
				return bytestoint(pp.data);
			} catch (KeyError &e) {
				throw e;
			}
        }




};


/************* Base abstract container for pattern storage  ********************/



/**
 * \brief Limited virtual interface to pattern stores.
 */
class PatternStoreInterface {
    public:
        /**
         * Does the pattern occur in the pattern store?
         */
        virtual bool has(const Pattern &) const =0;
        /**
         * Does the pattern occur in the pattern store?
         */
        virtual bool has(const PatternPointer &) const =0;
        /**
         * How many patterns are in the pattern store?
         */
        virtual size_t size() const =0;



};


/**
 * \brief Abstract Pattern store class, not to be instantiated directly.
 *
 * This is an abstract class, all Pattern storage containers are derived from
 * this.
 * @tparam ContainerType The low-level container type used (an STL container such as set/map).
 * @tparam ReadWriteSizeType Data type for addressing, influences only the maximum number of items that can be stored (2**64) in the container, as this will be represented in the very beginning of the binary file. No reason to change this unless the container is very deeply nested in others and contains only few items.
 */
template<class ContainerType,class ReadWriteSizeType = uint64_t,class PatternType = Pattern>
class PatternStore: public PatternStoreInterface {
    protected:
        unsigned char * corpusstart; //used only when PatternType=PatternPointer
        unsigned int corpussize;
        unsigned char classencodingversion;
        int patterntype;
    public:
        PatternStore<ContainerType,ReadWriteSizeType,PatternType>() {corpusstart = NULL; corpussize = 0; classencodingversion = 2; patterntype = PatternType::patterntype; };
        virtual ~PatternStore<ContainerType,ReadWriteSizeType,PatternType>() {};

        virtual void attachcorpus(unsigned char * corpusstart, unsigned int corpussize) {
            this->corpusstart = corpusstart;
            this->corpussize = corpussize;
        }
        virtual void attachcorpus(const IndexedCorpus & corpus) {
            this->corpusstart = corpus.beginpointer();
            this->corpussize = corpus.bytesize();
        }
        virtual void detachcorpus() {
            this->corpusstart = NULL;
            this->corpussize = 0;
        }
        unsigned char * getcorpus() const {
            return corpusstart;
        }
        unsigned int getcorpussize() const {
            return corpussize;
        }


        /**
         * Set this to read patterns using the v1 classencoding (pre Colibri Core v2). Only for reading, write actions will always use the newest version.
         */
        virtual void use_v1_format() { this->classencodingversion = 1; }

        virtual void insert(const PatternType & pattern)=0; //might be a noop in some implementations that require a value

        virtual bool has(const Pattern &) const =0;
        virtual bool has(const PatternPointer &) const =0;

        virtual bool erase(const PatternType &) =0;

        virtual size_t size() const =0;
        virtual void reserve(size_t) =0; //might be a noop in some implementations


        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find(const Pattern & pattern)=0;
        virtual typename ContainerType::iterator find(const PatternPointer & pattern)=0;


        virtual void write(std::ostream * out)=0;
        //virtual void read(std::istream * in, int MINTOKENS)=0;

        virtual PatternStoreInterface * getstoreinterface() {
            return (PatternStoreInterface*) this;
        }


};


/************* Abstract datatype for all kinds of maps ********************/


/**
 * \brief Abstract class for map-like pattern stores, do not instantiate directly.
 * @tparam ContainerType The low-level container type used (an STL container such as set/map).
 * @tparam ValueType The type of Value this container stores
 * @tparam ValueHandler A handler class for this type of value
 * @tparam ReadWriteSizeType Data type for addressing, influences only the maximum number of items that can be stored (2**64) in the container, as this will be represented in the very beginning of the binary file. No reason to change this unless the container is very deeply nested in others and contains only few items.
 */
template<class ContainerType, class ValueType, class ValueHandler,class ReadWriteSizeType = uint32_t,class PatternType=Pattern>
class PatternMapStore: public PatternStore<ContainerType,ReadWriteSizeType,PatternType> {
     protected:
        ValueHandler valuehandler;
     public:
        PatternMapStore<ContainerType,ValueType,ValueHandler,ReadWriteSizeType,PatternType>(): PatternStore<ContainerType,ReadWriteSizeType,PatternType>() { };
        virtual ~PatternMapStore<ContainerType,ValueType,ValueHandler,ReadWriteSizeType,PatternType>() {};


        virtual void insert(const PatternType & pattern, ValueType & value)=0;

        virtual bool has(const Pattern &) const =0;
        virtual bool has(const PatternPointer &) const =0;

        virtual bool erase(const PatternType &) =0;


        virtual size_t size() const =0;
        virtual void reserve(size_t) =0;


        virtual ValueType & operator [](const Pattern & pattern)=0;
        virtual ValueType & operator [](const PatternPointer & pattern)=0;

        typedef typename ContainerType::iterator iterator;
        typedef typename ContainerType::const_iterator const_iterator;

        virtual typename ContainerType::iterator begin()=0;
        virtual typename ContainerType::iterator end()=0;
        virtual typename ContainerType::iterator find(const Pattern & pattern)=0;
        virtual typename ContainerType::iterator find(const PatternPointer & pattern)=0;


        /**
         * Write the map to stream output (in binary format)
         */
        virtual void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = this->begin(); iter != this->end(); iter++) {
                PatternType p = iter->first;
                p.write(out, this->corpusstart);
                this->valuehandler.write(out, iter->second);
            }
        }

        /**
         * Write the map to file (in binary format)
         */
        virtual void write(std::string filename) {
            std::ofstream * out = new std::ofstream(filename.c_str());
            this->write(out);
            out->close();
            delete out;
        }


        /**
         * Read a map from input stream (in binary format)
         */
        template<class ReadValueType=ValueType, class ReadValueHandler=ValueHandler,class ReadPatternType=PatternType>
        void read(std::istream * in, int MINTOKENS=0, int MINLENGTH=0, int MAXLENGTH=999999, PatternStoreInterface * constrainstore = NULL, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true, bool DORESET=false,  bool DEBUG=false) {
            ReadValueHandler readvaluehandler = ReadValueHandler();
            ReadWriteSizeType s; //read size:
            ReadPatternType p;
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            reserve(s);
            if (DEBUG) std::cerr << "Reading " << s << " patterns, classencodingversion=" << (int) this->classencodingversion << ", @corpusstart=" << (size_t) this->corpusstart << std::endl;
            if (MINTOKENS == -1) MINTOKENS = 0;
            for (ReadWriteSizeType i = 0; i < s; i++) {
                try {
                    p = ReadPatternType(in, false, this->classencodingversion, this->corpusstart, DEBUG);
                } catch (std::exception &e) {
                    std::cerr << "ERROR: Exception occurred at pattern " << (i+1) << " of " << s << std::endl;
                    throw InternalError();
                }
                if (!DONGRAMS || !DOSKIPGRAMS || !DOFLEXGRAMS) {
                    const PatternCategory c = p.category();
                    if ((!DONGRAMS && c == NGRAM) || (!DOSKIPGRAMS && c == SKIPGRAM) || (!DOFLEXGRAMS && c == FLEXGRAM)) continue;
                }
                const int n = p.size();
                if (DEBUG) std::cerr << "Read pattern #" << (i+1) << ", size=" << n << ", valuehandler=" << readvaluehandler.id() << ", classencodingversion="<<(int)this->classencodingversion;
                ReadValueType readvalue;
                readvaluehandler.read(in, readvalue);
                if (n >= MINLENGTH && n <= MAXLENGTH)  {
                    if ((readvaluehandler.count(readvalue) >= (unsigned int) MINTOKENS) && ((constrainstore == NULL) || (constrainstore->has(p)))) {
                            ValueType * convertedvalue = NULL;
                            if (DORESET) {
                                convertedvalue = new ValueType();
                            } else {
                                readvaluehandler.convertto(&readvalue, convertedvalue);
                                if (DEBUG) std::cerr << "...converted";
                                if (convertedvalue == NULL) {
                                    if (DEBUG) std::cerr << std::endl;
                                    std::cerr << "ERROR: Converted value yielded NULL at pattern #" << (i+1) << ", size=" << n << ", valuehandler=" << readvaluehandler.id() <<std::endl;
                                    throw InternalError();
                                }
                            }
                            if (DEBUG) std::cerr << "...adding";
                            this->insert(p,*convertedvalue);
                            if ((convertedvalue != NULL) && ((void*) convertedvalue != (void*) &readvalue)) delete convertedvalue;
                    } else if (DEBUG) {
                        if (readvaluehandler.count(readvalue) < (unsigned int) MINTOKENS) {
                            std::cerr << "...skipping because of occurrence (" << readvaluehandler.count(readvalue) << " below " << MINTOKENS;
                        } else {
                            std::cerr << "...skipping because of constraints";
                        }
                    }
                } else if (DEBUG) {
                  std::cerr << "...skipping because of length";
                }
                if (DEBUG) std::cerr << std::endl;
            }
        }

        /**
         * Read a map from file (in binary format)
         */
        void read(std::string filename,int MINTOKENS=0, int MINLENGTH=0, int MAXLENGTH=999999, PatternStoreInterface * constrainstore = NULL, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true, bool DORESET = false, bool DEBUG=false) { //no templates for this one, easier on python/cython
            std::ifstream * in = new std::ifstream(filename.c_str());
            this->read<ValueType,ValueHandler>(in,MINTOKENS,MINLENGTH,MAXLENGTH,constrainstore,DONGRAMS,DOSKIPGRAMS,DOFLEXGRAMS, DORESET, DEBUG);
            in->close();
            delete in;
        }

};


/************* Specific STL-like containers for patterns ********************/



typedef std::unordered_set<Pattern> t_patternset;

/**
 * \brief A pattern store in the form of an unordered set (i.e, no duplicates).
 * Stores only patterns, no values.
 * @tparam ReadWriteSizeType The data type for addressing, determines the
 * maximum amount of patterns that can be held, only used in
 * serialisation/deserialisation
 */
template<class ReadWriteSizeType = uint32_t>
class PatternSet: public PatternStore<t_patternset,ReadWriteSizeType,Pattern> {
    protected:
        t_patternset data;
    public:

        /**
         * Empty set constructor
         */
        PatternSet<ReadWriteSizeType>(): PatternStore<t_patternset,ReadWriteSizeType>() {};


        /**
         * Constructs a pattern set from a ClassDecoder
         */
        PatternSet<ReadWriteSizeType>(const ClassDecoder & classdecoder): PatternStore<t_patternset,ReadWriteSizeType>() {
            for (ClassDecoder::const_iterator iter = classdecoder.begin(); iter != classdecoder.end(); iter++) {
                const int cls = iter->first;
                unsigned char * buffer = new unsigned char[64];
                int length = inttobytes(buffer, cls); //will be set by inttobytes
                data.insert( Pattern(buffer, length) );
                delete [] buffer;
            }
        }

        /**
         * Constructs a pattern set from a ClassEncoder
         */
        PatternSet<ReadWriteSizeType>(const ClassEncoder & classencoder): PatternStore<t_patternset,ReadWriteSizeType>() {
            for (ClassEncoder::const_iterator iter = classencoder.begin(); iter != classencoder.end(); iter++) {
                const int cls = iter->second;
                unsigned char * buffer = new unsigned char[64];
                int length = inttobytes(buffer, cls); //will be set by inttobytes
                data.insert( Pattern(buffer, length) );
                delete [] buffer;
            }
        }

        virtual ~PatternSet<ReadWriteSizeType>() {};

        /**
         * Add a new pattern to the set
         */
        void insert(const Pattern & pattern) {
            data.insert(pattern);
        }

        /**
         * Checks if a pattern is in the set
         */
        bool has(const Pattern & pattern) const { return data.count(pattern); }

        /**
         * Checks if a pattern is in the set
         */
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }

        /**
         * Returns the number of patterns in the set
         */
        size_t size() const { return data.size(); }
        void reserve(size_t s) { data.reserve(s); }

        typedef t_patternset::iterator iterator;
        typedef t_patternset::const_iterator const_iterator;

        /**
         * Returns an iterator to iterate over the set
         */
        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        /**
         * Returns an iterator to the pattern in the set or end() if no such
         * pattern was found.
         */
        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        iterator find(const PatternPointer & pattern) { return data.find(pattern); }
        const_iterator find(const PatternPointer & pattern) const { return data.find(pattern); }

        /**
         * Removes the specified pattern from the set, returns true if successful
         */
        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }


        /**
         * Write the set to output stream, in binary format
         */
        void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = begin(); iter != end(); iter++) {
                Pattern p = *iter;
                p.write(out, this->corpusstart);
            }
        }

        /**
         * Read the set from input stream, in binary format
         */
        void read(std::istream * in, int MINLENGTH=0, int MAXLENGTH=999999, PatternStoreInterface * constrainstore = NULL, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true) {
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            reserve(s);
            for (unsigned int i = 0; i < s; i++) {
                Pattern p = Pattern(in, false, this->classencodingversion);
                if (!DONGRAMS || !DOSKIPGRAMS || !DOFLEXGRAMS) {
                    const PatternCategory c = p.category();
                    if ((!DONGRAMS && c == NGRAM) || (!DOSKIPGRAMS && c == SKIPGRAM) || (!DOFLEXGRAMS && c == FLEXGRAM)) continue;
                }
                const int n = p.size();
                if ((n >= MINLENGTH && n <= MAXLENGTH) && ((constrainstore == NULL) || (constrainstore->has(p)))) {
                    insert(p);
                }
            }
        }

        /**
         * Reads a map from input stream, in binary format, but ignores the values
         * and retains only the keys for the set.
         */
        template<class ReadValueType, class ReadValueHandler=BaseValueHandler<ReadValueType>>
        void readmap(std::istream * in, int MINTOKENS=0, int MINLENGTH=0, int MAXLENGTH=999999, PatternStoreInterface * constrainstore = NULL, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true) {
            ReadValueHandler readvaluehandler = ReadValueHandler();
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            reserve(s);
            //std::cerr << "Reading " << (int) s << " patterns" << std::endl;
            for (ReadWriteSizeType i = 0; i < s; i++) {
                Pattern p;
                try {
                    p = Pattern(in, false, this->classencodingversion);
                } catch (std::exception &e) {
                    std::cerr << "ERROR: Exception occurred at pattern " << (i+1) << " of " << s << std::endl;
                    throw InternalError();
                }
                if (!DONGRAMS || !DOSKIPGRAMS || !DOFLEXGRAMS) {
                    const PatternCategory c = p.category();
                    if ((!DONGRAMS && c == NGRAM) || (!DOSKIPGRAMS && c == SKIPGRAM) || (!DOFLEXGRAMS && c == FLEXGRAM)) continue;
                }
                const int n = p.size();
                ReadValueType readvalue;
                //std::cerr << "Read pattern: " << std::endl;
                readvaluehandler.read(in, readvalue);
                if (n >= MINLENGTH && n <= MAXLENGTH)  {
                    if ((readvaluehandler.count(readvalue) >= (unsigned int) MINTOKENS) && ((constrainstore == NULL) || (constrainstore->has(p)))) {
                        this->insert(p);
                    }
                }
            }
        }
};


typedef std::set<Pattern> t_hashorderedpatternset;


/**
 * \brief A pattern store in the form of an ordered set (i.e, no duplicates).
 * Stores only patterns, no values.
 * @tparam ReadWriteSizeType The data type for addressing, determines the
 * maximum amount of patterns that can be held, only used in
 * serialisation/deserialisation
 */
template<class ReadWriteSizeType = uint64_t>
class HashOrderedPatternSet: public PatternStore<t_hashorderedpatternset,ReadWriteSizeType> {
    protected:
        t_hashorderedpatternset data;
    public:

        HashOrderedPatternSet<ReadWriteSizeType>(): PatternStore<t_hashorderedpatternset,ReadWriteSizeType>() {};
        virtual ~HashOrderedPatternSet<ReadWriteSizeType>();

        void insert(const Pattern pattern) {
            data.insert(pattern);
        }

        bool has(const Pattern & pattern) const { return data.count(pattern); }
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }
        size_t size() const { return data.size(); }
        void reserve(size_t s) {} //noop

        typedef t_hashorderedpatternset::iterator iterator;
        typedef t_hashorderedpatternset::const_iterator const_iterator;

        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }
        iterator find(const PatternPointer & pattern) { return data.find(pattern); }
        const_iterator find(const PatternPointer & pattern) const { return data.find(pattern); }

        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }


        void write(std::ostream * out) {
            ReadWriteSizeType s = (ReadWriteSizeType) size();
            out->write( (char*) &s, sizeof(ReadWriteSizeType));
            for (iterator iter = begin(); iter != end(); iter++) {
                Pattern p = *iter;
                p.write(out, this->corpusstart);
            }
        }

        void read(std::istream * in, int MINLENGTH=0, int MAXLENGTH=999999, bool DONGRAMS=true, bool DOSKIPGRAMS=true, bool DOFLEXGRAMS=true) {
            ReadWriteSizeType s; //read size:
            in->read( (char*) &s, sizeof(ReadWriteSizeType));
            reserve(s);
            for (unsigned int i = 0; i < s; i++) {
                Pattern p = Pattern(in, false, this->classencodingversion);
                if (!DONGRAMS || !DOSKIPGRAMS || !DOFLEXGRAMS) {
                    const PatternCategory c = p.category();
                    if ((!DONGRAMS && c == NGRAM) || (!DOSKIPGRAMS && c == SKIPGRAM) || (!DOFLEXGRAMS && c == FLEXGRAM)) continue;
                }
                const int n = p.size();
                if (n >= MINLENGTH && n <= MAXLENGTH)  {
                    insert(p);
                }
            }
        }

};


/**
 * \brief A pattern map storing patterns and their values in a hash map (unordered_map).
 * @tparam ValueType The type of Value this container stores
 * @tparam ValueHandler A handler class for this type of value
 * @tparam ReadWriteSizeType The data type for addressing, determines the
 * maximum amount of patterns that can be held, only used in
 * serialisation/deserialisation
 */
template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class ReadWriteSizeType = uint64_t>
class PatternMap: public PatternMapStore<std::unordered_map<Pattern,ValueType>,ValueType,ValueHandler,ReadWriteSizeType,Pattern> {
    protected:
        std::unordered_map<Pattern, ValueType> data;
    public:
        //PatternMap(): PatternMapStore<std::unordered_map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        PatternMap<ValueType,ValueHandler,ReadWriteSizeType>() {};

        void insert(const Pattern & pattern, ValueType & value) {
            data[pattern] = value;
        }

        void insert(const Pattern & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType, usually 0

        bool has(const Pattern & pattern) const {
            return data.count(pattern);
        }
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); }
        void reserve(size_t s) { data.reserve(s); }


        ValueType& operator [](const Pattern & pattern) { return data[pattern]; }
        ValueType& operator [](const PatternPointer & pattern) { return data[pattern]; }

        typedef typename std::unordered_map<Pattern,ValueType>::iterator iterator;
        typedef typename std::unordered_map<Pattern,ValueType>::const_iterator const_iterator;

        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }
        iterator find(const PatternPointer & pattern) { return data.find(pattern); }
        const_iterator find(const PatternPointer & pattern) const { return data.find(pattern); }

        bool erase(const Pattern & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }

};


template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class ReadWriteSizeType = uint64_t>
class PatternPointerMap: public PatternMapStore<std::unordered_map<PatternPointer,ValueType>,ValueType,ValueHandler,ReadWriteSizeType,PatternPointer> {
    protected:
        std::unordered_map<PatternPointer, ValueType> data;
    public:
		IndexedCorpus * corpus;
        //PatternMap(): PatternMapStore<std::unordered_map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        PatternPointerMap<ValueType,ValueHandler,ReadWriteSizeType>(IndexedCorpus * corpus) {
			this->corpus = corpus;
		};

        PatternPointerMap<ValueType,ValueHandler,ReadWriteSizeType>() { corpus = NULL; }


        void insert(const PatternPointer & pattern, ValueType & value) {
            data[pattern] = value;
        }

        void insert(const PatternPointer & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType, usually 0

        bool has(const Pattern & pattern) const {
            return data.count(pattern);
        }
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); }
        void reserve(size_t s) { data.reserve(s); }


        ValueType& operator [](const Pattern & pattern) { return data[pattern]; }
        ValueType& operator [](const PatternPointer & pattern) { return data[pattern]; }

        typedef typename std::unordered_map<PatternPointer,ValueType>::iterator iterator;
        typedef typename std::unordered_map<PatternPointer,ValueType>::const_iterator const_iterator;

        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) {
            PatternPointer pp = pattern.getpointer();
            return data.find(pp);
        }
        //const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        iterator find(const PatternPointer & pattern) { return data.find(pattern); }
        const_iterator find(const PatternPointer & pattern) const { return data.find(pattern); }

        bool erase(const PatternPointer & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }
};

template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class ReadWriteSizeType = uint64_t>
class OrderedPatternPointerMap: public PatternMapStore<std::map<PatternPointer,ValueType>,ValueType,ValueHandler,ReadWriteSizeType,PatternPointer> {
    protected:
        std::map<PatternPointer, ValueType> data;
    public:
		IndexedCorpus * corpus;
        //PatternMap(): PatternMapStore<std::unordered_map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        OrderedPatternPointerMap<ValueType,ValueHandler,ReadWriteSizeType>(IndexedCorpus * corpus) {
			this->corpus = corpus;
		};

        OrderedPatternPointerMap<ValueType,ValueHandler,ReadWriteSizeType>() { corpus = NULL; }


        void insert(const PatternPointer & pattern, ValueType & value) {
            data[pattern] = value;
        }

        void insert(const PatternPointer & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType, usually 0

        bool has(const Pattern & pattern) const {
            return data.count(pattern);
        }
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); }
        void reserve(size_t s) { } //noop


        ValueType& operator [](const Pattern & pattern) { return data[pattern]; }
        ValueType& operator [](const PatternPointer & pattern) { return data[pattern]; }

        typedef typename std::map<PatternPointer,ValueType>::iterator iterator;
        typedef typename std::map<PatternPointer,ValueType>::const_iterator const_iterator;

        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) {
            PatternPointer pp = pattern.getpointer();
            return data.find(pp);
        }
        //const_iterator find(const Pattern & pattern) const { return data.find(pattern); }

        iterator find(const PatternPointer & pattern) { return data.find(pattern); }
        const_iterator find(const PatternPointer & pattern) const { return data.find(pattern); }

        bool erase(const PatternPointer & pattern) { return data.erase(pattern); }
        iterator erase(const_iterator position) { return data.erase(position); }
};

template<class ValueType,class ValueHandler = BaseValueHandler<ValueType>,class ReadWriteSizeType = uint64_t>
class HashOrderedPatternMap: public PatternMapStore<std::map<const Pattern,ValueType>,ValueType,ValueHandler,ReadWriteSizeType,Pattern> {
    protected:
        std::map<const Pattern, ValueType> data;
    public:
        HashOrderedPatternMap<ValueType,ValueHandler,ReadWriteSizeType>(): PatternMapStore<std::map<const Pattern, ValueType>,ValueType,ValueHandler,ReadWriteSizeType>() {};
        virtual ~HashOrderedPatternMap<ValueType,ValueHandler,ReadWriteSizeType>() {};

        void insert(const Pattern & pattern, ValueType & value) {
            data[pattern] = value;
        }

        void insert(const Pattern & pattern) {  data[pattern] = ValueType(); } //singular insert required by PatternStore, implies 'default' ValueType

        bool has(const Pattern & pattern) const { return data.count(pattern); }
        bool has(const PatternPointer & pattern) const { return data.count(pattern); }

        size_t size() const { return data.size(); }
        void reserve(size_t) {} //noop

        ValueType& operator [](const Pattern & pattern) { return data[pattern]; }
        ValueType& operator [](const PatternPointer & pattern) { return data[pattern]; }

        typedef typename std::map<const Pattern,ValueType>::iterator iterator;
        typedef typename std::map<const Pattern,ValueType>::const_iterator const_iterator;

        iterator begin() { return data.begin(); }
        const_iterator begin() const { return data.begin(); }

        iterator end() { return data.end(); }
        const_iterator end() const { return data.end(); }

        iterator find(const Pattern & pattern) { return data.find(pattern); }
        const_iterator find(const Pattern & pattern) const { return data.find(pattern); }
        iterator find(const PatternPointer & pattern) { return data.find(pattern); }
        const_iterator find(const PatternPointer & pattern) const { return data.find(pattern); }

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
    unsigned int count(std::array<T,N> & a) const {
        return (int) a[countindex];
    }
    void add(std::array<T,N> * value, const IndexReference & ref ) const {
        (*value)[countindex] += 1;
    }
};


/**
 * \brief A complex value handler for values that are themselves pattern stores (allows building nested maps).
 */
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
    virtual std::string tostring(  PatternStoreType & ) {
        std::cerr << "PatternStoreValueHandler::tostring() is not supported" << std::endl;
        throw InternalError();
    }
    unsigned int count( PatternStoreType & value) const {
        return value.size();
    }
    void add( PatternStoreType *, const IndexReference & ) const {
        std::cerr << "PatternStoreValueHandler::add() is not supported" << std::endl;
        throw InternalError();
    }
};

/**
 * \brief A nested pattern map, useful for storing patterns that map to other patterns, which in turn map to values.
 * An example is phrase-translation tables in Machine Translation.
 */
template<class ValueType,class ValueHandler=BaseValueHandler<ValueType>, class NestedSizeType = uint16_t >
class AlignedPatternMap: public PatternMap< PatternMap<ValueType,ValueHandler,NestedSizeType>,PatternStoreValueHandler<PatternMap<ValueType,ValueHandler,NestedSizeType>>, uint64_t > {
    public:
        typedef PatternMap<ValueType,ValueHandler,NestedSizeType> valuetype;
        typedef typename PatternMap< PatternMap<ValueType,ValueHandler,NestedSizeType>,PatternStoreValueHandler<PatternMap<ValueType,ValueHandler,NestedSizeType>>, uint64_t >::iterator iterator;
        typedef typename PatternMap< PatternMap<ValueType,ValueHandler,NestedSizeType>,PatternStoreValueHandler<PatternMap<ValueType,ValueHandler,NestedSizeType>>, uint64_t >::const_iterator const_iterator;

};


//TODO: Implement a real Trie, conserving more memory
#endif
