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

#include "pattern.h"
#include "classencoder.h"
#include "algorithms.h"
#include <limits>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <sstream>




enum ModelType {
	UNINDEXEDPATTERNMODEL = 10, 
    INDEXEDPATTERNMODEL = 20,
};

int getmodeltype(const std::string filename);


class IndexedData {
   protected:
    std::set<IndexReference> data;
   public:
    IndexedData() {};
    IndexedData(std::istream * in);
    void write(std::ostream * out) const; 
    
    bool has(const IndexReference & ref) const { return data.count(ref); }
    int count() const { return data.size(); }

    void insert(IndexReference ref) { data.insert(ref); }
    size_t size() const { return data.size(); }

    typedef std::set<IndexReference>::iterator iterator;
    typedef std::set<IndexReference>::const_iterator const_iterator;
    
    iterator begin() { return data.begin(); }
    const_iterator begin() const { return data.begin(); }

    iterator end() { return data.end(); }
    const_iterator end() const { return data.end(); }

    iterator find(const IndexReference & ref) { return data.find(ref); }
    const_iterator find(const IndexReference & ref) const { return data.find(ref); }    

    std::set<int> sentences() const {
        std::set<int> sentences;
        for (iterator iter = this->begin(); iter != this->end(); iter++) {
            const IndexReference ref = *iter;
            sentences.insert(ref.sentence); 
        }
        return sentences;
    }
    friend class IndexedDataHandler;
};

class IndexedDataHandler: public AbstractValueHandler<IndexedData> {
   public:
    const static bool indexed = true;
    virtual std::string id() { return "PatternStoreValueHandler"; }
    void read(std::istream * in, IndexedData & v) {
        uint32_t c;
        in->read((char*) &c, sizeof(uint32_t));
        for (unsigned int i = 0; i < c; i++) {
            IndexReference ref = IndexReference(in);
            v.insert(ref);
        }
    }
    void write(std::ostream * out, IndexedData & value) {
        const uint32_t c = value.count();
        out->write((char*) &c, sizeof(uint32_t));
        for (std::set<IndexReference>::iterator iter = value.data.begin(); iter != value.data.end(); iter++) {
            iter->write(out);
        }
    }
    virtual std::string tostring(IndexedData & value) {
        std::string s = "";
        for (std::set<IndexReference>::iterator iter = value.data.begin(); iter != value.data.end(); iter++) {
            if (!s.empty()) s += " ";
            s += iter->tostring();
        }
        return s;
    }
    int count(IndexedData & value) const {
        return value.data.size();
    }
    void add(IndexedData * value, const IndexReference & ref ) const {
        if (value == NULL) {
            std::cerr << "ValueHandler: Value is NULL!" << std::endl;
            throw InternalError();
        }
        value->insert(ref);
    }
    void convertto(IndexedData & source , IndexedData & target) const { if (&source != &target) target = source;  }; //noop
    void convertto(IndexedData & value, unsigned int & convertedvalue) const { convertedvalue = value.count(); };
};


class PatternModelOptions {
    public:
        int MINTOKENS;
        int MAXLENGTH;
        
        bool DOSKIPGRAMS;
        bool DOSKIPGRAMS_EXHAUSTIVE;
        int MINSKIPTYPES; 

        bool DOREVERSEINDEX;


        bool QUIET;
        bool DEBUG;

        PatternModelOptions() {
            MINTOKENS = -1; //defaults to 2 for building, 1 for loading
            MAXLENGTH = 100;

            MINSKIPTYPES = 2;
            DOSKIPGRAMS = false;
            DOSKIPGRAMS_EXHAUSTIVE = false;

            DOREVERSEINDEX = true; //only for indexed models

            DEBUG = false;
            QUIET = false;
        }


};

typedef PatternMap<uint32_t> t_relationmap;
typedef PatternMap<double> t_relationmap_double;

//basic read-only interface for pattern models, abstract base class.
class PatternModelInterface {
    public:
        virtual int getmodeltype() const=0;
        virtual int getmodelversion() const=0;
        virtual bool has(const Pattern &) const =0;
        virtual bool has(const PatternPointer &) const =0;
        virtual size_t size() const =0; 
        virtual int occurrencecount(const Pattern & pattern)=0;
        virtual double frequency(const Pattern &) =0;
        virtual int maxlength() const=0;
        virtual int minlength() const=0;
        virtual int types() const=0;
        virtual int tokens() const=0;
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
        
        std::multimap<IndexReference,Pattern> reverseindex; 

        std::set<int> cache_categories;
        std::set<int> cache_n;
        std::map<int,std::map<int,int>> cache_grouptotal; //total occurrences (used for frequency computation, within a group)
        std::map<int,std::map<int,int>> cache_grouptotalpatterns ; //total distinct patterns per group
        std::map<int,std::map<int,int>> cache_grouptotalwordtypes; //total covered word types per group
        std::map<int,std::map<int,int>> cache_grouptotaltokens; //total covered tokens per group

        virtual void postread(const PatternModelOptions options) {
            //this function has a specialisation specific to indexed pattern models,
            //this is the generic version
            for (iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern p = iter->first;
                const int n = p.n();
                if (n > maxn) maxn = n;
                if (n < minn) minn = n;
            }
        }
    public:
        PatternModel<ValueType,ValueHandler,MapType>() {
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
        }
        PatternModel<ValueType,ValueHandler,MapType>(std::istream *f, PatternModelOptions options) { //load from file
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            this->load(f,options);
        }

        PatternModel<ValueType,ValueHandler,MapType>(const std::string filename, const PatternModelOptions options) { //load from file
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
            this->load( (std::istream *) in, options);
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
        
        virtual void load(std::string filename, const PatternModelOptions options) {
            if (!options.QUIET) std::cerr << "Loading " << filename << std::endl;
            std::ifstream * in = new std::ifstream(filename.c_str());
            if (!in->good()) {
                std::cerr << "ERROR: Unable to load file " << filename << std::endl;
                throw InternalError();
            }
            this->load( (std::istream *) in, options);
            in->close();
            delete in;
        }

        virtual void load(std::istream * f, const PatternModelOptions options) { //load from file
            char null;
            f->read( (char*) &null, sizeof(char));        
            f->read( (char*) &model_type, sizeof(char));        
            f->read( (char*) &model_version, sizeof(char));        
            if ((null != 0) || ((model_type != UNINDEXEDPATTERNMODEL) && (model_type != INDEXEDPATTERNMODEL) ))  {
                std::cerr << "File is not a colibri model file (or a very old one)" << std::endl;
                throw InternalError();
            }
            f->read( (char*) &totaltokens, sizeof(uint64_t));        
            f->read( (char*) &totaltypes, sizeof(uint64_t)); 

            if ((model_type == INDEXEDPATTERNMODEL) && (this->getmodeltype() == UNINDEXEDPATTERNMODEL)) {
                //reading indexed pattern model as unindexed, ok:
                 MapType::template read<IndexedData,IndexedDataHandler>(f, options.MINTOKENS);
            } else if ((model_type == UNINDEXEDPATTERNMODEL) && (this->getmodeltype() == INDEXEDPATTERNMODEL)) {
                std::cerr << "ERROR: PAttern model is unindexed, unable to read as indexed" << std::endl;
                throw InternalError();
            } else {
                 MapType::template read(f, options.MINTOKENS); //read PatternStore
            }
            this->postread(options);
        }
        
        virtual void train(std::istream * in , PatternModelOptions options,  PatternModelInterface * constrainbymodel = NULL) {
            if (options.MINTOKENS == -1) options.MINTOKENS = 2;
            uint32_t sentence = 0;
            std::map<int, std::vector< std::vector< std::pair<int,int> > > > gapconf;
            if (!in->good()) {
                std::cerr << "ERROR: inputstream not good, file does not exist?" << std::endl;
                throw InternalError();
            }

            if (!options.QUIET) std::cerr << "Training patternmodel" << std::endl;
            int maxlength;
            std::vector<std::pair<Pattern,int>> ngrams;
            std::vector<Pattern> subngrams;
            bool found;
            IndexReference ref;
            int prevsize = 0;
            for (int n = 1; n <= options.MAXLENGTH; n++) { 
                int foundngrams = 0;
                int foundskipgrams = 0;
                in->clear();
                in->seekg(0);
                if (!options.QUIET) {
                    if (options.MINTOKENS > 1) {
                        std::cerr << "Counting " << n << "-grams" << std::endl; 
                    } else {
                        std::cerr << "Counting *all* n-grams (occurrence threshold=1)" << std::endl; 
                    }
                }

                if ((options.DOSKIPGRAMS_EXHAUSTIVE) && (gapconf[n].empty())) compute_multi_skips(gapconf[n], std::vector<std::pair<int,int> >(), n);
                
                sentence = 0; //reset
                while (!in->eof()) {
                    Pattern line = Pattern(in);
                    sentence++;
                    if (in->eof()) break;
                    if (n==1) totaltokens += line.size();
                    ngrams.clear();
                    if (options.MINTOKENS > 1) {
                        line.ngrams(ngrams, n);
                    } else if (options.MINTOKENS == 1) {
                        line.subngrams(ngrams,1,options.MAXLENGTH); //extract ALL ngrams if MINTOKENS == 1, no need to look back anyway, only one iteration over corpus
                    }

                    for (std::vector<std::pair<Pattern,int>>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
                        if ((constrainbymodel != NULL) && (!constrainbymodel->has(iter->first))) continue;
                        ref = IndexReference(sentence, iter->second);
                        found = true;
                        if ((n > 1) && (options.MINTOKENS > 1)) {
                            //check if sub-parts were counted
                            subngrams.clear();
                            iter->first.ngrams(subngrams,n-1);
                            for (std::vector<Pattern>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {
                                if (!this->has(*iter2)) {
                                    found = false;
                                    break;
                                }
                            }
                        }
                        if (found) {
                            ValueType * data = getdata(iter->first, true);
                            add(iter->first, data, ref );
                            if (options.DOREVERSEINDEX) {
                                reverseindex.insert(std::pair<IndexReference,Pattern>(ref,iter->first));
                            }
                        } else if (((n >= 3) || (options.MINTOKENS == 1)) && (options.DOSKIPGRAMS_EXHAUSTIVE)) {
                            foundskipgrams += this->computeskipgrams(iter->first, options, gapconf, &ref, NULL, constrainbymodel, true);
                        }
                    }
                }

                
                foundngrams = this->size() - foundskipgrams - prevsize;
        
                if (foundngrams) {
                    if (n > this->maxn) this->maxn = n;
                    if (n < this->minn) this->minn = n;
                } else {
                    std::cerr << "None found" << std::endl;
                    break;
                }
                if (!options.QUIET) std::cerr << " Found " << foundngrams << " ngrams...";
                if (foundskipgrams && !options.QUIET) std::cerr << foundskipgrams << " skipgram occurrences...";
                if ((options.MINTOKENS > 1) && (n == 1)) totaltypes += this->size(); //total unigrams, also those not in model
                int pruned = this->prune(options.MINTOKENS,n);
                if (!options.QUIET) std::cerr << "pruned " << pruned;
                if (foundskipgrams) {
                    int prunedextra = this->pruneskipgrams(options.MINTOKENS, options.MINSKIPTYPES, n);
                    if (prunedextra && !options.QUIET) std::cerr << " plus " << prunedextra << " extra skipgrams..";
                    pruned += prunedextra;
                }
                if (!options.QUIET) std::cerr << "...total kept: " << (foundngrams + foundskipgrams) - pruned << std::endl;
                if (options.MINTOKENS == 1) break; //no need for further n iterations, we did all in one pass since there's no point in looking back
                prevsize = this->size();
            }
            if (options.DOSKIPGRAMS && !options.DOSKIPGRAMS_EXHAUSTIVE) {
                this->trainskipgrams(options, constrainbymodel);
            }
        }


        virtual void train(const std::string filename, const PatternModelOptions options, PatternModelInterface * constrainbymodel = NULL) {
            std::ifstream * in = new std::ifstream(filename.c_str());
            this->train((std::istream*) in, options, constrainbymodel);
            in->close();
            delete in;
        }


        virtual int computeskipgrams(const Pattern & pattern, PatternModelOptions  options, std::map<int, std::vector< std::vector< std::pair<int,int>>>> & gapconf, const IndexReference * singleref= NULL, const IndexedData * multiplerefs = NULL,  PatternModelInterface * constrainbymodel = NULL, const bool exhaustive = false) {
            if (options.MINTOKENS == -1) options.MINTOKENS = 2;
            //internal function for computing skipgrams for a single pattern
            int foundskipgrams = 0;
            const int n = pattern.n();
            //loop over all possible gap configurations
            for (std::vector<std::vector<std::pair<int,int>>>::iterator iter2 =  gapconf[n].begin(); iter2 != gapconf[n].end(); iter2++) {
                std::vector<std::pair<int,int>> * gapconfiguration = &(*iter2);

                //add skips
                const Pattern skipgram = pattern.addskips(*gapconfiguration);                            
                if (options.DEBUG) {
                    std::cerr << "Checking for: " << std::endl;
                    skipgram.out();
                }

                if ((constrainbymodel != NULL) && (!constrainbymodel->has(skipgram))) continue;
                if ((int) skipgram.n() != n) {
                    std::cerr << "Generated invalid skipgram, n=" << skipgram.n() << ", expected " << n << std::endl;
                    throw InternalError();
                }

                bool skipgram_valid = true;
                bool check_extra = false;
                //check if sub-parts were counted
                std::vector<Pattern> subngrams;
                skipgram.ngrams(subngrams,n-1); //this also works for and returns skipgrams, despite the name
                for (std::vector<Pattern>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {
                    const Pattern subpattern = *iter2;
                    if (!subpattern.isgap(0) && !subpattern.isgap(subpattern.n() - 1)) {
                        if (options.DEBUG) {
                            std::cerr << "Subpattern: " << std::endl;
                            subpattern.out();
                        }
                        //this subpattern is a valid
                        //skipgram (no beginning or ending
                        //gaps) that should occur
                        if (!this->has(subpattern)) {
                            if (options.DEBUG) std::cerr << "  discarded" << std::endl;
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
                    if (exhaustive) { //by definition the case in non-exhaustive mode
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
                            if (options.DEBUG) {
                                std::cerr << "Subskipgram: " << std::endl;
                                subskipgram.out();
                            }
                            if (!this->has(subskipgram)) {
                                if (options.DEBUG) std::cerr << "  discarded" << std::endl;
                                skipgram_valid = false;
                                break;
                            }
                        }
                    }
                }


                if (skipgram_valid) {
                    if (options.DEBUG) std::cerr << "  counted!" << std::endl;
                    if (!has(skipgram)) foundskipgrams++;
                    ValueType * data = this->getdata(skipgram,true);
                    if (singleref != NULL) {
                        add(skipgram, data, *singleref ); //counts the actual skipgram, will add it to the model
                        if (options.DOREVERSEINDEX) {
                            reverseindex.insert(std::pair<IndexReference,Pattern>(*singleref,skipgram));
                        }
                    } else if (multiplerefs != NULL) {
                        for (IndexedData::iterator refiter =  multiplerefs->begin(); refiter != multiplerefs->end(); refiter++) {
                            const IndexReference ref = *refiter;
                            add(skipgram, data, ref ); //counts the actual skipgram, will add it to the model
                            if (options.DOREVERSEINDEX) {
                                reverseindex.insert(std::pair<IndexReference,Pattern>(ref,skipgram));
                            }
                        }
                    } else {
                        std::cerr << "ERROR: computeskipgrams() called with no singleref and no multiplerefs" << std::endl;
                        throw InternalError();
                    }

                }
            }
            return foundskipgrams;
        }

        
        virtual void trainskipgrams(const PatternModelOptions options,  PatternModelInterface * constrainbymodel = NULL) {
            std::cerr << "Can not compute skipgrams on unindexed model (except exhaustively)" << std::endl;
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
            out->write( (char*) &totaltypes, sizeof(uint64_t)); 
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

        virtual int maxlength() const { return maxn; };
        virtual int minlength() const { return minn; };
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

        int types() const { return totaltypes; }
        int tokens() const { return totaltokens; }

        unsigned char type() const { return model_type; }
        unsigned char version() const { return model_version; }

        void output(std::ostream *);
        
        
        int coveragecount(const Pattern &  key) {
           return this->occurrencecount(key) * key.size();
        }
        double coverage(const Pattern & key) {
            return this->coveragecount(key) / this->tokens();
        }
       

        std::vector<Pattern> getreverseindex(IndexReference ref) {
            //Auxiliary function
            std::vector<Pattern> result;
            if (this->reverseindex.count(ref)) {
                for (std::multimap<IndexReference,Pattern>::iterator it = this->reverseindex.lower_bound(ref); it != this->reverseindex.upper_bound(ref); it++) {
                    const Pattern pattern = it->second;
                    result.push_back(pattern);

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
                        if (((*itern == 0) || ((int) pattern.n() == *itern))  && ((*iterc == 0) || (pattern.category() == *iterc))) {
                            std::vector<Pattern> unigrams;
                            pattern.ngrams(unigrams, 1);
                            for (std::vector<Pattern>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                                const Pattern p = *iter2;
                                types.insert(p);
                            }
                        }
                        cache_grouptotaltokens[*iterc][*itern] += this->valuehandler.count(iter->second);
                        iter++;
                    }
                    cache_grouptotalwordtypes[*iterc][*itern] += types.size();
                }
            }
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

        int prune(int threshold,int _n=0) {
            int pruned = 0;
            PatternModel::iterator iter = this->begin(); 
            while (iter != this->end()) {
                const Pattern pattern = iter->first;
                if (( (_n == 0) || (pattern.n() == (unsigned int) _n) )&& (occurrencecount(pattern) < threshold)) {
                    //std::cerr << occurrencecount(pattern) << std::endl;
                    //std::cerr << "preprune:" << this->size() << std::endl;
                    iter = this->erase(iter); 
                    //std::cerr << "postprune:" << this->size() << std::endl;
                    pruned++;
                } else {
                    iter++;
                }
            };       

            if (pruned) prunereverseindex();
            return pruned;
        }

        virtual int pruneskipgrams(int threshold, int minskiptypes=2, int _n = 0) {
            return 0; //only works for indexed models
        }

        int prunereverseindex(int _n = 0) {
            //prune patterns from reverse index if they don't exist anymore
            int pruned = 0;
            std::multimap<IndexReference,Pattern>::iterator iter = reverseindex.begin(); 
            while (iter != reverseindex.end()) {
                const Pattern pattern = iter->second;
                if (( (_n == 0) || (pattern.n() == (unsigned int) _n) ) && (!this->has(pattern))) {
                    iter = reverseindex.erase(iter);
                    pruned++;
                } else {
                    iter++;
                }
            }
            return pruned;
        }

        template<class ValueType2,class ValueHandler2,class MapType2>
        int prunebymodel(PatternModel<ValueType2,ValueHandler2,MapType2> & secondmodel) {
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

        void histogram(std::ostream * OUT) {
            std::map<int,int> hist;
            for (PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern pattern = iter->first;
                int c = this->occurrencecount(pattern);
                hist[c]++;
            }
            *OUT << "HISTOGRAM" << std::endl;
            *OUT << "------------------------------" << std::endl;
            *OUT << "OCCURRENCES\tPATTERNS" << std::endl;
            for (std::map<int,int>::iterator iter = hist.begin(); iter != hist.end(); iter++) {
                *OUT << iter->first << "\t" << iter->second << std::endl;
            }
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

            int coveredtypes = totalwordtypesingroup(0,0);
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
                        if (c == 0) {
                            *OUT << std::setw(10) << "all";
                        } else if (c == NGRAM) {
                            *OUT << std::setw(10) << "n-gram";
                        } else if (c == SKIPGRAM) {
                            *OUT << std::setw(10) << "skipgram";
                        } else if (c == FLEXGRAM) {
                            *OUT << std::setw(10) << "flexgram";
                        }
                        if (n == 0) {
                            *OUT << std::setw(10) << "all";
                        } else {
                            *OUT << std::setw(10) << n;
                        }
                        *OUT << std::setw(10) << cache_grouptotalpatterns[c][n];
                        if (this->getmodeltype() != UNINDEXEDPATTERNMODEL) {
                            *OUT << std::setw(10) << cache_grouptotaltokens[c][n];
                            *OUT << std::setw(10) << cache_grouptotaltokens[c][n] / (double) this->tokens();
                        }
                        *OUT << std::setw(10) << cache_grouptotalwordtypes[c][n];
                        *OUT << std::setw(12) << cache_grouptotal[c][n] << std::endl;;
                    }
                }
            }
        }
        
        

        virtual void outputrelations(const Pattern & pattern, ClassDecoder & classdecoder, std::ostream * OUT) {} //does nothing for unindexed models
        virtual t_relationmap getsubchildren(const Pattern & pattern) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getsubparents(const Pattern & pattern) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getskipcontent(const Pattern & pattern) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getleftneighbours(const Pattern & pattern) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap getrightneighbours(const Pattern & pattern) { return t_relationmap(); } //does nothing for unindexed models
        virtual t_relationmap_double getnpmi(const Pattern & pattern, double threshold) { return t_relationmap_double(); } //does nothing for unindexed models
        virtual int computeflexgrams_fromskipgrams() { return 0; }//does nothing for unindexed models
        virtual int computeflexgrams_fromcooc() {return 0; }//does nothing for unindexed models
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
                IndexedData * data = this->getdata(p);
                if (options.DOREVERSEINDEX) {
                    //construct the reverse index
                    for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {                    
                        const IndexReference ref = *iter2;
                        this->reverseindex.insert(std::pair<IndexReference,Pattern>(ref,p));
                    }
                }
            }
        }
   public:



       
    IndexedPatternModel<MapType>(): PatternModel<IndexedData,IndexedDataHandler,MapType>() {
        this->model_type = this->getmodeltype();
        this->model_version = this->getmodelversion();
    }
    IndexedPatternModel<MapType>(std::istream *f, const PatternModelOptions options):  PatternModel<IndexedData,IndexedDataHandler,MapType>(){ //load from file
        this->model_type = this->getmodeltype();
        this->model_version = this->getmodelversion();
        this->load(f,options);
    }

    IndexedPatternModel<MapType>(const std::string filename, const PatternModelOptions options): PatternModel<IndexedData,IndexedDataHandler,MapType>() { //load from file
        this->model_type = this->getmodeltype();
        this->model_version = this->getmodelversion();
        std::ifstream * in = new std::ifstream(filename.c_str());
        this->load( (std::istream *) in, options);
        in->close();
        delete in;
    }

    int getmodeltype() const { return INDEXEDPATTERNMODEL; }
    int getmodelversion() const { return 1;} 


    void add(const Pattern & pattern, IndexedData * value, const IndexReference & ref) {
        if (value == NULL) {
            value = getdata(pattern,true);
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
        std::map<int, std::vector< std::vector< std::pair<int,int> > > > gapconf;
        for (int n = 3; n <= options.MAXLENGTH; n++) {
            if ((options.DOSKIPGRAMS) && (gapconf[n].empty())) compute_multi_skips(gapconf[n], std::vector<std::pair<int,int> >(), n);
            if (!options.QUIET) std::cerr << "Counting " << n << "-skipgrams" << std::endl; 
            int foundskipgrams = 0;
            for (typename MapType::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern pattern = iter->first;
                const IndexedData multirefs = iter->second;
                if (((int) pattern.n() == n) && (pattern.category() == NGRAM) ) foundskipgrams += this->computeskipgrams(pattern,options, gapconf, NULL, &multirefs, constrainbymodel, false);
            }
            if (!foundskipgrams) {
                std::cerr << " None found" << std::endl;
                break;
            }
            if (!options.QUIET) std::cerr << " Found " << foundskipgrams << " skipgrams...";
            int pruned = this->prune(options.MINTOKENS,n);
            if (!options.QUIET) std::cerr << "pruned " << pruned;
            int prunedextra = this->pruneskipgrams(options.MINTOKENS, options.MINSKIPTYPES, n);
            if (prunedextra && !options.QUIET) std::cerr << " plus " << prunedextra << " extra skipgrams..";
            if (!options.QUIET) std::cerr << "...total kept: " <<  foundskipgrams << std::endl;
        }
    }

    Pattern * getpatternfromtoken(IndexReference ref) {
        for (std::multimap<IndexReference,Pattern>::iterator iter = this->reverseindex.lower_bound(ref); iter != this->reverseindex.upper_bound(ref); iter++) {
            if (iter->second.n() == 1) return &(iter->second);
        }
        return NULL;
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
                        Pattern * p = this->getpatternfromtoken(ref + i);
                        if (p == NULL) {
                            //std::cerr << "notoken@" << ref.sentence << ":" << ref.token + i << std::endl;
                            notoken = true; break;
                        } else {
                            //std::cerr << "foundtoken@" << ref.sentence << ":" << ref.token + i << std::endl;
                            skipcontent_atref = skipcontent_atref +  *p;
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

    t_relationmap getsubchildren(const Pattern & pattern) {
        //returns patterns that are subsumed by the specified pattern (i.e.
        //smaller patterns)
        if (this->reverseindex.empty()) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            std::cerr << "ERROR: No data found for pattern!" << std::endl;
            throw InternalError();
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
                for (std::multimap<IndexReference,Pattern>::iterator iter2 = this->reverseindex.lower_bound(begin); iter2 != this->reverseindex.upper_bound(begin); iter2++) {
                    //const IndexReference ref2 = iter2->first;
                    const Pattern candidate = iter2->second;
                    //std::cerr << "Considering candidate @" << ref2.sentence << ":" << ref2.token << ", n=" << candidate.n() << ", bs=" << candidate.bytesize() <<  std::endl;
                    //candidate.out();
                    if (((int) candidate.n() <= maxsubn) && (candidate != pattern)) {
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
        return subchildren;
    }

    t_relationmap getsubparents(const Pattern & pattern) {
        //returns patterns that subsume the specified pattern (i.e. larger
        //patterns)
        if (this->reverseindex.empty()) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            std::cerr << "ERROR: No data found for pattern!" << std::endl;
            throw InternalError();
        }
        
        t_relationmap subsumes;
        const int _n = pattern.n();
        //search in forward index
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;

            IndexReference bos = IndexReference(ref.sentence, 0);

            //search in reverse index
            for (std::multimap<IndexReference,Pattern>::iterator iter2 = this->reverseindex.lower_bound(bos); iter2 != this->reverseindex.end(); iter2++) {
                if ((iter2->first.sentence != ref.sentence) || (iter2->first.token > ref.token)) break;
                
                const Pattern candidate = iter2->second;

                int minsubsize = _n + (ref.token - iter2->first.token);

                if (((int) candidate.n() >= minsubsize)  && (candidate != pattern)) {
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
        return subsumes;
    }


    t_relationmap getleftneighbours(const Pattern & pattern) {
        if (this->reverseindex.empty()) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            std::cerr << "ERROR: No data found for pattern!" << std::endl;
            throw InternalError();
        }
        
        t_relationmap neighbours;
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;

            IndexReference bos =  IndexReference(ref.sentence, 0);

            for (std::multimap<IndexReference,Pattern>::iterator iter2 = this->reverseindex.lower_bound(bos); iter2 != this->reverseindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const Pattern neighbour = iter2->second;
                if (ref2.token + neighbour.n() == ref.token) {
                    neighbours[neighbour]++;
                } else if ((ref2.token > ref.token) || (ref2.sentence > ref.sentence)) break;
            }
        }
        return neighbours;
    }

    t_relationmap getrightneighbours(const Pattern & pattern) {
        if (this->reverseindex.empty()) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            std::cerr << "ERROR: No data found for pattern!" << std::endl;
            throw InternalError();
        }
        
        t_relationmap neighbours;
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;
            IndexReference nextref = ref + 1;

            for (std::multimap<IndexReference,Pattern>::iterator iter2 = this->reverseindex.lower_bound(nextref); iter2 != this->reverseindex.upper_bound(nextref); iter2++) {
                const Pattern neighbour = iter2->second;
                neighbours[neighbour]++;
            }
        }
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
        if (pruned) this->prunereverseindex();
        return pruned;
    } 

    virtual void computecoveragestats() {
        //opting for memory over speed (more iterations, less memory)
        // Indexed model overloads this for better cache_grouptotaltokens computation! 
        if ((this->cache_grouptotal.empty()) && (!this->data.empty())) this->computestats();
        for (std::set<int>::iterator iterc = this->cache_categories.begin(); iterc != this->cache_categories.end(); iterc++) {
            for (std::set<int>::iterator itern = this->cache_n.begin(); itern != this->cache_n.end(); itern++) {
                std::set<Pattern> types;
                std::set<IndexReference> tokens;
                typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); 
                while (iter != this->end()) {
                    const Pattern pattern = iter->first;                        
                    if (((*itern == 0) || ((int) pattern.n() == *itern))  && ((*iterc == 0) || (pattern.category() == *iterc))) {
                        std::vector<Pattern> unigrams;
                        pattern.ngrams(unigrams, 1);
                        for (std::vector<Pattern>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                            const Pattern p = *iter2;
                            types.insert(p);
                        }
                        IndexedData * data = this->getdata(pattern);
                        for (IndexedData::iterator dataiter = data->begin(); dataiter != data->end(); dataiter++) {
                            tokens.insert(*dataiter);
                        }
                    }
                    iter++;
                }
                this->cache_grouptotalwordtypes[*iterc][*itern] += types.size();
                this->cache_grouptotaltokens[*iterc][*itern] += tokens.size();
            }
        }
    }

    t_relationmap getrightcooc(const Pattern & pattern, IndexedData * matches = NULL) { 
        if (this->reverseindex.empty()) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            std::cerr << "ERROR: No data found for pattern!" << std::endl;
            throw InternalError();
        }
       
        const int _n = pattern.n();
        t_relationmap cooc;
        //find everything that co-occurs *without overlap* TO THE RIGHT 
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;


            for (std::multimap<IndexReference,Pattern>::iterator iter2 = this->reverseindex.lower_bound(ref); iter2 != this->reverseindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const Pattern neighbour = iter2->second;
                if (ref2.token > ref.token + _n) {
                    cooc[neighbour]++;
                    if (matches != NULL) matches->insert(ref2);
                } else if (ref2.sentence > ref.sentence) break;
            }
        }
        return cooc;
    }


    t_relationmap getleftcooc(const Pattern & pattern) { 
        if (this->reverseindex.empty()) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            std::cerr << "ERROR: No data found for pattern!" << std::endl;
            throw InternalError();
        }
       
        t_relationmap cooc;
        //find everything that co-occurs *without overlap* TO THE RIGHT 
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;

            IndexReference bos =  IndexReference(ref.sentence, 0);

            for (std::multimap<IndexReference,Pattern>::iterator iter2 = this->reverseindex.lower_bound(bos); iter2 != this->reverseindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const Pattern neighbour = iter2->second;
                const int _n = neighbour.n();
                if (ref2.token + _n < ref.token ) {
                    cooc[neighbour]++;
                } else if (ref2.sentence > ref.sentence) break;
            }
        }
        return cooc;
    }


    t_relationmap getcooc(const Pattern & pattern) { 
        if (this->reverseindex.empty()) {
            std::cerr << "ERROR: No reverse index present" << std::endl;
            throw InternalError();
        }

        IndexedData * data = this->getdata(pattern);
        if (data == NULL) {
            std::cerr << "ERROR: No data found for pattern!" << std::endl;
            throw InternalError();
        }
       
        const int _n = pattern.n();
        t_relationmap cooc;
        //find everything that co-occurs *without overlap* TO THE RIGHT 
        for (IndexedData::iterator iter = data->begin(); iter != data->end(); iter++) {
            const IndexReference ref = *iter;

            IndexReference bos =  IndexReference(ref.sentence, 0);

            for (std::multimap<IndexReference,Pattern>::iterator iter2 = this->reverseindex.lower_bound(bos); iter2 != this->reverseindex.end(); iter2++) {
                const IndexReference ref2 = iter2->first;
                const Pattern neighbour = iter2->second;
                const int _n2 = neighbour.n();
                if ((ref2.token + _n2 < ref.token ) || (ref2.token > ref.token + _n)) {
                    cooc[neighbour]++;
                } else if (ref2.sentence > ref.sentence) break;
            }
        }
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


    void computenpmi( std::map<Pattern,t_relationmap_double> &  coocmap , double threshold, bool right=true, bool left=false) { 
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const Pattern pattern = iter->first;
            t_relationmap tmp;
            if ((right)&&(!left)) {
                tmp =  this->getrightcooc(pattern);
            } else if ((left)&&(!right)) {
                tmp =  this->getleftcooc(pattern);
            } else if (left && right) {
                tmp =  this->getcooc(pattern);
            }
            for (t_relationmap::iterator iter2 = tmp.begin(); iter2 != tmp.end(); iter2++) {
                const Pattern pattern2 = iter2->first;
                const double value = npmi(pattern,pattern2,iter2->second);
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
            t_relationmap tmp =  this->getrightcooc(pattern, &matches);
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

    void outputcooc(std::ostream * OUT, ClassDecoder& classdecoder, double threshold) {
        std::map<Pattern,t_relationmap_double> npmimap;
        computenpmi(npmimap, threshold); 
        //we want the reverse,, so we can sort by co-occurrence
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
        for (std::multimap<Pattern,IndexReference>::iterator iter = this->reverseindex.lower_bound(begin); iter != this->reversindex.end(); iter++) {            
            const Pattern part = iter->first;
            IndexReference ref = iter->second;
            if (ref.sentence > begin.sentence) break;
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
