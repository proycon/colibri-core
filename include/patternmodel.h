#ifndef PATTERNMODEL_H
#define PATTERNMODEL_H

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

int getmodeltype(const std::string filename) {
    unsigned char null;
    unsigned char model_type;
    unsigned char model_version;
    std::ifstream * f = new std::ifstream(filename.c_str());
    f->read( (char*) &null, sizeof(char));        
    f->read( (char*) &model_type, sizeof(char));        
    f->read( (char*) &model_version, sizeof(char));        
    f->close();
    delete f;
    if ((null != 0) || ((model_type != UNINDEXEDPATTERNMODEL) && (model_type != INDEXEDPATTERNMODEL) ))  {
        return 0;
    } else {
        return model_type;
    }
}




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

    typedef std::set<IndexReference>::iterator iterator;
    typedef std::set<IndexReference>::const_iterator const_iterator;
    
    iterator begin() { return data.begin(); }
    const_iterator begin() const { return data.begin(); }

    iterator end() { return data.end(); }
    const_iterator end() const { return data.end(); }

    iterator find(const IndexReference & ref) { return data.find(ref); }
    const_iterator find(const IndexReference & ref) const { return data.find(ref); }    
    friend class IndexedDataHandler;
};

class IndexedDataHandler: public AbstractValueHandler<IndexedData> {
   public:
    const static bool indexed = true;
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
};


class PatternModelOptions {
    public:
        int MINTOKENS;
        int MAXLENGTH;
        
        bool DOFIXEDSKIPGRAMS;
        int MINSKIPTYPES; 
        int MINSKIPTOKENS;

        bool DOREVERSEINDEX;
        
        bool DEBUG;

        PatternModelOptions() {
            MINTOKENS = 2;
            MAXLENGTH = 8;

            MINSKIPTYPES = 2;
            MINSKIPTOKENS = 2;
            DOFIXEDSKIPGRAMS = false;

            DOREVERSEINDEX = false; //only for indexed models

            DEBUG = false;
        }


};


template<class ValueType, class ValueHandler = BaseValueHandler<ValueType>, class MapType = PatternMap<ValueType, BaseValueHandler<ValueType>>>
class PatternModel: public MapType {
    protected:
        unsigned char model_type;
        unsigned char model_version;
        uint64_t totaltokens; //INCLUDES TOKENS NOT COVERED BY THE MODEL!
        uint64_t totaltypes; //TOTAL UNIGRAM TYPES, INCLUDING NOT COVERED BY THE MODEL!

        int maxn; 
        int minn; 
        
        std::multimap<IndexReference,Pattern> reverseindex; 
        
        void postread(const PatternModelOptions options) {
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
        }
        PatternModel<ValueType,ValueHandler,MapType>(std::istream *f, const PatternModelOptions options) { //load from file
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            this->load(f,options);
        }

        PatternModel<ValueType,ValueHandler,MapType>(const std::string filename, const PatternModelOptions options) { //load from file
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            std::ifstream * in = new std::ifstream(filename.c_str());
            this->load( (std::istream *) in, options);
            in->close();
            delete in;
        }

        void load(std::istream * f, const PatternModelOptions options) { //load from file
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

            this->read(f, options.MINTOKENS); //read PatternStore
            this->postread(options);
        }
        
        void train(std::istream * in , const PatternModelOptions options) {
            uint32_t sentence = 0;
            std::map<int, std::vector< std::vector< std::pair<int,int> > > > gapconf;
            if (!in->good()) {
                std::cerr << "ERROR: inputstream not good, file does not exist?" << std::endl;
                throw InternalError();
            }

            std::cerr << "Training patternmodel" << std::endl;
            for (int n = 1; n <= options.MAXLENGTH; n++) {
                int foundcount = 0;
                in->clear();
                in->seekg(0);
                std::cerr << "Counting " << n << "-grams" << std::endl; 

                if ((options.DOFIXEDSKIPGRAMS) && (gapconf[n].empty())) compute_multi_skips(gapconf[n], std::vector<std::pair<int,int> >(), n);
                
                sentence = 0; //reset
                while (!in->eof()) {
                    Pattern line = Pattern(in);
                    sentence++;
                    if (in->eof()) break;
                    if (n==1) totaltokens += line.size();
                    std::vector<std::pair<Pattern,int>> ngrams;
                    line.ngrams(ngrams, n);

                    for (std::vector<std::pair<Pattern,int>>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
                        const Pattern pattern = iter->first;
                        if (pattern.category() == NGRAM) {
                            const IndexReference ref = IndexReference(sentence, iter->second);
                            bool found = true;
                            if (n > 1) {
                                //check if sub-parts were counted
                                std::vector<Pattern> subngrams;
                                pattern.ngrams(subngrams,n-1);
                                for (std::vector<Pattern>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {
                                    const Pattern subpattern = *iter2;
                                    if ((subpattern.category() == NGRAM) && (!this->has(subpattern))) {
                                        found = false;
                                        break;
                                    }
                                }
                            }
                            if (found) {
                                ValueType * data = getdata(pattern);
                                foundcount++;
                                add(pattern, data, ref );
                                if (options.DOREVERSEINDEX) {
                                    reverseindex.insert(std::pair<IndexReference,Pattern>(ref,pattern));
                                }
                            }                
                            if (options.DOFIXEDSKIPGRAMS) {
                                //loop over all possible gap configurations
                                for (std::vector<std::vector<std::pair<int,int>>>::iterator iter =  gapconf[n].begin(); iter != gapconf[n].end(); iter++) {
                                    std::vector<std::pair<int,int>> * gapconfiguration = &(*iter);

                                    //add skips
                                    const Pattern skippattern = pattern.addfixedskips(*gapconfiguration);                            
                                    if (skippattern.n() != n) {
                                        std::cerr << "Generated invalid skipgram, n=" << skippattern.n() << ", expected " << n << std::endl;
                                        throw InternalError();
                                    }

                                    //test whether parts occur in model, otherwise skip
                                    //can't occur either and we can discard it
                                    bool skippattern_valid = true;
                                    std::vector<Pattern> parts;
                                    skippattern.parts(parts);
                                    for (std::vector<Pattern>::iterator iter3 = parts.begin(); iter3 != parts.end(); iter3++) {
                                        const Pattern part = *iter3;
                                        if (!this->has(part)) {
                                            skippattern_valid = false;
                                            break;
                                        }
                                    }

                                    if (skippattern_valid) {
                                        ValueType * data = getdata(skippattern);
                                        add(skippattern, data, ref );
                                        foundcount++;
                                        if (options.DOREVERSEINDEX) {
                                            reverseindex.insert(std::pair<IndexReference,Pattern>(ref,skippattern));
                                        }
                                    }
                                }
                            }

                        }           
                    }
                }

                if (foundcount) {
                    if (n > this->maxn) this->maxn = n;
                    if (n < this->minn) this->minn = n;
                }
                std::cerr << " Found " << foundcount << "...";
                if (n == 1) totaltypes += this->size(); //total unigrams, also those not in model
                int pruned = this->prune(options.MINTOKENS,n);
                std::cerr << "pruned " << pruned;
                int prunedextra = this->pruneskipgrams(options.MINTOKENS, options.MINSKIPTYPES, options.MINSKIPTOKENS, n);
                if (prunedextra) {
                    std::cerr << " plus " << prunedextra << " extra skipgrams (=" << pruned + prunedextra << ")" << std::endl;
                } else {
                    std::cerr << std::endl;
                }

            }
        }
        void train(const std::string filename, const PatternModelOptions options) {
            std::ifstream * in = new std::ifstream(filename.c_str());
            this->train((std::istream*) in, options);
            in->close();
            delete in;
        }

        
        //creates a new test model using the current model as training
        // i.e. only fragments existing in the training model are counted
        // remaining fragments are 'uncovered'
        void test(MapType & target, std::istream * in);

        void write(std::ostream * out) {
            const char null = 0;
            out->write( (char*) &null, sizeof(char));        
            out->write( (char*) &model_type, sizeof(char));        
            out->write( (char*) &model_version, sizeof(char));        
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
        
        virtual ValueType * getdata(const Pattern & pattern) { 
            typename MapType::iterator iter = this->find(pattern);
            if (iter == this->end()) {
                return NULL;
            } else {
                return &(iter->second); 
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

        virtual int add(const Pattern & pattern, ValueType * value, const IndexReference & ref) {
            if (value == NULL) {
                (*this)[pattern]; //creates the data point if it didn't exist yet
                value = getdata(pattern);
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

        virtual int pruneskipgrams(int threshold, int minskiptypes=2, int minskiptokens=2, int _n = 0) {
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

        void print(std::ostream * out, ClassDecoder & decoder) {
            *out << "PATTERN\tCOUNT\tSIZE\tCOVERAGECOUNT\tCOVERAGE\tCATEGORY" << std::endl;
            for (PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern pattern = iter->first;
                const std::string pattern_s = pattern.tostring(decoder);
                const int count = this->occurrencecount(pattern); //TODO: can be sped up by using iter->second!
                //const double freq = count / (double) this->tokens(); 
                const int covcount = this->coveragecount(pattern);
                const double coverage = covcount / (double) this->tokens();
                *out << pattern_s << "\t" << count << "\t" << pattern.size() << "\t" << covcount << "\t" << coverage << "\t" << pattern.category() <<  std::endl;

            }
        }
        
        void histogram(std::ostream * OUT) {
            std::map<int,int> hist;
            for (PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern pattern = iter->first;
                int c = this->occurrencecount(pattern);
                hist[c]++;
            }
            *OUT << "HISTOGRAM" << std::endl;
            *OUT << "Frequency\tpatterns" << std::endl;
            for (std::map<int,int>::iterator iter = hist.begin(); iter != hist.end(); iter++) {
                *OUT << iter->first << "\t" << iter->second << std::endl;
            }
        }

        void report(std::ostream * OUT) {
            *OUT << std::setiosflags(std::ios::fixed) << std::setprecision(4) << std::endl;       
            *OUT << "REPORT" << std::endl;
            *OUT << "   Warning: Model is unindexed, token coverage counts are mere maximal projections" << std::endl;
            *OUT << "            assuming no overlap at all!!! Use an indexed model for accurate coverage counts" << std::endl;
            *OUT << "----------------------------------" << std::endl;
            *OUT << "                          " << std::setw(10) << "PATTERNS" << std::setw(10) << "PR.TOKENS" << std::setw(10) << "TYPES" << std::setw(10) << std::endl;
            *OUT << "Total:                    " << std::setw(10) << this->size() << std::setw(10) << this->tokens() << std::setw(10) << this->types() <<  std::endl;
            std::set<IndexReference> coverage;
            int coveredtypes = 0;
            int coveredtokens = 0;
            for (typename PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern pattern = iter->first;
                const int _n = pattern.n();
                coveredtokens += this->occurrencecount(pattern) * _n;
                if (_n == 1) coveredtypes++;
            }
            if (coveredtokens > this->tokens()) coveredtokens = this->tokens();
            int uncoveredtokens = this->tokens() - coveredtokens;
            if (uncoveredtokens < 0) uncoveredtokens = 0;
            *OUT << "Uncovered: (min.projection): " << std::setw(10) << 0 << std::setw(10) << uncoveredtokens << std::setw(10) << this->types() - coveredtypes <<  std::endl;
            *OUT << "Covered (max. projection):   " << std::setw(10) << this->size() << std::setw(10) << coveredtokens << std::setw(10) << coveredtypes <<  std::endl;
            
            
            *OUT << "Per n:" << std::endl;
            std::set<int> sizes;
            std::set<int> cats;
            int maxn = 1;
            int n = 1;
            do {
                coverage.clear();
                int coveredtypes_n = 0;
                int coveredtokens_n = 0;
                int totalpatterns_n = 0;
                std::set<Pattern> types_n;
                if ((n == 1) || (sizes.count(n))) { 
                    for (typename PatternModel::iterator iter = this->begin(); iter != this->end(); iter++) {
                        const Pattern pattern = iter->first;
                        const int _n = pattern.n();
                        if (n == 1) {
                            sizes.insert(pattern.size());
                            cats.insert(pattern.category());
                        }
                        if (_n == n) {
                            coveredtokens_n += this->occurrencecount(pattern) * _n;
                            if (_n == 1) {  
                                coveredtypes_n++;
                            } else {
                                std::vector<Pattern> unigrams;
                                pattern.ngrams(unigrams, 1);
                                for (std::vector<Pattern>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                                    const Pattern p = *iter2;
                                    types_n.insert(p);
                                }
                            }

                            totalpatterns_n++;
                        }
                    }
                    if (n > 1) coveredtypes_n = types_n.size();
                    *OUT << " n=" << n << "                      " << std::setw(10) << totalpatterns_n << std::setw(10) << coveredtokens << std::setw(10) << coveredtypes_n << std::endl; 
                    if (n == 1) {
                        std::set<int>::reverse_iterator iter = sizes.rbegin();
                        maxn = *iter; //get last item from sizes
                    }
                }
                n++;
            } while (n < maxn);

            std::set<Pattern> types_c;

            *OUT << "Per category:" << std::endl;
            for (int cat = 1; cat <= 3; cat++) {
                if (cats.count(cat)) {
                    int totalpatterns_c = 0;
                    int coveredtypes_c = 0;
                    int coveredtokens_c = 0;
                    coverage.clear();
                    for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
                        const Pattern pattern = iter->first;
                        const int _n = pattern.n();
                        if (pattern.category() == cat) {
                            coveredtokens_c += this->occurrencecount(pattern) * _n;
                            if (_n == 1) {  
                                coveredtypes_c++;
                            } else {
                                std::vector<Pattern> unigrams;
                                pattern.ngrams(unigrams, 1);
                                for (std::vector<Pattern>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                                    const Pattern p = *iter2;
                                    types_c.insert(p);
                                }
                            }
                            totalpatterns_c++;
                        }
                    }
                    if (n > 1) coveredtypes_c = types_c.size();
                    if (cat == 1) {
                        *OUT << " N-GRAMS                  ";
                    } else if (cat == 2) {
                        *OUT << " FIXED-SKIPGRAMS          ";
                    } else if (cat == 3) {
                        *OUT << " DYNAMIC-SKIPGRAMS        ";
                    }
                    *OUT << std::setw(10) << totalpatterns_c << std::setw(10) << coveredtokens_c << std::setw(10) << coveredtypes_c << std::endl; 
                }

            }
        }
};



template<class MapType = PatternMap<IndexedData,IndexedDataHandler>> 
class IndexedPatternModel: public PatternModel<IndexedData,IndexedDataHandler,MapType> {
   public:
    IndexedPatternModel<MapType>(): PatternModel<IndexedData,IndexedDataHandler,MapType>() {}; 
    
    IndexedPatternModel<MapType>(std::istream *f, const PatternModelOptions options): PatternModel<IndexedData,IndexedDataHandler,MapType>(f, options) {}; 
    IndexedPatternModel<MapType>(const std::string filename, const PatternModelOptions options): PatternModel<IndexedData,IndexedDataHandler,MapType>(filename, options) {}; 
                                                                   

    int add(const Pattern & pattern, IndexedData * value, const IndexReference & ref) {
        if (value == NULL) {
            (*this)[pattern]; //creates the data point if it didn't exist yet
            value = getdata(pattern);
        }
        this->valuehandler.add(value, ref);
    }
    
    IndexedData * getdata(const Pattern & pattern)  { 
        typename MapType::iterator iter = this->find(pattern);
        if (iter != this->end()) {
            return &(iter->second); 
        } else {
            return NULL;
        }
    }

    void postread(const PatternModelOptions options) {
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
                    this->reverseindex.insert(ref,p);
                }
            }
/*            if ((p.category() == FIXEDSKIPGRAM) && (options.DOSKIPCONTENT)) {
                skipcontent[p
            }*/
        }
    }
    
    std::unordered_set<Pattern> getskipcontent(const Pattern & pattern) {
        std::unordered_set<Pattern> skipcontent;
        if (pattern.category() == FIXEDSKIPGRAM) {
            //find the gaps
            std::vector<std::pair<int,int>> gapdata;
            pattern.gaps(gapdata);

            IndexedData * data = getdata(pattern);
            for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {                    
                const IndexReference ref = *iter2;
                
                //compute all the gaps 
                for (std::vector<std::pair<int,int>>::iterator iter3 = gapdata.begin(); iter3 != gapdata.end(); iter3++) {
                    const IndexReference gapref = IndexReference(ref.sentence, ref.token + iter3->first);
                    const int requiredsize = iter3->second;

                    //find patterns through reverse index
                    for (std::multimap<IndexReference,Pattern>::iterator iter4 = this->reverseindex.lower_bound(gapref); iter4 != this->reverseindex.upper_bound(gapref); iter4++) {
                        const Pattern candidate = iter4->second;
                        if (requiredsize == (int) candidate.n()) {
                            skipcontent.insert(candidate);
                        }
                    }

                }
            }
        }
        return skipcontent;
    }

    std::unordered_set<Pattern> getsubsumed(const Pattern & pattern) {
        //TODO: implement
    }

    std::unordered_set<Pattern> getsubsumedby(const Pattern & pattern) {
        //TODO: implement
    }


    int pruneskipgrams(int threshold, int minskiptypes, int minskiptokens, int _n = 0) {
        int pruned = 0;
        if ((minskiptypes <=1)  && (minskiptokens <= threshold)) return pruned; //nothing to do

        typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); 
        while(iter != this->end()) { 
            const Pattern pattern = iter->first;
            if (( (_n == 0) || ((int) pattern.n() == _n) ) && (pattern.category() == FIXEDSKIPGRAM)) {
                std::unordered_set<Pattern> skipcontent = getskipcontent(pattern);
                if ((int) skipcontent.size() < minskiptypes) {
                    iter = this->erase(iter);
                    pruned++;
                    continue;
                }

                std::set<IndexReference> occurrences;
                for (std::unordered_set<Pattern>::iterator iter2 = skipcontent.begin(); iter2 != skipcontent.end(); iter2++) {
                    const Pattern contentpattern = *iter2;
                    IndexedData * data = getdata(contentpattern);
                    for (IndexedData::iterator iter3 = data->begin(); iter3 != data->end(); iter3++) {                    
                        const IndexReference ref = *iter3;
                        occurrences.insert(ref);
                    }
                }                
                const int s = occurrences.size();
                if ((s < minskiptokens) || (s < threshold)) {
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

    void report(std::ostream * OUT) {
        *OUT << std::setiosflags(std::ios::fixed) << std::setprecision(4) << std::endl;       
        *OUT << "REPORT" << std::endl;
        *OUT << "----------------------------------" << std::endl;
        *OUT << "                          " << std::setw(10) << "PATTERNS" << std::setw(10) << "TOKENS" << std::setw(10) << "TYPES" << std::setw(10) << std::endl;
        *OUT << "Total:                    " << std::setw(10) << this->size() << std::setw(10) << this->tokens() << std::setw(10) << this->types() <<  std::endl;
        std::set<IndexReference> coverage;
        int coveredtypes = 0;
        for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
            const Pattern pattern = iter->first;
            const int _n = pattern.n();
            IndexedData * data = getdata(pattern);
            for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {
                const IndexReference ref = *iter2;
                for (int i = 0; i < _n; i++) coverage.insert(ref+i);
            }
            if (_n == 1) coveredtypes++;
        }
        int coveredtokens = coverage.size();
        *OUT << "Uncovered:                " << std::setw(10) << 0 << std::setw(10) << this->tokens() - coveredtokens << std::setw(10) << this->types() - coveredtypes <<  std::endl;
        *OUT << "Covered:                  " << std::setw(10) << this->size() << std::setw(10) << coveredtokens << std::setw(10) << coveredtypes <<  std::endl;
        
        
        *OUT << "Per n:" << std::endl;
        std::set<int> sizes;
        std::set<int> cats;
        int maxn = 1;
        int n = 1;
        do {
            coverage.clear();
            int coveredtypes_n = 0;
            int totalpatterns_n = 0;
            std::set<Pattern> types_n;
            if ((n == 1) || (sizes.count(n))) { 
                for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
                    const Pattern pattern = iter->first;
                    const int _n = pattern.n();
                    if (n == 1) {
                        sizes.insert(pattern.size());
                        cats.insert(pattern.category());
                    }
                    if (_n == n) {
                        IndexedData * data = getdata(pattern);
                        //std::cerr << data->count() << std::endl;
                        for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {
                            const IndexReference ref = *iter2;
                            for (int i = 0; i < _n; i++) {
                                const IndexReference ref2 = ref + i;
                                coverage.insert(ref2);
                            }
                        }
                        if (_n == 1) {  
                            coveredtypes_n++;
                        } else {
                            std::vector<Pattern> unigrams;
                            pattern.ngrams(unigrams, 1);
                            for (std::vector<Pattern>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                                const Pattern p = *iter2;
                                types_n.insert(p);
                            }
                        }

                        totalpatterns_n++;
                    }
                }
                if (n > 1) coveredtypes_n = types_n.size();
                *OUT << " n=" << n << "                      " << std::setw(10) << totalpatterns_n << std::setw(10) << coverage.size() << std::setw(10) << coveredtypes_n << std::endl; 
                if (n == 1) {
                    std::set<int>::reverse_iterator iter = sizes.rbegin();
                    maxn = *iter; //get last item from sizes
                }
            }
            n++;
        } while (n < maxn);


        std::set<Pattern> types_c;

        *OUT << "Per category:" << std::endl;
        for (int cat = 1; cat <= 3; cat++) {
            if (cats.count(cat)) {
                int totalpatterns_c = 0;
                int coveredtypes_c = 0;
                coverage.clear();
                for (typename PatternModel<IndexedData,IndexedDataHandler,MapType>::iterator iter = this->begin(); iter != this->end(); iter++) {
                    const Pattern pattern = iter->first;
                    const int _n = pattern.n();
                    if (pattern.category() == cat) {
                        IndexedData * data = getdata(pattern);
                        for (IndexedData::iterator iter2 = data->begin(); iter2 != data->end(); iter2++) {
                            const IndexReference ref = *iter2;
                            for (int i = 0; i < _n; i++) coverage.insert(ref+i);
                        }
                        if (_n == 1) {  
                            coveredtypes_c++;
                        } else {
                            std::vector<Pattern> unigrams;
                            pattern.ngrams(unigrams, 1);
                            for (std::vector<Pattern>::iterator iter2 = unigrams.begin(); iter2 != unigrams.end(); iter2++) {
                                const Pattern p = *iter2;
                                types_c.insert(p);
                            }
                        }

                        totalpatterns_c++;
                    }
                }
                if (cat == 1) {
                    *OUT << " N-GRAMS                  ";
                } else if (cat == 2) {
                    *OUT << " FIXED-SKIPGRAMS          ";
                } else if (cat == 3) {
                    *OUT << " DYNAMIC-SKIPGRAMS        ";
                }
                *OUT << std::setw(10) << totalpatterns_c << std::setw(10) << coverage.size() << std::setw(10) << coveredtypes_c << std::endl; 
            }

        }
    }
};



/*


class GraphFilter {
   public:
    bool DOPARENTS;
    bool DOCHILDREN;
    bool DOXCOUNT;
    bool DOTEMPLATES;
    bool DOINSTANCES;
    bool DOSKIPUSAGE;
    bool DOSKIPCONTENT;
    bool DOSUCCESSORS;
    bool DOPREDECESSORS;
    bool DOCOOCCURRENCE;  
    CoocStyle COOCSTYLE; 
  
  GraphFilter() {
    DOPARENTS = false;
    DOCHILDREN = false;
    DOXCOUNT = false;
    DOTEMPLATES = false;
    DOINSTANCES = false; 
    DOSKIPUSAGE = false;
    DOSKIPCONTENT = false;
    DOSUCCESSORS = false;
    DOPREDECESSORS = false;
    DOCOOCCURRENCE = false;
    CoocStyle COOCSTYLE = COOCSTYLE_COUNT;
  }
};    
*/

        






/*


class ModelReader {
   protected:
    
    uint64_t totaltypes;
    bool DEBUG; 
    
    int FOUNDMINN; 
    int FOUNDMAXN; 
   public:
    uint64_t totaltokens; //INCLUDES TOKENS NOT COVERED BY THE MODEL!
   
    uint64_t model_id;
    virtual uint64_t id() =0;
    
    virtual void readheader(std::istream * in, bool ignore=false) =0;
    virtual void readfooter(std::istream * in, bool ignore=false) =0;    
    virtual void readdata(std::istream * in); 

    virtual void readfile(const std::string & filename, const bool DEBUG=false);
    
    int getminn() { return FOUNDMINN; };
    int getmaxn() { return FOUNDMAXN; };
};

class ModelWriter {

   public:
    virtual uint64_t id() =0;    
    virtual void writeheader(std::ostream * out) =0;
    virtual void writedata(std:ostream * out)=0;
    virtual void writefooter(std::ostream * out) =0;

    

    virtual uint64_t types() const =0;
    virtual uint64_t tokens() const =0;
    virtual void writefile(const std::string & filename);
};

class ModelQuerierBase {
    std::map<int, std::vector< std::vector< std::pair<int,int> > > > gapconf;
   public:
    ModelQuerierBase();
    virtual const EncAnyGram * getfocuskey(const EncAnyGram * anygram) { return getkey(anygram); }; //without context, defaults to getkey (for models not supporting context). getpatterns uses only this! The extracted patterns will always be without context
    virtual const EncAnyGram * getkey(const EncAnyGram * anygram) =0; //default, includes context if available
    std::vector<std::pair<const EncAnyGram*, CorpusReference> > getpatterns(const unsigned char * data, const unsigned char datasize, bool doskipgrams=true, uint32_t linenum=0, const int minn = 1, const int maxn = MAXN);
};

class ModelQuerier: public ModelQuerierBase {
	public:
	 //ModelQuerier() {} ;
	 virtual int maxlength() const =0;
	 virtual bool exists(const EncAnyGram* key) const =0;
     virtual int occurrencecount(const EncAnyGram* key) =0;
     virtual int coveragecount(const EncAnyGram* key) =0;    
     virtual double coverage(const EncAnyGram* key) =0;	 
	 virtual void outputinstance(const EncAnyGram *, CorpusReference, ClassDecoder &) =0;	 	 	  
	 void querier(ClassEncoder & encoder, ClassDecoder & decoder, bool exact = false,bool repeat = true, const int minn = 1, const int maxn = MAXN);	 	  
};


class IndexedPatternModel: public ModelReader, public ModelWriter, public ModelQuerier {    
   private:
    int MINTOKENS; // = 2;
    int MINSKIPTOKENS; // = 2;
    int MINSKIPTYPES; //= 2;
    int MAXLENGTH; //= 8;
    bool DOSKIPGRAMS; //= false;
    bool DOINITIALONLYSKIP; //= true;
    bool DOFINALONLYSKIP; //= true;


    void computestats(); //compute occurrence count sums
   public:

    //occurence counts
    unsigned long totalngramcount;
    unsigned long totalskipgramcount;     
    unsigned int ngramcount[MAXN]; 
    unsigned int skipgramcount[MAXN];    
    unsigned int ngramtypes[MAXN]; 
    unsigned int skipgramtypes[MAXN];
    
    std::unordered_map<const EncNGram,NGramData > ngrams;
    std::unordered_map<const EncSkipGram,SkipGramData > skipgrams;    
    
    std::unordered_map< int,std::vector<EncNGram> > ngram_reverse_index;
    std::unordered_map< int,std::vector<EncSkipGram> > skipgram_reverse_index;
           
    IndexedPatternModel(const std::string & filename = "", const bool DEBUG=false);
    IndexedPatternModel(const std::string & corpusfile, int MAXLENGTH, int MINTOKENS = 2, bool DOSKIPGRAMS = true, int MINSKIPTOKENS = 2, int MINSKIPTYPES = 2,  bool DOINITIALONLYSKIP= true, bool DOFINALONLYSKIP = true);
    IndexedPatternModel(const std::string & corpusfile, IndexedPatternModel & refmodel, int MAXLENGTH, int MINTOKENS = 2, bool DOSKIPGRAMS = true, int MINSKIPTOKENS = 2, int MINSKIPTYPES = 2,  bool DOINITIALONLYSKIP= true, bool DOFINALONLYSKIP = true);
    
    int maxlength() const { return MAXLENGTH; }
    
    std::vector<unsigned char> sentencesize;
    
    uint64_t types() const { return ngrams.size() + skipgrams.size(); }    
    uint64_t tokens() const { return totaltokens; }
    uint64_t occurrences() const { return totalngramcount + totalskipgramcount; }    
    
    bool exists(const EncAnyGram* key) const;
    const EncAnyGram* getkey(const EncAnyGram* key);
    const AnyGramData* getdata(const EncAnyGram* key);
    
    
    int occurrencecount(const EncAnyGram* key);
    int coveragecount(const EncAnyGram* key);    
    double coverage(const EncAnyGram* key);    
    double pmi(const EncAnyGram *, const EncAnyGram *);
    double npmi(const EncAnyGram *, const EncAnyGram *); 

    
    void outputinstance(const EncAnyGram *, CorpusReference, ClassDecoder &);	 	 

    
    std::set<int> reverse_index_keys(); 
    bool reverse_index_haskey(const int i) const;    
    int reverse_index_size(const int i);
    int reverse_index_size();
    std::vector<EncAnyGram*> reverse_index(const int i);
    EncAnyGram* get_reverse_index_item(const int, const int);
    
    std::set<int> getsentences(const EncAnyGram * anygram);
    std::unordered_map<const EncAnyGram*, int>  getcooccurrences(const EncAnyGram * anygram, IndexedPatternModel * targetmodel = NULL, std::set<int> * sentenceconstraints = NULL);
    
    
    virtual uint64_t id() { return INDEXEDPATTERNMODEL + INDEXEDPATTERNMODELVERSION; }
    virtual void readheader(std::istream * in, bool ignore = false);
    virtual void readngramdata(std::istream * in, const EncNGram & ngram, int ngramversion=1,bool ignore = false);
    virtual void readskipgramdata(std::istream * in, const EncSkipGram & skipgram, int ngramversion=1,bool ignore = false);
    virtual void readfooter(std::istream * in, bool ignore = false);    
    
    virtual void writeheader(std::ostream * out) {};
    virtual void writengrams(std::ostream * out);
    virtual void writengramdata(std::ostream * out, const EncNGram & ngram);
    virtual void writeskipgrams(std::ostream * out);
    virtual void writeskipgramdata(std::ostream * out, const EncSkipGram & skipgram);
    virtual void writefooter(std::ostream * out); 
        
    void writeanygram(const EncAnyGram * anygram, std::ostream * out); //write the anygram itself (not its data!)
        
    void save(const std::string & filename) { ModelWriter::writefile(filename); }
    
    size_t hash();
    
    
    void decode(ClassDecoder & classdecoder, std::ostream *OUT, bool outputhash=false);
    void decode(IndexedPatternModel & testmodel, ClassDecoder & classdecoder, std::ostream *OUT, bool outputhash=false);
    
    bool skipgramvarietycheck(SkipGramData & skipgramdata, int mintypecount=2);
    void coveragereport(std::ostream *OUT, const std::string & corpusfile = "", std::ostream *HTMLOUT = NULL, ClassDecoder * decoder = NULL, int segmentsize = 100000);
    void histogram(std::ostream *OUT);
    void report(std::ostream *OUT);
    //unsigned int prunebyalignment(std::unordered_map<const EncAnyGram*,std::unordered_map<const EncAnyGram*, double> > & alignmatrix, double threshold = 0.0);
};


class UnindexedPatternModel: public ModelReader, public ModelWriter, public ModelQuerier {
   private:
    int MINTOKENS; // = 2;
    int MINSKIPTOKENS; // = 2;
    int MINSKIPTYPES; //= 2;
    int MAXLENGTH; //= 8;
    bool DOSKIPGRAMS; //= false;
    bool DOINITIALONLYSKIP; //= true;
    bool DOFINALONLYSKIP; //= true;


    void computestats(); //compute occurrence count sums
   public:

    //occurence counts
    unsigned long totalngramcount;
    unsigned long totalskipgramcount;     
    unsigned int ngramcount[MAXN]; 
    unsigned int skipgramcount[MAXN];    
    unsigned int ngramtypes[MAXN]; 
    unsigned int skipgramtypes[MAXN];
    
    
    std::unordered_map<const EncNGram,uint32_t > ngrams;
    std::unordered_map<const EncSkipGram,uint32_t > skipgrams;    
            
    UnindexedPatternModel(const std::string & filename, const bool DEBUG=false);
    UnindexedPatternModel(const std::string & corpusfile, int MAXLENGTH, int MINTOKENS = 2, bool DOSKIPGRAMS = true, int MINSKIPTOKENS = 2, bool DOINITIALONLYSKIP= true, bool DOFINALONLYSKIP = true);
    UnindexedPatternModel(const std::string & corpusfile, UnindexedPatternModel & refmodel, int MAXLENGTH, int MINTOKENS = 2, bool DOSKIPGRAMS = true, int MINSKIPTOKENS = 2, int MINSKIPTYPES = 2,  bool DOINITIALONLYSKIP= true, bool DOFINALONLYSKIP = true);
    
        
    int maxlength() const { return MAXLENGTH; }
    
    uint64_t types() const { return ngrams.size() + skipgrams.size(); }
    uint64_t tokens() const { return totaltokens; }
    uint64_t occurrences() const { return totalngramcount + totalskipgramcount; }
    
    bool exists(const EncAnyGram* key) const;
    const EncAnyGram* getkey(const EncAnyGram* key);


    int occurrencecount(const EncAnyGram* key);
    int coveragecount(const EncAnyGram* key);    
    double coverage(const EncAnyGram* key);           
    //std::set<int> * index(const EncAnyGram* key);    
    //int index_size() const;

	void outputinstance(const EncAnyGram *, CorpusReference, ClassDecoder &);
    
    virtual uint64_t id() { return UNINDEXEDPATTERNMODEL + UNINDEXEDPATTERNMODELVERSION; }
    virtual void readheader(std::istream * in,  bool ignore = false) {};
    virtual void readngramdata(std::istream * in, const EncNGram & ngram, int ngramversion=1,bool ignore = false);
    virtual void readskipgramdata(std::istream * in, const EncSkipGram & skipgram, int ngramversion=1,bool ignore = false);
    virtual void readfooter(std::istream * in, bool ignore = false) {};    
    
    virtual void writeheader(std::ostream * out) {};
    virtual void writengrams(std::ostream * out);
    virtual void writengramdata(std::ostream * out, const EncNGram & ngram);
    virtual void writeskipgrams(std::ostream * out);
    virtual void writeskipgramdata(std::ostream * out, const EncSkipGram & skipgram);
    virtual void writefooter(std::ostream * out) {}; 
        
    void save(const std::string & filename) { ModelWriter::writefile(filename); }
    
    size_t hash();
    
    void decode(ClassDecoder & classdecoder, std::ostream *OUT, bool outputhash=false);
    void decode(UnindexedPatternModel & testmodel, ClassDecoder & classdecoder, std::ostream *OUT, bool outputhash=false);
    
    void report(std::ostream *OUT);
    void histogram(std::ostream *OUT);
        
};


//typedef std::unordered_map<EncNGram,set<CorpusReference> > freqlist;
//typedef std::unordered_map<EncSkipGram,set<CorpusReference> > skipgram_freqlist;


int transitivereduction(std::unordered_map<const EncAnyGram*,std::unordered_set<const EncAnyGram*> > & relations );


enum CoocStyle {
       COOCSTYLE_COUNT = 0, //join count
       COOCSTYLE_JACCARD = 1,
       /COOCSTYLE_DICE = 2,
       COOCSTYLE_PMI = 3,
       COOCSTYLE_NPMI = 4
};


class GraphFilter {
   public:
    bool DOPARENTS;
    bool DOCHILDREN;
    bool DOXCOUNT;
    bool DOTEMPLATES;
    bool DOINSTANCES;
    bool DOSKIPUSAGE;
    bool DOSKIPCONTENT;
    bool DOSUCCESSORS;
    bool DOPREDECESSORS;
    bool DOCOOCCURRENCE;  
    CoocStyle COOCSTYLE; 
  
  GraphFilter() {
    DOPARENTS = false;
    DOCHILDREN = false;
    DOXCOUNT = false;
    DOTEMPLATES = false;
    DOINSTANCES = false; 
    DOSKIPUSAGE = false;
    DOSKIPCONTENT = false;
    DOSUCCESSORS = false;
    DOPREDECESSORS = false;
    DOCOOCCURRENCE = false;
    CoocStyle COOCSTYLE = COOCSTYLE_COUNT;
  }
};    



typedef std::unordered_map<const EncAnyGram*,std::unordered_set<const EncAnyGram*> > t_relations;
typedef std::unordered_map<const EncAnyGram*,std::unordered_map<const EncAnyGram*, uint64_t> > t_weightedrelations;

class GraphRelations {
   public:
    bool DOPARENTS;
    bool DOCHILDREN;
    bool DOXCOUNT;
    bool DOTEMPLATES;
    bool DOINSTANCES;
    bool DOSKIPUSAGE;
    bool DOSKIPCONTENT;
    bool DOSUCCESSORS;
    bool DOPREDECESSORS;
    bool DOCOOCCURRENCE;
    CoocStyle COOCSTYLE; //cooc style for outputrelations
    
    bool HASPARENTS;
    bool HASCHILDREN;
    bool HASXCOUNT;
    bool HASTEMPLATES;
    bool HASINSTANCES;
    bool HASSKIPUSAGE;
    bool HASSKIPCONTENT;
    bool HASSUCCESSORS;
    bool HASPREDECESSORS;
    bool HASCOOCCURRENCE;
    
    bool TRANSITIVE;
    bool secondpass;          
  
    t_relations rel_subsumption_parents;
    t_relations rel_subsumption_children;        
    std::unordered_map<const EncAnyGram*,int> data_xcount;        
   
    virtual uint64_t id() =0;
    
    t_relations rel_templates; //instance -> skipgram
    t_relations rel_instances; //skipgram -> instance
    
    t_weightedrelations  rel_skipusage; //skipcontent -> skipgram           
    t_weightedrelations  rel_skipcontent; //skipgram -> skipcontent       
    
    t_weightedrelations  rel_successors;  
    t_weightedrelations  rel_predecessors;
    
    t_weightedrelations  rel_cooccurences;    
    
    void readrelations(std::istream * in,const EncAnyGram* = NULL, t_relations * = NULL, int ngramversion=1,bool ignore = false);
    void readweightedrelations(std::istream * in,const EncAnyGram* = NULL, t_weightedrelations * = NULL, int ngramversion=1,bool ignore = false);
    
    void getrelations(t_relations & relations, const EncAnyGram * anygram, std::unordered_set<const EncAnyGram*> & container);
    void getrelations(t_weightedrelations & relations, const EncAnyGram * anygram, std::unordered_set<const EncAnyGram*> & container);
    
    int transitivereduction();
    
    bool has_xcount() { return (HASXCOUNT); }
    bool has_parents() { return (HASPARENTS) ; }
    bool has_children() { return (HASCHILDREN) ; }
    bool has_templates() { return (HASTEMPLATES) ; }
    bool has_instances() { return (HASINSTANCES) ; }
    bool has_skipusage() { return (HASSKIPUSAGE) ; }
    bool has_skipcontent() { return (HASSKIPCONTENT) ; }
    bool has_successors() { return (HASSUCCESSORS) ; }
    bool has_predecessors() { return (HASPREDECESSORS) ; }
    bool has_cooccurrence() { return (HASCOOCCURRENCE) ; }

  void applyfilter(const GraphFilter & model) {
    DOPARENTS = model.DOPARENTS;
    DOCHILDREN = model.DOCHILDREN;
    DOXCOUNT = model.DOXCOUNT;
    DOTEMPLATES = model.DOTEMPLATES;
    DOINSTANCES = model.DOINSTANCES;
    DOSKIPUSAGE = model.DOSKIPUSAGE;
    DOSKIPCONTENT = model.DOSKIPCONTENT;
    DOPREDECESSORS = model.DOPREDECESSORS;
    DOSUCCESSORS = model.DOSUCCESSORS;
    DOCOOCCURRENCE = model.DOCOOCCURRENCE;
    COOCSTYLE = model.COOCSTYLE;  
  }
    
   virtual const EncAnyGram* getkey(const EncAnyGram* key) =0;
   
   
};

class GraphPatternModel: public ModelReader, public ModelWriter, public GraphRelations {               
   protected:

    
    bool DELETEMODEL;
    
    void writerelations(std::ostream * out, const EncAnyGram*, t_relations & );
    void writerelations(std::ostream * out, const EncAnyGram*, t_weightedrelations & );
   public:
   
    IndexedPatternModel * model;

    
    uint64_t types() const { return model->types(); }
    uint64_t tokens() const { return model->tokens(); }
    uint64_t occurrences() const { return model->occurrences(); }
    

   
    GraphPatternModel(IndexedPatternModel * model, const GraphFilter & filter); //compute entire model
    GraphPatternModel(const std::string & graphmodelfilename, IndexedPatternModel * model, const GraphFilter & filter, const bool DEBUG=false) {
        //do everything (provided that it exists in file)
        applyfilter(filter);
        model->model_id = GRAPHPATTERNMODEL+GRAPHPATTERNMODELVERSION;
    	DELETEMODEL = false;        
        this->model = model;
        secondpass = true;
    	readfile(graphmodelfilename, DEBUG);        
    }
    GraphPatternModel(const std::string & graphmodelfilename, const GraphFilter & filter, const bool DEBUG=false) {
        //do everything (provided that it exists in file)
        applyfilter(filter);    	
    	DELETEMODEL = true;
    	model = new IndexedPatternModel();
    	model->model_id = GRAPHPATTERNMODEL+GRAPHPATTERNMODELVERSION;
    	std::cerr << "Pass one, reading implied indexedpatternmodel..." << std::endl;
    	//reading is done in two passes
    	secondpass = false;
    	readfile(graphmodelfilename, DEBUG);
    	model->totaltokens = totaltokens;    
        std::cerr << "Pass two, reading graph data..." << std::endl;
        secondpass = true;
        readfile(graphmodelfilename, DEBUG);
    }    
        
    
    
    ~GraphPatternModel();
    
    int transitivereduction();
    
    int computexcount(const EncAnyGram* anygram); //exclusive count    
        
    void save(const std::string & filename) { writefile(filename); }
    
    virtual uint64_t id() { return GRAPHPATTERNMODEL + GRAPHPATTERNMODELVERSION; }
        
    
    const EncAnyGram* getkey(const EncAnyGram* key) { return model->getkey(key); }
    
    virtual void readheader(std::istream * in, bool ignore = false);
    virtual void readngramdata(std::istream * in, const EncNGram & ngram, int ngramversion=1,bool ignore = false);
    virtual void readskipgramdata(std::istream * in, const EncSkipGram & skipgram, int ngramversion=1,bool ignore = false);
    virtual void readfooter(std::istream * in, bool ignore = false) {};    
    
    virtual void writeheader(std::ostream * out);
    virtual void writengrams(std::ostream * out);
    virtual void writengramdata(std::ostream * out, const EncNGram & ngram);
    virtual void writeskipgramdata(std::ostream * out, const EncSkipGram & skipgram);
    virtual void writeskipgrams(std::ostream * out);
    virtual void writefooter(std::ostream * out) {};    
    
    void decode(ClassDecoder & classdecoder, std::ostream *OUT, bool outputrelations = false);
    void stats(std::ostream *OUT);
    void coveragereport(std::ostream *OUT, int segmentsize = 100000, double xratiothreshold = 0.8);
    
    void outputgraph(ClassDecoder & classdecoder, std::ostream *OUT);
    void outputgraph(ClassDecoder & classdecoder, std::ostream *OUT, const EncAnyGram *);
    void outputgraphvizrelations( const EncAnyGram * anygram, std::ostream *OUT, t_relations & relationhash, const std::string & colour);
    void outputgraphvizrelations( const EncAnyGram * anygram, std::ostream *OUT, t_weightedrelations & relationhash, const std::string & colour);
    void outputgraphvizrelations( const std::unordered_set<const EncAnyGram *> &, std::ostream *OUT, t_relations & relationhash, const std::string & colour);
    void outputgraphvizrelations( const std::unordered_set<const EncAnyGram *> &, std::ostream *OUT, t_weightedrelations & relationhash, const std::string & colour);
    //void outputgraphvizrelations( const EncAnyGram * anygram, t_weightedrelations & relationhash, const std::string & colour);
    //void outputgraphvizrelations( const std::unordered_set<const EncAnyGram *> &, std::ostream *OUT, t_weightedrelations & relationhash, const std::string & colour);        
    
  
  
    void outputrelations(ClassDecoder & classdecoder, std::ostream *OUT, const EncAnyGram * focusinput, bool outputquery=false);
    void outputrelations(ClassDecoder & classdecoder, std::ostream *OUT, std::unordered_set<const EncAnyGram*>   & relations );
    void outputrelations(ClassDecoder & classdecoder, std::ostream *OUT, std::unordered_map<const EncAnyGram*,uint64_t>   & relations ); //weighted
    void outputcoocrelations(const EncAnyGram * pivot, ClassDecoder & classdecoder, std::ostream *OUT, std::unordered_map<const EncAnyGram*,uint64_t>   & relations ); //weighted

    void outputcoverage(ClassDecoder & classdecoder, std::ostream *OUT);
    
    void findincomingnodes(const EncAnyGram * focus, std::unordered_set<const EncAnyGram *> & relatednodes);
    void findincomingnodes(const EncAnyGram * focus, const EncAnyGram * anygram, std::unordered_set<const EncAnyGram *> & relatednodes, t_relations  & relationhash );
    void findincomingnodes(const EncAnyGram * focus, const EncAnyGram * anygram, std::unordered_set<const EncAnyGram *> & relatednodes, t_weightedrelations  & relationhash );
    
    double pmi(const EncAnyGram * key1, const EncAnyGram * key2);
    double npmi(const EncAnyGram * key1, const EncAnyGram * key2);
};


class AlignConstraintInterface {
	public: 
	  virtual const EncAnyGram * getsourcekey(const EncAnyGram* key, bool allowfallback=true) =0;
      virtual const EncAnyGram * gettargetkey(const EncAnyGram* key, bool returnselfifnotfound=false, bool forcemodel=false) =0;
};

class IndexCountData {
	public:
	 uint32_t count;
	 uint32_t xcount;
	 std::multiset<uint32_t> sentences; //may occur multiple times in same sentence 
};


class SelectivePatternModel: public ModelReader, public ModelQuerier, public GraphRelations {
    // Read only model, reads graphpatternmodel/indexedmodel/unindexedmodel in a simplified, selective, less memory intensive representation. For for example alignment tasks, supports double indexes if fed an indexed model, and exclusive counts if fed a graph model. Whilst offering more functionality, it is also limited in the sense that it does not offer the full representation the complete model does.
    private:
     bool DOFORWARDINDEX;
     bool DOREVERSEINDEX;

     
     
     
     int COUNTTHRESHOLD;
     double FREQTHRESHOLD;
     double XCOUNTTHRESHOLD;
     double XCOUNTRATIOTHRESHOLD;
     
     int MINLENGTH;
     int MAXLENGTH;
     bool DOSKIPGRAMS;
    
     int ngramtypecount;
     int skipgramtypecount;   
     
    
     AlignConstraintInterface * alignconstrain;
     bool alignconstrainsource;
     
     //void readrelations(std::istream * in, const EncAnyGram * anygram = NULL, std::unordered_map<const EncAnyGram*,std::unordered_set<const EncAnyGram*> > * relationhash = NULL, bool ignore = false);
    
     void computestats(); //compute occurrence count sums
   public:

    //occurence countspatt
    unsigned long totalngramcount;
    unsigned long totalskipgramcount;     
    unsigned int ngramcount[MAXN]; 
    unsigned int skipgramcount[MAXN];    
    unsigned int ngramtypes[MAXN]; 
    unsigned int skipgramtypes[MAXN];
    
      
     unsigned long ignoredtypes;
     unsigned long ignoredoccurrences;
     std::unordered_map<const EncNGram, IndexCountData> ngrams;
     std::unordered_map<const EncSkipGram,IndexCountData> skipgrams;
     
              
    
     std::unordered_map<uint32_t,std::unordered_set<const EncAnyGram*> > reverseindex; 

     SelectivePatternModel(const std::string & filename, const GraphFilter & filter, bool DOFORWARDINDEX = true, bool DOREVERSEINDEX = true, int COUNTTHRESHOLD = 0, double FREQTHRESHOLD = 0, double XCOUNTRATIOTHRESHOLD = 0, int XCOUNTTHRESHOLD = 0, bool DOSKIPGRAMS = true,  int MINLENGTH = 0, int MAXLENGTH=99, AlignConstraintInterface * alignconstrain = NULL, bool alignconstrainsource = true , const bool DEBUG=false); //read a graph pattern model
  
     uint64_t types() const { return ngrams.size() + skipgrams.size(); }
     uint64_t tokens() const { return totaltokens; }
     
    virtual uint64_t id() { return model_id; }
    
    int maxlength() const { return MAXLENGTH; }
    const EncAnyGram* getkey(const EncAnyGram* key);
    IndexCountData getdata(const EncAnyGram* key);
    
    bool exists(const EncAnyGram* key) const;
    int occurrencecount(const EncAnyGram* key);
    int coveragecount(const EncAnyGram* key);    
    double coverage(const EncAnyGram* key);
        
    int xcount(const EncAnyGram* key);
    double xcountratio(const EncAnyGram* key);
    
    int countforsentence(const EncAnyGram* key, const uint64_t sentence);
    
	void outputinstance(const EncAnyGram *, CorpusReference, ClassDecoder &);    
    
    
    
    int transitivereduction();
    
    bool has_xcount() { return (HASXCOUNT); }
    bool has_index() { return ((model_id != UNINDEXEDPATTERNMODEL)) ; }
    bool has_parents() { return (HASPARENTS) ; }
    bool has_children() { return (HASCHILDREN) ; }
    
    virtual void readheader(std::istream * in, bool ignore = false);
    virtual void readngramdata(std::istream * in, const EncNGram & ngram, int ngramversion=1,bool ignore = false);
    virtual void readskipgramdata(std::istream * in, const EncSkipGram & skipgram,int ngramversion=1,bool ignore = false);
    virtual void readfooter(std::istream * in, bool ignore = false) {};
    
    std::set<int> getsentences(const EncAnyGram * anygram);
    std::unordered_map<const EncAnyGram*, int>  getcooccurrences(const EncAnyGram * anygram, SelectivePatternModel * targetmodel = NULL, std::set<int> * sentenceconstraints = NULL);    
};




void readcorpus( const std::string & corpusfile, std::unordered_map<CorpusReference, const EncNGram *> & tokens);
void replaceAll(std::string& str, const std::string& from, const std::string& to);

namespace std {
    template <>
    struct hash<CorpusReference> {
     public: 
            size_t operator()(CorpusReference ref) const throw() {            
                return (ref.sentence * 1000) + ref.token;
            }
    };    
}

*/
#endif
