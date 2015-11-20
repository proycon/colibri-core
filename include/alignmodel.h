#ifndef ALIGNMODEL_H
#define ALIGNMODEL_H

#include "patternmodel.h"


template<class FeatureType>
class PatternAlignmentModel: public PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>, public PatternModelInterface {
    protected:
        //some duplication from PatternModel, but didn't want to inherit from
        //it, as too much is different
        unsigned char model_type;
        unsigned char model_version;
        uint64_t totaltokens; //INCLUDES TOKENS NOT COVERED BY THE MODEL!
        uint64_t totaltypes; //TOTAL UNIGRAM TYPES, INCLUDING NOT COVERED BY THE MODEL!

        int maxn; 
        int minn; 
        

        virtual void postread(const PatternModelOptions options) {
            for (iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern p = iter->first;
                const int n = p.n();
                if (n > maxn) maxn = n;
                if (n < minn) minn = n;
            }
        }
    public:
        PatternAlignmentModel<FeatureType>() {
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
        }
        PatternAlignmentModel<FeatureType>(std::istream *f, PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) { //load from file
            totaltokens = 0;
            totaltypes = 0;
            maxn = 0;
            minn = 999;
            model_type = this->getmodeltype();
            model_version = this->getmodelversion();
            this->load(f,options,constrainmodel);
        }

        PatternAlignmentModel<FeatureType>(const std::string filename, const PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) { //load from file
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

        virtual int getmodeltype() const { return PATTERNALIGNMENTMODEL; }
        virtual int getmodelversion() const { return 2; }

        virtual size_t size() const {
            return PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>::size();
        }
        virtual bool has(const Pattern & pattern) const {
            return PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>::has(pattern);
        }
        virtual bool has(const PatternPointer & pattern) const {
            return PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>::has(pattern);
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

        virtual void load(std::istream * f, PatternModelOptions options, PatternModelInterface * constrainmodel = NULL) { //load from file
            options.MINTOKENS = 1; //other values would be meaningless

            char null;
            f->read( (char*) &null, sizeof(char));        
            f->read( (char*) &model_type, sizeof(char));        
            f->read( (char*) &model_version, sizeof(char));  
            if (model_version == 1) this->classencodingversion = 1;
            if ((null != 0) || (model_type != PATTERNALIGNMENTMODEL ))  {
                std::cerr << "File is not a colibri alignment model file (did you try to load a different type of pattern model?)" << std::endl;
                throw InternalError();
            }
            if (model_version > 2) {
                std::cerr << "WARNING: Model is created with a newer version of Colibri Core! Attempting to continue but failure is likely..." << std::endl;
            }
            f->read( (char*) &totaltokens, sizeof(uint64_t));        
            f->read( (char*) &totaltypes, sizeof(uint64_t)); 

            if (options.DEBUG) { 
                std::cerr << "Debug enabled, loading Alignment Model type " << (int) model_type << ", version " << (int) model_version << std::endl;   
                std::cerr << "Total tokens: " << totaltokens << ", total types: " << totaltypes << std::endl;;   
            }

            PatternStoreInterface * constrainstore = NULL;
            if (constrainmodel) constrainstore = constrainmodel->getstoreinterface();

            PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>::template read(f, options.MINTOKENS,options.MINLENGTH, options.MAXLENGTH, constrainstore, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET, options.DEBUG);  
            if (options.DEBUG) std::cerr << "Read " << this->size() << " patterns" << std::endl;
            this->postread(options);
        }

        PatternModelInterface * getinterface() {
            return (PatternModelInterface*) this;
        }

        void write(std::ostream * out) {
            const char null = 0;
            out->write( (char*) &null, sizeof(char));       
            unsigned char t = this->getmodeltype();
            out->write( (char*) &t, sizeof(char));        
            unsigned char v = this->getmodelversion();
            out->write( (char*) &v, sizeof(char));        
            out->write( (char*) &totaltokens, sizeof(uint64_t));        
            out->write( (char*) &totaltypes, sizeof(uint64_t)); 
            PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>::write(out); //write PatternStore 
        }

        void write(const std::string filename) {
            std::ofstream * out = new std::ofstream(filename.c_str());
            this->write(out);
            out->close();
            delete out;
        }

        typedef typename PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>::iterator iterator;
        typedef typename PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>::const_iterator const_iterator;        


        virtual int maxlength() const { return maxn; };
        virtual int minlength() const { return minn; };

        virtual unsigned int occurrencecount(const Pattern & pattern)  { 
            return 0; // we don't do occurrence counts 
        }
        virtual double frequency(const Pattern & pattern)  { 
            return 0; // we don't do frequency
        }

        
        virtual PatternFeatureVectorMap<FeatureType> * getdata(const Pattern & pattern, bool makeifnew=false) { 
            typename PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>::iterator iter = this->find(pattern);
            if (iter != this->end()) {
                return &(iter->second); 
            } else if (makeifnew) {
                return &((*this)[pattern]);
            } else {
                return NULL;
            }
        }
        
        virtual PatternFeatureVectorMap<FeatureType> * getdata(const PatternPointer & patternpointer, bool makeifnew=false) { 
            const Pattern pattern = Pattern(patternpointer);
            typename PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorMapHandler<FeatureType>>::iterator iter = this->find(pattern);
            if (iter != this->end()) {
                return &(iter->second); 
            } else if (makeifnew) {
                return &((*this)[pattern]);
            } else {
                return NULL;
            }
        }

        //not really useful in this context, but required by the interface
        virtual unsigned int types() { return totaltypes; }
        virtual unsigned int tokens() const { return totaltokens; }


        //(source,target) pair versions of has, getdata
        virtual bool has(const Pattern & pattern, const Pattern & pattern2) {
            return (this->has(pattern) && this->getdata(pattern)->has(pattern2));
        }
        virtual bool has(const PatternPointer & patternpointer, const PatternPointer & patternpointer2) {
            const Pattern pattern2 = Pattern(patternpointer2);
            return (this->has(patternpointer) && this->getdata(patternpointer)->has(pattern2));
        }

        virtual PatternFeatureVector<FeatureType> * getdata(const Pattern & pattern, const Pattern & pattern2, bool makeifnew=false) { 
            PatternFeatureVectorMap<FeatureType> * fvmap = this->getdata(pattern, makeifnew);
            if (fvmap == NULL) return NULL;
            return fvmap->getdata(pattern2);
        }
       
        void add(const Pattern & pattern, const Pattern & pattern2, std::vector<FeatureType> & features, bool checkifexists= true) {
            PatternFeatureVector<FeatureType> * fv = NULL;
            if (checkifexists) {
                fv = getdata(pattern,pattern2,true);
            }
            if (fv == NULL) {
                PatternFeatureVectorMap<FeatureType> * fvm = this->getdata(pattern, true);
                PatternFeatureVector<FeatureType> * pfv = new PatternFeatureVector<FeatureType>(pattern2, features);
                fvm->insert(pfv, checkifexists); //(will be freed again by fvm destructor)
            } else {
                fv->clear(); //will be overwritten by new features 
                for (typename std::vector<FeatureType>::iterator iter = features.begin(); iter != features.end(); iter++) {
                    fv->push_back(*iter);
                }
            }
        }

        virtual void printmodel(std::ostream * out, ClassDecoder & sourcedecoder, ClassDecoder & targetdecoder) { //alias for cython (doesn't like methods named print)
            print(out,sourcedecoder, targetdecoder);
        }

        virtual void print(std::ostream * out, ClassDecoder & sourcedecoder, ClassDecoder & targetdecoder) {
            *out << "PATTERN\tPATTERN2\tFEATURES" << std::endl;
            for (iterator iter = this->begin(); iter != this->end(); iter++) {
                const Pattern sourcepattern = iter->first;
                for (typename PatternFeatureVectorMap<FeatureType>::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
                    PatternFeatureVector<FeatureType> * pfv = *iter2;
                    const Pattern targetpattern = pfv->pattern;
                    *out << sourcepattern.tostring(sourcedecoder) << "\t" << targetpattern.tostring(targetdecoder);
                    for (typename std::vector<FeatureType>::iterator iter3 = pfv->data.begin(); iter3 != pfv->data.end(); iter3++) {
                        *out << "\t" << *iter3;
                    }
                    *out << std::endl;
                }
            }
        }
};


#endif
