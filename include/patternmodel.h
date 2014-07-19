#ifndef PATTERNMODEL_H
#define PATTERNMODEL_H

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


//TODO: Reverse index currently does not hold flexgrams

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
#include "bz2stream.h"


enum ModelType {
	UNINDEXEDPATTERNMODEL = 10, 
    INDEXEDPATTERNMODEL = 20,
    PATTERNSETMODEL = 30,
    PATTERNALIGNMENTMODEL = 40,
};

enum ReverseIndexType {
    NONE = 0,
    QUICK = 1,
    COMPACT = 2,
};

int getmodeltype(const std::string filename);

class NoSuchPattern: public std::exception {
  virtual const char* what() const throw()
  {
    return "Pattern not found in model";
  }
};



class PatternModelOptions {
    public:
        int MINTOKENS;
        int MINTOKENS_SKIPGRAMS;
        int MINLENGTH;
        int MAXLENGTH;
        int MAXBACKOFFLENGTH;
        
        bool DOSKIPGRAMS;
        bool DOSKIPGRAMS_EXHAUSTIVE;
        int MINSKIPTYPES; 

        bool DOREVERSEINDEX;
        bool DOPATTERNPERLINE;

        bool DOREMOVEINDEX;
        bool DOREMOVENGRAMS;
        bool DOREMOVESKIPGRAMS;
        bool DOREMOVEFLEXGRAMS;
        bool DORESET; //sets all counts to zero upon loading, clears indices

        bool QUIET;
        bool DEBUG;

        PatternModelOptions() {
            MINTOKENS = -1; //defaults to 2 for building, 1 for loading
            MINTOKENS_SKIPGRAMS = -1; //defaults to MINTOKENS
            MINLENGTH = 1;
            MAXLENGTH = 100;
            MAXBACKOFFLENGTH = 100;

            MINSKIPTYPES = 2;
            DOSKIPGRAMS = false;
            DOSKIPGRAMS_EXHAUSTIVE = false;

            DOREVERSEINDEX = true; //only for indexed models
            DOPATTERNPERLINE = false;
            DORESET = false;

            DOREMOVEINDEX = false; //only for indexed models
            DOREMOVENGRAMS = false;
            DOREMOVESKIPGRAMS = false;
            DOREMOVEFLEXGRAMS = false;

            DEBUG = false;
            QUIET = false;
        }

        //copy constructor
        PatternModelOptions(const PatternModelOptions & ref) {
            MINTOKENS = ref.MINTOKENS; //defaults to 2 for building, 1 for loading
            MINTOKENS_SKIPGRAMS = ref.MINTOKENS_SKIPGRAMS; //defaults to 2 for building, 1 for loading
            MINLENGTH = ref.MINLENGTH;
            MAXLENGTH = ref.MAXLENGTH;
            MAXBACKOFFLENGTH = ref.MAXBACKOFFLENGTH;

            MINSKIPTYPES = ref.MINSKIPTYPES;
            DOSKIPGRAMS = ref.DOSKIPGRAMS;
            DOSKIPGRAMS_EXHAUSTIVE = ref.DOSKIPGRAMS_EXHAUSTIVE;

            DOREVERSEINDEX = ref.DOREVERSEINDEX; //only for indexed models
            DOPATTERNPERLINE = ref.DOPATTERNPERLINE;
            DORESET = ref.DORESET;

            DOREMOVEINDEX = ref.DOREMOVEINDEX; //only for indexed models
            DOREMOVENGRAMS = ref.DOREMOVENGRAMS;
            DOREMOVESKIPGRAMS = ref.DOREMOVESKIPGRAMS;
            DOREMOVEFLEXGRAMS = ref.DOREMOVEFLEXGRAMS;

            DEBUG = ref.DEBUG;
            QUIET = ref.QUIET;
        }


};

typedef PatternMap<uint32_t> t_relationmap;
typedef PatternMap<double> t_relationmap_double;

//basic read-only interface for pattern models, abstract base class.
class PatternModelInterface: public PatternStoreInterface {
    public:
        virtual int getmodeltype() const=0;
        virtual int getmodelversion() const=0;
        
        //these are already in PatternStoreInterface:
            //virtual bool has(const Pattern &) const =0;
            //virtual bool has(const PatternPointer &) const =0;
            //virtual size_t size() const =0; 

        virtual int occurrencecount(const Pattern & pattern)=0;
        virtual double frequency(const Pattern &) =0;
        virtual int maxlength() const=0;
        virtual int minlength() const=0;
        virtual int types() =0;
        virtual int tokens() const=0;

        virtual PatternStoreInterface * getstoreinterface() {
            return (PatternStoreInterface*) this;
        };
};



//A pattern model based on an unordered set, does not hold data, only patterns
//Very suitable for loading constraint models.  (uint64 refers to max count)
class PatternSetModel: public PatternSet<uint64_t>, public PatternModelInterface {
    protected:
        unsigned char model_type;
        unsigned char model_version;
        uint64_t totaltokens; //INCLUDES TOKENS NOT COVERED BY THE MODEL!
        uint64_t totaltypes; //TOTAL UNIGRAM TYPES, INCLUDING NOT COVERED BY THE MODEL!
        int maxn; 
        int minn; 
    public:
        PatternSetModel() {
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
        }
        PatternSetModel(std::istream *f, PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) { //load from file
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            this->load(f,options, constrainmodel);
        }
        PatternSetModel(const std::string filename, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) { //load from file
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
        virtual int getmodelversion() const { return 1; }

        virtual size_t size() const {
            return PatternSet<uint64_t>::size();
        }
        virtual bool has(const Pattern & pattern) const {
            return PatternSet<uint64_t>::has(pattern);
        }
        virtual bool has(const PatternPointer & pattern) const {
            return PatternSet<uint64_t>::has(pattern);
        }
        
        virtual void load(std::string filename, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) {
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

        virtual void load(std::istream * f, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) { //load from file
            char null;
            f->read( (char*) &null, sizeof(char));        
            f->read( (char*) &model_type, sizeof(char));        
            f->read( (char*) &model_version, sizeof(char));        
            if ((null != 0) || ((model_type != UNINDEXEDPATTERNMODEL) && (model_type != INDEXEDPATTERNMODEL) && (model_type != PATTERNSETMODEL) && (model_type != PATTERNALIGNMENTMODEL) ))  {
                std::cerr << "ERROR: File is not a colibri patternmodel file (or a very old one)" << std::endl;
                throw InternalError();
            }
            f->read( (char*) &totaltokens, sizeof(uint64_t));        
            f->read( (char*) &totaltypes, sizeof(uint64_t)); 

            PatternStoreInterface * constrainstore = NULL;
            if (constrainmodel) constrainstore = constrainmodel->getstoreinterface();

            if (options.DEBUG) { 
                std::cerr << "Debug enabled, loading PatternModel type " << (int) model_type << ", version " << (int) model_version << std::endl;   
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

        void write(const std::string filename) {
            std::ofstream * out = new std::ofstream(filename.c_str());
            this->write(out);
            out->close();
            delete out;
        }

        PatternModelInterface * getinterface() {
            return (PatternModelInterface*) this;
        }

        //these are useless in set context, they always return 0
        virtual int occurrencecount(const Pattern & pattern) { return 0;  }
        virtual double frequency(const Pattern &) { return 0; }

        typedef typename PatternSet<uint64_t>::iterator iterator;
        typedef typename PatternSet<uint64_t>::const_iterator const_iterator;        

        virtual int maxlength() const { return maxn; };
        virtual int minlength() const { return minn; };
        
        virtual int types()  { 
            return totaltypes;
        }
        virtual int tokens() const { return totaltokens; }

        unsigned char type() const { return model_type; }
        unsigned char version() const { return model_version; }
};

template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class MapType = PatternMap<ValueType, BaseValueHandler<ValueType>>>
class PatternModel: public MapType, public PatternModelInterface {
    protected:
        unsigned char model_type;
        unsigned char model_version;
        uint64_t totaltokens; //INCLUDES TOKENS NOT COVERED BY THE MODEL!
        uint64_t totaltypes; //TOTAL UNIGRAM TYPES, INCLUDING NOT COVERED BY THE MODEL!

        int maxn; 
        int minn; 
        
        //std::multimap<IndexReference,Pattern> reverseindex; 
        std::set<int> cache_categories;
        std::set<int> cache_n;
        std::map<int,std::map<int,int>> cache_grouptotal; //total occurrences (used for frequency computation, within a group)
        std::map<int,std::map<int,int>> cache_grouptotalpatterns ; //total distinct patterns per group
        std::map<int,std::map<int,int>> cache_grouptotalwordtypes; //total covered word types per group
        std::map<int,std::map<int,int>> cache_grouptotaltokens; //total covered tokens per group
        

        std::map<int, std::vector< std::vector< std::pair<int,int> > > > gapconf;

        virtual void postread(const PatternModelOptions options) {
            //this function has a specialisation specific to indexed pattern models,
            //this is the generic version
            for (iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern p = iter->first;
                const int n = p.n();
                if (n > maxn) maxn = n;
                if (n < minn) minn = n;
                if ((!hasskipgrams) && (p.isskipgram())) hasskipgrams = true;
            }
        }
        virtual void posttrain(const PatternModelOptions options) {
            //nothing to do here, indexed model specialised this function to
            //sort indices
        }
    public:
        IndexedCorpus * reverseindex;
        bool externalreverseindex; //true if reverse index was loaded externally and passed to the model (implies it won't be destroyed when the model is) //only used by IndexedPatternModel but stored here to ease things for cython
        bool hasskipgrams;

        PatternModel<ValueType,ValueHandler,MapType>(IndexedCorpus * corpus = NULL) {
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            hasskipgrams = false;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
            } else {
                this->reverseindex = NULL;
            }
            this->externalreverseindex = true;
        }
        PatternModel<ValueType,ValueHandler,MapType>(std::istream *f, PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL) { //load from file
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            hasskipgrams = false;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            this->load(f,options,constrainmodel);
            if (corpus) {
                this->reverseindex = corpus;
            } else {
                this->reverseindex = NULL;
            }
            this->externalreverseindex = true;
        }

        PatternModel<ValueType,ValueHandler,MapType>(const std::string filename, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL) { //load from file
            //IndexedPatternModel will overload this
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            hasskipgrams = false;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            if (corpus) {
                this->reverseindex = corpus;
            } else {
                this->reverseindex = NULL;
            }
            this->externalreverseindex = true;
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

        virtual int getmodeltype() const { return UNINDEXEDPATTERNMODEL; }
        virtual int getmodelversion() const { return 1; }

        virtual size_t size() const {
            return MapType::size();
        }
        virtual bool has(const Pattern & pattern) const {
            return MapType::has(pattern);
        }
        virtual bool has(const PatternPointer & pattern) const {
            return MapType::has(pattern);
        }
        
        virtual void load(std::string filename, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) {
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

        virtual void load(std::istream * f, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) { //load from file
            char null;
            f->read( (char*) &null, sizeof(char));        
            f->read( (char*) &model_type, sizeof(char));        
            f->read( (char*) &model_version, sizeof(char));        
            if ((null != 0) || ((model_type != UNINDEXEDPATTERNMODEL) && (model_type != INDEXEDPATTERNMODEL) && (model_type != PATTERNALIGNMENTMODEL) ))  {
                std::cerr << "File is not a colibri model file (or a very old one)" << std::endl;
                throw InternalError();
            }
            f->read( (char*) &totaltokens, sizeof(uint64_t));        
            f->read( (char*) &totaltypes, sizeof(uint64_t)); 

            PatternStoreInterface * constrainstore = NULL;
            if (constrainmodel) constrainstore = constrainmodel->getstoreinterface();

            if (options.DEBUG) { 
                std::cerr << "Debug enabled, loading PatternModel type " << (int) model_type << ", version " << (int) model_version << std::endl;   
                std::cerr << "Total tokens: " << totaltokens << ", total types: " << totaltypes << std::endl;;   
            }

            if ((model_type == INDEXEDPATTERNMODEL) && (this->getmodeltype() == UNINDEXEDPATTERNMODEL)) {
                //reading indexed pattern model as unindexed, ok:
                 MapType::template read<IndexedData,IndexedDataHandler>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore,  !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET, options.DEBUG);
            } else if ((model_type == UNINDEXEDPATTERNMODEL) && (this->getmodeltype() == INDEXEDPATTERNMODEL)) {
               //reading unindexed model as indexed, this will load the patterns but lose all the counts
                 MapType::template read<uint32_t,BaseValueHandler<uint32_t>>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET, options.DEBUG);
            } else if (model_type == PATTERNALIGNMENTMODEL)  {
                 //reading pattern alignment model as pattern model, can be
                 //done, but semantics change:  count corresponds to the number of distinct alignments (for unindexed models)
                 //indexed models will lose all counts
                MapType::template read<PatternFeatureVectorMap<double>,PatternFeatureVectorMapHandler<double>>(f, options.MINTOKENS, options.MINLENGTH,options.MAXLENGTH, constrainstore,  !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS,options.DORESET,options.DEBUG);
            } else {
                 MapType::template read(f, options.MINTOKENS,options.MINLENGTH, options.MAXLENGTH, constrainstore, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET, options.DEBUG); //read PatternStore (also works for reading unindexed pattern models as indexed, which will load patterns but lose the counts)
            }
            this->postread(options);
        }

        PatternModelInterface * getinterface() {
            return (PatternModelInterface*) this;
        }

        virtual void train(std::istream * in , PatternModelOptions options,  PatternModelInterface * constrainbymodel = NULL) {
            if (options.MINTOKENS == -1) options.MINTOKENS = 2;
            if (options.MINTOKENS == 0)  options.MINTOKENS = 1;
            if (options.MINTOKENS_SKIPGRAMS < options.MINTOKENS) options.MINTOKENS_SKIPGRAMS = options.MINTOKENS;            
            if (constrainbymodel == this) {
                totaltypes = 0;
                totaltokens = 0;
            }
            uint32_t sentence = 0;
            if (!in->good()) {
                std::cerr << "ERROR: inputstream not good, file does not exist?" << std::endl;
                throw InternalError();
            }

            if (!options.QUIET) {
                std::cerr << "Training patternmodel";
                if (constrainbymodel != NULL) std::cerr << ", constrained by another model";
                std::cerr << ", occurrence threshold: " << options.MINTOKENS;
                std::cerr << std::endl; 
            }
            std::vector<std::pair<PatternPointer,int>> ngrams;
            std::vector<PatternPointer> subngrams;
            bool found;
            IndexReference ref;
            int prevsize = 0;
            int backoffn = 0;
            for (int n = 1; n <= options.MAXLENGTH; n++) { 
                int foundngrams = 0;
                int foundskipgrams = 0;
                in->clear();
                in->seekg(0);
                if (!options.QUIET) {
                    if (options.DOPATTERNPERLINE) {
                        std::cerr << "Counting patterns from list, one per line" << std::endl; 
                    } else if (constrainbymodel != NULL) {
                        std::cerr << "Counting n-grams that occur in constraint model" << std::endl;
                    } else if (options.MINTOKENS > 1) {
                        std::cerr << "Counting " << n << "-grams" << std::endl; 
                    } else {
                        std::cerr << "Counting *all* n-grams (occurrence threshold=1)" << std::endl;
                    }
                }

                if ((options.DOSKIPGRAMS_EXHAUSTIVE) && (gapconf[n].empty())) compute_multi_skips(gapconf[n], std::vector<std::pair<int,int> >(), n);
                
                sentence = 0; //reset
                while (!in->eof()) {
                    //read line
                    Pattern line = Pattern(in);
                    sentence++;
                    //if (in->eof()) break;
                    const int linesize = line.size();
                    if (linesize == 0) {
                        //skip empty lines
                        continue;
                    }
                    //count total tokens
                    if (n==1) totaltokens += linesize;

                    ngrams.clear();
                    if (options.DOPATTERNPERLINE) {
                        if (linesize > options.MAXLENGTH) continue;
                        ngrams.push_back(std::pair<PatternPointer,int>(PatternPointer(&line),0));
                    } else {
                        if ((options.MINTOKENS > 1) && (constrainbymodel == NULL)) {
                            line.ngrams(ngrams, n);
                        } else {
                            line.subngrams(ngrams,options.MINLENGTH,options.MAXLENGTH); //extract ALL ngrams if MINTOKENS == 1 or a constraint model is set, no need to look back anyway, only one iteration over corpus
                        }
                    }

                    for (std::vector<std::pair<PatternPointer,int>>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
                        //check against constraint model
                        if ((constrainbymodel != NULL) && (!constrainbymodel->has(iter->first))) continue; 

                        ref = IndexReference(sentence, iter->second);
                        found = true; //default to true (for mintokens == 1)
                        if ((n > 1) && (options.MINTOKENS > 1) && (!options.DOPATTERNPERLINE) && (constrainbymodel == NULL)) {
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
                        if (found) {
                            const Pattern pattern = Pattern(iter->first);
                            ValueType * data = getdata(pattern, true);
                            add(pattern, data, ref );
                            if ((options.DOREVERSEINDEX) && (n == 1) && (reverseindex != NULL) && (!externalreverseindex)) {
                               reverseindex->push_back(ref, pattern); //TODO: make patternpointer
                            }
                        } 
                        if (((n >= 3) || (options.MINTOKENS == 1)) //n is always 1 when mintokens == 1 !!
                                && (options.DOSKIPGRAMS_EXHAUSTIVE)) {
                            int foundskipgrams_thisround = this->computeskipgrams(iter->first, options, &ref, NULL, constrainbymodel, true);
                            if (foundskipgrams_thisround > 0) hasskipgrams = true;
                            foundskipgrams += foundskipgrams_thisround; 
                        }
                    }
                }

                
                foundngrams = this->size() - foundskipgrams - prevsize;
        
                if (foundngrams) {
                    if (n > this->maxn) this->maxn = n;
                    if (n < this->minn) this->minn = n;
                } else {
                    if (!options.QUIET) std::cerr << "None found" << std::endl;
                    break;
                }
                if (!options.QUIET) std::cerr << " Found " << foundngrams << " ngrams...";
                if (options.DOSKIPGRAMS_EXHAUSTIVE && !options.QUIET) std::cerr << foundskipgrams << " skipgram occurrences...";
                if ((options.MINTOKENS > 1) && (n == 1)) totaltypes += this->size(); //total unigrams, also those not in model
                int pruned;
                if ((options.MINTOKENS == 1) || (constrainbymodel != NULL)) {
                    pruned = this->prune(options.MINTOKENS,0); //prune regardless of size
                } else {
                    pruned = this->prune(options.MINTOKENS,n); //prune only in size-class
                    if ( (!options.DOSKIPGRAMS) && (!options.DOSKIPGRAMS_EXHAUSTIVE) &&  ( n - 1 >= 1) &&  ( (n - 1) < options.MINLENGTH) && (n - 1 != options.MAXBACKOFFLENGTH) ) {
                        //we don't need n-1 anymore now we're done with n, it
                        //is below our threshold, prune it all (== -1)
                        this->prune(-1, n-1);
                        if (!options.QUIET) std::cerr << "(pruned last iteration due to minimum length)" << pruned;
                    }
                }
                if (!options.QUIET) std::cerr << "pruned " << pruned;
                if (foundskipgrams) {
                    int prunedextra = this->pruneskipgrams(options.MINTOKENS_SKIPGRAMS, options.MINSKIPTYPES, n);
                    if (prunedextra && !options.QUIET) std::cerr << " plus " << prunedextra << " extra skipgrams..";
                    pruned += prunedextra;
                }
                if (!options.QUIET) std::cerr << "...total kept: " << (foundngrams + foundskipgrams) - pruned << std::endl;
                if ((options.MINTOKENS == 1) || (constrainbymodel != NULL)) break; //no need for further n iterations, we did all in one pass since there's no point in looking back
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
            this->posttrain(options);
        }


        virtual void train(const std::string filename, const PatternModelOptions options, PatternModelInterface * constrainbymodel = NULL) {
            if ((filename.size() > 3) && (filename.substr(filename.size()-3) == ".bz2")) {
                std::ifstream * in = new std::ifstream(filename.c_str(), std::ios::in|std::ios::binary);
                bz2istream * decompressor = new bz2istream(in->rdbuf());
                this->train( (std::istream*) decompressor, options, constrainbymodel);
                delete decompressor;
                delete in;
            } else {
                std::ifstream * in = new std::ifstream(filename.c_str());
                this->train((std::istream*) in, options, constrainbymodel);
                in->close();
                delete in;
            }
        }




        
        virtual int computeskipgrams(const Pattern & pattern, int mintokens = 2,  const IndexReference * singleref= NULL, const IndexedData * multiplerefs = NULL,  PatternModelInterface * constrainbymodel = NULL, std::vector<Pattern> * targetcontainer = NULL,  const bool exhaustive = false, const bool DEBUG = false) {

            //if targetcontainer is NULL, skipgrams will be added to the model,
            // if not null , they will be added to the targetcontainer instead

            if (mintokens == -1) mintokens = 2;;
            if (mintokens  <= 1) {
                mintokens = 1;
            }
            //internal function for computing skipgrams for a single pattern
            int foundskipgrams = 0;
            const int n = pattern.n();
            if (gapconf[n].empty()) compute_multi_skips(gapconf[n], std::vector<std::pair<int,int> >(), n);
            //loop over all possible gap configurations
            for (std::vector<std::vector<std::pair<int,int>>>::iterator iter2 =  gapconf[n].begin(); iter2 != gapconf[n].end(); iter2++) {
                std::vector<std::pair<int,int>> * gapconfiguration = &(*iter2);

                //integrity check
                for (std::vector<std::pair<int,int>>::iterator giter = gapconfiguration->begin(); giter != gapconfiguration->end(); giter++) {
                    if (giter->first + giter->second > n) {
                        std::cerr << "ERROR: Gapconf is not valid for n=" << n << ": " << giter->first << ","  << giter->second << " is out of bounds!" << std::endl;
                        throw InternalError();
                    }
                }

                //add skips
                try {
                    const Pattern skipgram = pattern.addskips(*gapconfiguration);                            
                    if (DEBUG) {
                        std::cerr << "Checking for: " << std::endl;
                        skipgram.out();
                    }

                    if ((constrainbymodel != NULL) && (!constrainbymodel->has(skipgram))) continue;
                    if ((int) skipgram.n() != n) {
                        std::cerr << "Generated invalid skipgram, n=" << skipgram.n() << ", expected " << n << std::endl;
                        throw InternalError();
                    }

                    bool skipgram_valid = true;
                    if ((mintokens != 1) && (constrainbymodel == NULL)) {
                        bool check_extra = false;
                        //check if sub-parts were counted
                        std::vector<Pattern> subngrams;
                        skipgram.ngrams(subngrams,n-1); //this also works for and returns skipgrams, despite the name
                        for (std::vector<Pattern>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) { //only two patterns
                            const Pattern subpattern = *iter2;
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
                                std::vector<Pattern> parts;
                                skipgram.parts(parts);
                                for (std::vector<Pattern>::iterator iter3 = parts.begin(); iter3 != parts.end(); iter3++) {
                                    const Pattern part = *iter3;
                                    if (!this->has(part)) {
                                        skipgram_valid = false;
                                        break;
                                    }
                                }
                                if (!skipgram_valid) continue;
                            }

                            //check whether the the gaps with single token context (X * Y) occur in model,
                            //otherwise skipgram can't occur
                            for (std::vector<std::pair<int,int>>::iterator iter3 = gapconfiguration->begin(); iter3 != gapconfiguration->end(); iter3++) {
                                if (!((iter3->first - 1 == 0) && (iter3->first + iter3->second + 1 == n))) { //entire skipgarm is already X * Y format
                                    const Pattern subskipgram = Pattern(skipgram, iter3->first - 1, iter3->second + 2);
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
                        if (DEBUG) std::cerr << "  counted!" << std::endl;
                        if (targetcontainer == NULL) {
                            //put in model
                            if  (!has(skipgram)) foundskipgrams++;
                            ValueType * data = this->getdata(skipgram,true);
                            if (singleref != NULL) {
                                add(skipgram, data, *singleref ); //counts the actual skipgram, will add it to the model
                            } else if (multiplerefs != NULL) {
                                for (IndexedData::const_iterator refiter =  multiplerefs->begin(); refiter != multiplerefs->end(); refiter++) {
                                    const IndexReference ref = *refiter;
                                    add(skipgram, data, ref ); //counts the actual skipgram, will add it to the model
                                }
                            } else {
                                std::cerr << "ERROR: computeskipgrams() called with no singleref and no multiplerefs" << std::endl;
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

        virtual int computeskipgrams(const Pattern & pattern, PatternModelOptions & options ,  const IndexReference * singleref= NULL, const IndexedData * multiplerefs = NULL,  PatternModelInterface * constrainbymodel = NULL, const bool exhaustive = false) { //backward compatibility
            if (options.MINTOKENS_SKIPGRAMS < options.MINTOKENS) options.MINTOKENS_SKIPGRAMS = options.MINTOKENS;
            return computeskipgrams(pattern, options.MINTOKENS_SKIPGRAMS, singleref, multiplerefs, constrainbymodel, NULL, exhaustive, options.DEBUG);
        }

        virtual std::vector<Pattern> findskipgrams(const Pattern & pattern, int occurrencethreshold = 1) {
            //given the pattern, find all skipgrams in it that occur in the model

            std::vector<Pattern> skipgrams;
            this->computeskipgrams(pattern, occurrencethreshold, NULL, NULL, this->getinterface(), &skipgrams);
            return skipgrams;
        }


        virtual void trainskipgrams(const PatternModelOptions options,  PatternModelInterface * constrainbymodel = NULL) { 
            std::cerr << "Can not compute skipgrams on unindexed model (except exhaustively during train() )" << std::endl;
            throw InternalError();
        }

        //creates a new test model using the current model as training
        // i.e. only fragments existing in the training model are counted
        // remaining fragments are 'uncovered'
        void test(MapType & target, std::istream * in);

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
            MapType::write(out); //write PatternStore
        }

        void write(const std::string filename) {
            std::ofstream * out = new std::ofstream(filename.c_str());
            this->write(out);
            out->close();
            delete out;
        }

        typedef typename MapType::iterator iterator;
        typedef typename MapType::const_iterator const_iterator;        

        virtual int maxlength() const { return this->maxn; };
        virtual int minlength() const { return this->minn; };
        virtual int occurrencecount(const Pattern & pattern)  { 
            ValueType * data = getdata(pattern);
            if (data != NULL) {
                return this->valuehandler.count(*data); 
            } else {
                return 0;
            }
        }
        
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
        
        virtual ValueType * getdata(const PatternPointer & patternpointer, bool makeifnew=false) { 
            const Pattern pattern = Pattern(patternpointer);
            typename MapType::iterator iter = this->find(pattern);
            if (iter != this->end()) {
                return &(iter->second); 
            } else if (makeifnew) {
                return &((*this)[pattern]);
            } else {
                return NULL;
            }
        }

        virtual int types() { 
            if ((totaltypes == 0) && (!this->data.empty())) totaltypes = this->totalwordtypesingroup(0, 0);
            return totaltypes; 
        }

        virtual int tokens() const { return totaltokens; }

        unsigned char type() const { return model_type; }
        unsigned char version() const { return model_version; }

        void output(std::ostream *);
        
        
        int coveragecount(const Pattern &  key) {
           return this->occurrencecount(key) * key.size();
        }
        double coverage(const Pattern & key) {
            return this->coveragecount(key) / (double) this->tokens();
        }
       

        std::vector<Pattern> getreverseindex(const IndexReference ref, int occurrencecount = 0, int category = 0, int size = 0) {
            //Auxiliary function
            std::vector<Pattern> result;
            if (!this->reverseindex) return result;
            const int sl = this->reverseindex->sentencelength(ref.sentence);
            //std::cerr << "DEBUG: getreverseindex sentencelength(" << ref.sentence << ")=" << sl << std::endl;
            const int minn = this->minlength();
            const int maxn = this->maxlength();
            for (int i = minn; i <= sl && i <= maxn; i++) {
                if ((size == 0) || (i == size)) {
                    try {
                        //std::cerr << "DEBUG: getreverseindex getpattern " << ref.tostring() << " + " << i << std::endl;
                        const Pattern ngram = this->reverseindex->getpattern(ref,i);
                        /*std::cerr << "n: " << ngram.n() << std::endl;
                        std::cerr << "bytesize: " << ngram.bytesize() << std::endl;;
                        std::cerr << "hash: " << ngram.hash() << std::endl;;*/
                        if ( (((occurrencecount == 0) && this->has(ngram)) || (this->occurrencecount(ngram) >= occurrencecount))
                            && ((category == 0) || (ngram.category() >= category)) ) {
                            result.push_back(ngram);

                            if (((category == 0) || (category == SKIPGRAM)) && (this->hasskipgrams))  {
                                
                                //(we can't use gettemplates() because
                                //gettemplates() depends on us, we have to
                                //solve it low-level, punching holes:

                                std::vector<Pattern> skipgrams = this->findskipgrams(ngram, occurrencecount);
                                for (auto skipgram : skipgrams) {
                                    result.push_back(skipgram);
                                }

                            }
                        }
                    } catch (KeyError &e) {
                        break;
                    }
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

        std::vector<std::pair<IndexReference,Pattern>> getreverseindex_bysentence(int sentence) {
            //Auxiliary function
            std::vector<std::pair<IndexReference,Pattern>> result;
            for (int i = 0; i < this->reverseindex->sentencelength(sentence); i++) {
                const IndexReference ref = IndexReference(sentence, i);
                std::vector<Pattern> tmpresult =  this->getreverseindex(ref);
                for (std::vector<Pattern>::iterator iter = tmpresult.begin(); iter != tmpresult.end(); iter++) {
                    const Pattern pattern = *iter;
                    result.push_back(std::pair<IndexReference,Pattern>(ref,pattern));
                }
            }
            return result;
        }

        std::vector<std::pair<IndexReference,Pattern>> getreverseindex_right(const IndexReference ref) {
            //Auxiliary function
            std::vector<std::pair<IndexReference,Pattern>> result;
            for (int i = ref.token+1; i < this->reverseindex->sentencelength(ref.sentence); i++) {
                const IndexReference ref2 = IndexReference(ref.sentence, i);
                std::vector<Pattern> tmpresult =  this->getreverseindex(ref);
                for (std::vector<Pattern>::iterator iter = tmpresult.begin(); iter != tmpresult.end(); iter++) {
                    const Pattern pattern = *iter;
                    result.push_back(std::pair<IndexReference,Pattern>(ref2,pattern));
                }
            }
            return result;
        }

        std::vector<std::pair<IndexReference,Pattern>> getreverseindex_left(const IndexReference ref) {
            //Auxiliary function
            std::vector<std::pair<IndexReference,Pattern>> result;
            for (int i = 0; i < ref.token; i++) {
                const IndexReference ref2 = IndexReference(ref.sentence, i);
                std::vector<Pattern> tmpresult =  this->getreverseindex(ref);
                for (std::vector<Pattern>::iterator iter = tmpresult.begin(); iter != tmpresult.end(); iter++) {
                    const Pattern pattern = *iter;
                    result.push_back(std::pair<IndexReference,Pattern>(ref2,pattern));
                }
            }
            return result;
        }

        void computestats() {
            cache_categories.clear();
            cache_n.clear();
            cache_grouptotal.clear();
            cache_grouptotalpatterns.clear();
            cache_categories.insert(0);
            cache_n.insert(0);
            PatternModel::iterator iter = this->begin(); 
            while (iter != this->end()) {
                const Pattern pattern = iter->first;
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
        

        virtual void computecoveragestats() {
            if ((cache_grouptotal.empty()) && (!this->data.empty())) this->computestats();
            //bool hasunigrams = false;
            cache_grouptotalwordtypes.clear();
            cache_grouptotaltokens.clear();
            //opting for memory over speed (more iterations, less memory)
            // Indexed model overloads this for better cache_grouptotaltokens computation! 
            for (std::set<int>::iterator iterc = cache_categories.begin(); iterc != cache_categories.end(); iterc++) {
                for (std::set<int>::iterator itern = cache_n.begin(); itern != cache_n.end(); itern++) {
                    std::set<Pattern> types;
                    PatternModel::iterator iter = this->begin(); 
                    while (iter != this->end()) {
                        const Pattern pattern = iter->first;                        
                        const int pn = (int) pattern.n();
                        if ( (pn == 1) && (*itern <= 1) && ((*iterc == 0) || (pattern.category() == *iterc))) {
                            types.insert(pattern);
                        } else {
                            if (((*itern == 0) || (pn == *itern))  && ((*iterc == 0) || (pattern.category() == *iterc))) {
                                std::vector<Pattern> unigrams;
                                pattern.ngrams(unigrams, 1);
                                for (std::vector<Pattern>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                                    const Pattern p = *iter2;
                                    types.insert(p);
                                }
                            }
                        }
                        cache_grouptotaltokens[*iterc][*itern] += this->valuehandler.count(iter->second);
                        iter++;
                    }
                    cache_grouptotalwordtypes[*iterc][*itern] += types.size();
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
        

        int totaloccurrencesingroup(int category, int n) {
            //category and n can be set to 0 to loop over all
            if ((cache_grouptotal.empty()) && (!this->data.empty())) this->computestats();
            return cache_grouptotal[category][n];
        }

        int totalpatternsingroup(int category, int n) {
            //category and n can be set to 0 to loop over all
            if ((cache_grouptotalpatterns.empty()) && (!this->data.empty())) this->computestats();
            return cache_grouptotalpatterns[category][n];
        }

        int totalwordtypesingroup(int category, int n) {
            //total covered word/unigram types
            //category and n can be set to 0 to loop over all
            if ((cache_grouptotalwordtypes.empty()) && (!this->data.empty())) this->computecoveragestats();
            return cache_grouptotalwordtypes[category][n];
        }
        int totaltokensingroup(int category, int n) {
            //total COVERED tokens
            //category and n can be set to 0 to loop over all
            if ((cache_grouptotaltokens.empty()) && (!this->data.empty())) this->computecoveragestats();
            return cache_grouptotaltokens[category][n];
        }
        
        double frequency(const Pattern & pattern) {
            //frequency within the same n and category class
            return this->occurrencecount(pattern) / (double) totaloccurrencesingroup(pattern.category(),pattern.n());
        }



        virtual void add(const Pattern & pattern, ValueType * value, const IndexReference & ref) {
            if (value == NULL) {
                std::cerr << "Add() value is NULL!" << std::endl;
                throw InternalError();
            }
            this->valuehandler.add(value, ref);
        }
        virtual void add(const PatternPointer & pattern, ValueType * value, const IndexReference & ref) {
            if (value == NULL) {
                std::cerr << "Add() value is NULL!" << std::endl;
                throw InternalError();
            }
            this->valuehandler.add(value, ref);
        }

        int prune(int threshold,int _n=0) {
            //prune all patterns under the specified threshold (set -1 for
            //all) and of the specified length (set _n==0 for all)
            int pruned = 0;
            PatternModel::iterator iter = this->begin(); 
            while (iter != this->end()) {
                const Pattern pattern = iter->first;
                if (( (_n == 0) || (pattern.n() == (unsigned int) _n) )&& ((threshold == -1) || (occurrencecount(pattern) < threshold))) {
                    //std::cerr << occurrencecount(pattern) << std::endl;
                    //std::cerr << "preprune:" << this->size() << std::endl;
                    iter = this->erase(iter); 
                    //std::cerr << "postprune:" << this->size() << std::endl;
                    pruned++;
                } else {
                    iter++;
                }
            };       

            return pruned;
        }

        virtual int pruneskipgrams(int threshold, int minskiptypes=2, int _n = 0) {
            //NOTE: minskiptypes is completely ignored! that only works for indexed models
            int pruned = 0;
            if (minskiptypes <=1) return pruned; //nothing to do

            typename PatternModel<ValueType,BaseValueHandler<ValueType>,MapType>::iterator iter = this->begin(); 
            while(iter != this->end()) { 
                const Pattern pattern = iter->first;
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


        template<class ValueType2,class ValueHandler2,class MapType2>
        int prunebymodel(PatternModel<ValueType2,ValueHandler2,MapType2> & secondmodel) {
            //is not used by default when working with constraint models
            //anymore, is directly processing during loading instead
            //
            //this is still useful if you have two models in memory though
            int pruned = 0;
            typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); 
            while(iter != this->end()) { 
                const Pattern pattern = iter->first;
                if (!secondmodel.has(pattern)) {
                    iter = this->erase(iter);
                    pruned++;
                    continue;
                }
                iter++;
            }
            return pruned;
        }


        std::vector<std::pair<Pattern, int> > getpatterns(const Pattern & pattern) { //get all patterns in pattern that occur in the patternmodel
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

        virtual void print(std::ostream * out, ClassDecoder & decoder) {
            *out << "PATTERN\tCOUNT\tTOKENS\tCOVERAGE\tCATEGORY\tSIZE\tFREQUENCY" << std::endl;
            for (PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern pattern = iter->first;
                this->print(out, decoder, pattern, true);
            }
        }

        virtual void printreverseindex(std::ostream * out, ClassDecoder & decoder) {
            if (!this->reverseindex) return;
            for (IndexedCorpus::iterator iter = reverseindex->begin(); iter != reverseindex->end(); iter++) {
                const IndexReference ref = iter->ref;
                std::vector<Pattern> rindex = this->getreverseindex(ref);
                *out << ref.tostring();
                for (std::vector<Pattern>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                    const Pattern p = *iter2;
                    *out << "\t" << p.tostring(decoder); 
                }
                *out << "\n";
            } 
            *out << std::endl;
        }


        void printmodel(std::ostream * out, ClassDecoder & decoder) { //an alias because cython can't deal with a method named print
            this->print(out, decoder); 
        }

        virtual void print(std::ostream* out, ClassDecoder &decoder, const Pattern & pattern, bool endline = true) {
            const std::string pattern_s = pattern.tostring(decoder);
            const int count = this->occurrencecount(pattern); 
            const int covcount = this->coveragecount(pattern);
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
        }

        
        void printpattern(std::ostream* out, ClassDecoder &decoder, const Pattern & pattern, bool endline = true) {  //another alias for cython who can't deal with methods named print
            return this->print(out,decoder,pattern,endline);
        }


        void histogram(std::map<int,int> & hist, unsigned int threshold = 0, unsigned int cap = 0, int category = 0, int size = 0) {
            for (PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern pattern = iter->first;
                if (((category != 0) && (pattern.category() != category)) || ((size != 0) && (size != pattern.size()))) continue;
                int c = this->occurrencecount(pattern);
                if (c >= threshold) hist[c]++;
            }  
            if (cap > 0) {
                unsigned int sum = 0;
                std::map<int,int>::reverse_iterator iter = hist.rbegin();
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
            std::map<int,int> hist;
            histogram(hist, 0, amount, category, size);
            std::map<int,int>::reverse_iterator iter = hist.rbegin();
            if (iter != hist.rend()) {
                return iter->first;
            } else {
                return 0;
            }
        }


        void histogram(std::ostream * OUT, unsigned int threshold = 0, unsigned int cap = 0) {
            std::map<int,int> hist;
            histogram(hist);
            *OUT << "HISTOGRAM" << std::endl;
            *OUT << "------------------------------" << std::endl;
            *OUT << "OCCURRENCES\tPATTERNS" << std::endl;
            for (std::map<int,int>::iterator iter = hist.begin(); iter != hist.end(); iter++) {
                *OUT << iter->first << "\t" << iter->second << std::endl;
            }
        }

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
                const Pattern pattern = iter->first;   
                totalkeybs += sizeof(Pattern) + pattern.bytesize();
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
                    ri_totalkeybs += sizeof(iter->ref.sentence) + sizeof(iter->ref.token);
                    ri_totalvaluebs += sizeof(IndexPattern); // sizeof(Pattern) + iter->pattern().bytesize();
                }
                *OUT << "Total key bytesize in reverse index (references): " <<  ri_totalkeybs << " bytes (" << (ri_totalkeybs/1024/1024) << " MB)" << std::endl;
                *OUT << "Total value bytesize in reverse index (patterns): " <<  ri_totalvaluebs << " bytes (" << (ri_totalvaluebs/1024/1024) << " MB)" << std::endl;
            }


            const unsigned int t = (totalkeybs + totalvaluebs + ri_totalkeybs + ri_totalvaluebs);
            *OUT << "Total bytesize (without overhead): " << t << " bytes (" << (t/1024/1024) << " MB)" << std::endl;
        }

        void report(std::ostream * OUT) {
            if ((cache_grouptotaltokens.empty()) && (!this->data.empty())) {
                std::cerr << "Computing statistics..." << std::endl;
                this->computecoveragestats();
            }
            *OUT << std::setiosflags(std::ios::fixed) << std::setprecision(4) << std::endl;       
            *OUT << "REPORT" << std::endl;
            if (this->getmodeltype() == UNINDEXEDPATTERNMODEL) {
                *OUT << "   Warning: Model is unindexed, token coverage counts are mere maximal projections" << std::endl;
                *OUT << "            assuming no overlap at all!!! Use an indexed model for accurate coverage counts" << std::endl;
            }
            *OUT << "----------------------------------" << std::endl;
            *OUT << "                          " << std::setw(10) << "PATTERNS" << std::setw(10) << "TOKENS" << std::setw(10) << "COVERAGE" << std::setw(10) << "TYPES" << std::setw(10) << std::endl;
            *OUT << "Total:                    " << std::setw(10) << "-" << std::setw(10) << this->tokens() << std::setw(10) << "-" << std::setw(10) << this->types() <<  std::endl;

            int coveredtypes = totalwordtypesingroup(0,0);  //will also word when no unigrams in model
            int coveredtokens = totaltokensingroup(0,0);

            if (coveredtokens > this->tokens()) coveredtokens = this->tokens();
            int uncoveredtokens = this->tokens() - coveredtokens;
            if (uncoveredtokens < 0) uncoveredtokens = 0;
            *OUT << "Uncovered:                " << std::setw(10) << "-" << std::setw(10) << uncoveredtokens << std::setw(10) << uncoveredtokens / (double) this->tokens() << std::setw(10) << this->types() - coveredtypes <<  std::endl;
            *OUT << "Covered:                  " << std::setw(10) << this->size() << std::setw(10) << coveredtokens << std::setw(10) << coveredtokens / (double) this->tokens() <<  std::setw(10) << coveredtypes <<  std::endl << std::endl;
            
            
            *OUT << std::setw(10) << "CATEGORY" << std::setw(10) << "N (SIZE) "<< std::setw(10) << "PATTERNS";
            if (this->getmodeltype() != UNINDEXEDPATTERNMODEL) *OUT << std::setw(10) << "TOKENS" << std::setw(10) << "COVERAGE";
            *OUT << std::setw(10) << "TYPES" << std::setw(12) << "OCCURRENCES" << std::endl;
            
            for (std::set<int>::iterator iterc = cache_categories.begin(); iterc != cache_categories.end(); iterc++) {
                const int c = *iterc;
                if (cache_grouptotalpatterns.count(c))
                for (std::set<int>::iterator itern = cache_n.begin(); itern != cache_n.end(); itern++) {
                    const int n = *itern;
                    if (cache_grouptotalpatterns[c].count(n)) {
                        //category
                        if (c == 0) {
                            *OUT << std::setw(10) << "all";
                        } else if (c == NGRAM) {
                            *OUT << std::setw(10) << "n-gram";
                        } else if (c == SKIPGRAM) {
                            *OUT << std::setw(10) << "skipgram";
                        } else if (c == FLEXGRAM) {
                            *OUT << std::setw(10) << "flexgram";
                        }
                        //size
                        if (n == 0) {
                            *OUT << std::setw(10) << "all";
                        } else {
                            *OUT << std::setw(10) << n;
                        }
                        //patterns
                        *OUT << std::setw(10) << cache_grouptotalpatterns[c][n];
                        if (this->getmodeltype() != UNINDEXEDPATTERNMODEL) {
                            //tokens
                            *OUT << std::setw(10) << cache_grouptotaltokens[c][n];
                            //coverage
                            *OUT << std::setw(10) << cache_grouptotaltokens[c][n] / (double) this->tokens();
                        }
                        //types
                        *OUT << std::setw(10) << cache_grouptotalwordtypes[c][n];
                        //occurrences
                        *OUT << std::setw(12) << cache_grouptotal[c][n] << std::endl;;
                    }
                }
            }
        }
        
        

        virtual void outputrelations(const Pattern & pattern, ClassDecoder & classdecoder, std::ostream * OUT) {} //does nothing for unindexed models
        virtual t_relationmap getsubchildren(const Pattern & pattern,int = 0, int = 0, int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getsubparents(const Pattern & pattern,int = 0, int = 0, int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap gettemplates(const Pattern & pattern,int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getinstances(const Pattern & pattern,int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getskipcontent(const Pattern & pattern) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getleftneighbours(const Pattern & pattern,int = 0, int = 0,int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getrightneighbours(const Pattern & pattern,int = 0, int = 0,int = 0) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap_double getnpmi(const Pattern & pattern, double threshold) { return t_relationmap_double(); } //does nothing for unindexed models
        virtual int computeflexgrams_fromskipgrams() { return 0; }//does nothing for unindexed models
        virtual int computeflexgrams_fromcooc() {return 0; }//does nothing for unindexed models
        virtual void outputcooc_npmi(std::ostream * OUT, ClassDecoder& classdecoder, double threshold) {}
        virtual void outputcooc(std::ostream * OUT, ClassDecoder& classdecoder, double threshold) {}
};



template<class MapType = PatternMap<IndexedData,IndexedDataHandler>> 
class IndexedPatternModel: public PatternModel<IndexedData,IndexedDataHandler,MapType> {
    protected:
        virtual void postread(const PatternModelOptions options) {
            for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern p = iter->first;
                const int n = p.n();
                if (n > this->maxn) this->maxn = n;
                if (n < this->minn) this->minn = n;
                if ((!this->hasskipgrams) && (p.isskipgram())) this->hasskipgrams = true;
            }
            buildreverseindex(options); //will only act if options.DOREVERSEINDEX is set
        }
        virtual void posttrain(const PatternModelOptions options) {
            if (!options.QUIET) std::cerr << "Sorting all indices..." << std::endl;
            for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
                iter->second.sort();
            }
            buildreverseindex(options);//will only act if options.DOREVERSEINDEX is set

        }
   public:
        void buildreverseindex(const PatternModelOptions options) {
            //build reverse index, requires options.DOREVERSEINDEX to be set or
            //won't act. Also won't build a reverse index if one is loaded
            //already
            if ((this->reverseindex) && (this->reverseindex->empty())) {
                if (!options.QUIET) std::cerr << "Building reverse index... (Consider preloading a reverse index to skip this (-r) )" << std::endl;
                for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
                    const Pattern p = iter->first;
                    const int n = p.n();
                    if (n > this->maxn) this->maxn = n;
                    if (n < this->minn) this->minn = n;
                    if ((n == 1) && (options.DOREVERSEINDEX)) {
                        IndexedData * data = this->getdata(p);
                        //construct the reverse index
                        for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {                    
                            const IndexReference ref = *iter2;
                            this->reverseindex->push_back(ref,p);
                        }
                    }
                }
                if (!options.QUIET) std::cerr << "Sorting reverse index... (Consider preloading a reverse index to skip this (-r) )" << std::endl;
                this->reverseindex->sort();
            }
        }

       
    IndexedPatternModel<MapType>(IndexedCorpus * corpus = NULL): PatternModel<IndexedData,IndexedDataHandler,MapType>() {
        this->model_type = this->getmodeltype();
        this->model_version = this->getmodelversion();
        if (corpus) {
            this->reverseindex = corpus;
            this->externalreverseindex = true;
        } else {
            this->reverseindex = new IndexedCorpus();
            this->externalreverseindex = false;
        }
    }

    IndexedPatternModel<MapType>(std::istream *f, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL):  PatternModel<IndexedData,IndexedDataHandler,MapType>(){ //load from file
        this->model_type = this->getmodeltype();
        this->model_version = this->getmodelversion();
        if (corpus) {
            this->reverseindex = corpus;
            this->externalreverseindex = true;
        } else {
            this->externalreverseindex = false;
            this->reverseindex = new IndexedCorpus();
        }
        this->load(f,options, constrainmodel);
    }

    IndexedPatternModel<MapType>(const std::string filename, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL, IndexedCorpus * corpus = NULL): PatternModel<IndexedData,IndexedDataHandler,MapType>() { //load from file
        this->model_type = this->getmodeltype();
        this->model_version = this->getmodelversion();
        if (corpus) {
            this->reverseindex = corpus;
            this->externalreverseindex = true;
        } else {
            this->externalreverseindex = false;
            this->reverseindex = new IndexedCorpus();
        }
        std::ifstream * in = new std::ifstream(filename.c_str());
        this->load( (std::istream *) in, options, constrainmodel);
        in->close();
        delete in;
    }

    virtual ~IndexedPatternModel<MapType>() {
        if ((this->reverseindex != NULL) && (!this->externalreverseindex)) {
            delete this->reverseindex;
            this->reverseindex = NULL;
        }
    }


    int getmodeltype() const { return INDEXEDPATTERNMODEL; }
    int getmodelversion() const { return 1;} 


    void add(const Pattern & pattern, IndexedData * value, const IndexReference & ref) {
        if (value == NULL) {
            value = getdata(pattern,true);
        }
        this->valuehandler.add(value, ref);
    }
    void add(const PatternPointer & patternpointer, IndexedData * value, const IndexReference & ref) {
        if (value == NULL) {
            value = getdata(Pattern(patternpointer),true);
        }
        this->valuehandler.add(value, ref);
    }
    
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

    IndexedData * getdata(const PatternPointer & patternpointer, bool makeifnew=false) { 
        const Pattern pattern = Pattern(patternpointer);
        typename MapType::iterator iter = this->find(pattern);
        if (iter != this->end()) {
            return &(iter->second); 
        } else if (makeifnew) {
            return &((*this)[pattern]);
        } else {
            return NULL;
        }
    }
    



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
                ri_totalkeybs += sizeof(iter->ref.sentence) + sizeof(iter->ref.token);
                ri_totalvaluebs += sizeof(IndexPattern); // sizeof(Pattern) + iter->pattern().bytesize();
            }
            *OUT << "Total key bytesize in reverse index (references): " << ri_totalkeybs << " bytes (" << (ri_totalkeybs/1024/1024) << " MB)" << std::endl;
            *OUT << "Total value bytesize in reverse index (patterns): " << ri_totalvaluebs << " bytes (" << (ri_totalvaluebs/1024/1024) << " MB)" << std::endl;
        }

        const unsigned int t = (totalkeybs + totalvaluebs + ri_totalkeybs + ri_totalvaluebs);
        *OUT << "Total bytesize (without overhead): " << t << " bytes (" << (t/1024/1024) << " MB)" << std::endl;
    }


    void print(std::ostream * out, ClassDecoder & decoder) {
        *out << "PATTERN\tCOUNT\tTOKENS\tCOVERAGE\tCATEGORY\tSIZE\tFREQUENCY\tREFERENCES" << std::endl;
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const Pattern pattern = iter->first;
            this->print(out, decoder, pattern, true);
        }
    }

    void print(std::ostream* out, ClassDecoder &decoder, const Pattern & pattern, bool endline = true) {
            const std::string pattern_s = pattern.tostring(decoder);
            const int count = this->occurrencecount(pattern); 
            const int covcount = this->coveragecount(pattern);
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
            *out << pattern_s << "\t" << count << "\t" << "\t" << covcount << "\t" << coverage << "\t" << cat_s << "\t" << pattern.size() << "\t" << freq << "\t"; 
            IndexedData * data = this->getdata(pattern);
            int i = 0;
            for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {                    
                i++;
                *out << iter2->tostring();
                if (i < count) *out << " ";
            }
            if (endline) *out << std::endl;
    }


    virtual void trainskipgrams(PatternModelOptions options,  PatternModelInterface * constrainbymodel = NULL) {
        if (options.MINTOKENS == -1) options.MINTOKENS = 2;
        this->cache_grouptotal.clear(); //forces recomputation of statistics
        for (int n = 3; n <= options.MAXLENGTH; n++) {
            if (this->gapconf[n].empty()) compute_multi_skips(this->gapconf[n], std::vector<std::pair<int,int> >(), n);
            if (!options.QUIET) std::cerr << "Counting " << n << "-skipgrams" << std::endl; 
            int foundskipgrams = 0;
            for (typename MapType::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern pattern = iter->first;
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
            int pruned = this->prune(options.MINTOKENS,n);
            if (!options.QUIET) std::cerr << "pruned " << pruned;
            int prunedextra = this->pruneskipgrams(options.MINTOKENS_SKIPGRAMS, options.MINSKIPTYPES, n);
            if (prunedextra && !options.QUIET) std::cerr << " plus " << prunedextra << " extra skipgrams..";
            if (!options.QUIET) std::cerr << "...total kept: " <<  foundskipgrams - pruned - prunedextra << std::endl;
        }
    }

    Pattern getpatternfromtoken(IndexReference ref) {
        return this->reverseindex->getpattern(ref,1);
    }
    


    t_relationmap getskipcontent(const Pattern & pattern) {
        //NOTE: skipcontent patterns of skipgrams with multiple gaps are
        //themselves skipgrams!
        unsigned char fixedgapbuffer[101];
        for (int i = 0; i< 100; i++) fixedgapbuffer[i] = 128;

        t_relationmap skipcontent;
        if (pattern.category() == SKIPGRAM) {
            //find the gaps
            std::vector<std::pair<int,int>> gapdata;
            pattern.gaps(gapdata);

            
            std::vector<std::pair<int,int>>::iterator gapiter = gapdata.begin();
            //int offset = gapiter->first;
            int begin = gapiter->first + gapiter->second; //begin of part in original, gap in content)

            std::vector<std::pair<int,int>> skipcontent_gaps;
            gapiter++;
            while (gapiter != gapdata.end()) {
                int gaplength = gapiter->first - begin;
                skipcontent_gaps.push_back(std::pair<int,int>(begin,gaplength));
                begin = gapiter->first + gapiter->second;
                gapiter++;
            }


            IndexedData * data = getdata(pattern);
            for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {                    
                const IndexReference ref = *iter2;
                Pattern skipcontent_atref;

                bool notoken = false;
                gapiter = gapdata.begin();
                std::vector<std::pair<int,int>>::iterator skipcontent_gapiter = skipcontent_gaps.begin();
                while (gapiter != gapdata.end()) {
                    Pattern part;
                    for (int i = gapiter->first; i < gapiter->first + gapiter->second; i++) {
                        try {
                            Pattern  p = this->getpatternfromtoken(ref + i);
                            skipcontent_atref = skipcontent_atref +  p;
                        } catch (const KeyError &e) { //when token does not exist
                            notoken=true; break;
                        }
                    }
                    if (notoken) break;
                    
                    if (skipcontent_gapiter != skipcontent_gaps.end()) {
                        skipcontent_atref = skipcontent_atref + Pattern(fixedgapbuffer, skipcontent_gapiter->second);
                    }
                    gapiter++;
                }
                if (notoken) continue;
                //std::cerr << "counting skipcontent " << skipcontent_atref.hash() << " at " << ref.sentence << ":" << ref.token << std::endl;
                skipcontent[skipcontent_atref] += 1;
            }

        } else if (pattern.category() == FLEXGRAM) {
            //TODO: implement
        }
        //std::cerr << "Total found " << skipcontent.size() << std::endl;
        return skipcontent;
    }
    
    void prunerelations(t_relationmap & relations, int occurrencethreshold) {
        //prune relations below the specified occurrence threshold
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

    t_relationmap gettemplates(const Pattern & pattern, int occurrencethreshold = 0) {
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
            std::vector<Pattern> rindex = this->getreverseindex(ref);
            for (std::vector<Pattern>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const Pattern candidate = *iter2;

                if (((int) candidate.n() == _n)  && (candidate != pattern) && (candidate.category() == SKIPGRAM)  && ((occurrencethreshold == 0) || (this->occurrencecount(pattern) >= occurrencethreshold)) ) {
                    templates[candidate] += 1;
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(templates, occurrencethreshold);
        return templates;
    }

    t_relationmap getinstances(const Pattern & pattern, int occurrencethreshold = 0) {
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
            std::vector<Pattern> rindex = this->getreverseindex(ref);
            for (std::vector<Pattern>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const Pattern candidate = *iter2;

                if (((int) candidate.n() == _n)  && (candidate != pattern) && (candidate.category() == NGRAM) && ((occurrencethreshold == 0) || (this->occurrencecount(pattern) >= occurrencethreshold))  ) {
                    instances[candidate] += 1;
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(instances, occurrencethreshold);
        return instances;
    }

    t_relationmap getsubchildren(const Pattern & pattern, int occurrencethreshold = 0, int category = 0, int size = 0) {
        //returns patterns that are subsumed by the specified pattern (i.e.
        //smaller patterns)
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
        const bool isfixedskipgram = (pattern.category() == SKIPGRAM);
        //search in forward index
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;
            for (int i = ref.token; i < ref.token + _n; i++) {
                const IndexReference begin = IndexReference(ref.sentence,i);
                int maxsubn = _n - (i - ref.token);

                //std::cerr << "Begin " << begin.sentence << ":" << begin.token << ",<< std::endl;

                //search in reverse index
                std::vector<Pattern> rindex = this->getreverseindex(begin);
                for (std::vector<Pattern>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                    const Pattern candidate = *iter2;
                    //std::cerr << "Considering candidate @" << ref2.sentence << ":" << ref2.token << ", n=" << candidate.n() << ", bs=" << candidate.bytesize() <<  std::endl;
                    //candidate.out();
                    if (((int) candidate.n() <= maxsubn) && (candidate != pattern)
                        && ((occurrencethreshold == 0) || (this->occurrencecount(candidate) >= occurrencethreshold))
                        && ((category == 0) || (candidate.category() >= category))
                        && ((size == 0) || (candidate.n() >= size))
                        ) {
                        if ((isfixedskipgram) || (candidate.category() == SKIPGRAM)) { //MAYBE TODO: I may check too much now... could be more efficient? 
                            //candidate may not have skips in places where the larger
                            //pattern does not
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

    t_relationmap getsubparents(const Pattern & pattern, int occurrencethreshold = 0, int category = 0, int size = 0) {
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
            std::vector<std::pair<IndexReference,Pattern>> rindex = this->getreverseindex_bysentence(ref.sentence);
            for (std::vector<std::pair<IndexReference,Pattern>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                if ((iter2->first.sentence != ref.sentence) || (iter2->first.token > ref.token)) break;
                const Pattern candidate = iter2->second;

                int minsubsize = _n + (ref.token - iter2->first.token);

                if (((int) candidate.n() >= minsubsize)  && (candidate != pattern)
                        && ((occurrencethreshold == 0) || (this->occurrencecount(candidate) >= occurrencethreshold))
                        && ((category == 0) || (candidate.category() >= category))
                        && ((size == 0) || (candidate.n() >= size))
                    ) {
                    if ((candidate.category() == SKIPGRAM) || (pattern.category() == SKIPGRAM))  {//MAYBE TODO: I may check too much now... could be more efficient? 
                        //instance may not have skips in places where the larger
                        //candidate pattern does not
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


    t_relationmap getleftneighbours(const Pattern & pattern, int occurrencethreshold = 0, int category = 0, int size = 0) {
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


            std::vector<std::pair<IndexReference,Pattern>> rindex = this->getreverseindex_bysentence(ref.sentence);
            for (std::vector<std::pair<IndexReference,Pattern>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const Pattern neighbour = iter2->second;
                if ((ref2.token + neighbour.n() == ref.token)
                        && ((occurrencethreshold == 0) || (this->occurrencecount(neighbour) >= occurrencethreshold))
                        && ((category == 0) || (neighbour.category() >= category))
                        && ((size == 0) || (neighbour.n() >= size))
                    ){
                    neighbours[neighbour]++;
                } else if ((ref2.token > ref.token) || (ref2.sentence > ref.sentence)) break;
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(neighbours, occurrencethreshold);
        return neighbours;
    }

    t_relationmap getrightneighbours(const Pattern & pattern, int occurrencethreshold = 0, int category = 0, int size = 0) {
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
            IndexReference nextref = ref + 1;

            //search in reverse index
            std::vector<Pattern> rindex = this->getreverseindex(ref);
            for (std::vector<Pattern>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const Pattern neighbour = *iter2;
                if ( ((occurrencethreshold == 0) || (this->occurrencecount(neighbour) >= occurrencethreshold))
                        && ((category == 0) || (neighbour.category() >= category))
                        && ((size == 0) || (neighbour.n() >= size)) ) {
                    neighbours[neighbour]++;
                }
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(neighbours, occurrencethreshold);
        return neighbours;
    }

    int pruneskipgrams(int threshold, int minskiptypes, int _n = 0) {
        int pruned = 0;
        if (minskiptypes <=1) return pruned; //nothing to do

        typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); 
        while(iter != this->end()) { 
            const Pattern pattern = iter->first;
            if (( (_n == 0) || ((int) pattern.n() == _n) ) && (pattern.category() == SKIPGRAM)) {
                t_relationmap skipcontent = getskipcontent(pattern);
                if ((int) skipcontent.size() < minskiptypes) { //will take care of token threshold too, patterns not meeting the token threshold are not included
                    iter = this->erase(iter);
                    pruned++;
                    continue;
                }
            }
            iter++;
        }
        return pruned;
    } 

    virtual void computecoveragestats() {
        //opting for memory over speed (more iterations, less memory)
        //overloaded version for indexedmodel
        if ((this->cache_grouptotal.empty()) && (!this->data.empty())) this->computestats();
        for (std::set<int>::iterator iterc = this->cache_categories.begin(); iterc != this->cache_categories.end(); iterc++) {
            for (std::set<int>::iterator itern = this->cache_n.begin(); itern != this->cache_n.end(); itern++) {
                std::set<Pattern> types;
                std::set<IndexReference> tokens;
                typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); 
                while (iter != this->end()) {
                    const Pattern pattern = iter->first;                        
                    const int n = pattern.n();
                    if ( (n == 1) && (*itern <= 1) && ((*iterc == 0) || (pattern.category() == *iterc))) {
                        types.insert(pattern);
                    } else {
                        if (((*itern == 0) || (n == *itern))  && ((*iterc == 0) || (pattern.category() == *iterc))) {
                            std::vector<Pattern> unigrams;
                            pattern.ngrams(unigrams, 1);
                            for (std::vector<Pattern>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                                const Pattern p = *iter2;
                                types.insert(p);
                            }
                            IndexedData * data = this->getdata(pattern);
                            for (IndexedData::iterator dataiter = data->begin(); dataiter != data->end(); dataiter++) {
                                //take into account all tokens 
                                for (int i = 0; i < pattern.n(); i++) {
                                    tokens.insert(*dataiter + i);
                                }
                            }
                        }
                    }
                    iter++;
                }
                this->cache_grouptotalwordtypes[*iterc][*itern] += types.size();
                this->cache_grouptotaltokens[*iterc][*itern] += tokens.size();
            }
        }
    }

    t_relationmap getrightcooc(const Pattern & pattern, int occurrencethreshold = 0, int category = 0, int size = 0,IndexedData * matches = NULL) { 
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


            std::vector<std::pair<IndexReference,Pattern>> rindex = this->getreverseindex_right(ref);
            for (std::vector<std::pair<IndexReference,Pattern>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const Pattern neighbour = iter2->second;
                if ( (ref2.token > ref.token + _n) 
                        && ((occurrencethreshold == 0) || (this->occurrencecount(neighbour) >= occurrencethreshold))
                        && ((category == 0) || (neighbour.category() >= category))
                        && ((size == 0) || (neighbour.n() >= size))
                     ) {
                    cooc[neighbour]++;
                    if (matches != NULL) matches->insert(ref2);
                } 
            }
        }
        if (occurrencethreshold > 0) this->prunerelations(cooc, occurrencethreshold);
        return cooc;
    }


    t_relationmap getleftcooc(const Pattern & pattern, int occurrencethreshold = 0, int category = 0, int size = 0) { 
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

            std::vector<std::pair<IndexReference,Pattern>> rindex = this->getreverseindex_left(ref);
            for (std::vector<std::pair<IndexReference,Pattern>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const Pattern neighbour = iter2->second;
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


    t_relationmap getcooc(const Pattern & pattern, int occurrencethreshold = 0, int category = 0, int size = 0,bool ordersignificant = false) { 
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

            IndexReference bos =  IndexReference(ref.sentence, 0);

            std::vector<std::pair<IndexReference,Pattern>> rindex = this->getreverseindex_bysentence(ref.sentence);
            for (std::vector<std::pair<IndexReference,Pattern>>::iterator iter2 = rindex.begin(); iter2 != rindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const Pattern neighbour = iter2->second;
                if ((ordersignificant) && (neighbour < pattern)) continue;
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

    double npmi(const Pattern & key1, const Pattern & key2, int jointcount) {
        //normalised pointwise mutual information
        return  log( (double) jointcount / (this->occurrencecount(key1) * this->occurrencecount(key2)) )  / -log((double)jointcount/(double)this->totaloccurrencesingroup(0,0) );    
    }

    void outputrelations(const Pattern & pattern, t_relationmap & relations, ClassDecoder & classdecoder, std::ostream *OUT, const std::string label = "RELATED-TO") {
        int total = 0;
        for (t_relationmap::iterator iter = relations.begin(); iter != relations.end(); iter++) {
            total += iter->second;
        }
        if (total == 0) return;
        double total_f = total;
        const std::string pattern_s = pattern.tostring(classdecoder);
        for (t_relationmap::iterator iter = relations.begin(); iter != relations.end(); iter++) {
            const Pattern pattern2 = iter->first;
            *OUT << "\t" << pattern_s << "\t" << label << "\t" << pattern2.tostring(classdecoder) << "\t" << iter->second << "\t" << iter->second / total_f << "\t" << this->occurrencecount(pattern2) << std::endl;
        }
    }
 
    void outputrelations(const Pattern & pattern, ClassDecoder & classdecoder, std::ostream * OUT, bool outputheader=true) {
        if (outputheader) *OUT << "#\tPATTERN1\tRELATION\tPATTERN2\tREL.COUNT\tREL.FREQUENCY\tCOUNT2" << std::endl;
        {
            t_relationmap relations = this->getsubparents(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "SUBSUMED-BY");
        }
        {
            t_relationmap relations = this->getsubchildren(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "SUBSUMES");
        }
        {
            t_relationmap relations = this->getleftneighbours(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "RIGHT-NEIGHBOUR-OF");
        }
        {
            t_relationmap relations = this->getrightneighbours(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "LEFT-NEIGHBOUR-OF");
        }
        {
            t_relationmap relations = this->getrightcooc(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "LEFT-COOC-OF");
        }
        {
            t_relationmap relations = this->getleftcooc(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "RIGHT-COOC-OF");
        }
        if (pattern.category() == SKIPGRAM) {
            t_relationmap relations = this->getskipcontent(pattern);
            this->outputrelations(pattern, relations, classdecoder, OUT, "INSTANTIATED-BY");
        }
    }


    void computenpmi( std::map<Pattern,t_relationmap_double> &  coocmap , double threshold, bool right=true, bool left=true) { 
        //compute npmi co-occurrence for all patterns

        //by default we do only right so we don't get double entries, cooc's
        //will always appear in sequential order in the data
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const Pattern pattern = iter->first;
            t_relationmap tmp;
            if ((right)&&(!left)) {
                tmp =  this->getrightcooc(pattern);
            } else if ((left)&&(!right)) {
                tmp =  this->getleftcooc(pattern);
            } else if (left && right) { //order not relevant
                tmp =  this->getcooc(pattern);
            }
            for (t_relationmap::iterator iter2 = tmp.begin(); iter2 != tmp.end(); iter2++) {
                const Pattern pattern2 = iter2->first;
                const double value = npmi(pattern,pattern2,iter2->second);
                if (value >= threshold) coocmap[pattern][pattern2] = value;
            }
        }
    } 

    void computecooc( std::map<Pattern,t_relationmap> &  coocmap , int threshold, bool right=true, bool left=true) { 
        //compute absolute co-occurrence for all patterns
        //
        //by default we do only right so we don't get double entries, cooc's
        //will always appear in sequential order in the data
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const Pattern pattern = iter->first;
            t_relationmap tmp;
            if ((right)&&(!left)) {
                tmp =  this->getrightcooc(pattern, threshold);
            } else if ((left)&&(!right)) {
                tmp =  this->getleftcooc(pattern, threshold);
            } else if (left && right) { //order not relevant
                tmp =  this->getcooc(pattern, threshold);
            }            
            for (t_relationmap::iterator iter2 = tmp.begin(); iter2 != tmp.end(); iter2++) {
                const Pattern pattern2 = iter2->first;
                const double value = iter2->second;
                if (value >= threshold) coocmap[pattern][pattern2] = value;
            }
        }
    } 

    int computeflexgrams_fromskipgrams() {
        this->cache_grouptotal.clear(); //forces recomputation of statistics
        int count = 0;
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const Pattern pattern = iter->first;
            if (pattern.category() == SKIPGRAM) {
                const Pattern flexgram = pattern.toflexgram();
                if (!this->has(flexgram)) count++;
                //copy data from pattern
                IndexedData * data = this->getdata(pattern);
                for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {
                    const IndexReference ref = *iter2;
                    this->data[flexgram].insert(ref);
                }
            }
        }
        return count;
    }

    int computeflexgrams_fromcooc(double threshold) {
        this->cache_grouptotal.clear(); //forces recomputation of statistics
        int found = 0;
        const unsigned char dynamicgap = 129;
        const Pattern dynamicpattern = Pattern(&dynamicgap,1);
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const Pattern pattern = iter->first;
            IndexedData matches;
            t_relationmap tmp =  this->getrightcooc(pattern, 0,0,0, &matches);
            for (t_relationmap::iterator iter2 = tmp.begin(); iter2 != tmp.end(); iter2++) {
                const Pattern pattern2 = iter2->first;
                const double value = npmi(pattern,pattern2,iter2->second);
                if (value >= threshold) {
                    const Pattern flexgram = pattern + dynamicpattern + pattern2;
                    if (!this->has(flexgram)) found++;
                    this->data[flexgram] = value;
                }
            }
        }
        return found;
    }

    void outputcooc_npmi(std::ostream * OUT, ClassDecoder& classdecoder, double threshold) {
        std::map<Pattern,t_relationmap_double> npmimap;
        std::cerr << "Collecting patterns and computing NPMI..." << std::endl;
        computenpmi(npmimap, threshold); 

        std::cerr << "Building inverse map..." << std::endl;
        //we want the reverse, so we can sort by co-occurrence
        std::multimap<double,std::pair<Pattern,Pattern>> inversemap;
        std::map<Pattern,t_relationmap_double>::iterator iter = npmimap.begin();
        while (iter != npmimap.end()) {
            for (t_relationmap_double::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
                inversemap.insert(std::pair<double,std::pair<Pattern,Pattern>>(iter2->second, std::pair<Pattern,Pattern>(iter->first, iter2->first)));
            }
            iter = npmimap.erase(iter);
        }

        *OUT << "Pattern1\tPattern2\tNPMI" << std::endl;
        for (std::multimap<double,std::pair<Pattern,Pattern>>::reverse_iterator iter2 = inversemap.rbegin(); iter2 != inversemap.rend(); iter2++) {
            const Pattern pattern1 = iter2->second.first;
            const Pattern pattern2 = iter2->second.second;
            *OUT << pattern1.tostring(classdecoder) << "\t" << pattern2.tostring(classdecoder) << "\t" << iter2->first << std::endl;
        }
    }


    void outputcooc(std::ostream * OUT, ClassDecoder& classdecoder, double threshold) {
        std::map<Pattern,t_relationmap> coocmap;
        std::cerr << "Collecting patterns and computing co-occurrence..." << std::endl;
        computecooc(coocmap, threshold); 

        std::cerr << "Building inverse map..." << std::endl;
        //we want the reverse, so we can sort by co-occurrence
        std::multimap<uint32_t,std::pair<Pattern,Pattern>> inversemap;
        std::map<Pattern,t_relationmap>::iterator iter = coocmap.begin();
        while (iter != coocmap.end()) {
            for (t_relationmap::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
                inversemap.insert(std::pair<uint32_t,std::pair<Pattern,Pattern>>(iter2->second, std::pair<Pattern,Pattern>(iter->first, iter2->first)));
            }
            iter = coocmap.erase(iter);
        }

        *OUT << "Pattern1\tPattern2\tCooc" << std::endl;
        for (std::multimap<uint32_t,std::pair<Pattern,Pattern>>::reverse_iterator iter2 = inversemap.rbegin(); iter2 != inversemap.rend(); iter2++) {
            const Pattern pattern1 = iter2->second.first;
            const Pattern pattern2 = iter2->second.second;
            *OUT << pattern1.tostring(classdecoder) << "\t" << pattern2.tostring(classdecoder) << "\t" << iter2->first << std::endl;
        }
    }

    int flexgramsize(const Pattern & pattern, IndexReference begin) {
        //attempt to find the flexgram size for the given begin position,
        //returns 0 if the flexgram was not found at all
        //if there are multiple matches, the shortest is returned
        
        if (pattern.category() != FLEXGRAM) return pattern.n();

        std::vector<Pattern> parts;
        int numberofparts = pattern.parts(parts);
        bool strictbegin = true;
        std::multimap<int, IndexReference> partmatches;
        int i = 0;
        std::vector<std::pair<IndexReference,Pattern>> rindex = this->getreverseindex_right(begin); //TODO: Check
        for (std::vector<std::pair<IndexReference,Pattern>>::iterator iter = rindex.begin(); iter != rindex.end(); iter++) {
            const Pattern part = iter->second;
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

#endif
