#ifndef PATTERNMODEL_H
#define PATTERNMODEL_H

/*****************************
* Colibri Core
*   by Maarten van Gompel
*   Centre for Language Studies
*   Radboud University Nijmegen
*
*   https://proycon.github.io/colibri-core
*
*   Licensed under GPLv3
*****************************/

/**
 * @file patternmodel.h
 * \brief Contains classes for Pattern Models.
 *
 * @author Maarten van Gompel (proycon) <proycon@anaproy.nl>
 *
 * @section LICENSE
 * Licensed under GPLv3
 *
 * @section DESCRIPTION
 * Contains classes for Pattern Models
 *
 */



/**
 * \mainpage Colibri Core
 *
 * Colibri Core is a set of tools as well as a C++ and Python library for
 * working with basic linguistic constructions such as n-grams and skipgrams (i.e
 * patterns with one or more gaps, either of fixed or dynamic size) in a quick
 * and memory-efficient way. At the core is the tool colibri-patternmodeller
 * which allows you to build, view, manipulate and query pattern models.
 *
 * In Colibri Core, text data is encoded as a compressed binary representation
 * using a class encoding. The ClassEncoder and ClassDecoder can be used to
 * create and decode such a class encoding. The Pattern class represents any
 * n-gram, skip-gram, flexgram.  These patterns can be stored in various
 * models, such as the PatternModel or it's indexed equivalent, the
 * IndexedPatternModel. These are high-level classes built on lower-level
 * containers such as PatternMap. Other containers such as PatternSet are
 * available too.
 *
 * Corpus data can also be read into an IndexedCorpus class, which also acts
 * as a reverse index for the pattern models.
 *
 */


#include "patternstore.h"
#include "classencoder.h"
#include "algorithms.h"
#include <limits>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <array>
#include <exception>
#include "bz2stream.h"


/**
 * Defines the various types of pattern models
 */
enum ModelType {
	UNINDEXEDPATTERNMODEL = 10,
	UNINDEXEDPATTERNPOINTERMODEL = 11,
    INDEXEDPATTERNMODEL = 20,
    INDEXEDPATTERNPOINTERMODEL = 21,
    PATTERNSETMODEL = 30,
    PATTERNALIGNMENTMODEL = 40,
};


/**
 * Defines Reverse Index Types
 */
enum ReverseIndexType {
    NONE = 0,
    QUICK = 1,
    COMPACT = 2,
};

/**
 * Extracts the model type (one of ModelType) from a file
 */
int getmodeltype(const std::string & filename);


class NoSuchPattern: public std::exception {
  virtual const char* what() const throw()
  {
    return "Pattern not found in model";
  }
};


/**
 * \brief Options for Pattern Model loading and training.
 *
 * This class defines all kinds of parameters that can be set for loading and
 * training Pattern Models, it is passed to various constructors and methods.
 */
class PatternModelOptions {
    public:
        int MINTOKENS; ///< The occurrence threshold, minimum amount of occurrences for a pattern to be included in a model
                       ///< Defaults to 2 for building, to 1 for loading.

        int MINTOKENS_SKIPGRAMS; ///< The occurrence threshold for skipgrams, minimum amount of occurrences for a pattern to be included in a model.
                                 ///< Defaults to the same value as MINTOKENS.
                                 ///< Only used if DOSKIPGRAMS or
                                 ///< DO_SKIPGRAMS_EXHAUSTIVE is set to true

        int MINTOKENS_UNIGRAMS; ///< The occurrence threshold for unigrams, unigrams must occur at least this many times for higher-order ngram/skipgram to be included in a model
                                ///< Defaults to the same value as MINTOKENS
                                ///< Only has an effect if MINTOKENS_UNIGRAMS > MINTOKENS.
                                //
        int MINLENGTH; ///< The minimum length of patterns to be loaded/extracted (in words/tokens) (default: 1)
        int MAXLENGTH; ///< The maximum length of patterns to be loaded/extracted, inclusive (in words/tokens) (default: 100)
        int MAXBACKOFFLENGTH; ///< Counting n-grams is done iteratively for each increasing n. (default: MAXLENGTH)
                              ///< For each n, presence of sub-ngrams in n-1 is
                              ///< checked. This values defines a maximum
                              ///< length for this back-off check. In
                              ///< combination with MINLENGTH, this allows earlier
                              ///< pruning and conserves memory.

        bool DOSKIPGRAMS; ///< Load/extract skipgrams? (default: false)
        bool DOSKIPGRAMS_EXHAUSTIVE; ///< Load/extract skipgrams in an exhaustive fashion? More memory intensive, but the only options for unindexed models (default: false). Use DOSKIPGRAMS instead, this will be automatically used as a fallback whnever EXHAUSTIVE computation is necessary.
        int MINSKIPTYPES;  ///< Minimum required amount of distinct patterns that can fit in a gap of a skipgram for the skipgram to be included (default: 2)
        int MAXSKIPS; ///< Maximum skips per skipgram

        //bool CROSSBOUNDARIES; ///< Include n-grams that cross unit (e.g. sentence) boundaries (newlines in the original text). //MAYBE TODO

        bool DOREVERSEINDEX; ///< Obsolete now, only here for backward-compatibility with v1
        bool DOPATTERNPERLINE; ///< Assume each line contains one integral pattern, rather than actively extracting all subpatterns on a line (default: false)

        int PRUNENONSUBSUMED; //< Prune all n-grams that are not subsumed by higher-order ngrams

        bool DOREMOVEINDEX; ///< Do not load index information (for indexed models), loads just the patterns without any counts
        bool DOREMOVENGRAMS; ///< Remove n-grams from the model upon loading it
        bool DOREMOVESKIPGRAMS; ///< Remove skip-grams from the model upon loading it
        bool DOREMOVEFLEXGRAMS; ///< Remove flexgrams from the model upon loading it
        bool DORESET; ///< sets all counts to zero upon loading, clears indices

        bool QUIET; ///< Don't output to stderr
        bool DEBUG; ///< Output extra debug information

        /**
         * Initialise with default values. All members are public and can be
         * set explicitly..
         */
        PatternModelOptions() {
            MINTOKENS = -1; //defaults to 2 for building, 1 for loading
            MINTOKENS_SKIPGRAMS = -1; //defaults to MINTOKENS
            MINTOKENS_UNIGRAMS = 1; //defaults to, effectively disabled
            MINLENGTH = 1;
            MAXLENGTH = 100;
            MAXBACKOFFLENGTH = 100;

            MINSKIPTYPES = 2;
            MAXSKIPS = 3;
            DOSKIPGRAMS = false;
            DOSKIPGRAMS_EXHAUSTIVE = false;

            DOREVERSEINDEX = true; //obsolete
            DOPATTERNPERLINE = false;
            DORESET = false;

            DOREMOVEINDEX = false; //only for indexed models
            DOREMOVENGRAMS = false;
            DOREMOVESKIPGRAMS = false;
            DOREMOVEFLEXGRAMS = false;

            PRUNENONSUBSUMED = false;

            DEBUG = false;
            QUIET = false;
        }

        /**
         * Copy constructor
         */
        PatternModelOptions(const PatternModelOptions & ref) {
            MINTOKENS = ref.MINTOKENS; //defaults to 2 for building, 1 for loading
            MINTOKENS_UNIGRAMS = ref.MINTOKENS_UNIGRAMS;
            MINTOKENS_SKIPGRAMS = ref.MINTOKENS_SKIPGRAMS; //defaults to 2 for building, 1 for loading
            MINLENGTH = ref.MINLENGTH;
            MAXLENGTH = ref.MAXLENGTH;
            MAXBACKOFFLENGTH = ref.MAXBACKOFFLENGTH;

            MINSKIPTYPES = ref.MINSKIPTYPES;
            MAXSKIPS = ref.MAXSKIPS;
            DOSKIPGRAMS = ref.DOSKIPGRAMS;
            DOSKIPGRAMS_EXHAUSTIVE = ref.DOSKIPGRAMS_EXHAUSTIVE;

            DOREVERSEINDEX = ref.DOREVERSEINDEX;
            DOPATTERNPERLINE = ref.DOPATTERNPERLINE;
            DORESET = ref.DORESET;

            DOREMOVEINDEX = ref.DOREMOVEINDEX; //only for indexed models
            DOREMOVENGRAMS = ref.DOREMOVENGRAMS;
            DOREMOVESKIPGRAMS = ref.DOREMOVESKIPGRAMS;
            DOREMOVEFLEXGRAMS = ref.DOREMOVEFLEXGRAMS;

            PRUNENONSUBSUMED = ref.PRUNENONSUBSUMED;

            DEBUG = ref.DEBUG;
            QUIET = ref.QUIET;
        }


};

/**
 * A relationmap is just a pattern map, the map is specific for a pattern and
 * holds patterns that are in a relation with the  first pattern. The value of
 * the map is an integer that expresses how often the relationship occurs.
 */
typedef PatternMap<uint32_t,BaseValueHandler<uint32_t>,uint64_t> t_relationmap;
/**
 * A relationmap_double is just a pattern map, the map is specific for a pattern and
 * holds patterns that are in a relation with the  first pattern. The value of
 * the map is an double that expresses the weight of the relation.
 */
typedef PatternMap<double,BaseValueHandler<double>,uint64_t> t_relationmap_double;

typedef PatternMap<uint32_t,BaseValueHandler<uint32_t>,uint64_t>::iterator t_relationmap_iterator;  //needed for Cython
typedef PatternMap<double,BaseValueHandler<double>,uint64_t>::iterator t_relationmap_double_iterator;

/**
 * \brief Basic read-only interface for pattern models, abstract base class.
 */
class PatternModelInterface: public PatternStoreInterface {
    public:
        /**
         * Get the type of the model
         * @return ModelType
         */
        virtual int getmodeltype() const=0;

        /**
         * Get the version number of the model
         */
        virtual int getmodelversion() const=0;

        //these are already in PatternStoreInterface:
            //virtual bool has(const Pattern &) const =0;
            //virtual bool has(const PatternPointer &) const =0;
            //virtual size_t size() const =0;

        /**
         * Returns the number of times this pattern occurs in the model
         */
        virtual unsigned int occurrencecount(const Pattern & pattern)=0;

        /**
         * Returns the frequency of the pattern in the
         * model, a relative/normalised value
         */
        virtual double frequency(const Pattern &) =0;

        /**
         * Return the maximum pattern length in this model
         */
        virtual int maxlength() const=0;
        /**
         * Returns the minumum pattern length in this model
         */
        virtual int minlength() const=0;

        /**
         * Return the number of distinct words/unigram in the original corpus,
         * includes types not covered by the model!
         */
        virtual unsigned int types() =0;

        /**
         * Returns the number of tokens in the original corpus, includes tokens
         * not covered by the model!
         */
        virtual unsigned int tokens() const=0;

        virtual PatternStoreInterface * getstoreinterface() {
            return (PatternStoreInterface*) this;
        };
};



/**
 * \brief A pattern model based on an unordered set, does not hold data, only patterns.
 * Very suitable for loading constraint models.
 */
class PatternSetModel: public PatternSet<uint64_t>, public PatternModelInterface {
    protected:
        unsigned char model_type;
        unsigned char model_version;
        uint64_t totaltokens; //INCLUDES TOKENS NOT COVERED BY THE MODEL!
        uint64_t totaltypes; //TOTAL UNIGRAM TYPES, INCLUDING NOT COVERED BY THE MODEL!
        int maxn;
        int minn;
    public:
        /**
         * Empty constructor
         */
        PatternSetModel() {
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
        }

        /**
         * Load a PatternSetModel from stream
         * @param options The options for loading
         * @param constrainmodel Load only patterns that occur in this model
         */
        PatternSetModel(std::istream *f, PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) {
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            this->load(f,options, constrainmodel);
        }

        /**
         * Load a PatternSetModel from file
         * @param filename The name of the file to load
         * @param options The options for loading
         * @param constrainmodel Load only patterns that occur in this model
         */
        PatternSetModel(const std::string & filename, const PatternModelOptions & options, PatternModelInterface * constrainmodel = NULL) {
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            if (!options.QUIET) std::cerr << "Loading " << filename << std::endl;
            std::ifstream * in = new std::ifstream(filename.c_str());
            if (!in->good()) {
                std::cerr << "ERROR: Unable to load file " << filename << std::endl;
                throw InternalError();
            }
            this->load( (std::istream *) in, options, constrainmodel);
            in->close();
            delete in;
        }
        virtual int getmodeltype() const { return PATTERNSETMODEL; }
        virtual int getmodelversion() const { return 2; }

        virtual size_t size() const {
            return PatternSet<uint64_t>::size();
        }
        virtual bool has(const Pattern & pattern) const {
            return PatternSet<uint64_t>::has(pattern);
        }
        virtual bool has(const PatternPointer & pattern) const {
            return PatternSet<uint64_t>::has(pattern);
        }

        /**
         * Load a PatternSetModel from file
         * @param filename The name of the file to load
         * @param options The options for loading
         * @param constrainmodel Load only patterns that occur in this model
         */
        virtual void load(std::string & filename, const PatternModelOptions & options, PatternModelInterface * constrainmodel = NULL) {
            if (!options.QUIET) std::cerr << "Loading " << filename << " as set-model" << std::endl;
            std::ifstream * in = new std::ifstream(filename.c_str());
            if (!in->good()) {
                std::cerr << "ERROR: Unable to load file " << filename << std::endl;
                throw InternalError();
            }
            this->load( (std::istream *) in, options, constrainmodel);
            in->close();
            delete in;
        }

        /**
         * Load a PatternSetModel from stream
         * @param options The options for loading
         * @param constrainmodel Load only patterns that occur in this model
         */
        virtual void load(std::istream * f, const PatternModelOptions & options, PatternModelInterface * constrainmodel = NULL) { //load from file
            char null;
            f->read( (char*) &null, sizeof(char));
            f->read( (char*) &model_type, sizeof(char));
            f->read( (char*) &model_version, sizeof(char));
            if (model_version == 1) this->classencodingversion = 1;
            if ((null != 0) || ((model_type != UNINDEXEDPATTERNMODEL) && (model_type != INDEXEDPATTERNMODEL) && (model_type != PATTERNSETMODEL) && (model_type != PATTERNALIGNMENTMODEL) ))  {
                std::cerr << "ERROR: File is not a colibri patternmodel file" << std::endl;
                throw InternalError();
            }
            if (model_version > 2) {
                std::cerr << "WARNING: Model is created with a newer version of Colibri Core! Attempting to continue but failure is likely..." << std::endl;
            }
            f->read( (char*) &totaltokens, sizeof(uint64_t));
            f->read( (char*) &totaltypes, sizeof(uint64_t));

            PatternStoreInterface * constrainstore = NULL;
            if (constrainmodel) constrainstore = constrainmodel->getstoreinterface();

            if (options.DEBUG) {
                std::cerr << "Debug enabled, loading PatternModel type " << (int) model_type << ", version " << (int) model_version << ", classencodingversion" << (int) this->classencodingversion << std::endl;
                std::cerr << "Total tokens: " << totaltokens << ", total types: " << totaltypes << std::endl;;
            }
            if (model_type == PATTERNSETMODEL) {
                //reading set
                PatternSet<uint64_t>::read(f, options.MINLENGTH, options.MAXLENGTH, constrainstore, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS); //read PatternStore
            } else if (model_type == INDEXEDPATTERNMODEL) {
                //reading from indexed pattern model, ok:
                 readmap<IndexedData,IndexedDataHandler>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore,  !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS);
            } else if (model_type == UNINDEXEDPATTERNMODEL)  {
                //reading from unindexed pattern model, ok:
                 readmap<uint32_t,BaseValueHandler<uint32_t>>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS);
            } else if (model_type == PATTERNALIGNMENTMODEL)  {
                 //ok:
                 readmap<PatternFeatureVectorMap<double>, PatternFeatureVectorMapHandler<double>>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore,  !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS);
            } else {
                std::cerr << "ERROR: Unknown model type" << std::endl;
                throw InternalError();
            }
        }

        /**
         * Write a PatternSetModel to an output stream
         */
        void write(std::ostream * out) {
            const char null = 0;
            out->write( (char*) &null, sizeof(char));
            unsigned char t = this->getmodeltype();
            out->write( (char*) &t, sizeof(char));
            unsigned char v = this->getmodelversion();
            out->write( (char*) &v, sizeof(char));
            out->write( (char*) &totaltokens, sizeof(uint64_t));
            const uint64_t tp = this->types(); //use this instead of totaltypes, as it may need to be computed on-the-fly still
            out->write( (char*) &tp, sizeof(uint64_t));
            PatternSet<uint64_t>::write(out); //write
        }

        /**
         * Write a PatternSetModel to an output file. This is a wrapper around
         * write(std::ostream *)
         */
        void write(const std::string & filename) {
            std::ofstream * out = new std::ofstream(filename.c_str());
            this->write(out);
            out->close();
            delete out;
        }

        /**
         * Get the interface (just a basic typecast)
         */
        virtual PatternModelInterface * getinterface() {
            return (PatternModelInterface*) this;
        }

        /**
         * This function does not perform anything in a set context, it always
         * returns zero
         */
        virtual unsigned int occurrencecount(const Pattern & ) { return 0;  }

        /**
         * This function does not perform anything in a set context, it always
         * returns zero
         */
        virtual double frequency(const Pattern &) { return 0; }

        typedef typename PatternSet<uint64_t>::iterator iterator;
        typedef typename PatternSet<uint64_t>::const_iterator const_iterator;

        /**
         * Return the maximum length of patterns in this model
         */
        virtual int maxlength() const { return maxn; };

        /**
         * Return the minimum length of patterns in this model
         */
        virtual int minlength() const { return minn; };

        /**
         * Returns the total amount of unigram/word types in the original corpus,
         * includes types not covered by the model!
         */
        virtual unsigned int types()  {
            return totaltypes;
        }
        /**
         * Returns the total amount of tokens in the original corpus,
         * includes tokens not covered by the model!
         */
        virtual unsigned int tokens() const { return totaltokens; }

        /**
         * Returns the type of the model, value is of the PatternType
         * enumeration.
         */
        unsigned char type() const { return model_type; }
        /**
         * Returns the version of the model's implementation and binary serialisation format.
         */
        unsigned char version() const { return model_version; }
};

typedef std::unordered_map<PatternPointer,std::vector<std::pair<uint32_t,unsigned char>>> t_matchskipgramhelper; //firstword => [pair<mask,n>]
typedef std::unordered_map<PatternPointer,std::unordered_set<PatternPointer>> t_matchflexgramhelper; //firstword => [flexgram]

/**
 * \brief A model mapping patterns to values, high-level interface.
 * @tparam ValueType The type of Value this model stores
 * @tparam ValueHandler A handler class for this type of value
 * @tparam MapType The type of container to use
 */
template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class MapType = PatternMap<ValueType, BaseValueHandler<ValueType>>, class PatternType = Pattern>
class PatternModel: public MapType, public PatternModelInterface {
    protected:
        unsigned char model_type;
        unsigned char model_version;
        uint64_t totaltokens; ///< Total number of tokens in the original corpus, so INCLUDES TOKENS NOT COVERED BY THE MODEL!
        uint64_t totaltypes; ///< Total number of unigram/word types in the original corpus, SO INCLUDING NOT COVERED BY THE MODEL!

        int maxn;
        int minn;

        t_matchskipgramhelper matchskipgramhelper; ///< Helper structure for finding skipgrams in corpus data: maps the first word of skipgrams to series of mask,length pairs, so skipgrams need not be recomputed exhaustively for each pattern
        t_matchflexgramhelper matchflexgramhelper; ///< Helper structure for finding flexgrams in corpus data: maps the first word of skipgrams to series of parts

        //std::multimap<IndexReference,Pattern> reverseindex;
        std::set<int> cache_categories;
        std::set<int> cache_n;
        std::map<int,std::map<int,unsigned int>> cache_grouptotal; ///< total occurrences (used for frequency computation, within a group)
        std::map<int,std::map<int,unsigned int>> cache_grouptotalpatterns ; ///< total distinct patterns per group
        std::map<int,std::map<int,unsigned int>> cache_grouptotalwordtypes; ///< total covered word types per group
        std::map<int,std::map<int,unsigned int>> cache_grouptotaltokens; ///< total covered tokens per group
        std::map<int,std::map<int,bool>> cache_processed; ///< are coverage statistics processed for this group?
        bool cache_processed_all; ///< are coverage statistics processed for all groups?


        std::map<int, std::vector< uint32_t > > gapmasks; ///< pre-computed masks representing possible gap configurations for various pattern lengths

        virtual void postread(const PatternModelOptions& ) {
            //this function has a specialisation specific to indexed pattern models,
            //this is the generic version
            for (iterator iter = this->begin(); iter != this->end(); iter++) {
                const PatternType p = iter->first;
                const PatternCategory category = p.category();
                const int n = p.n();
                if (n > maxn) maxn = n;
                if (n < minn) minn = n;
                if ((!hasskipgrams) && (category == SKIPGRAM)) hasskipgrams = true;
                if ((!hasflexgrams) && (category == FLEXGRAM)) hasflexgrams = true;
            }
        }
        virtual void posttrain(const PatternModelOptions& ) {
            //nothing to do here, indexed model specialised this function to
            //sort indices
        }

        virtual void compute_matchhelpers(bool quiet = false) {
            const bool doskipgrams = (matchskipgramhelper.empty() && hasskipgrams);
            const bool doflexgrams = (matchflexgramhelper.empty() && hasflexgrams);
            if (!doskipgrams && !doflexgrams) return;

            unsigned int skipgramcount = 0;
            unsigned int flexgramcount = 0;

            for (iterator iter = this->begin(); iter != this->end(); iter++) {
                const PatternPointer pattern = iter->first;
                const PatternCategory category = pattern.category();
                const PatternPointer firstword = PatternPointer(pattern,0,1);
                if (!firstword.unknown() && (!firstword.isgap(0))) {
                    if ((category == SKIPGRAM) && (doskipgrams)) {
                        bool found = false;
                        for (std::vector<std::pair<uint32_t,unsigned char>>::iterator iter2 = matchskipgramhelper[firstword].begin(); iter2 != matchskipgramhelper[firstword].end(); iter2++) {
                            if ((iter2->first == pattern.getmask()) && (iter2->second == pattern.n())) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            matchskipgramhelper[firstword].push_back(std::pair<uint32_t,unsigned char>(pattern.getmask(),pattern.n()));
                            skipgramcount++;
                        }
                    } else if ((category == FLEXGRAM) && (doflexgrams)) {
                        matchflexgramhelper[firstword].insert(pattern);
                        flexgramcount++;
                    }
                }
            }
            for (t_matchskipgramhelper::iterator iter = matchskipgramhelper.begin(); iter != matchskipgramhelper.end(); iter++) {
                iter->second.shrink_to_fit();
            }
            if (!quiet && !matchskipgramhelper.empty()) std::cerr << "(helper structure has " << matchskipgramhelper.size() << " unigrams mapping to " << skipgramcount << " skipgrams total)" << std::endl;
            if (!quiet && !matchflexgramhelper.empty()) std::cerr << "(helper structure has " << matchflexgramhelper.size() << " unigrams mapping to " << flexgramcount << " flexgrams total)" << std::endl;
        }
    public:
        IndexedCorpus * reverseindex; ///< Pointer to the reverse index and corpus data for this model (or NULL)
        bool reverseindex_internal;
        bool hasskipgrams; ///< Does this model have skipgrams?
        bool hasflexgrams; ///< Does this model have flexgrams?

        /**
         * Begin a new pattern model, optionally pre-setting a reverseindex.
         */
        PatternModel<ValueType,ValueHandler,MapType,PatternType>(IndexedCorpus * corpus = NULL) {
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            hasskipgrams = false;
            hasflexgrams = false;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
                this->attachcorpus(*corpus);
            } else {
                this->reverseindex = NULL;
            }
            reverseindex_internal = false;
        }

        /**
         * Read a pattern model from an input stream
         * @param f The input stream
         * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
         * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
         * @param corpus Pointer to the loaded corpus, used as a reverse index.
         */
        PatternModel<ValueType,ValueHandler,MapType,PatternType>(std::istream *f, PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL) {
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            hasskipgrams = false;
            hasflexgrams = false;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            this->load(f,options,constrainmodel);
            if (corpus) {
                this->reverseindex = corpus;
                this->attachcorpus(*corpus);
            } else {
                this->reverseindex = NULL;
            }
            reverseindex_internal = false;
        }

        ~PatternModel<ValueType,ValueHandler,MapType,PatternType>() {
            if (reverseindex_internal && reverseindex != NULL) delete reverseindex;
        }
        /**
         * Read a pattern model from file
         * @param filename The input filename
         * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
         * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
         * @param corpus Pointer to the loaded corpus, used as a reverse index.
         */
        PatternModel<ValueType,ValueHandler,MapType,PatternType>(const std::string & filename, const PatternModelOptions & options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL) { //load from file
            //IndexedPatternModel will overload this
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            hasskipgrams = false;
            hasflexgrams = false;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
                this->attachcorpus(*corpus);
            } else {
                this->reverseindex = NULL;
            }
            reverseindex_internal = false;
            if (!options.QUIET) std::cerr << "Loading " << filename << std::endl;
            std::ifstream * in = new std::ifstream(filename.c_str());
            if (!in->good()) {
                std::cerr << "ERROR: Unable to load file " << filename << std::endl;
                throw InternalError();
            }
            this->load( (std::istream *) in, options, constrainmodel);
            in->close();
            delete in;
        }


        /**
        * Returns the type of model (a value from ModelType)
        */
        virtual int getmodeltype() const { return UNINDEXEDPATTERNMODEL; }
        /**
         * Returns the version of the model implementation and binary serialisation format
         */
        virtual int getmodelversion() const { return 2; }

        /**
         * Returns the number of distinct patterns in the model
         */
        virtual size_t size() const {
            return MapType::size();
        }

        /**
         * Checks whether the given pattern occurs in the model
         */
        virtual bool has(const Pattern & pattern) const {
            return MapType::has(pattern);
        }
        virtual bool has(const PatternPointer & pattern) const {
            return MapType::has(pattern);
        }

        /**
         * Read a pattern model from file
         * @param filename The input filename
         * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
         * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
         */
        virtual void load(std::string & filename, const PatternModelOptions & options, PatternModelInterface * constrainmodel = NULL) {
            if (!options.QUIET) std::cerr << "Loading " << filename << std::endl;
            std::ifstream * in = new std::ifstream(filename.c_str());
            if (!in->good()) {
                std::cerr << "ERROR: Unable to load file " << filename << std::endl;
                throw InternalError();
            }
            this->load( (std::istream *) in, options, constrainmodel);
            in->close();
            delete in;
        }

        /**
         * Read a pattern model from an input stream
         * @param f The input stream
         * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
         * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
         */
        virtual void load(std::istream * f, const PatternModelOptions & options, PatternModelInterface * constrainmodel = NULL) { //load from file
            char null;
            f->read( (char*) &null, sizeof(char));
            f->read( (char*) &model_type, sizeof(char));
            f->read( (char*) &model_version, sizeof(char));
            if (model_version == 1) this->classencodingversion = 1;
            if ((null != 0) || ((model_type != UNINDEXEDPATTERNMODEL) && (model_type != UNINDEXEDPATTERNPOINTERMODEL) && (model_type != INDEXEDPATTERNMODEL) && (model_type != INDEXEDPATTERNPOINTERMODEL) && (model_type != PATTERNALIGNMENTMODEL) ))  {
                std::cerr << "File is not a colibri model file (or a very old one)" << std::endl;
                throw InternalError();
            }
            if (model_version > 2) {
                std::cerr << "WARNING: Model is created with a newer version of Colibri Core! Attempting to continue but failure is likely..." << std::endl;
            }
            if (options.DEBUG) {
                std::cerr << "Debug enabled, loading PatternModel type " << (int) model_type << ", version " << (int) model_version << ", classencodingversion=" << (int) this->classencodingversion << std::endl;
            }
            if ((model_type == UNINDEXEDPATTERNPOINTERMODEL) || (model_type == INDEXEDPATTERNPOINTERMODEL)) {
                this->patterntype = PATTERNPOINTER;
                if (options.DEBUG) std::cerr << "Reading corpus data" << std::endl;
                unsigned int corpussize;
                f->read( (char*) &corpussize, sizeof(unsigned int));
                unsigned char * corpusdata = new unsigned char[corpussize];
                f->read((char*) corpusdata,sizeof(unsigned char) * corpussize);
                reverseindex = new IndexedCorpus(corpusdata, corpussize);
                this->attachcorpus(*reverseindex);
                reverseindex_internal = true;
                if (options.DEBUG) std::cerr << "(read " << corpussize << " bytes)" << std::endl;
            }
            f->read( (char*) &totaltokens, sizeof(uint64_t));
            f->read( (char*) &totaltypes, sizeof(uint64_t));

            PatternStoreInterface * constrainstore = NULL;
            if (constrainmodel) constrainstore = constrainmodel->getstoreinterface();

            if (options.DEBUG) {
                std::cerr << "Total tokens: " << totaltokens << ", total types: " << totaltypes << std::endl;;
            }


            if (((model_type == INDEXEDPATTERNMODEL) && (this->getmodeltype() == UNINDEXEDPATTERNMODEL)) || ((model_type == INDEXEDPATTERNPOINTERMODEL) && (this->getmodeltype() == UNINDEXEDPATTERNPOINTERMODEL)))  {
                //reading indexed pattern model as unindexed, (or indexed patternPOINTErmodels as unindexed patternPOINTERmodels)
                 MapType::template read<IndexedData,IndexedDataHandler,PatternType>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore,  !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET,   options.DEBUG);
            } else if ((model_type == UNINDEXEDPATTERNMODEL) && (this->getmodeltype() == INDEXEDPATTERNMODEL)) {
               //reading unindexed model as indexed, this will load the patterns but lose all the counts
                 MapType::template read<uint32_t,BaseValueHandler<uint32_t>,PatternType>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET,   options.DEBUG);
            } else if ((model_type == UNINDEXEDPATTERNPOINTERMODEL) && (this->getmodeltype() == UNINDEXEDPATTERNMODEL)) {
                 //reading unindexed pointermodel as unindexed patternmodel
                 MapType::template read<uint32_t,BaseValueHandler<uint32_t>,PatternPointer>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET,   options.DEBUG);
            } else if ((model_type == INDEXEDPATTERNPOINTERMODEL) && ((this->getmodeltype() == INDEXEDPATTERNMODEL) || (this->getmodeltype() == UNINDEXEDPATTERNMODEL))) {
                 //reading indexed patternpointermodel as (un)indexed patternmodel
                 MapType::template read<IndexedData,IndexedDataHandler,PatternPointer>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore,  !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET,   options.DEBUG);
            } else if (model_type == PATTERNALIGNMENTMODEL)  {
                 //reading pattern alignment model as pattern model, can be
                 //done, but semantics change:  count corresponds to the number of distinct alignments (for unindexed models)
                 //indexed models will lose all counts
                MapType::template read<PatternFeatureVectorMap<double>,PatternFeatureVectorMapHandler<double>,PatternType>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore,  !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS,options.DORESET,  options.DEBUG);
            } else {
                 MapType::template read(f, options.MINTOKENS,options.MINLENGTH, options.MAXLENGTH, constrainstore, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET,  options.DEBUG); //read PatternStore (also works for reading unindexed pattern models as indexed, which will load patterns but lose the counts)
            }
            this->postread(options);
        }

        /**
         * Returns a more generic but limited PatternModelInterface instance (polymorphism)
         */
        PatternModelInterface * getinterface() {
            return (PatternModelInterface*) this;
        }

        /**
         * Train a pattern model on corpus data (given an input stream)
         * @param in The input stream of the corpus data (*.colibri.dat), may be NULL if a reverse index is loaded.
         * @param options Options for training
         * @param constrainbymodel Pointer to another pattern model which should be used to constrain the training of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
         * @param filter A limited set of skipgrams/flexgrams to use as a filter, patterns will only be included if they are an instance of a skipgram in this list (i.e. disjunctive). Ngrams can also be included as filters, if a pattern subsumes one of the ngrams in the filter, it counts as a match (or if it matches it exactly).
         * @param continued Continued training on the same corpus data
         * @param firstsentence First sentence index, useful for augmenting a model with another corpus (keep continued set to false in this case), defaults to 1
         * @param ignoreerrors Try to ignore errors (use for debug only)
         */
        virtual void train(std::istream * in , PatternModelOptions options,  PatternModelInterface * constrainbymodel = NULL, PatternSet<> * filter = NULL, bool continued=false, uint32_t firstsentence=1, bool ignoreerrors=false) {
            if (options.MINTOKENS == -1) options.MINTOKENS = 2;
            if (options.MINTOKENS == 0)  options.MINTOKENS = 1;
            if (options.MINTOKENS_SKIPGRAMS < options.MINTOKENS) options.MINTOKENS_SKIPGRAMS = options.MINTOKENS;
            if (constrainbymodel == this) {
                totaltypes = 0;
                totaltokens = 0;
            } else if (constrainbymodel != NULL) {
                totaltypes = constrainbymodel->types();
                totaltokens = constrainbymodel->tokens();
            }
            uint32_t sentence = firstsentence-1;
            const unsigned char version = (in != NULL) ? getdataversion(in) : 2;

            bool filterhasngrams = false;
            bool filterhasskipgrams = false; //(or flexgrams)
            if (filter != NULL) {
                if (filter->size() == 0) { //cython will pass empty sets
                    filter = NULL;
                } else {
                    for (PatternSet<>::iterator iter = filter->begin(); iter != filter->end(); iter++) {
                        if (iter->category() == NGRAM) {
                            filterhasngrams = true;
                        } else {
                            filterhasskipgrams = true;
                        }
                        if (filterhasngrams && filterhasskipgrams) break;
                    }
                }
            }


            bool iter_unigramsonly = false; //only needed for counting unigrams when we need them but they would be discarded
            bool skipunigrams = false; //will be set to true later only when MINTOKENS=1,MINLENGTH=1 to prevent double counting of unigrams
            if (( (options.MINLENGTH > 1) ||(options.MINTOKENS == 1)) && (options.MINTOKENS_UNIGRAMS > options.MINTOKENS)) {
                iter_unigramsonly = true;
            }

            if (!options.QUIET) {
                std::cerr << "Training patternmodel";
                if (constrainbymodel != NULL) std::cerr << ", constrained by another model";
                std::cerr << ", occurrence threshold: " << options.MINTOKENS;
                if (iter_unigramsonly) std::cerr << ", secondary word occurrence threshold: " << options.MINTOKENS_UNIGRAMS;
                if (version < 2) std::cerr << ", class encoding version: " << (int) version;
                std::cerr << std::endl;
                if (filterhasngrams) {
                    std::cerr << "Filter with ngrams provided, only patterns that either match a filtered pattern or contain a smaller filtered pattern will be included..." << std::endl;
                }
                if (filterhasskipgrams) {
                    std::cerr << "Filter with skipgrams provided, only matching instances will be included..." << std::endl;
                }
            }

            if (constrainbymodel != NULL) {
                if ((options.DOSKIPGRAMS) && (!options.DOSKIPGRAMS_EXHAUSTIVE)) {
                    if (constrainbymodel != this) {
                        options.DOSKIPGRAMS = false;
                        options.DOSKIPGRAMS_EXHAUSTIVE = true;
                        if (!options.QUIET) std::cerr << "WARNING: Skipgrams will be extracted exhaustively on the basis of the ngrams found; the constraint model will be applied only afterwards. This implies some skipgrams in the constraint model that are present may be missed, and it will not be most efficient. Use in-place rebuilding of your constraint model instead." << std::endl;
                        if (options.MAXLENGTH >= 31) std::cerr << "WARNING: No maximum pattern length set or maximum pattern length is very high! This will drastically slow down skipgram computation" << std::endl;
                    }
                }
            }

            if (options.DOSKIPGRAMS && options.DOSKIPGRAMS_EXHAUSTIVE) {
                std::cerr << "ERROR: Both DOSKIPGRAMS as well as DOSKIPGRAMS_EXHAUSTIVE are set, this shouldn't happen, choose one." << std::endl;
                if (!ignoreerrors) throw InternalError();
                options.DOSKIPGRAMS = false;
            }

            std::vector<std::pair<PatternPointer,int> > ngrams;
            std::vector<PatternPointer> subngrams;
            bool found;
            IndexReference ref;
            int prevsize = this->size();
            if (constrainbymodel == this) prevsize = 0; //going over same model
            int backoffn = 0;
            Pattern * linepattern = NULL;

            if (!this->data.empty()) {
                if ((continued) && (!options.QUIET)) std::cerr << "Continuing training on preloaded model, computing statistics..." << std::endl;
                this->computestats();
            }

            for (int n = 1; n <= options.MAXLENGTH; n++) {
                bool skipgramsonly = false; //only used when continued==true, prevent double counting of n-grams whilst allowing skipgrams to be counted later
                if (continued) {
                    if ((options.MINTOKENS > 1) && (constrainbymodel == NULL)) {
                       if (cache_grouptotal[NGRAM][n] > 0) {
                           if ((options.DOSKIPGRAMS_EXHAUSTIVE) && (cache_grouptotal[SKIPGRAM][n] == 0) ) {
                               skipgramsonly= true;
                           } else {
                                if (!options.QUIET) std::cerr << "Skipping " << n << "-grams, already in model" << std::endl;
                               continue;
                           }
                       }
                    }
                }
                int foundngrams = 0;
                int foundskipgrams = 0;
                if (in != NULL) {
                    in->clear();
                    if (version >= 2) {
                        in->seekg(2);
                    } else {
                        in->seekg(0);
                    }
                }
                if (!options.QUIET) {
                    if (iter_unigramsonly) {
                        std::cerr << "Counting unigrams using secondary word occurrence threshold (" << options.MINTOKENS_UNIGRAMS << ")" << std::endl;
                    } else if (options.DOPATTERNPERLINE) {
                        std::cerr << "Counting patterns from list, one per line" << std::endl;
                    } else if (constrainbymodel != NULL) {
                        std::cerr << "Counting n-grams that occur in constraint model" << std::endl;
                    } else if (options.MINTOKENS > 1) {
                        std::cerr << "Counting " << n << "-grams" << std::endl;
                        if (skipgramsonly) std::cerr << "(only counting skipgrams actually, n-grams already counted earlier)" << std::endl;
                    } else {
                        std::cerr << "Counting *all* n-grams (occurrence threshold=1)" << std::endl;
                    }
                }

                if ((options.DOSKIPGRAMS_EXHAUSTIVE) && (gapmasks[n].empty())) gapmasks[n] = compute_skip_configurations(n, options.MAXSKIPS);



                sentence = firstsentence-1; //reset
                bool singlepass = false;
                bool ignorefilter = false;
                const unsigned int sentences = (reverseindex != NULL) ? reverseindex->sentences() : 0;
                while (((reverseindex != NULL) && (sentence < sentences)) ||  ((reverseindex == NULL) && (in != NULL) && (!in->eof())))  {
                    sentence++;
                    //read line
                    if (linepattern != NULL) delete linepattern;
                    if (reverseindex == NULL) linepattern = new Pattern(in,false,version);
                    PatternPointer line = (reverseindex != NULL) ? reverseindex->getsentence(sentence) : PatternPointer(linepattern);
                    //if (in->eof()) break;
                    const unsigned int linesize = line.n();
                    if (options.DEBUG) std::cerr << "Processing line " << sentence << ", size (tokens) " << linesize << " (bytes) " << line.bytesize() << ", n=" << n <<  std::endl;
                    if (linesize == 0) {
                        //skip empty lines
                        continue;
                    }
                    //count total tokens
                    if ((n==1) && (!continued)) totaltokens += linesize;


                    ngrams.clear();
                    ngrams.reserve(linesize);
                    if (options.DEBUG) std::cerr << "   (container ready)" << std::endl;

                    if (options.DOPATTERNPERLINE) {
                        if (linesize > (unsigned int) options.MAXLENGTH) continue;
                        ngrams.push_back(std::pair<PatternPointer,int>(line,0));
                    } else {
                        if (iter_unigramsonly) {
                            line.ngrams(ngrams, n);
                        } else if ((options.MINTOKENS > 1) && (constrainbymodel == NULL)) {
                            line.ngrams(ngrams, n);
                        } else {
                            singlepass = true;
                            int minlength = options.MINLENGTH;
                            if (continued) minlength = this->maxn + 1;
                            line.subngrams(ngrams,minlength,options.MAXLENGTH); //extract ALL ngrams if MINTOKENS == 1 or a constraint model is set, no need to look back anyway, only one iteration over corpus
                        }
                    }
                    if (options.DEBUG) std::cerr << "\t" << ngrams.size() << " ngrams in line" << std::endl;


                    // *** ITERATION OVER ALL NGRAMS OF CURRENT ORDER (n) IN THE LINE/SENTENCE ***
                    for (std::vector<std::pair<PatternPointer,int>>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {

                        try {
                            if ((singlepass) && (options.MINLENGTH == 1) && (skipunigrams) && (iter->first.n() == 1)) {
                                //prevent double counting of unigrams after a iter_unigramsonly run with mintokens==1
                                continue;
                            }


                            if (!skipgramsonly) {
                                //check against constraint model
                                if ((constrainbymodel != NULL) && (!iter_unigramsonly) && (!constrainbymodel->has(iter->first)))  continue;  //skip when we have a constraintmodel and the pattern is not found

                                found = true; //are the submatches in order? (default to true, attempt to falsify, needed for mintokens==1)

                                //unigram check, special scenario, not usually processed!! (normal lookback suffices for most uses)
                                if ((!iter_unigramsonly) && (options.MINTOKENS_UNIGRAMS > options.MINTOKENS) && ((n > 1) || (singlepass)) ) {
                                    subngrams.clear();
                                    iter->first.ngrams(subngrams,1); //get all unigrams
                                    for (std::vector<PatternPointer>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {
                                        //check if unigram reaches threshold
                                        if (this->occurrencecount(*iter2) < (unsigned int) options.MINTOKENS_UNIGRAMS) {
                                            found = false;
                                            break;
                                        }
                                    }
                                }

                                if ((found) && (filter != NULL) && (constrainbymodel == NULL)) {
                                    //special behaviour: filter
                                    ignorefilter = false;
                                    if (((options.MINTOKENS > 1) && (n >= options.MINLENGTH)) || ((options.MINTOKENS == 1) && (iter->first.n() >= (unsigned int) options.MINLENGTH))) {
                                        //we only apply the filter if minlength is satisfied, earlier stages are not filtered (but will be pruned later when no longer needed)
                                        bool matchfilter = false;
                                        if (filterhasngrams) {
                                            subngrams.clear();
                                            iter->first.subngrams(subngrams,1, options.MINTOKENS > 1 ? n : iter->first.n());
                                            for (std::vector<PatternPointer>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {
                                                if (filter->has(*iter2)) {
                                                    matchfilter = true;
                                                    break;
                                                }
                                            }
                                        }
                                        if ((!matchfilter) && (filterhasskipgrams)) {
                                            for (PatternSet<Pattern>::iterator iter2 = filter->begin(); iter2 != filter->end(); iter2++) {
                                                if (iter->first.instanceof(*iter2)) {
                                                    matchfilter = true;
                                                    break;
                                                }
                                            }
                                        }
                                        if (!matchfilter) continue;
                                    } else {
                                        ignorefilter = true;
                                    }
                                }

                                if ((filter == NULL) || (ignorefilter))  {
                                    //normal behaviour: ngram (n-1) lookback
                                    if ((found) && (n > 1) && (options.MINTOKENS > 1) && (!options.DOPATTERNPERLINE) && (constrainbymodel == NULL)) {
                                        //check if sub-parts were counted
                                        subngrams.clear();
                                        backoffn = n - 1;
                                        if (backoffn > options.MAXBACKOFFLENGTH) backoffn = options.MAXBACKOFFLENGTH;
                                            iter->first.ngrams(subngrams, backoffn);
                                        for (std::vector<PatternPointer>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {
                                            if (!this->has(*iter2)) {
                                                found = false;
                                                break;
                                            }
                                        }
                                    }
                                }


                                ref = IndexReference(sentence, iter->second); //this is one token, we add the tokens as we find them, one by one
                                if ((found) && (!skipgramsonly)) {
                                    if (options.DEBUG) std::cerr << "\t\tAdding @" << ref.sentence << ":" << ref.token << " n=" << iter->first.n() << " category=" <<(int) iter->first.category()<< std::endl;
                                    add(iter->first, ref);
                                }

                            }
                            if (((n >= 3) || (options.MINTOKENS == 1)) //n is always 1 when mintokens == 1
                                    && (options.DOSKIPGRAMS_EXHAUSTIVE)) {
                                //if we have a constraint model, we need to compute skipgrams even if the ngram was not found
                                ref = IndexReference(sentence, iter->second); //this is one token, we add the tokens as we find them, one by one
                                int foundskipgrams_thisround = this->computeskipgrams(iter->first, options, &ref, NULL, constrainbymodel, true );
                                if (foundskipgrams_thisround > 0) hasskipgrams = true;
                                foundskipgrams += foundskipgrams_thisround;
                            }
                        } catch (std::exception &e) {
                            std::cerr << "ERROR: An internal error has occured during training!!!" << std::endl;
                            if (ignoreerrors) continue;
                            throw InternalError();
                        }
                    }

                }


                if (!iter_unigramsonly) {
                    foundngrams = this->size() - foundskipgrams - prevsize;

                    if ((foundngrams) || (foundskipgrams)) {
                        if (n > this->maxn) this->maxn = n;
                        if (n < this->minn) this->minn = n;
                    } else {
                        if (!options.QUIET) std::cerr << "None found" << std::endl;
                        if (!continued) break;
                    }
                    if (!options.QUIET) std::cerr << " Found " << foundngrams << " ngrams...";
                    if (options.DOSKIPGRAMS_EXHAUSTIVE && !options.QUIET) std::cerr << foundskipgrams << " skipgram occurrences...";
					if ((!continued) && ((constrainbymodel == NULL) or (constrainbymodel == this))) {
						if ((options.MINTOKENS > 1) && (n == 1)) {
							totaltypes = this->size(); //total unigrams, also those not in model
						} else if ((options.MINTOKENS == 1) && (options.MINLENGTH == 1)) {
							if (!options.QUIET) std::cerr << " computing total word types prior to pruning...";
							totaltypes = totalwordtypesingroup(NGRAM,1);
							if (!options.QUIET) std::cerr << totaltypes << "...";
						}
					}
                    unsigned int pruned;
                    if (singlepass) {
                        if (options.DOSKIPGRAMS) {
                            pruned = this->prune(options.MINTOKENS,0, NGRAM); //prune regardless of size, but only n-grams (if we have a constraint model with skipgrams with count 0, we need that later and can't prune it)
                        } else {
                            pruned = this->prune(options.MINTOKENS,0); //prune regardless of size
                        }
                    } else {
                        pruned = this->prune(options.MINTOKENS,n); //prune only in size-class
                        if ( (!options.DOSKIPGRAMS) && (!options.DOSKIPGRAMS_EXHAUSTIVE) &&  ( n - 1 >= 1) &&  ( (n - 1) < options.MINLENGTH) && (n - 1 != options.MAXBACKOFFLENGTH) &&
                            !( (n-1 == 1) && (options.MINTOKENS_UNIGRAMS > options.MINTOKENS)  ) //don't delete unigrams if we're gonna need them
                            ) {
                            //we don't need n-1 anymore now we're done with n, it
                            //is below our threshold, prune it all (== -1)
                            this->prune(-1, n-1);
                            if (!options.QUIET) std::cerr << " (pruned last iteration due to minimum length) ";
                        }
                    }
                    if (!options.QUIET) std::cerr << "pruned " << pruned;
                    if (foundskipgrams) {
                        unsigned int prunedextra;
                        if ((options.MINTOKENS == 1) || (constrainbymodel != NULL)) {
                            prunedextra = this->pruneskipgrams(options.MINTOKENS_SKIPGRAMS, options.MINSKIPTYPES, 0);
                        } else {
                            prunedextra = this->pruneskipgrams(options.MINTOKENS_SKIPGRAMS, options.MINSKIPTYPES, n);
                        }
                        if (prunedextra && !options.QUIET) std::cerr << " plus " << prunedextra << " extra skipgrams..";
                        pruned += prunedextra;
                    }
                    if (!options.QUIET) std::cerr << "...total kept: " << (foundngrams + foundskipgrams) - pruned << std::endl;
                    if (((options.MINTOKENS == 1) || (constrainbymodel != NULL))) break; //no need for further n iterations, we did all in one pass since there's no point in looking back
                } else { //iter_unigramsonly
                    if (!options.QUIET) {
                        std::cerr <<  "found " << this->size() << std::endl;
                    }

					if ((!continued) && ((constrainbymodel == NULL) || (constrainbymodel == this))) {
						if (!options.QUIET) std::cerr << " computing total word types prior to pruning...";
						totaltypes = this->size();
						if (!options.QUIET) std::cerr << totaltypes << "...";
					}
                    //prune the unigrams based on the word occurrence threshold
                    this->prune(options.MINTOKENS_UNIGRAMS,1);
                    //normal behaviour next round
                    iter_unigramsonly = false;
                    if ((n == 1) && (options.MINLENGTH ==1)) skipunigrams = true; //prevent double counting of unigrams
                    //decrease n so it will be the same (always 1) next (and by definition last) iteration
                    n--;
                }
                prevsize = this->size();
            }
            if (options.DOSKIPGRAMS && !options.DOSKIPGRAMS_EXHAUSTIVE) {
                this->trainskipgrams(options, constrainbymodel);
            }
            if (options.MINTOKENS == 1) {
                //needed to compute maxn, minn
                this->postread(options);
            }
            if (options.MAXBACKOFFLENGTH < options.MINLENGTH) {
                this->prune(-1, options.MAXBACKOFFLENGTH);
            }
            if ((options.MINLENGTH > 1) && (options.MINTOKENS_UNIGRAMS > options.MINTOKENS)) {
                //prune the unigrams again
                this->prune(-1,1);
            }
            if (options.PRUNENONSUBSUMED) {
                if (!options.QUIET) std::cerr << "Pruning non-subsumed n-grams"  << std::endl;
                int begin_n = options.PRUNENONSUBSUMED;
                if ((begin_n > options.MAXLENGTH)) begin_n = options.MAXLENGTH;
                for (int n = begin_n; n > 1; n--) {
                    std::unordered_set<Pattern> subsumed;
                    unsigned int prunednonsubsumed = 0;
                    PatternModel::iterator iter = this->begin();
                    while (iter != this->end()) {
                        const unsigned int pattern_n = iter->first.n();
                        if (pattern_n == (unsigned int) n) {
                            subngrams.clear();
                            iter->first.ngrams(subngrams, n-1);
                            for (std::vector<PatternPointer>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) subsumed.insert(Pattern(*iter2));
                        }
                        iter++;
                    };
                    prunednonsubsumed += this->prunenotinset(subsumed, n-1);
                    if (!options.QUIET) std::cerr << " pruned " << prunednonsubsumed << " non-subsumed " << (n-1) << "-grams"  << std::endl;
                }
            }
            if ((options.MINLENGTH > 1) && (options.DOSKIPGRAMS || options.DOSKIPGRAMS_EXHAUSTIVE))  {
                unsigned int pruned = this->prunebylength(options.MINLENGTH-1);
                if (!options.QUIET) std::cerr << " pruned " << pruned << " patterns below minimum length (" << options.MINLENGTH << ")"  << std::endl;
            }
            this->posttrain(options);
            if (linepattern != NULL) delete linepattern;
        }


        /**
         * Train a pattern model on corpus data
         * @param filename The filename of the corpus data (*.colibri.dat)
         * @param options Options for training
         * @param constrainbymodel Pointer to another pattern model which should be used to constrain the training of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
         */
        virtual void train(const std::string & filename, PatternModelOptions options, PatternModelInterface * constrainbymodel = NULL, PatternSet<> * filter = NULL,  bool continued=false, uint32_t firstsentence=1, bool ignoreerrors=false) {
            if ((filename.size() > 3) && (filename.substr(filename.size()-3) == ".bz2")) {
                std::ifstream * in = new std::ifstream(filename.c_str(), std::ios::in|std::ios::binary);
                bz2istream * decompressor = new bz2istream(in->rdbuf());
                this->train( (std::istream*) decompressor, options, constrainbymodel, filter, continued, firstsentence, ignoreerrors);
                delete decompressor;
                delete in;
            } else {
                std::ifstream * in = new std::ifstream(filename.c_str());
                this->train((std::istream*) in, options, constrainbymodel, filter, continued, firstsentence, ignoreerrors);
                in->close();
                delete in;
            }
        }


        /**
         * Low-level function to compute skipgrams for a given pattern . See
         * higher-level function instead
         */
        virtual int computeskipgrams(const PatternPointer & pattern, int mintokens = 2,  const IndexReference * singleref= NULL, const IndexedData * multiplerefs = NULL,  PatternModelInterface * constrainbymodel = NULL, std::vector<PatternPointer> * targetcontainer = NULL,  const bool exhaustive = false, const int maxskips = 3, const bool DEBUG = false) {

            //if targetcontainer is NULL, skipgrams will be added to the model,
            // if not null , they will be added to the targetcontainer instead


            if (mintokens == -1) mintokens = 2;;
            if (mintokens  <= 1) {
                mintokens = 1;
            }
            //internal function for computing skipgrams for a single pattern
            int foundskipgrams = 0;
            const int n = pattern.n();
            std::vector<PatternPointer> subngrams;

            if (gapmasks[n].empty()) gapmasks[n] = compute_skip_configurations(n, maxskips);

            if (DEBUG) std::cerr << "Computing skipgrams (" << gapmasks[n].size() << " permutations)" << std::endl;

            //loop over all possible gap configurations
            int gapconf_i = 0;
            for (std::vector<uint32_t>::iterator iter2 =  gapmasks[n].begin(); iter2 != gapmasks[n].end(); iter2++, gapconf_i++) {
                if (*iter2 == 0) continue; //precaution (doesn't really happen anyway, but better safe than sorry)

                //add skips
                try {
                    PatternPointer skipgram = pattern;
                    skipgram.mask = *iter2;

                    if (DEBUG) {
                        std::cerr << "Checking for skipgram: " << std::endl;
                        skipgram.out();
                    }

                    if ((constrainbymodel != NULL) && (!constrainbymodel->has(skipgram))) continue;

                    if (DEBUG) {
                        if ((int) skipgram.n() != n) {
                            std::cerr << "Generated invalid skipgram, n=" << skipgram.n() << ", expected " << n << std::endl;
                            throw InternalError();
                        }
                    }

                    bool skipgram_valid = true;
                    if ((mintokens != 1) && (constrainbymodel == NULL)) {
                        bool check_extra = false;
                        //check if sub-parts were counted
                        subngrams.clear();
                        skipgram.ngrams(subngrams,n-1); //this also works for and returns skipgrams, despite the name
                        for (std::vector<PatternPointer>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) { //only two patterns
                            const PatternPointer subpattern = *iter2;
                            if (!subpattern.isgap(0) && !subpattern.isgap(subpattern.n() - 1)) {
                                //this subpattern is a valid
                                //skipgram or ngram (no beginning or ending
                                //gaps) that should occur
                                if (DEBUG) {
                                    std::cerr << "Subpattern: " << std::endl;
                                    subpattern.out();
                                }
                                if (!this->has(subpattern)) {
                                    if (DEBUG) std::cerr << "  discarded" << std::endl;
                                    skipgram_valid = false;
                                    break;
                                }
                            } else {
                                //this check isn't enough, subpattern
                                //starts or ends with gap
                                //do additional checks
                                check_extra = true;
                                break;
                            }
                        }
                        if (!skipgram_valid) continue;


                        if (check_extra) {
                            if (exhaustive) { //the following is by definition the case in non-exhaustive mode, so we need only do it in exhaustive mode:

                                //test whether parts occur in model, otherwise skip
                                //can't occur either and we can discard it
                                std::vector<PatternPointer> parts;
                                skipgram.parts(parts);
                                for (std::vector<PatternPointer>::iterator iter3 = parts.begin(); iter3 != parts.end(); iter3++) {
                                    const PatternPointer part = *iter3;
                                    if (!this->has(part)) {
                                        skipgram_valid = false;
                                        break;
                                    }
                                }
                                if (!skipgram_valid) continue;
                            }

                            //check whether the gaps with single token context (X * Y) occur in model,
                            //otherwise skipgram can't occur
                            const std::vector<std::pair<int,int>> gapconfiguration = mask2vector(skipgram.mask, n);
                            for (std::vector<std::pair<int,int>>::const_iterator iter3 = gapconfiguration.begin(); iter3 != gapconfiguration.end(); iter3++) {
                                if (!((iter3->first - 1 == 0) && (iter3->first + iter3->second + 1 == n))) { //entire skipgram is already X * Y format
                                    const PatternPointer subskipgram = PatternPointer(skipgram, iter3->first - 1, iter3->second + 2);
                                    if (DEBUG) {
                                        std::cerr << "Subskipgram: " << std::endl;
                                        subskipgram.out();
                                    }
                                    if (!this->has(subskipgram)) {
                                        if (DEBUG) std::cerr << "  discarded" << std::endl;
                                        skipgram_valid = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }


                    if (skipgram_valid) {
                        /*Pattern test = skipgram;
                        if (test.category() != SKIPGRAM) {
                            std::cerr << "ERROR: skipgram is not a skipgram!!!" << std::endl;
                            std::cerr << "n=" << test.n() << std::endl;
                            std::cerr << "mask=" << skipgram.mask << std::endl;
                            std::cerr << "patternpointer out: " ;
                            skipgram.out();
                            std::cerr << "pattern out: ";
                            test.out();
                        }*/
                        if (DEBUG) std::cerr << "  counted!" << std::endl;
                        if (targetcontainer == NULL) {
                            //put in model
                            if  (!has(skipgram)) foundskipgrams++;
                            if (singleref != NULL) {
                                add(skipgram, *singleref ); //counts the actual skipgram, will add it to the model
                            } else if (multiplerefs != NULL) {
                                for (IndexedData::const_iterator refiter =  multiplerefs->begin(); refiter != multiplerefs->end(); refiter++) {
                                    const IndexReference ref = *refiter;
                                    add(skipgram, ref ); //counts the actual skipgram, will add it to the model
                                }
                            } else {
                                std::cerr << "ERROR: computeskipgrams() called with no singleref, no multiplerefs and no targetcontainer" << std::endl;
                                throw InternalError();
                            }
                        } else {
                            //put in target container, may contain duplicates
                            foundskipgrams++;
                            targetcontainer->push_back(skipgram);
                        }

                    }

                } catch (InternalError &e) {
                    std::cerr << "IGNORING ERROR and continuing with next skipgram" << std::endl;
                }
            }
            return foundskipgrams;
        }

        /**
         * Low-level function to compute skipgrams for a given pattern. See trainskipgrams() instead.
         */
        virtual int computeskipgrams(const PatternPointer & pattern, PatternModelOptions & options ,  const IndexReference * singleref= NULL, const IndexedData * multiplerefs = NULL,  PatternModelInterface * constrainbymodel = NULL, const bool exhaustive = false) { //backward compatibility
            if (options.MINTOKENS_SKIPGRAMS < options.MINTOKENS) options.MINTOKENS_SKIPGRAMS = options.MINTOKENS;
            return computeskipgrams(pattern, options.MINTOKENS_SKIPGRAMS, singleref, multiplerefs, constrainbymodel, NULL, exhaustive, options.MAXSKIPS,options.DEBUG);
        }

        /**
        * Returns a vector of all skipgrams  that can be extracted from the
        * given pattern
         */
        virtual std::vector<PatternPointer> findskipgrams(const PatternPointer & pattern, unsigned int occurrencethreshold = 1, int maxskips = 3) {
            //given the pattern, find all skipgrams in it that occur in the model

            std::vector<PatternPointer> skipgrams;
            this->computeskipgrams(pattern, occurrencethreshold, NULL, NULL, this->getinterface(), &skipgrams, false, maxskips);
            return skipgrams;
        }


        /**
         * Train skipgrams, for indexed models only
         */
        virtual void trainskipgrams(const PatternModelOptions options,  PatternModelInterface * constrainbymodel = NULL) {
            if (constrainbymodel == this) {
                trainskipgrams_selfconstrained(options);
            } else {
                std::cerr << "Can not compute skipgrams on unindexed model (except exhaustively during train() )" << std::endl;
                throw InternalError();
            }
        }

        /*
         * Train skipgrams *and flexgrams* for self-constrained model (in-place
         * rebuild). The model already needs to have skipgrams and/or flexgrams loaded with no
         * occurrence count. This will iterate over the corpusdata (reverseindex) again and find and count all skipgrams/flexgrams in the data.
         * Will be invoked by trainskipgrams usually.
         */
        virtual void trainskipgrams_selfconstrained(PatternModelOptions options) {
            if (options.MINTOKENS == -1) options.MINTOKENS = 2;
            this->cache_grouptotal.clear(); //forces recomputation of statistics
            if (!options.QUIET) std::cerr << "Finding skipgrams/flexgrams matching preloaded constraints, occurrence threshold " << options.MINTOKENS << " ..." << std::endl;
            unsigned int foundskipgrams = 0;
            unsigned int foundflexgrams = 0;
            for (IndexedCorpus::iterator iter = this->reverseindex->begin(); iter != this->reverseindex->end(); iter++) {
                const IndexReference ref = iter.index();
                std::unordered_set<PatternPointer> skipgrams = this->getreverseindex(ref,0, SKIPGRAMORFLEXGRAM);  //will also match flexgrams
                if (options.DEBUG && !skipgrams.empty()) std::cerr << skipgrams.size() << " skipgrams found @" << ref << std::endl;
                for (std::unordered_set<PatternPointer>::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++) {
                    add(*iter,ref); //add to model
                    if (iter->category() == SKIPGRAM) {
                        foundskipgrams++;
                    } else {
                        foundflexgrams++;
                    }
                }
            }
            if ((!foundskipgrams) && (!foundflexgrams)) {
                std::cerr << " None found" << std::endl;
            } else {
                this->hasskipgrams = (foundskipgrams > 1);
                this->hasflexgrams = (foundflexgrams > 1);
                if (!options.QUIET) std::cerr << " Found " << foundskipgrams << " skipgrams, " << foundflexgrams << " flexgrams ...";
            }
            unsigned int pruned = this->prune(options.MINTOKENS);
            if (!options.QUIET) std::cerr << " pruned " << pruned << std::endl;;
        }

        //creates a new test model using the current model as training
        // i.e. only fragments existing in the training model are counted
        // remaining fragments are 'uncovered'
        void test(MapType & target, std::istream * in);

        /**
         * Write the pattern model to output stream
         */
        void write(std::ostream * out) {
            const char null = 0;
            out->write( (char*) &null, sizeof(char));
            unsigned char t = this->getmodeltype();
            out->write( (char*) &t, sizeof(char));
            unsigned char v = this->getmodelversion();
            out->write( (char*) &v, sizeof(char));
            if ((this->getmodeltype()== UNINDEXEDPATTERNPOINTERMODEL) || (this->getmodeltype() == INDEXEDPATTERNPOINTERMODEL)) {
                out->write( (char*) &this->corpussize, sizeof(unsigned int));
                out->write((char*) this->corpusstart, sizeof(unsigned char) * this->corpussize);
            }
            out->write( (char*) &totaltokens, sizeof(uint64_t));
            const uint64_t tp = this->types(); //use this instead of totaltypes, as it may need to be computed on-the-fly still
            out->write( (char*) &tp, sizeof(uint64_t));
            MapType::write(out); //write PatternStore
        }

        /**
         * Save the entire pattern model to file
         */
        void write(const std::string filename) {
            std::ofstream * out = new std::ofstream(filename.c_str());
            this->write(out);
            out->close();
            delete out;
        }

        typedef typename MapType::iterator iterator;
        typedef typename MapType::const_iterator const_iterator;

        /**
         * Returns the maximum length of patterns in this model
         */
        virtual int maxlength() const { return this->maxn; };
        /**
         * Returns the minimum length of patterns in this model
         */
        virtual int minlength() const { return this->minn; };
        /**
         * Returns the occurrenc count of the specified pattern, will return 0
         * if it does not exist in the model
         */
        virtual unsigned int occurrencecount(const Pattern & pattern)  {
            ValueType * data = this->getdata(pattern);
            if (data != NULL) {
                return this->valuehandler.count(*data);
            } else {
                return 0;
            }
        }

        virtual unsigned int occurrencecount(const PatternPointer & pattern)  {
            ValueType * data = this->getdata(pattern);
            if (data != NULL) {
                return this->valuehandler.count(*data);
            } else {
                return 0;
            }
        }

        /**
         * Get the value stored for the specified pattern.
         * @param makeifnew Add the pattern with empty value if it does not exist (default: false)
         **/
        virtual ValueType * getdata(const Pattern & pattern, bool makeifnew=false) {
            typename MapType::iterator iter = this->find(pattern);
            if (iter != this->end()) {
                return &(iter->second);
            } else if (makeifnew) {
                return &((*this)[pattern]);
            } else {
                return NULL;
            }
        }

        virtual ValueType * getdata(const PatternPointer & pattern, bool makeifnew=false) {
            typename MapType::iterator iter = this->find(pattern);
            if (iter != this->end()) {
                return &(iter->second);
            } else if (makeifnew) {
                return &((*this)[pattern]);
            } else {
                return NULL;
            }
        }

        /**
         * Return the total amount of word/unigram types in the model
         */
        virtual unsigned int types() {
            if ((totaltypes == 0) && (!this->data.empty())) totaltypes = this->totalwordtypesingroup(0, 0);
            return totaltypes;
        }

        /**
         * Return the total amount of word/unigram tokens in the model
         */
        virtual unsigned int tokens() const { return totaltokens; }

        unsigned char type() const { return model_type; }
        unsigned char version() const { return model_version; }

        void output(std::ostream *);


        /**
         * Returns the coverage count for the given pattern, for unindexed
         * models, the coverage count is a mere maximum projection equal to the
         * product of the occurence count and the size.
         */
        unsigned int coveragecount(const Pattern &  key) {
           return this->occurrencecount(key) * key.size();
        }
        /**
         * Return coverage as a fraction of the total number of tokens in the
         * model. For unindexed models this is a maximal projection rather than
         * exact number.
         */
        double coverage(const Pattern & key) {
            return this->coveragecount(key) / (double) this->tokens();
        }

        /**
         * Given a position in the corpus , return a set of all the patterns (including skipgrams and flexgrams) that cover this position and are in the model.
         * Note that the flexgrams are computed from skipgrams and the maximum length is therefore constrained to 31 words.
         * @param ref The position in the corpus
         * @param occurrencecount If set above zero, filters to only include patterns occurring above or equal to this threshold
         * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to include only this category. Set to 0 for unfiltered (default)
         * @param size Set to any value above zero to only include patterns of the specified length.
         * @param maxskips Maximum number of skips in a skipgram
         */
        std::unordered_set<PatternPointer> getreverseindex(const IndexReference ref, int occurrencecount = 0, int category = 0, unsigned int size = 0, int = 3) {

            std::unordered_set<PatternPointer> result;
            if (!this->reverseindex) return result;

            bool includeskipgrams = false;
            bool includeflexgrams = false;


            if (category != NGRAM) {
                this->compute_matchhelpers(); //computes only ONCE (and caches) a datastructure to aid skipgram matching, mapping unigrams (first words) to (mask,length,isflexgram) tuples
                includeskipgrams = ((category != FLEXGRAM) && (this->hasskipgrams) && (!matchskipgramhelper.empty()));
                includeflexgrams = ((category != SKIPGRAM) && (this->hasflexgrams) && (!matchskipgramhelper.empty()));
            }

            const unsigned int sl = this->reverseindex->sentencelength(ref.sentence);
            //std::cerr << "DEBUG: getreverseindex sentencelength(" << ref.sentence << ")=" << sl << std::endl;
            const unsigned int minn = this->minlength();
            const unsigned int maxn = this->maxlength();
            for (unsigned int n = minn; ref.token + n <= sl && n <= maxn; n++) {
                if ((size == 0) || (n == size)) {
                    try {
                        //std::cerr << "DEBUG: getreverseindex getpattern " << ref.tostring() << " + " << n << std::endl;
                        const PatternPointer ngram = this->reverseindex->getpattern(ref,n);
                        /*std::cerr << "n: " << ngram.n() << std::endl;
                        std::cerr << "bytesize: " << ngram.bytesize() << std::endl;;
                        std::cerr << "hash: " << ngram.hash() << std::endl;*/
                        if ( (((occurrencecount == 0) && this->has(ngram)) || ((occurrencecount != 0) && (this->occurrencecount(ngram) >= (unsigned int) occurrencecount)))
                            && ((category == 0) || (ngram.category() == category)) ) {
                                result.insert(ngram);
                        }

                        if ((includeskipgrams) && (n >= 3))  {

                            //(we can't use gettemplates() because
                            //gettemplates() depends on us to do the actual
                            //work, we use the matchskipgramhelper
                            //datastructure, mapping unigrams (first words) to
                            //(mask,length) tuples

                            const PatternPointer firstword = PatternPointer(ngram,0,1);


                            t_matchskipgramhelper::iterator iter = this->matchskipgramhelper.find(firstword);
                            if (iter != matchskipgramhelper.end()) {
                                for (std::vector<std::pair<uint32_t,unsigned char>>::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
                                    if (n == iter2->second) {
                                        PatternPointer skipgram = ngram;
                                        skipgram.mask = iter2->first;
                                        if ( ((occurrencecount == 0) && this->has(skipgram)) || ((occurrencecount != 0) && (this->occurrencecount(skipgram) >= (unsigned int) occurrencecount)) ) {
                                            result.insert(skipgram);
                                        }
                                    }
                                }
                            }

                        }
                    } catch (KeyError &e) {
                        break;
                    }
                }
            }

            if (includeflexgrams) {
                try{
                    const PatternPointer firstword = this->reverseindex->getpattern(ref,1);
                    t_matchflexgramhelper::iterator iter = this->matchflexgramhelper.find(firstword);
                    if (iter != this->matchflexgramhelper.end()) {
                        for (std::unordered_set<PatternPointer>::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
                            PatternPointer flexgram = *iter2;
                            try  {
                                flexgram = this->reverseindex->getflexgram(ref, flexgram);
                                if ( ((occurrencecount == 0) && this->has(flexgram)) || ((occurrencecount != 0) && (this->occurrencecount(flexgram) >= (unsigned int) occurrencecount)) ) {
                                    result.insert(flexgram);
                                }
                            } catch (KeyError &e) {
                                ;
                            }
                        }
                    }
                } catch (KeyError &e) {
                    ;
                }
            }
            return result;
        }

        /*std::vector<Pattern> getreverseindex_bysentence(int sentence) {
            //Auxiliary function
            std::vector<Pattern> result;
            for (int i = 0; i < this->reverseindex.sentencelength(sentence); i++) {
                const IndexReference ref = IndexReference(sentence, i);
                std::vector<Pattern> tmpresult =  this->getreverseindex(ref);
                for (std::vector<Pattern>::iterator iter = tmpresult.begin(); iter != tmpresult.end(); iter++) {
                    const Pattern pattern = *iter;
                    result.push_back(pattern);
                }
            }
            return result;
        }*/

        /**
         * Returns pairs of positions and patterns, consisting of all patterns
         * from the model found in the specified sentence (or whatever unit delimites your
         * corpus)
         * @param sentence The sentence index (starts at 1)
         * @param occurrencecount If set above zero, filters to only include patterns occurring above this threshold
         * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to include only this category. Set to 0 for unfiltered (default)
         * @param size Set to any value above zero to only include patterns of the specified length.
         */
        std::vector<std::pair<IndexReference,PatternPointer>> getreverseindex_bysentence(int sentence, int occurrencecount = 0, int category = 0, unsigned int size = 0) {
            //Auxiliary function
            std::vector<std::pair<IndexReference,PatternPointer>> result;
            for (int i = 0; i < this->reverseindex->sentencelength(sentence); i++) {
                const IndexReference ref = IndexReference(sentence, i);
                std::unordered_set<PatternPointer> tmpresult =  this->getreverseindex(ref, occurrencecount, category, size);
                for (std::unordered_set<PatternPointer>::iterator iter = tmpresult.begin(); iter != tmpresult.end(); iter++) {
                    const PatternPointer pattern = *iter;
                    result.push_back(std::pair<IndexReference,PatternPointer>(ref,pattern));
                }
            }
            return result;
        }

        /**
         * Given a position in the corpus , return a vector of all the positions and patterns (as pairs) that occur to the right of this position
         * @param ref The position in the corpus
         */
        std::vector<std::pair<IndexReference,PatternPointer>> getreverseindex_right(const IndexReference ref,int occurrencecount = 0, int category = 0, unsigned int size = 0) {
            //Auxiliary function
            std::vector<std::pair<IndexReference,PatternPointer>> result;
            for (int i = ref.token+1; i < this->reverseindex->sentencelength(ref.sentence); i++) {
                const IndexReference ref2 = IndexReference(ref.sentence, i);
                std::unordered_set<PatternPointer> tmpresult =  this->getreverseindex(ref, occurrencecount, category, size);
                for (std::unordered_set<PatternPointer>::iterator iter = tmpresult.begin(); iter != tmpresult.end(); iter++) {
                    const PatternPointer pattern = *iter;
                    result.push_back(std::pair<IndexReference,PatternPointer>(ref2,pattern));
                }
            }
            return result;
        }

        /**
         * Given a position in the corpus , return a vector of all the positions and patterns (as pairs) that occur to the left of this position
         * @param ref The position in the corpus
         */
        std::vector<std::pair<IndexReference,PatternPointer>> getreverseindex_left(const IndexReference ref, int occurrencecount = 0, int category = 0, unsigned int size = 0) {
            //Auxiliary function
            std::vector<std::pair<IndexReference,PatternPointer>> result;
            for (int i = 0; i < ref.token; i++) {
                const IndexReference ref2 = IndexReference(ref.sentence, i);
                std::unordered_set<PatternPointer> tmpresult =  this->getreverseindex(ref,occurrencecount, category, size);
                for (std::unordered_set<PatternPointer>::iterator iter = tmpresult.begin(); iter != tmpresult.end(); iter++) {
                    const PatternPointer pattern = *iter;
                    result.push_back(std::pair<IndexReference,PatternPointer>(ref2,pattern));
                }
            }
            return result;
        }

        /**
         * Compute statistics on the model, will generally be called
         * automatically by methods who use it, and the statistics are cached
         * after computation.
         */
        void computestats() {
            cache_categories.clear();
            cache_n.clear();
            cache_grouptotal.clear();
            cache_grouptotalpatterns.clear();
            cache_categories.insert(0);
            cache_n.insert(0);
            PatternModel::iterator iter = this->begin();
            while (iter != this->end()) {
                const PatternType pattern = iter->first;
                const int c = pattern.category();
                cache_categories.insert(c);
                const int n = pattern.n();
                cache_n.insert(n);

                //total of occurrences in a group, used for frequency computation
                if (c != FLEXGRAM){
                    //no storage per N for dynamic skipgrams
                    cache_grouptotal[c][n] += this->valuehandler.count(iter->second);
                    cache_grouptotal[0][n] += this->valuehandler.count(iter->second);
                    cache_grouptotalpatterns[c][n]++;
                    cache_grouptotalpatterns[0][n]++;
                }
                cache_grouptotal[c][0] += this->valuehandler.count(iter->second);
                cache_grouptotal[0][0] += this->valuehandler.count(iter->second);

                //total of distinct patterns in a group
                cache_grouptotalpatterns[c][0]++;
                cache_grouptotalpatterns[0][0]++;
                iter++;
            }
        }

        virtual void resetstats() {
            cache_grouptotalwordtypes.clear();
            cache_grouptotaltokens.clear();
        }

        /**
         * Compute coverage statistics on the model, will generally be called
         * automatically by methods who use it, and the statistics are cached
         * after computation.
         */
        virtual void computecoveragestats(int category = 0, int n = 0) {
            if ((cache_grouptotal.empty()) && (!this->data.empty())) this->computestats();
            //bool hasunigrams = false;

            //opting for memory over speed (more iterations, less memory)
            // Indexed model overloads this for better cache_grouptotaltokens computation!
            for (std::set<int>::iterator iterc = cache_categories.begin(); iterc != cache_categories.end(); iterc++) {
                if ((category == 0) || (*iterc == category)) {
                 for (std::set<int>::iterator itern = cache_n.begin(); itern != cache_n.end(); itern++) {
                  if (((n == 0) || (*itern == n)) && (!cache_processed[*iterc][*itern]) )  {
                    std::unordered_set<PatternType> types;
                    PatternModel::iterator iter = this->begin();
                    while (iter != this->end()) {
                        const PatternType pattern = iter->first;
                        const int pn = (int) pattern.n();
                        if ( (pn == 1) && (*itern <= 1) && ((*iterc == 0) || (pattern.category() == *iterc))) {
                            types.insert(pattern);
                        } else {
                            if (((*itern == 0) || (pn == *itern))  && ((*iterc == 0) || (pattern.category() == *iterc))) {
                                std::vector<PatternType> unigrams;
                                pattern.ngrams(unigrams, 1);
                                for (typename std::vector<PatternType>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                                    const PatternType p = *iter2;
                                    types.insert(p);
                                }
                            }
                        }
                        cache_grouptotaltokens[*iterc][*itern] += this->valuehandler.count(iter->second);
                        iter++;
                    }
                    cache_grouptotalwordtypes[*iterc][*itern] += types.size();
                    cache_processed[*iterc][*itern] = true;
                  }
                 }
                }
            }

            /*
            if (!hasunigrams) {
                for (std::set<int>::iterator iterc = cache_categories.begin(); iterc != cache_categories.end(); iterc++) {
                    int max = 0;
                    for (std::set<int>::iterator itern = cache_n.begin(); itern != cache_n.end(); itern++) {
                        if cache_grouptotalwordtypes[*iterc][*itern]
                    }
                }

            }*/
        }


        /**
         * Obtains statistics of the model: returns the total amount of occurrences within the specified group, the group consist of a category and a size.
         * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) or to 0 to cover all
         * @param n Set to any value above zero to only cover only patterns of the specified length. (0 for all sizes)
         */
        unsigned int totaloccurrencesingroup(int category, int n) {
            //category and n can be set to 0 to loop over all
            if ((cache_grouptotal.empty()) && (!this->data.empty())) this->computestats();
            return cache_grouptotal[category][n];
        }

        /**
         * Obtains statistics of the model: returns the total amount of distinct patterns within the specified group, the group consist of a category and a size.
         * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) or to 0 to cover all
         * @param n Set to any value above zero to only cover only patterns of the specified length. (0 for all sizes)
         */
        unsigned int totalpatternsingroup(int category, int n) {
            //category and n can be set to 0 to loop over all
            if ((cache_grouptotalpatterns.empty()) && (!this->data.empty())) this->computestats();
            return cache_grouptotalpatterns[category][n];
        }

        /**
         * Obtains statistics of the model: returns the total amount of word/unigtams types within the specified group, the group consist of a category and a size.
         * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) or to 0 to cover all
         * @param n Set to any value above zero to only cover only patterns of the specified length. (0 for all sizes)
         */
        unsigned int totalwordtypesingroup(int category, int n) {
            //total covered word/unigram types
            //category and n can be set to 0 to loop over all
            if ((cache_grouptotalwordtypes.empty()) && (!this->data.empty())) this->computecoveragestats(category,n);
            return cache_grouptotalwordtypes[category][n];
        }
        /**
         * Obtains statistics of the model: returns the total amount of covered tokens within the specified group, the group consist of a category and a size.
         * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) or to 0 to cover all
         * @param n Set to any value above zero to only cover only patterns of the specified length. (0 for all sizes)
         */
        unsigned int totaltokensingroup(int category, int n) {
            //total COVERED tokens
            //category and n can be set to 0 to loop over all
            if ((cache_grouptotaltokens.empty()) && (!this->data.empty())) this->computecoveragestats(category,n);
            return cache_grouptotaltokens[category][n];
        }

        /**
         * Returns the frequency of a pattern within its own group (category and size). For instance, if you pass a bigram you will get the occurence count as a fraction of the total occurrences of bigrams.
         */
        double frequency(const Pattern & pattern) {
            //frequency within the same n and category class
            return this->occurrencecount(pattern) / (double) totaloccurrencesingroup(pattern.category(),pattern.n());
        }



        /**
         * Add a pattern, with a given position, to the model. This
         * is called during training at every time an instance of a pattern is found in the data.
         * This is the high-level version.
         * @param pattern The pattern to add (a patternpointer)
         * @param ref The position in the corpus where the patterns occurs
         */
        virtual void add(const PatternPointer & patternpointer, const IndexReference & ref) {
            const Pattern pattern = Pattern(patternpointer);
            /*if ((pattern.isskipgram()) || (pattern.isflexgram())) { //TODO: remove
                std::cerr << "Adding skipgram!" << std::endl;
                std::cerr << "pp.mask=" << patternpointer.mask << std::endl;
                std::cerr << "pp.b=" << patternpointer.bytesize() << std::endl;
                std::cerr << "p.b=" << pattern.bytesize() << std::endl;
                patternpointer.out();
                std::cerr << std::endl;
                pattern.out();
                throw InternalError();
            }*/
            ValueType * data = getdata(pattern, true);
            this->add(pattern, data, ref );
        }

        /**
         * Add a pattern, with a given position, and a value to the model. This
         * is called during training at every time an instance of a pattern is found in the data.
         * This is the low-level version.
         * @param pattern The pattern to add
         * @param value A pointer to the value for this pattern, what kind of value depends on the ValueType template parameter.
         * @param ref The position in the corpus where the patterns occurs
         */
        virtual void add(const Pattern &, ValueType * value, const IndexReference & ref) {
            if (value == NULL) {
                std::cerr << "Add() value is NULL!" << std::endl;
                throw InternalError();
            }
            this->valuehandler.add(value, ref);
        }
        virtual void add(const PatternPointer &, ValueType * value, const IndexReference & ref) {
            if (value == NULL) {
                std::cerr << "Add() value is NULL!" << std::endl;
                throw InternalError();
            }
            this->valuehandler.add(value, ref);
        }


        /**
         * Prune all patterns under the specified occurrence threshold (or -1
         * for all). Pruning can be limited to patterns of a particular size or
         * category.
         * @param threshold The occurrence threshold (set to -1 to prune everything)
         * @param _n The size constraint, limit to patterns of this size only (set to 0 for no constraint, default)
         * @param category Category constraint
         * @return the number of distinct patterns pruned
         */
        unsigned int prune(int threshold,int _n=0, int category = 0) {
            //prune all patterns under the specified threshold (set -1 for
            //all) and of the specified length (set _n==0 for all)
            unsigned int pruned = 0;
            PatternModel::iterator iter = this->begin();
            while (iter != this->end()) {
                const PatternType pattern = iter->first;
                if (( (_n == 0) || (pattern.n() == (unsigned int) _n) ) && ( ( category == 0) || (pattern.category() == category)) && ((threshold == -1) || (occurrencecount(pattern) < (unsigned int) threshold))) {
                    /*std::cerr << "preprune:" << this->size() << std::endl;
                    std::cerr << "DEBUG: pruning " << (int) pattern.category() << ",n=" << pattern.n() << ",skipcount=" << pattern.skipcount() << ",hash=" << pattern.hash() << std::endl;
                    std::cerr << occurrencecount(pattern) << std::endl;*/
                    iter = this->erase(iter);
                    //std::cerr << "postprune:" << this->size() << std::endl;
                    pruned++;
                } else {
                    iter++;
                }
            };

            return pruned;
        }

        /**
         * Prune all patterns small or equal than the specified length
         * Pruning can be limited to patterns of a particular category.
         * @param _n The size constraint, pattern of and below size will be pruned
         * @param category Category constraint
         * @return the number of distinct patterns pruned
         */
        unsigned int prunebylength(int _n, int category = 0, int threshold = -1) {
            //prune all patterns under the specified threshold (set -1 for
            //all) and of the specified length (set _n==0 for all)
            unsigned int pruned = 0;
            PatternModel::iterator iter = this->begin();
            while (iter != this->end()) {
                const PatternType pattern = iter->first;
                if  (  (pattern.n() <= (unsigned int) _n)  && ( ( category == 0) || (pattern.category() == category)) && ((threshold == -1) || (occurrencecount(pattern) < (unsigned int) threshold))) {
                    /*std::cerr << "preprune:" << this->size() << std::endl;
                    std::cerr << "DEBUG: pruning " << (int) pattern.category() << ",n=" << pattern.n() << ",skipcount=" << pattern.skipcount() << ",hash=" << pattern.hash() << std::endl;
                    std::cerr << occurrencecount(pattern) << std::endl;*/
                    iter = this->erase(iter);
                    //std::cerr << "postprune:" << this->size() << std::endl;
                    pruned++;
                } else {
                    iter++;
                }
            }
            return pruned;
        }

        /**
         * Prune all skipgrams under the specified occurrence threshold (or -1
         * for all). Pruning can be limited to patterns of a particular size
         * only.
         * @param threshold The occurrence threshold (set to -1 to prune everything)
         * @param _n The size constraint, limit to patterns of this size only (set to 0 for no constraint, default)
         * @return the number of distinct patterns pruned
         */
        virtual unsigned int pruneskipgrams(unsigned int threshold, int minskiptypes=2, int _n = 0) {
            //NOTE: minskiptypes is completely ignored! that only works for indexed models
            unsigned int pruned = 0;
            if (minskiptypes <=1) return pruned; //nothing to do

            typename PatternModel<ValueType,BaseValueHandler<ValueType>,MapType>::iterator iter = this->begin();
            while(iter != this->end()) {
                const PatternType pattern = iter->first;
                if (( (_n == 0) || ((int) pattern.n() == _n) ) && (pattern.category() == SKIPGRAM)) {
                    if (this->occurrencecount(pattern) < threshold) {
                        iter = this->erase(iter);
                        pruned++;
                        continue;
                    }
                }
                iter++;
            }
            return pruned;
        }

        /**
         * Prune all patterns that are not in the specified set.
         * @param s The set containing the patterns not to prune
         * @param _n The size constraint, limit to patterns of this size only (set to 0 for no constraint, default)
         * @return the number of distinct patterns pruned
         */
        unsigned int prunenotinset(const std::unordered_set<Pattern> & s, int _n) {
            unsigned int pruned = 0;
            if (s.empty()) {
                return pruned;
            }
            PatternModel::iterator iter = this->begin();
            while (iter != this->end()) {
                const PatternType pattern = iter->first;
                if ( (_n == 0) || (pattern.n() == (unsigned int) _n) ) {
                    if (s.find(pattern) == s.end()) {
                        //not found in set
                        iter = this->erase(iter);
                        pruned++;
                        continue;
                    }
                }
                iter++;
            };

            return pruned;
        }

        /**
         * Prune all patterns that are not in the second model
         * @return the number of distinct patterns pruned
         */
        template<class ValueType2,class ValueHandler2,class MapType2>
        unsigned int prunebymodel(PatternModel<ValueType2,ValueHandler2,MapType2> & secondmodel) {
            //is not used by default when working with constraint models
            //anymore, is directly processing during loading instead
            //
            //this is still useful if you have two models in memory though
            unsigned int pruned = 0;
            typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin();
            while(iter != this->end()) {
                const PatternType pattern = iter->first;
                if (!secondmodel.has(pattern)) {
                    iter = this->erase(iter);
                    pruned++;
                    continue;
                }
                iter++;
            }
            return pruned;
        }

        /**
         * get all patterns in pattern that occur in the patternmodel as a
         * vector of pairs of Patterns and occurrence count.
         */
        std::vector<std::pair<Pattern, int> > getpatterns(const Pattern & pattern) {
            //get all patterns in pattern
            std::vector<std::pair<Pattern, int> > v;
            std::vector<std::pair<Pattern, int> > ngrams;
            pattern.subngrams(ngrams, minlength(), maxlength());
            for (std::vector<std::pair<Pattern, int> >::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
                const Pattern p = iter->first;
                if (this->has(p)) v.push_back(*iter);

                //TODO: match with skipgrams
            }
            return v;
        }

        /**
         * Print the contents of the pattern model, i.e. all patterns and
         * associated counts, to the output stream.
         * @param out The output stream
         * @param decoder The class decoder to use
         * @param instantiate Explicitly instantiate all skipgrams/flexgrams (default: false)
         */
        virtual void print(std::ostream * out, ClassDecoder & decoder, bool instantiate=false) {
            bool haveoutput = false;
            for (PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                if (!haveoutput) {
                    *out << "PATTERN\tCOUNT\tTOKENS\tCOVERAGE\tCATEGORY\tSIZE\tFREQUENCY" << std::endl;
                    haveoutput = true;
                }
                const PatternType pattern = iter->first;
                this->print(out, decoder, pattern, instantiate, true);
            }
            if (haveoutput) {
                std::cerr << std::endl << "Legend:" << std::endl;
                std::cerr << " - PATTERN    : The pattern, Gaps in skipgrams are represented as {*}. Variable-width gaps in flexgrams are shown using  {**}." << std::endl;
                std::cerr << " - COUNT      : The occurrence count - the amount of times the pattern occurs in the data" << std::endl;
                std::cerr << " - TOKENS     : The maximum number of tokens in the corpus that this pattern covers. *THIS IS JUST A MAXIMUM PROJECTION* rather than an exact number because your model is not indexed" << std::endl;
                std::cerr << " - COVERAGE   : The maximum number of tokens covered, as a fraction of the total in the corpus (projection)" << std::endl;
                std::cerr << " - CATEGORY   : The pattern type category (ngram,skipgram,flexgram)" << std::endl;
                std::cerr << " - SIZE       : The size of the pattern (in tokens)" << std::endl;
                std::cerr << " - FREQUENCY  : The frequency of the pattern *within it's pattern type category and size-class*." << std::endl;
                std::cerr << " - REFERENCES : A space-delimited list of sentence:token position where the pattern occurs in the data. Sentences start at 1, tokens at 0" << std::endl;
            }
        }

        /**
         * Print the full reverse index, a mapping of indices and all patterns
         * that occur at those positions.
         * @param out The output stream
         * @param decoder The class decoder to use
         */
        virtual void printreverseindex(std::ostream * out, ClassDecoder & decoder) {
            if (!this->reverseindex) return;
            for (IndexedCorpus::iterator iter = reverseindex->begin(); iter != reverseindex->end(); iter++) {
                const IndexReference ref = iter.index();
                std::unordered_set<PatternPointer> rindex = this->getreverseindex(ref);
                *out << ref.tostring();
                for (std::unordered_set<PatternPointer>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                    const Pattern p = *iter2;
                    *out << "\t" << p.tostring(decoder);
                }
                *out << "\n";
            }
            *out << std::endl;
        }


        /**
         * Just an alias for print()
         */
        void printmodel(std::ostream * out, ClassDecoder & decoder) { //an alias because cython can't deal with a method named print
            this->print(out, decoder);
        }

        /**
         * Print for one pattern only.
         * @param out The output stream
         * @param decoder The class decoder to use
         * @param instantiate Explicitly instantiate all skipgrams and flexgrams (for indexed models only, requires a reverse index)
         * @param endline Output an end-of-line
         */
        virtual void print(std::ostream* out, ClassDecoder &decoder, const PatternType & pattern, bool=false, bool endline = true) {
            const std::string pattern_s = pattern.tostring(decoder);
            const unsigned int count = this->occurrencecount(pattern);
            const unsigned int covcount = this->coveragecount(pattern);
            const double coverage = covcount / (double) this->tokens();
            const double freq = this->frequency(pattern);
            const int cat = pattern.category();
            std::string cat_s;
            if (cat == 1) {
                cat_s = "ngram";
            } else if (cat == 2) {
                cat_s = "skipgram";
            } else if (cat == 3) {
                cat_s = "flexgram";
            }
            *out << pattern_s << "\t" << count << "\t" << "\t" << covcount << "\t" << coverage << "\t" << cat_s << "\t" << pattern.size() << "\t" << freq;
            if (endline) *out << std::endl;
            //*out << pattern.hash() << "\t" << (size_t) pattern.data << std::endl;
        }


        /**
         * Alias for per-pattern print()
         */
        void printpattern(std::ostream* out, ClassDecoder &decoder, const Pattern & pattern, bool instantiate=false, bool endline = true) {  //another alias for cython who can't deal with methods named print
            return this->print(out,decoder,pattern,instantiate,endline);
        }


        /**
         * Generate a histogram for the occurrence count of patterns
         * @param hist This will contain the to-be-computed histogram
         * @param threshold Include only patterns at or above this occurrence threshold
         * @param cap Include only this many of the top frequencies (0=unconstrained)
         * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to filter or to 0 to cover all
         * @param size Set to any value above zero to only include only patterns of the specified length. (0 for all sizes)
         */
        void histogram(std::map<unsigned int,unsigned int> & hist, unsigned int threshold = 0, unsigned int cap = 0, int category = 0, unsigned int size = 0) {
            for (PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                const PatternType pattern = iter->first;
                if (((category != 0) && (pattern.category() != category)) || ((size != 0) && (size != pattern.size()))) continue;
                unsigned int c = this->occurrencecount(pattern);
                if (c >= threshold) hist[c]++;
            }
            if (cap > 0) {
                unsigned int sum = 0;
                std::map<unsigned int,unsigned int>::reverse_iterator iter = hist.rbegin();
                while ((sum < cap) && (iter != hist.rend())) {
                    iter++;
                    sum += iter->second;
                }
                //delete everything else
                hist.erase(iter.base(), hist.end());
            }
        }

        unsigned int topthreshold(int amount, int category=0, int size=0) {
            //compute occurrence threshold that holds the top $amount occurrences
            std::map<unsigned int,unsigned int> hist;
            histogram(hist, 0, amount, category, size);
            std::map<unsigned int,unsigned int>::reverse_iterator iter = hist.rbegin();
            if (iter != hist.rend()) {
                return iter->first;
            } else {
                return 0;
            }
        }


        /**
         * Generate a histogram for the occurrence count of patterns and output it to the output stream.
         * @param OUT the output stream
         * @param threshold Include only patterns at or above this occurrence threshold
         * @param cap Include only this many of the top frequencies (0=unconstrained)
         * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to filter or to 0 to cover all
         * @param size Set to any value above zero to only include only patterns of the specified length. (0 for all sizes)
         */
        void histogram(std::ostream * OUT, unsigned int threshold = 0, unsigned int cap = 0 , int category = 0, unsigned int size = 0) {
            std::map<unsigned int,unsigned int> hist;
            histogram(hist,threshold,cap,category,size);
            *OUT << "HISTOGRAM" << std::endl;
            *OUT << "------------------------------" << std::endl;
            *OUT << "OCCURRENCES\tPATTERNS" << std::endl;
            for (std::map<unsigned int,unsigned int>::iterator iter = hist.begin(); iter != hist.end(); iter++) {
                *OUT << iter->first << "\t" << iter->second << std::endl;
            }
        }

        /**
         * Output information about the model to the output stream, includes some statistics and technical details such as space requirements.
         */
        void info(std::ostream * OUT) {
            if (this->getmodeltype() == INDEXEDPATTERNMODEL) {
                *OUT << "Type: indexed" << std::endl;
            } else if (this->getmodeltype() == UNINDEXEDPATTERNMODEL) {
                *OUT << "Type: unindexed" << std::endl;
            } else {
                //should never happen
                *OUT << "Type: unknown" << std::endl;
            }
            *OUT << "Total tokens: " << this->totaltokens << std::endl;
            *OUT << "Total word types: " << this->totaltypes << std::endl;
            *OUT << "Types patterns loaded: " << this->size() << std::endl;
            *OUT << "Min n: " << this->minn << std::endl;
            *OUT << "Max n: " << this->maxn << std::endl;
            if (this->reverseindex)  {
                *OUT << "Reverse index: yes" << std::endl;
                *OUT << "References in reverse index: " << this->reverseindex->size() << std::endl;
            } else {
                *OUT << "Reverse index: no" << std::endl;
            }
            *OUT << "Size of Pattern: " << sizeof(Pattern) << " byte" << std::endl;
            *OUT << "Size of ValueType: " << sizeof(ValueType) << " byte" << std::endl;
            unsigned int totalkeybs = 0;
            unsigned int totalvaluebs = 0;
            for (PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                const PatternType pattern = iter->first;
                totalkeybs += sizeof(PatternType) + pattern.bytesize();
                totalvaluebs += sizeof(ValueType);
            }
            *OUT << "Total key bytesize (patterns): " <<  totalkeybs << " bytes (" << (totalkeybs/1024/1024) << " MB)" << std::endl;
            *OUT << "Total value bytesize (counts/index): " <<  totalvaluebs << " bytes (" << (totalvaluebs/1024/1024) << " MB)" << std::endl;
            *OUT << "Mean key bytesize: " << (totalkeybs / (float) this->size()) << std::endl;
            *OUT << "Mean value bytesize: " << (totalvaluebs / (float) this->size()) << std::endl;

            unsigned int ri_totalkeybs = 0;
            unsigned int ri_totalvaluebs = 0;
            if (this->reverseindex) {
                for (IndexedCorpus::iterator iter = this->reverseindex->begin(); iter != this->reverseindex->end(); iter++) {
                    ri_totalkeybs += sizeof(iter.index().sentence) + sizeof(iter.index().token);
                    ri_totalvaluebs += sizeof(IndexPattern); // sizeof(Pattern) + iter->pattern().bytesize();
                }
                *OUT << "Total key bytesize in reverse index (references): " <<  ri_totalkeybs << " bytes (" << (ri_totalkeybs/1024/1024) << " MB)" << std::endl;
                *OUT << "Total value bytesize in reverse index (patterns): " <<  ri_totalvaluebs << " bytes (" << (ri_totalvaluebs/1024/1024) << " MB)" << std::endl;
            }


            const unsigned int t = (totalkeybs + totalvaluebs + ri_totalkeybs + ri_totalvaluebs);
            *OUT << "Total bytesize (without overhead): " << t << " bytes (" << (t/1024/1024) << " MB)" << std::endl;
        }

        /**
         * Output an elaborate statistical report to the output stream.
         * Computes on first call when necessary.
         */
        void report(std::ostream * OUT, bool nocoverage=false) {
            if ((!this->cache_processed_all) && (!this->data.empty()) ) {
                if (nocoverage) {
                    std::cerr << "Computing statistics without coverage information..." << std::endl;
                    this->computestats();
                } else {
                    std::cerr << "Computing statistics with coverage information (may take a while)..." << std::endl;
                    this->computecoveragestats();
                }
            }
            *OUT << std::setiosflags(std::ios::fixed) << std::setprecision(4) << std::endl;
            *OUT << "REPORT" << std::endl;
            if ((this->getmodeltype() == UNINDEXEDPATTERNMODEL) && (!nocoverage)) {
                *OUT << "   Warning: Model is unindexed, token coverage counts are mere maximal projections" << std::endl;
                *OUT << "            assuming no overlap at all!!! Use an indexed model for accurate coverage counts" << std::endl;
            }
            *OUT << "----------------------------------" << std::endl;
            *OUT << "                          " << std::setw(15) << "PATTERNS" << std::setw(15) << "TOKENS" << std::setw(15) << "COVERAGE" << std::setw(15) << "TYPES" << std::setw(15) << std::endl;
            *OUT << "Total:                    " << std::setw(15) << "-" << std::setw(15) << this->tokens() << std::setw(15) << "-" << std::setw(15) << this->types() <<  std::endl;

            if (!nocoverage) {
                unsigned int coveredtypes = totalwordtypesingroup(0,0);  //will also work when no unigrams in model!
                unsigned int coveredtokens = totaltokensingroup(0,0);

                if (coveredtokens > this->tokens()) coveredtokens = this->tokens();
                unsigned int uncoveredtokens = this->tokens() - coveredtokens;
                if (uncoveredtokens < 0) uncoveredtokens = 0;
                *OUT << "Uncovered:                " << std::setw(15) << "-" << std::setw(15) << uncoveredtokens << std::setw(15) << uncoveredtokens / (double) this->tokens() << std::setw(15) << this->types() - coveredtypes <<  std::endl;
                *OUT << "Covered:                  " << std::setw(15) << this->size() << std::setw(15) << coveredtokens << std::setw(15) << coveredtokens / (double) this->tokens() <<  std::setw(15) << coveredtypes <<  std::endl << std::endl;
            } else {
                *OUT << std:: endl;
            }



            bool haveoutput = false;
            for (std::set<int>::iterator iterc = cache_categories.begin(); iterc != cache_categories.end(); iterc++) {
                const int c = *iterc;
                if (cache_grouptotalpatterns.count(c))
                for (std::set<int>::iterator itern = cache_n.begin(); itern != cache_n.end(); itern++) {
                    const int n = *itern;
                    if (cache_grouptotalpatterns[c].count(n)) {
                        if (!haveoutput) {
                            //output headers
                            *OUT << std::setw(15) << "CATEGORY" << std::setw(15) << "N (SIZE) "<< std::setw(15) << "PATTERNS";
                            if ((this->getmodeltype() != UNINDEXEDPATTERNMODEL) && (!nocoverage)) *OUT << std::setw(15) << "TOKENS" << std::setw(15) << "COVERAGE";
                            if (!nocoverage) *OUT << std::setw(15) << "TYPES";
                            *OUT << std::setw(15) << "OCCURRENCES" << std::endl;
                            haveoutput = true;
                        }
                        //category
                        if (c == 0) {
                            *OUT << std::setw(15) << "all";
                        } else if (c == NGRAM) {
                            *OUT << std::setw(15) << "n-gram";
                        } else if (c == SKIPGRAM) {
                            *OUT << std::setw(15) << "skipgram";
                        } else if (c == FLEXGRAM) {
                            *OUT << std::setw(15) << "flexgram";
                        }
                        //size
                        if (n == 0) {
                            *OUT << std::setw(15) << "all";
                        } else {
                            *OUT << std::setw(15) << n;
                        }
                        //patterns
                        *OUT << std::setw(15) << cache_grouptotalpatterns[c][n];
                        if ((this->getmodeltype() != UNINDEXEDPATTERNMODEL) && (!nocoverage)) {
                            //tokens
                            *OUT << std::setw(15) << cache_grouptotaltokens[c][n];
                            //coverage
                            *OUT << std::setw(15) << cache_grouptotaltokens[c][n] / (double) this->tokens();
                        }
                        if (!nocoverage) {
                            //types
                            *OUT << std::setw(15) << cache_grouptotalwordtypes[c][n];
                        }
                        //occurrences
                        *OUT << std::setw(15) << cache_grouptotal[c][n] << std::endl;;
                    }
                }
            }

            if (haveoutput) {
                std::cerr << std::endl << "Legend:" << std::endl;
                std::cerr << " - PATTERNS    : The number of distinct patterns within the group" << std::endl;
                if (this->getmodeltype() != UNINDEXEDPATTERNMODEL) {
                    std::cerr << " - TOKENS      : The number of tokens that is covered by the patterns in the group." << std::endl;
                    std::cerr << " - COVERAGE    : The number of tokens covered, as a fraction of the total in the corpus" << std::endl;
                }
                std::cerr << " - TYPES       : The number of unique *word/unigram* types in this group" << std::endl;
                std::cerr << " - OCCURRENCES : The total number of occurrences of the patterns in this group" << std::endl;
            }
        }


        /**
         * Returns a PatternSet containing patterns of the specified length.
         * Patterns are actively reconstructed from patterns in the model,
         * if necessary. So this includes patterns that are not in the model
         * explicitly (i.e, smaller patterns that have been pruned.
         */
        PatternSet<uint64_t> extractset(int minlength = 1, int maxlength = 1) {

            PatternSet<uint64_t> result;
            for (PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                const PatternType pattern = iter->first;
                const int patternlength = pattern.n();
                if ((patternlength >= minlength) && (patternlength <= maxlength)) {
                    result.insert(pattern);
                } else if (patternlength > maxlength) {
                    std::vector<Pattern> subngrams;
                    pattern.subngrams(subngrams,minlength, maxlength);
                    for (std::vector<Pattern>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {
                        const Pattern pattern2 = *iter2;
                        result.insert(pattern2);
                    }
                }
            }
            return result;
        }


        virtual void outputrelations(const PatternPointer & , ClassDecoder & , std::ostream *, const std::string = "", bool=true) {} //does nothing for unindexed models
        virtual t_relationmap getsubchildren(const PatternPointer & ,int = 0, int = 0, int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getsubparents(const PatternPointer &,int = 0, int = 0, int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap gettemplates(const PatternPointer & ,int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getinstances(const PatternPointer & ,int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getskipcontent(const PatternPointer & ) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getleftneighbours(const PatternPointer &, int = 0, int = 0,int = 0,int =0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getrightneighbours(const PatternPointer &, int = 0, int = 0,int = 0,int =0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap_double getnpmi(const Pattern & , double ) { return t_relationmap_double(); } //does nothing for unindexed models
        virtual int computeflexgrams_fromskipgrams() { return 0; }//does nothing for unindexed models
        virtual int computeflexgrams_fromcooc(double) {return 0; }//does nothing for unindexed models
        virtual void outputcooc_npmi(std::ostream *, ClassDecoder& , double) {}
        virtual void outputcooc(std::ostream *, ClassDecoder&, double) {}

        /**
         * Get the instance of the pattern at the specified position
         * Alias for IndexedCorpus.findpattern()
         * Throws a KeyError if the pattern was not found.
         */
        PatternPointer getinstance(const IndexReference & begin, const PatternPointer & pattern) {
            if ((this->reverseindex == NULL)) {
                std::cerr << "ERROR: You must specify a reverse index to use getinstance()!" << std::endl;
                throw InternalError();
            }
            return this->reverseindex->findpattern(begin, pattern);
        }
};




/**
 * \brief An indexed model mapping patterns to values, high-level interface.
 * This is a specialised subclass of PatternMap.
 * @tparam MapType The type of container to use
 */
template<class MapType = PatternMap<IndexedData,IndexedDataHandler>,class PatternType = Pattern>
class IndexedPatternModel: public PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType> {
    protected:
        virtual void postread(const PatternModelOptions&) {
            for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern p = iter->first;
                const PatternCategory category = p.category();
                const int n = p.n();
                if (n > this->maxn) this->maxn = n;
                if (n < this->minn) this->minn = n;
                if ((!this->hasskipgrams) && (category == SKIPGRAM)) this->hasskipgrams = true;
                if ((!this->hasflexgrams) && (category == FLEXGRAM)) this->hasflexgrams = true;
            }
        }
        virtual void posttrain(const PatternModelOptions& options ) {
            if (!options.QUIET) std::cerr << "Sorting all indices..." << std::endl;
            for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
                iter->second.sort();
            }

        }
   public:


    /**
    * Begin a new pattern model, optionally pre-setting a reverseindex.
    */
    IndexedPatternModel<MapType,PatternType>(IndexedCorpus * corpus = NULL): PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>() {
        this->model_type = this->getmodeltype();
        this->model_version = this->getmodelversion();
        if (corpus) {
            this->reverseindex = corpus;
            this->attachcorpus(*corpus);
        } else {
            this->reverseindex = NULL;
        }
    }

    /**
    * Read a pattern model from an input stream
    * @param f The input stream
    * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
    * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
    * @param corpus Pointer to the loaded corpus, used as a reverse index.
    */
    IndexedPatternModel<MapType,PatternType>(std::istream *f, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL):  PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>(){ //load from file
        this->model_type = this->getmodeltype();
        this->model_version = this->getmodelversion();
        if (corpus) {
            this->reverseindex = corpus;
            this->attachcorpus(*corpus);
        } else {
            this->reverseindex = NULL;
        }
        this->load(f,options, constrainmodel);
    }

    /**
    * Read a pattern model from file
    * @param filename The filename
    * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
    * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
    * @param corpus Pointer to the loaded corpus, used as a reverse index.
    */
    IndexedPatternModel<MapType,PatternType>(const std::string filename, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL): PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>() { //load from file
        this->model_type = this->getmodeltype();
        this->model_version = this->getmodelversion();
        if (corpus) {
            this->reverseindex = corpus;
            this->attachcorpus(*corpus);
        } else {
            this->reverseindex = NULL;
        }
        std::ifstream * in = new std::ifstream(filename.c_str());
        this->load( (std::istream *) in, options, constrainmodel);
        in->close();
        delete in;
    }

    virtual ~IndexedPatternModel<MapType,PatternType>() { }


    int getmodeltype() const { return INDEXEDPATTERNMODEL; }
    int getmodelversion() const { return 2;}


    /**
     * Add a pattern, with a given position, and a value to the model. This
     * is called during training at every time an instance of a pattern is found in the data.
     * @param pattern The pattern to add
     * @param value A pointer to the value for this pattern, set to NULL and it will be automatically determined
     * @param IndexReference The position in the corpus where the patterns occurs
     */
    virtual void add(const Pattern & pattern, IndexedData * value, const IndexReference & ref) {
        if (value == NULL) {
            value = getdata(pattern,true);
        }
        this->valuehandler.add(value, ref);
    }
    virtual void add(const PatternPointer & patternpointer, IndexedData * value, const IndexReference & ref) {
        if (value == NULL) {
            value = getdata(patternpointer,true);
        }
        this->valuehandler.add(value, ref);
    }

    /**
     * Get the indices stored for the specified pattern.
     * @param makeifnew Add the pattern with empty value if it does not exist (default: false)
     **/
    IndexedData * getdata(const Pattern & pattern, bool makeifnew=false)  {
        typename MapType::iterator iter = this->find(pattern);
        if (iter != this->end()) {
            return &(iter->second);
        } else if (makeifnew) {
            return &((*this)[pattern]);
        } else {
            return NULL;
        }
    }

    IndexedData * getdata(const PatternPointer & pattern, bool makeifnew=false) {
        typename MapType::iterator iter = this->find(pattern);
        if (iter != this->end()) {
            return &(iter->second);
        } else if (makeifnew) {
            return &((*this)[pattern]);
        } else {
            return NULL;
        }
    }

    virtual void train(std::istream * in , PatternModelOptions options,  PatternModelInterface * constrainbymodel = NULL, PatternSet<> * filter = NULL, bool continued=false, uint32_t firstsentence=1, bool ignoreerrors=false) {
        if ((options.DOSKIPGRAMS) && (this->reverseindex == NULL)) {
            std::cerr << "ERROR: You must specify a reverse index if you want to train skipgrams (or train skipgrams exhaustively)" << std::endl;
            throw InternalError();
        }
        PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>::train(in,options,constrainbymodel,filter,continued,firstsentence,ignoreerrors);
    }

    virtual void train(const std::string & filename, PatternModelOptions options, PatternModelInterface * constrainbymodel = NULL,  PatternSet<> * filter = NULL,bool continued=false, uint32_t firstsentence=1, bool ignoreerrors=false) {
        if ((options.DOSKIPGRAMS) && (this->reverseindex == NULL)) {
            std::cerr << "ERROR: You must specify a reverse index if you want to train skipgrams (or train skipgrams exhaustively)" << std::endl;
            throw InternalError();
        }
        PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>::train(filename,options,constrainbymodel,filter,continued,firstsentence,ignoreerrors);
    }

    /**
     * Output information about the model to the output stream, includes some statistics and technical details such as space requirements.
     */
    void info(std::ostream * OUT) {
        if (this->getmodeltype() == INDEXEDPATTERNMODEL) {
            *OUT << "Type: indexed" << std::endl;
        } else if (this->getmodeltype() == UNINDEXEDPATTERNMODEL) {
            *OUT << "Type: unindexed" << std::endl;
        } else {
            //should never happen
            *OUT << "Type: unknown" << std::endl;
        }
        *OUT << "Total tokens: " << this->totaltokens << std::endl;
        *OUT << "Total word types: " << this->totaltypes << std::endl;
        *OUT << "Types patterns loaded: " << this->size() << std::endl;
        *OUT << "Min n: " << this->minn << std::endl;
        *OUT << "Max n: " << this->maxn << std::endl;
        if (this->reverseindex)  {
            *OUT << "Reverse index: yes" << std::endl;
            *OUT << "References in reverse index: " << this->reverseindex->size() << std::endl;
        } else {
            *OUT << "Reverse index: no" << std::endl;
        }
        *OUT << "Size of Pattern: " << sizeof(Pattern) << " byte" << std::endl;
        unsigned int totalkeybs = 0;
        unsigned int totalvaluebs = 0;
        unsigned int indexlengthsum = 0;
        for (typename IndexedPatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
            const Pattern pattern = iter->first;
            totalkeybs += sizeof(Pattern) + pattern.bytesize();
            totalvaluebs += iter->second.size() * sizeof(IndexReference); //sentence + token;
            indexlengthsum += iter->second.size();
        }
        *OUT << "Total key bytesize (patterns): " << totalkeybs << " bytes (" << (totalkeybs/1024/1024) << " MB)" << std::endl;
        *OUT << "Total value bytesize (counts/index): " << totalvaluebs << " bytes (" << (totalvaluebs/1024/1024) << " MB)" << std::endl;
        *OUT << "Mean key bytesize: " << (totalkeybs / (float) this->size()) << std::endl;
        *OUT << "Mean value bytesize: " << (totalvaluebs / (float) this->size()) << std::endl;
        *OUT << "Mean index length (ttr): " << (indexlengthsum / (float) this->size()) << std::endl;

        unsigned int ri_totalkeybs = 0;
        unsigned int ri_totalvaluebs = 0;
        if (this->reverseindex) {
            for (IndexedCorpus::iterator iter = this->reverseindex->begin(); iter != this->reverseindex->end(); iter++) {
                ri_totalkeybs += sizeof(iter.index().sentence) + sizeof(iter.index().token);
                ri_totalvaluebs += sizeof(IndexPattern); // sizeof(Pattern) + iter->pattern().bytesize();
            }
            *OUT << "Total key bytesize in reverse index (references): " << ri_totalkeybs << " bytes (" << (ri_totalkeybs/1024/1024) << " MB)" << std::endl;
            *OUT << "Total value bytesize in reverse index (patterns): " << ri_totalvaluebs << " bytes (" << (ri_totalvaluebs/1024/1024) << " MB)" << std::endl;
        }

        const unsigned int t = (totalkeybs + totalvaluebs + ri_totalkeybs + ri_totalvaluebs);
        *OUT << "Total bytesize (without overhead): " << t << " bytes (" << (t/1024/1024) << " MB)" << std::endl;
    }


    /**
    * Print the contents of the pattern model, i.e. all patterns and
    * associated counts, to the output stream.
    * @param out The output stream
    * @param decoder The class decoder to use
    * @param instantiate Explicitly instantiate all skipgrams/flexgrams (default: false)
    */
    void print(std::ostream * out, ClassDecoder & decoder, bool instantiate=false) {
        bool haveoutput = false;
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            if (!haveoutput) {
                *out << "PATTERN\tCOUNT\tTOKENS\tCOVERAGE\tCATEGORY\tSIZE\tFREQUENCY\tREFERENCES" << std::endl;
                haveoutput = true;
            }
            const PatternPointer pattern = iter->first;
            this->print(out, decoder, pattern, instantiate, true);
        }
        if (haveoutput) {
            std::cerr << std::endl << "Legend:" << std::endl;
            std::cerr << " - PATTERN    : The pattern, Gaps in skipgrams are represented as {*}. Variable-width gaps in flexgrams are shown using {**}." << std::endl;
            std::cerr << " - COUNT      : The occurrence count - the amount of times the pattern occurs in the data" << std::endl;
            std::cerr << " - TOKENS     : The number of tokens in the corpus that this pattern covers" << std::endl;
            std::cerr << " - COVERAGE   : The number of tokens covered, as a fraction of the total in the corpus" << std::endl;
            std::cerr << " - CATEGORY   : The pattern type category (ngram,skipgram,flexgram)" << std::endl;
            std::cerr << " - SIZE       : The size of the pattern (in tokens)" << std::endl;
            std::cerr << " - FREQUENCY  : The frequency of the pattern *within it's pattern type category and size-class*." << std::endl;
            std::cerr << " - REFERENCES : A space-delimited list of sentence:token position where the pattern occurs in the data. Sentences start at 1, tokens at 0" << std::endl;
        }
    }

    void print(std::ostream* out, ClassDecoder &decoder, const PatternPointer & pattern, bool instantiate=false, bool endline = true) {
            const std::string pattern_s = pattern.tostring(decoder);
            const unsigned int count = this->occurrencecount(pattern);
            const unsigned int covcount = this->coveragecount(pattern);
            const double coverage = covcount / (double) this->tokens();
            const double freq = this->frequency(pattern);
            const PatternCategory cat = pattern.category();
            std::string cat_s;
            if (cat == 1) {
                cat_s = "ngram";
            } else if (cat == 2) {
                cat_s = "skipgram";
            } else if (cat == 3) {
                cat_s = "flexgram";
            }
            *out << pattern_s << "\t" << count << "\t" << "\t" << covcount << "\t" << coverage << "\t" << cat_s << "\t" << pattern.size() << "\t" << freq << "\t";
            IndexedData * data = this->getdata(pattern);
            unsigned int i = 0;
            for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {
                i++;
                *out << iter2->tostring();
                if (cat != NGRAM) {
                    if ((this->reverseindex != NULL) && (instantiate)) {
                        const PatternPointer instance = this->reverseindex->findpattern(*iter2,pattern);
                        *out << "[" << instance.tostring(decoder) <<  "]";
                    }
                }
                if (i < count) *out << " ";
            }
            if (endline) *out << std::endl;
    }




    /**
     * Train skipgrams, for indexed models only
     * @param options Pattern model options
     * @param constrainbymodel Pointer to a pattern model to use as contraint: only include skipgrams that occur in the constraint model (default: NULL)
     */
    virtual void trainskipgrams(PatternModelOptions options,  PatternModelInterface * constrainbymodel = NULL) {
        if (options.MINTOKENS == -1) options.MINTOKENS = 2;
        this->cache_grouptotal.clear(); //forces recomputation of statistics
        if (constrainbymodel == this) {
            this->trainskipgrams_selfconstrained(options);
        } else {
            //normal behaviour: extract skipgrams from n-grams
            if (!options.QUIET) std::cerr << "Finding skipgrams on the basis of extracted n-grams..." << std::endl;
            for (int n = 3; n <= options.MAXLENGTH; n++) {
                if (this->gapmasks[n].empty()) this->gapmasks[n] = compute_skip_configurations(n, options.MAXSKIPS);
                if (!options.QUIET) std::cerr << "Counting " << n << "-skipgrams" << std::endl;
                unsigned int foundskipgrams = 0;
                for (typename MapType::iterator iter = this->begin(); iter != this->end(); iter++) {
                    const PatternPointer pattern = PatternPointer(&(iter->first));
                    const IndexedData multirefs = iter->second;
                    if (((int) pattern.n() == n) && (pattern.category() == NGRAM) ) foundskipgrams += this->computeskipgrams(pattern,options, NULL, &multirefs, constrainbymodel, false);
                }
                if (!foundskipgrams) {
                    std::cerr << " None found" << std::endl;
                    break;
                } else {
                    this->hasskipgrams = true;
                }
                if (!options.QUIET) std::cerr << " Found " << foundskipgrams << " skipgrams...";
                unsigned int pruned = this->prune(options.MINTOKENS,n);
                if (!options.QUIET) std::cerr << "pruned " << pruned;
                unsigned int prunedextra = this->pruneskipgrams(options.MINTOKENS_SKIPGRAMS, options.MINSKIPTYPES, n);
                if (prunedextra && !options.QUIET) std::cerr << " plus " << prunedextra << " extra skipgrams..";
                if (!options.QUIET) std::cerr << "...total kept: " <<  foundskipgrams - pruned - prunedextra << std::endl;
            }
        }
    }

    /**
     * Return the unigram Pattern that occurs on the specified position, using
     * the reverse index.
     */
    Pattern getpatternfromtoken(IndexReference ref) {
        if (this->reverseindex == NULL) {
            std::cerr << "ERROR: getpatternfromtoken() No reverse index loaded" << std::endl;
            throw InternalError();
        }
        return this->reverseindex->getpattern(ref,1);
    }



    /**
     * Given a skipgram, returns patterns in the model which would fit the gaps and instantiate the skipgram. It is the inverse of the skipgram.
     * if inserted into the gaps. For skipgrams with multiple gaps, these skip content patterns are themselves skipgrams. Skipgram and skip content complement eachother
     * @returns A relation map
     */
    t_relationmap getskipcontent(const PatternPointer & pattern) {
        t_relationmap skipcontent; //will hold all skipcontent
        if (this->reverseindex == NULL) {
            std::cerr << "ERROR: No corpus data loaded! (in PatternModel::getskipcontent)" << std::endl;
            throw InternalError();
        }

        if (pattern.category() != NGRAM) {
			const unsigned int n = pattern.n();
			const uint32_t skipcontent_mask = reversemask(pattern.mask,n );

			const int head = maskheadskip(skipcontent_mask, n); //skip at begin
			const int tail = masktailskip(skipcontent_mask,n); //skip at end

            //iterate over forward index
            const IndexedData * data = getdata(pattern);
            for (IndexedData::const_iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {
                const IndexReference ref = *iter2;

				//raw skipcontent with leading and trailing skips
				PatternPointer skipcontent_atref_raw = this->reverseindex->getpattern(ref,n);
				skipcontent_atref_raw.mask = skipcontent_mask;

				//trim leading and trailing skips
				Pattern skipcontent_atref = PatternPointer(skipcontent_atref_raw, head, n-head-tail); //pattern from patternpointer
                skipcontent[skipcontent_atref] += 1;
            }
        }
        //std::cerr << "Total found " << skipcontent.size() << std::endl;
        return skipcontent;
    }

    /**
     * Given a relation map, prune relations below the specified occurrence threshold
     * @param relations The relationmap to manipulate
     * @param occurrencethreshold The occurrence threshold
     */
    void prunerelations(t_relationmap & relations, unsigned int occurrencethreshold) {
        t_relationmap::iterator eraseiter;
        t_relationmap::iterator iter = relations.begin();
        while (iter != relations.end()) {
            if (iter->second < occurrencethreshold) {
                eraseiter = iter;
                iter++;
                relations.erase(eraseiter);
            } else {
                iter++;
            }
        }
    }

    /**
     * Returns all skipgrams and flexgrams in the model that are an abstraction of the
     * specified pattern. Pattern itself may be a skipgram too. An optional
     * occurrence threshold may be used to filter.
     * @return a relation map
     */
    t_relationmap gettemplates(const Pattern & pattern, unsigned int occurrencethreshold = 0) {
        //returns patterns that are an abstraction of the specified pattern
        //skipgrams
        if ((this->reverseindex == NULL) || (this->reverseindex->empty())) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            throw NoSuchPattern();
        }

        t_relationmap templates;


        const int _n = pattern.n();
        //search in forward index
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;

            //search in reverse index
            std::unordered_set<PatternPointer> rindex = this->getreverseindex(ref);
            for (std::unordered_set<PatternPointer>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const PatternPointer candidate = *iter2;

                if (((int) candidate.n() == _n)  && (candidate != pattern) && (candidate.category() == SKIPGRAM)  && ((occurrencethreshold == 0) || (this->occurrencecount(pattern) >= occurrencethreshold)) ) {
                    templates[candidate] += 1;
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(templates, occurrencethreshold);
        return templates;
    }

    /**
     * Returns all ngrams in the model that instantiate the given
     * skipgram/flexgram. If all the gaps in a skipgram/flexgram are filled, we
     * speak of such an instantiation.
     * An occurrence threshold may be used to filter.
     * @return a relation map
     */
    t_relationmap getinstances(const Pattern & pattern, unsigned int occurrencethreshold = 0) {
        //returns patterns that instantiate the specified pattern
        if ((this->reverseindex == NULL) || (this->reverseindex->empty())) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            throw NoSuchPattern();
        }

        t_relationmap instances;


        const int _n = pattern.n();
        //search in forward index
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;

            //search in reverse index
            std::unordered_set<PatternPointer> rindex = this->getreverseindex(ref, occurrencethreshold,NGRAM);
            for (std::unordered_set<PatternPointer>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const PatternPointer candidate = *iter2;

                if (((int) candidate.n() == _n)  && (candidate != pattern) ) {
                    instances[candidate] += 1;
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(instances, occurrencethreshold);
        return instances;
    }


    /**
     * Returns all patterns in the model that are subsumed by the specified pattern. Subsumed patterns are smaller than the subsuming pattern. Every n-gram (except unigram) by definition subsumes two n-1-grams.
    * @param occurrencethreshold If set above zero, filters to only include patterns occurring above this threshold
    * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to include only this category. Set to 0 for unfiltered (default)
    * @param size Set to any value above zero to only include patterns of the specified length.
    * @return a relation map
    */
    t_relationmap getsubchildren(const PatternPointer & pattern, unsigned int occurrencethreshold = 0, int category = 0, unsigned int size = 0) {
        if ((this->reverseindex == NULL) || (this->reverseindex->empty())) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            throw NoSuchPattern();
        }




        t_relationmap subchildren;
        const int _n = pattern.n();
        const bool isskipgram = (pattern.category() == SKIPGRAM);
        //search in forward index
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;
            for (int i = ref.token; i < ref.token + _n; i++) {
                const IndexReference begin = IndexReference(ref.sentence,i);
                int maxsubn = _n - (i - ref.token);

                //std::cerr << "Begin " << begin.sentence << ":" << begin.token << ",<< std::endl;

                //search in reverse index
                std::unordered_set<PatternPointer> rindex = this->getreverseindex(begin, occurrencethreshold, category,size);
                for (std::unordered_set<PatternPointer>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                    const PatternPointer candidate = *iter2;
                    //std::cerr << "Considering candidate @" << ref2.sentence << ":" << ref2.token << ", n=" << candidate.n() << ", bs=" << candidate.bytesize() <<  std::endl;
                    //candidate.out();
                    if (((int) candidate.n() <= maxsubn) && (candidate != pattern)) {
                        if ((isskipgram) || (candidate.category() == SKIPGRAM)) {
                            //candidate may not have skips in places where the larger pattern does
                            Pattern tmpl = Pattern(pattern, i, candidate.n()); //get the proper slice to match
                            if (candidate.instanceof(tmpl)) {
                                subchildren[candidate] = subchildren[candidate] + 1;
                            }
                        } else if (candidate.category() == FLEXGRAM) {
                            //TODO
                        } else {
                            subchildren[candidate]++;
                        }
                    }
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(subchildren, occurrencethreshold);
        return subchildren;
    }

    /**
     * Returns all patterns in the model that subsume the specified pattern. Subsuming patterns are larger than the subsuming pattern. Every n-gram (except unigram) by definition subsumes two n-1-grams.
    * @param occurrencethreshold If set above zero, filters to only include patterns occurring above this threshold
    * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to include only this category. Set to 0 for unfiltered (default)
    * @param size Set to any value above zero to only include patterns of the specified length.
    * @return a relation map
    */
    t_relationmap getsubparents(const PatternPointer & pattern, unsigned int occurrencethreshold = 0, int category = 0, unsigned int size = 0) {
        //returns patterns that subsume the specified pattern (i.e. larger
        //patterns)
        if ((this->reverseindex == NULL) || (this->reverseindex->empty())) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            throw NoSuchPattern();
        }

        t_relationmap subsumes;
        const int _n = pattern.n();
        //search in forward index
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;


            //search in reverse index
            std::vector<std::pair<IndexReference,PatternPointer>> rindex = this->getreverseindex_bysentence(ref.sentence, occurrencethreshold, category, size);
            for (std::vector<std::pair<IndexReference,PatternPointer>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                if ((iter2->first.sentence != ref.sentence) || (iter2->first.token > ref.token)) break;
                const PatternPointer candidate = iter2->second;

                int minsubsize = _n + (ref.token - iter2->first.token);

                if (((int) candidate.n() >= minsubsize)  && (candidate != pattern)
                        && ((occurrencethreshold == 0) || (this->occurrencecount(candidate) >= occurrencethreshold))
                        && ((category == 0) || (candidate.category() >= category))
                        && ((size == 0) || (candidate.n() >= size))
                    ) {
                    if ((candidate.category() == SKIPGRAM) || (pattern.category() == SKIPGRAM))  {
                        //instance may not have skips in places where the larger candidate pattern does
                        Pattern inst = Pattern(candidate, iter2->first.token, pattern.n()); //get the proper slice to match
                        if (pattern.instanceof(candidate)) {
                            subsumes[candidate] += 1;
                        }
                    } else if (candidate.category() == FLEXGRAM) {
                        //TODO
                    } else {
                        subsumes[candidate] += 1;
                    }
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(subsumes, occurrencethreshold);
        return subsumes;
    }


    /**
     * Returns all patterns in the model that directly neighbour the given pattern at the left side
    * @param occurrencethreshold If set above zero, filters to only include patterns occurring above this threshold
    * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to include only this category. Set to 0 for unfiltered (default)
    * @param size Set to any value above zero to only include patterns of the specified length.
    * @return a relation map
    */
    t_relationmap getleftneighbours(const PatternPointer & pattern, unsigned int occurrencethreshold = 0, int category = 0, unsigned int size = 0, unsigned int cutoff=0) {
        if ((this->reverseindex == NULL) || (this->reverseindex->empty())) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            throw NoSuchPattern();
        }

        t_relationmap neighbours;
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;


            std::vector<std::pair<IndexReference,PatternPointer>> rindex = this->getreverseindex_bysentence(ref.sentence);
            for (std::vector<std::pair<IndexReference,PatternPointer>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const PatternPointer neighbour = iter2->second;
                if ((ref2.token + neighbour.n() == ref.token)
                        && ((occurrencethreshold == 0) || (this->occurrencecount(neighbour) >= occurrencethreshold))
                        && ((category == 0) || (neighbour.category() >= category))
                        && ((size == 0) || (neighbour.n() >= size))
                    ){
                    neighbours[neighbour]++;
                    if ((cutoff > 0) && (neighbours.size() >= cutoff)) break;
                } else if ((ref2.token > ref.token) || (ref2.sentence > ref.sentence)) break;
            }
            if ((cutoff > 0) && (neighbours.size() >= cutoff)) break;
        }
        if (occurrencethreshold > 0) this->prunerelations(neighbours, occurrencethreshold);
        return neighbours;
    }

    /**
     * Returns all patterns in the model that directly neighbour the given pattern at the right side
    * @param occurrencethreshold If set above zero, filters to only include patterns occurring above this threshold
    * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to include only this category. Set to 0 for unfiltered (default)
    * @param size Set to any value above zero to only include patterns of the specified length.
    * @return a relation map
    */
    t_relationmap getrightneighbours(const PatternPointer & pattern, unsigned int occurrencethreshold = 0, int category = 0, unsigned int size = 0, unsigned int cutoff=0) {
        if ((this->reverseindex == NULL) || (this->reverseindex->empty())) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            throw NoSuchPattern();
        }

        t_relationmap neighbours;
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            IndexReference ref = *iter;
            ref.token += pattern.size();

            //search in reverse index
            std::unordered_set<PatternPointer> rindex = this->getreverseindex(ref);
            for (std::unordered_set<PatternPointer>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const PatternPointer neighbour = *iter2;
                if ( ((occurrencethreshold == 0) || (this->occurrencecount(neighbour) >= occurrencethreshold))
                        && ((category == 0) || (neighbour.category() >= category))
                        && ((size == 0) || (neighbour.n() >= size)) ) {
                    neighbours[neighbour]++;
                    if ((cutoff > 0) && (neighbours.size() >= cutoff)) break;
                }
            }
            if ((cutoff > 0) && (neighbours.size() >= cutoff)) break;
        }
        if (occurrencethreshold > 0) this->prunerelations(neighbours, occurrencethreshold);
        return neighbours;
    }

    /**
     * Prune skipgrams based on an occurrence threshold, and a skiptype
     * threshold. The latter enforces that at least the specified number of
     * distinct patterns must fit in the gap for the skipgram to be retained.
    * @param _n Set to any value above zero to only include patterns of the specified length.
     */
    int pruneskipgrams(int, int minskiptypes, int _n = 0) {
        int pruned = 0;
        if (minskiptypes <=1) return pruned; //nothing to do

        typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin();
        while(iter != this->end()) {
            const PatternType pattern = iter->first;
            if (( (_n == 0) || ((int) pattern.n() == _n) ) && (pattern.category() == SKIPGRAM)) {
                try {
					t_relationmap skipcontent = getskipcontent(pattern);
					//std::cerr << " Pattern " << pattern.hash() << " occurs: " << this->occurrencecount(pattern) << " skipcontent=" << skipcontent.size() << std::endl;
					if ((int) skipcontent.size() < minskiptypes) { //will take care of token threshold too, patterns not meeting the token threshold are not included
						//std::cerr << "..pruning" << std::endl;
						iter = this->erase(iter);
						pruned++;
						continue;
					}
                } catch(KeyError &e) { } // ignore
            }
            iter++;
        }
        return pruned;
    }

    /**
    * Compute coverage statistics on the model, will generally be called
    * automatically by methods who use it, and the statistics are cached
    * after computation.
    */
    virtual void computecoveragestats(int category = 0, int n = 0) {
        //opting for memory over speed (more iterations, less memory)
        //overloaded version for indexedmodel
        if ((this->cache_grouptotal.empty()) && (!this->data.empty())) this->computestats();

        if ((this->cache_n.size() == 1) && (*this->cache_n.begin() == 1) && (n <= 1)) {
            //special condition, only unigrams, we can be done quicker
            this->cache_grouptotalwordtypes[0][1] = this->size();
            typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin();
            while (iter != this->end()) {
                this->cache_grouptotaltokens[0][1] += this->valuehandler.count(iter->second);
                iter++;
            }
            this->cache_processed_all = true;
            return;
        }

        for (std::set<int>::iterator iterc = this->cache_categories.begin(); iterc != this->cache_categories.end(); iterc++) {
          if ((category == 0) || (*iterc == category)) {
            for (std::set<int>::iterator itern = this->cache_n.begin(); itern != this->cache_n.end(); itern++) {
              if (((n == 0) || (*itern == n)) && (!this->cache_processed[*iterc][*itern]) )  {
                std::unordered_set<PatternPointer> types;
                std::set<IndexReference> tokens;
                typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin();
                while (iter != this->end()) {
                    const PatternPointer pattern = iter->first;
                    const int n = pattern.n();
                    if ( (n == 1) && (*itern <= 1) && ((*iterc == 0) || (pattern.category() == *iterc))) {
                        types.insert(pattern);
                    } else {
                        if (((*itern == 0) || (n == *itern))  && ((*iterc == 0) || (pattern.category() == *iterc))) {
                            std::vector<PatternPointer> unigrams;
                            pattern.ngrams(unigrams, 1);
                            for (std::vector<PatternPointer>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                                const PatternPointer p = *iter2;
                                types.insert(p);
                            }
                        }
                    }
                    if (((*itern == 0) || (n == *itern))  && ((*iterc == 0) || (pattern.category() == *iterc))) {
                        IndexedData * data = this->getdata(pattern);
                        for (IndexedData::iterator dataiter = data->begin(); dataiter != data->end(); dataiter++) {
                            //take into account all tokens
                            for (unsigned int i = 0; i < pattern.n(); i++) {
                                tokens.insert(*dataiter + i);
                            }
                        }
                    }
                    iter++;
                }
                this->cache_grouptotalwordtypes[*iterc][*itern] += types.size();
                this->cache_grouptotaltokens[*iterc][*itern] += tokens.size();
                this->cache_processed[*iterc][*itern] = true;
            }
          }
        }
      }

      if ((category == 0) && (n == 0)) this->cache_processed_all = true;
    }

    /**
     * Returns all patterns in the model that co-occur with the given pattern in the same sentence and appear to the right of the given pattern
    * @param occurrencethreshold If set above zero, filters to only include patterns occurring above this threshold
    * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to include only this category. Set to 0 for unfiltered (default)
    * @param size Set to any value above zero to only include patterns of the specified length.
    * @param matches Pointer to a data structure that will be filled by this method to hold all the matches in pairs: first the match of the pattern, then the match of the co-occurrence.
    * @return a relation map
    */
    t_relationmap getrightcooc(const PatternPointer & pattern, unsigned int occurrencethreshold = 0, int category = 0, unsigned int size = 0, std::vector<std::pair<IndexReference,IndexReference>> * matches = NULL) {
        if ((this->reverseindex == NULL) || (this->reverseindex->empty())) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            throw NoSuchPattern();
        }

        const int _n = pattern.n();
        t_relationmap cooc;
        //find everything that co-occurs *without overlap* TO THE RIGHT
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;


            std::vector<std::pair<IndexReference,PatternPointer>> rindex = this->getreverseindex_right(ref);
            for (std::vector<std::pair<IndexReference,PatternPointer>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const PatternPointer neighbour = iter2->second;
                if ( (ref2.token > ref.token + _n)
                        && ((occurrencethreshold == 0) || (this->occurrencecount(neighbour) >= occurrencethreshold))
                        && ((category == 0) || (neighbour.category() >= category))
                        && ((size == 0) || (neighbour.n() >= size))
                     ) {
                    cooc[neighbour]++;
                    if (matches != NULL) matches->push_back(std::pair<IndexReference,IndexReference>(ref,ref2));
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(cooc, occurrencethreshold);
        return cooc;
    }


    /**
     * Returns all patterns in the model that co-occur with the given pattern in the same sentence and appear to the left of the given pattern
    * @param occurrencethreshold If set above zero, filters to only include patterns occurring above this threshold
    * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to include only this category. Set to 0 for unfiltered (default)
    * @param size Set to any value above zero to only include patterns of the specified length.
    * @return a relation map
    */
    t_relationmap getleftcooc(const PatternPointer & pattern, unsigned int occurrencethreshold = 0, int category = 0, unsigned int size = 0) {
        if ((this->reverseindex == NULL) || (this->reverseindex->empty())) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            throw NoSuchPattern();
        }

        t_relationmap cooc;
        //find everything that co-occurs *without overlap* TO THE LEFT
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;

            std::vector<std::pair<IndexReference,PatternPointer>> rindex = this->getreverseindex_left(ref);
            for (std::vector<std::pair<IndexReference,PatternPointer>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const PatternPointer neighbour = iter2->second;
                const int _n = neighbour.n();
                if ( (ref2.token + _n < ref.token )
                        && ((occurrencethreshold == 0) || (this->occurrencecount(neighbour) >= occurrencethreshold))
                        && ((category == 0) || (neighbour.category() >= category))
                        && ((size == 0) || (neighbour.n() >= size))
                    ) {
                    cooc[neighbour]++;
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(cooc, occurrencethreshold);
        return cooc;
    }


    /**
     * Returns all patterns in the model that co-occur with the given pattern in the same sentence
    * @param occurrencethreshold If set above zero, filters to only include patterns occurring above this threshold
    * @param category Set to any value of PatternCategory (NGRAM,SKIPGRAM,FLEXGRAM) to include only this category. Set to 0 for unfiltered (default)
    * @param size Set to any value above zero to only include patterns of the specified length.
    * @param ordersignificant If set to true, each co-occuring pair will occur at least once in the result, if false (default) it will appear twice, once in A,B form and once in B,A form.
    * @return a relation map
    */
    t_relationmap getcooc(const PatternPointer & pattern, unsigned int occurrencethreshold = 0, int category = 0, unsigned int size = 0,bool ordersignificant = false) {
        if ((this->reverseindex == NULL) || (this->reverseindex->empty())) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            throw NoSuchPattern();
        }

        const int _n = pattern.n();
        t_relationmap cooc;
        //find everything that co-occurs *without overlap* TO THE RIGHT
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;


            std::vector<std::pair<IndexReference,PatternPointer>> rindex = this->getreverseindex_bysentence(ref.sentence);
            for (std::vector<std::pair<IndexReference,PatternPointer>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const PatternPointer neighbour = iter2->second;
                if ((ordersignificant) && (neighbour.pattern() < pattern)) continue;
                const int _n2 = neighbour.n();
                if ( ((ref2.token + _n2 < ref.token ) || (ref2.token > ref.token + _n))
                        && ((occurrencethreshold == 0) || (this->occurrencecount(neighbour) >= occurrencethreshold))
                        && ((category == 0) || (neighbour.category() >= category))
                        && ((size == 0) || (neighbour.n() >= size))
                    ) {
                    cooc[neighbour]++;
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(cooc, occurrencethreshold);
        return cooc;
    }

    /**
     * Compute normalised pointwise mutual information given two patterns and
     * their joint occurrence count.
     */
    double npmi(const PatternPointer & key1, const PatternPointer & key2, int jointcount) {
        //normalised pointwise mutual information
        return  log( (double) jointcount / (this->occurrencecount(key1) * this->occurrencecount(key2)) )  / -log((double)jointcount/(double)this->totaloccurrencesingroup(0,0) );
    }

    /**
     * Output the specified relation map for the specified pattern to output stream. Low-level function.
     * @param pattern The pattern
     * @param relations A relation map
     * @param classdecoder A class decoder
     * @param OUT The output stream
     * @param label A label to insert between relations (defaults to: RELATED-TO)
     */
    void outputrelations(const PatternPointer & pattern, t_relationmap & relations, ClassDecoder & classdecoder, std::ostream *OUT, const std::string label = "RELATED-TO") {
        int total = 0;
        for (t_relationmap::iterator iter = relations.begin(); iter != relations.end(); iter++) {
            total += iter->second;
        }
        if (total == 0) return;
        double total_f = total;
        const std::string pattern_s = pattern.tostring(classdecoder);
        for (t_relationmap::iterator iter = relations.begin(); iter != relations.end(); iter++) {
            const PatternPointer pattern2 = iter->first;
            *OUT << "\t" << pattern_s << "\t" << label << "\t" << pattern2.tostring(classdecoder) << "\t" << iter->second << "\t" << iter->second / total_f << "\t" << this->occurrencecount(pattern2) << std::endl;
        }
    }

    /**
     * Compute and output all possible relations for a given pattern. High-level function.
     * @param pattern The pattern
     * @param classdecoder A class decoder
     * @param OUT The output stream
     * @param filter default: none, set to string: subsumed/subparents, subsumes/subchildren,
     * rightneighbours, leftneighbours, rightcooc, leftcooc, skipcontent,
     * instances, templates
     * @param outputheader Output a header (default: true)
     */
    void outputrelations(const PatternPointer & pattern, ClassDecoder & classdecoder, std::ostream * OUT, const std::string filter="", bool outputheader=true) {
        if (outputheader) *OUT << "#\tPATTERN1\tRELATION\tPATTERN2\tREL.COUNT\tREL.FREQUENCY\tCOUNT2" << std::endl;


        if (filter.empty() || (filter == "subparents") || (filter == "subsumed")) {
            t_relationmap relations = this->getsubparents(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "SUBSUMED-BY");
        }
        if (filter.empty() || (filter == "subchildren") || (filter == "subsumes")){
            t_relationmap relations = this->getsubchildren(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "SUBSUMES");
        }
        if (filter.empty() || (filter == "rightneighbours") || (filter == "rightneighbors")){
            t_relationmap relations = this->getleftneighbours(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "RIGHT-NEIGHBOUR-OF");
        }
        if (filter.empty() || (filter == "leftneighbours") || (filter == "leftneighbors")){
            t_relationmap relations = this->getrightneighbours(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "LEFT-NEIGHBOUR-OF");
        }
        if (filter.empty() || (filter == "rightcooc")){
            t_relationmap relations = this->getrightcooc(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "LEFT-COOC-OF");
        }
        if (filter.empty() || (filter == "leftcooc")){
            t_relationmap relations = this->getleftcooc(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "RIGHT-COOC-OF");
        }
        if (filter.empty() || (filter == "skipcontent")){
            t_relationmap relations = this->getskipcontent(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "INSTANTIATED-BY");
        }
        if (filter.empty() || (filter == "instances")){
            t_relationmap relations = this->getinstances(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "INSTANCE-OF");
        }
        if (filter.empty() || (filter == "templates")){
            t_relationmap relations = this->gettemplates(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "TEMPLATE-OF");
        }

    }


    /*
     * Compute co-occurence as normalised pointwise mutual information for all patterns
     * @param coocmap The map that will store the results
     * @param threshold Only include pairs passing this NPMI threshold
     * @param right Compute co-occurence to the right  (default: true)
     * @param left Compute co-occurence to the left  (default: true)
     */
    void computenpmi( std::map<PatternPointer,t_relationmap_double> &  coocmap , double threshold, bool right=true, bool left=true) {
        //compute npmi co-occurrence for all patterns

        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const PatternType pattern = iter->first;
            t_relationmap tmp;
            if ((right)&&(!left)) {
                tmp =  this->getrightcooc(pattern);
            } else if ((left)&&(!right)) {
                tmp =  this->getleftcooc(pattern);
            } else if (left && right) { //order not relevant
                tmp =  this->getcooc(pattern);
            }
            for (t_relationmap::iterator iter2 = tmp.begin(); iter2 != tmp.end(); iter2++) {
                const PatternPointer pattern2 = iter2->first;
                const double value = npmi(pattern,pattern2,iter2->second);
                if (value >= threshold) coocmap[pattern][pattern2] = value;
            }
        }
    }

    /**
     * Compute co-occurence as absolute joint occurrence count, for all patterns
     * @param coocmap The map that will store the results
     * @param threshold Only include pairs passing this NPMI threshold
     * @param right Compute co-occurence to the right  (default: true)
     * @param left Compute co-occurence to the left  (default: true)
     */
    void computecooc( std::map<PatternPointer,t_relationmap> &  coocmap , int threshold, bool right=true, bool left=true) {
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const PatternType  pattern = iter->first;
            t_relationmap tmp;
            if ((right)&&(!left)) {
                tmp =  this->getrightcooc(pattern, threshold);
            } else if ((left)&&(!right)) {
                tmp =  this->getleftcooc(pattern, threshold);
            } else if (left && right) { //order not relevant
                tmp =  this->getcooc(pattern, threshold);
            }
            for (t_relationmap::iterator iter2 = tmp.begin(); iter2 != tmp.end(); iter2++) {
                const PatternPointer pattern2 = iter2->first;
                const double value = iter2->second;
                if (value >= threshold) coocmap[pattern][pattern2] = value;
            }
        }
    }

    /**
     * Compute flexgrams by abstracting from existing skipgrams in the model
     * @return The number of flexgrams found
     */
    int computeflexgrams_fromskipgrams() {
        this->cache_grouptotal.clear(); //forces recomputation of statistics
        int count = 0;
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const PatternType pattern = iter->first;
            if (pattern.category() == SKIPGRAM) {
                const PatternType flexgram = pattern.toflexgram();
                if (!this->has(flexgram)) count++;
                //copy data from pattern
                IndexedData * data = this->getdata(pattern);
                for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {
                    const IndexReference ref = *iter2;
                    this->data[flexgram].insert(ref);
                }
            }
        }
        if (count) this->hasflexgrams = true;
        return count;
    }


    /**
     * Compute flexgrams using co-occurrence
     * @param threshold Normalised Pointwise Mutual Information threshold
     * @return The number of flexgrams found
     */
    int computeflexgrams_fromcooc(double threshold) { //TODO: won't work in pattern pointer model!!
        this->cache_grouptotal.clear(); //forces recomputation of statistics
        int found = 0;
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType,PatternType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const Pattern pattern = iter->first;
            std::vector<std::pair<IndexReference,IndexReference>> matches;
            t_relationmap tmp =  this->getrightcooc(pattern, 0,0,0, &matches);
            for (t_relationmap::iterator iter2 = tmp.begin(); iter2 != tmp.end(); iter2++) {
                const Pattern pattern2 = iter2->first;
                const double value = npmi(pattern,pattern2,iter2->second);
                if (value >= threshold) {
                    const Pattern flexgram = pattern + FLEXPATTERN + pattern2;
                    if (!this->has(flexgram)) found++;
                    for (std::vector<std::pair<IndexReference,IndexReference>>::iterator iter3 = matches.begin(); iter3 != matches.end(); iter3++) {
                        this->data[flexgram].insert(iter3->first);
                    }
                }
            }
        }
        if (found) this->hasflexgrams = true;
        return found;
    }

    /**
     * Compute and output co-occurrence relations as Normalised Pointwise Mutual Information
     * @param threshold Normalised Pointwise Mutual Information threshold
     */
    void outputcooc_npmi(std::ostream * OUT, ClassDecoder& classdecoder, double threshold) {
        std::map<PatternPointer,t_relationmap_double> npmimap;
        std::cerr << "Collecting patterns and computing NPMI..." << std::endl;
        computenpmi(npmimap, threshold);

        std::cerr << "Building inverse map..." << std::endl;
        //we want the reverse, so we can sort by co-occurrence
        std::multimap<double,std::pair<PatternPointer,PatternPointer>> inversemap;
        std::map<PatternPointer,t_relationmap_double>::iterator iter = npmimap.begin();
        while (iter != npmimap.end()) {
            for (t_relationmap_double::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
                inversemap.insert(std::pair<double,std::pair<PatternPointer,PatternPointer>>(iter2->second, std::pair<Pattern,Pattern>(iter->first, iter2->first)));
            }
            iter = npmimap.erase(iter);
        }

        *OUT << "Pattern1\tPattern2\tNPMI" << std::endl;
        for (std::multimap<double,std::pair<PatternPointer,PatternPointer>>::reverse_iterator iter2 = inversemap.rbegin(); iter2 != inversemap.rend(); iter2++) {
            const PatternPointer pattern1 = iter2->second.first;
            const PatternPointer pattern2 = iter2->second.second;
            *OUT << pattern1.tostring(classdecoder) << "\t" << pattern2.tostring(classdecoder) << "\t" << iter2->first << std::endl;
        }
    }

    /**
     * Compute and output co-occurrence relations as joint occurrence count
     * @param threshold Normalised Pointwise Mutual Information threshold
     */
    void outputcooc(std::ostream * OUT, ClassDecoder& classdecoder, double threshold) {
        std::map<PatternPointer,t_relationmap> coocmap;
        std::cerr << "Collecting patterns and computing co-occurrence..." << std::endl;
        computecooc(coocmap, threshold);

        std::cerr << "Building inverse map..." << std::endl;
        //we want the reverse, so we can sort by co-occurrence
        std::multimap<uint32_t,std::pair<PatternPointer,PatternPointer>> inversemap;
        std::map<PatternPointer,t_relationmap>::iterator iter = coocmap.begin();
        while (iter != coocmap.end()) {
            for (t_relationmap::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
                inversemap.insert(std::pair<uint32_t,std::pair<PatternPointer,PatternPointer>>(iter2->second, std::pair<PatternPointer,PatternPointer>(iter->first, iter2->first)));
            }
            iter = coocmap.erase(iter);
        }

        *OUT << "Pattern1\tPattern2\tCooc" << std::endl;
        for (std::multimap<uint32_t,std::pair<PatternPointer,PatternPointer>>::reverse_iterator iter2 = inversemap.rbegin(); iter2 != inversemap.rend(); iter2++) {
            const Pattern pattern1 = iter2->second.first;
            const Pattern pattern2 = iter2->second.second;
            *OUT << pattern1.tostring(classdecoder) << "\t" << pattern2.tostring(classdecoder) << "\t" << iter2->first << std::endl;
        }
    }

    /**
     * attempt to find the flexgram size for the given begin position,
     * returns 0 if the flexgram was not found at all
     * if there are multiple matches, the shortest is returned
     */
    int flexgramsize(const Pattern & pattern, IndexReference begin) {

        if (pattern.category() != FLEXGRAM) return pattern.n();

        std::vector<Pattern> parts;
        int numberofparts = pattern.parts(parts);
        bool strictbegin = true;
        std::multimap<int, IndexReference> partmatches;
        int i = 0;
        std::vector<std::pair<IndexReference,PatternPointer>> rindex = this->getreverseindex_right(begin); //TODO: Check
        for (std::vector<std::pair<IndexReference,PatternPointer>>::iterator iter = rindex.begin(); iter != rindex.end(); iter++) {
            const PatternPointer part = iter->second;
            IndexReference ref = iter->first;
            partmatches.insert(std::pair<int,IndexReference>(i, ref));
            i++;
        }

        int firsttoken = begin.token;
        IndexReference nextbegin = IndexReference(begin.sentence,999);
        for (int j = 0; j < numberofparts; j++) {
           //find a path
           int prevlevel = -1;
           bool found = false;
           for (std::multimap<int, IndexReference>::iterator iter = partmatches.lower_bound(j); iter != partmatches.upper_bound(j); iter++) {
                found = true;
                if (iter->first != prevlevel) {
                    begin = nextbegin;
                    nextbegin = IndexReference(begin.sentence,999); //reset
                }
                if (((iter->second == begin) || (begin < iter->second)) && (iter->second + parts[j].n() + 1 < nextbegin)) {
                    nextbegin = iter->second + parts[j].n() + 1;
                }
                prevlevel = iter->first;
           }
           if (!found) return 0;
        }
        return (nextbegin.token - firsttoken);
    }

};

template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class MapType = PatternPointerMap<ValueType, BaseValueHandler<ValueType>>>
class PatternPointerModel: public PatternModel<ValueType,ValueHandler,MapType,PatternPointer> {
    public:

        PatternPointerModel<ValueType,ValueHandler,MapType>(IndexedCorpus * corpus): PatternModel<ValueType,ValueHandler,MapType,PatternPointer>() {
            this->model_type = this->getmodeltype();
            this->model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
                this->attachcorpus(*corpus);
            } else {
                this->reverseindex = NULL;
            }
        }


        /**
        * Read a pattern model from an input stream
        * @param f The input stream
        * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
        * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
        * @param corpus Pointer to the loaded corpus, used as a reverse index.
        */
        PatternPointerModel<ValueType,ValueHandler,MapType>(std::istream *f, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL):  PatternModel<ValueType,ValueHandler,MapType,PatternPointer>(){ //load from file
            this->model_type = this->getmodeltype();
            this->model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
                this->attachcorpus(*corpus);
            } else {
                this->reverseindex = NULL;
            }
            this->load(f,options, constrainmodel);
        }

        /**
        * Read a pattern model from file
        * @param filename The filename
        * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
        * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
        * @param corpus Pointer to the loaded corpus, used as a reverse index.
        */
        PatternPointerModel<ValueType,ValueHandler,MapType>(const std::string filename, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL): PatternModel<ValueType,ValueHandler,MapType,PatternPointer>() { //load from file
            this->model_type = this->getmodeltype();
            this->model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
                this->attachcorpus(*corpus);
            } else {
                this->reverseindex = NULL;
            }
            std::ifstream * in = new std::ifstream(filename.c_str());
            this->load( (std::istream *) in, options, constrainmodel);
            in->close();
            delete in;
        }

        int getmodeltype() const { return UNINDEXEDPATTERNPOINTERMODEL; }
        int getmodelversion() const { return 2;}

        /**
         * Add a pattern, with a given position, to the model. This
         * is called during training at every time an instance of a pattern is found in the data.
         * This is the high-level version.
         * @param pattern The pattern to add (a patternpointer)
         * @param ref The position in the corpus where the patterns occurs
         */
        virtual void add(const PatternPointer & patternpointer, const IndexReference & ref) {
            if ((patternpointer.data < this->reverseindex->beginpointer()) || (patternpointer.data > this->reverseindex->beginpointer() + this->reverseindex->bytesize())) {
                std::cerr << "Pattern Pointer points outside contained corpus data..." << std::endl;
                throw InternalError();
            }
            ValueType * data = this->getdata(patternpointer, true);
            /*std::cerr << "Adding: n="<< patternpointer.n() << ",b=" << patternpointer.bytesize() << ",hash="<<patternpointer.hash()<<", value=" << *data << ",valuetype="<< (size_t) data << ",mask=" << patternpointer.mask << ",pattern=";
            patternpointer.out();
            std::cerr << std::endl;*/
            this->add(patternpointer, data, ref );
            /*std::cerr << "  Hash recheck: " << patternpointer.hash() << std::endl;
            std::cerr << "  Pattern hash recheck: " << Pattern(patternpointer).hash() << std::endl;
            std::cerr << "  Equivalence with Pattern: " << (int) (patternpointer == Pattern(patternpointer)) << std::endl;
            std::cerr << "  Equivalence with Pattern 2: " << (int) (Pattern(patternpointer) == patternpointer) << std::endl;
            std::cerr << "  New value verification: " << this->occurrencecount(patternpointer) << " == " << *data << std::endl;
            ValueType * data2 = this->getdata(patternpointer, true);
            std::cerr << "  New value verification (2): " << *data << " == " << *data2 << std::endl;
            std::cerr << "  New value verification (pointer): " << (size_t) data << " == " << (size_t) data2 << std::endl;*/
        }

        virtual void add(const PatternPointer &, ValueType * value, const IndexReference & ref) {
            if (value == NULL) {
                std::cerr << "Add() value is NULL!" << std::endl;
                throw InternalError();
            }
            this->valuehandler.add(value, ref);
        }

};



template<class MapType=PatternPointerMap<IndexedData, IndexedDataHandler>>
class IndexedPatternPointerModel: public IndexedPatternModel<MapType,PatternPointer> {
    public:

        IndexedPatternPointerModel<MapType>(IndexedCorpus * corpus): IndexedPatternModel<MapType,PatternPointer>() {
            this->model_type = this->getmodeltype();
            this->model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
                this->attachcorpus(*corpus);
            } else {
                this->reverseindex = NULL;
            }
        }


        /**
        * Read a pattern model from an input stream
        * @param f The input stream
        * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
        * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
        * @param corpus Pointer to the loaded corpus, used as a reverse index.
        */
        IndexedPatternPointerModel<MapType>(std::istream *f, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL): IndexedPatternModel<MapType,PatternPointer>(){ //load from file
            this->model_type = this->getmodeltype();
            this->model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
                this->attachcorpus(*corpus);
            } else {
                this->reverseindex = NULL;
            }
            this->load(f,options, constrainmodel);
        }

        /**
        * Read a pattern model from file
        * @param filename The filename
        * @param options Options for reading, these act as filter for the data, allowing you to raise thresholds etc
        * @param constrainmodel Pointer to another pattern model which should be used to constrain the loading of this one, only patterns also occurring in the other model will be included. Defaults to NULL (no constraining)
        * @param corpus Pointer to the loaded corpus, used as a reverse index.
        */
        IndexedPatternPointerModel<MapType>(const std::string filename, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL): IndexedPatternModel<MapType,PatternPointer>() { //load from file
            this->model_type = this->getmodeltype();
            this->model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
                this->attachcorpus(*corpus);
            } else {
                this->reverseindex = NULL;
            }
            std::ifstream * in = new std::ifstream(filename.c_str());
            this->load( (std::istream *) in, options, constrainmodel);
            in->close();
            delete in;
        }

        int getmodeltype() const { return INDEXEDPATTERNPOINTERMODEL; }
        int getmodelversion() const { return 2;}

        /**
        * Add a pattern, with a given position, and a value to the model. This
        * is called during training at every time an instance of a pattern is found in the data.
        * @param pattern The pattern to add
        * @param value A pointer to the value for this pattern, set to NULL and it will be automatically determined
        * @param IndexReference The position in the corpus where the patterns occurs
        */
        void add(const PatternPointer & patternpointer, const IndexReference & ref) {
            if ((patternpointer.data < this->reverseindex->beginpointer()) || (patternpointer.data > this->reverseindex->beginpointer() + this->reverseindex->bytesize())) {
                std::cerr << "Pattern Pointer points outside contained corpus data..." << std::endl;
                throw InternalError();
            }
            IndexedData * data = this->getdata(patternpointer, true);
            this->add(patternpointer, data, ref );
        }

        void add(const PatternPointer & patternpointer, IndexedData * value, const IndexReference & ref) {
            if (value == NULL) {
                value = this->getdata(patternpointer,true);
            }
            this->valuehandler.add(value, ref);
        }
};

double comparemodels_loglikelihood(const Pattern pattern, std::vector<PatternModel<uint32_t>* > & models);
void comparemodels_loglikelihood(std::vector<PatternModel<uint32_t>* > & models, PatternMap<double> * resultmap, bool conjunctiononly = false, std::ostream * output = NULL, ClassDecoder * classdecoder = NULL );


#endif
