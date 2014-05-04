#ifndef ALIGNMODEL_H
#define ALIGNMODEL_H

#include "patternmodel.h"


template<class FeatureType>
class PatternAlignmentModel: public PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>> {
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
        virtual int getmodelversion() const { return 1; }

        virtual size_t size() const {
            return PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>>::size();
        }
        virtual bool has(const Pattern & pattern) const {
            return PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>>::has(pattern);
        }
        virtual bool has(const PatternPointer & pattern) const {
            return PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>>::has(pattern);
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
            if ((null != 0) || (model_type != PATTERNALIGNMENTMODEL ))  {
                std::cerr << "File is not a colibri alignment model file (did you try to load a different type of pattern model?)" << std::endl;
                throw InternalError();
            }
            f->read( (char*) &totaltokens, sizeof(uint64_t));        
            f->read( (char*) &totaltypes, sizeof(uint64_t)); 

            if (options.DEBUG) { 
                std::cerr << "Debug enabled, loading Alignment Model type " << (int) model_type << ", version " << (int) model_version << std::endl;   
                std::cerr << "Total tokens: " << totaltokens << ", total types: " << totaltypes << std::endl;;   
            }

            PatternStoreInterface * constrainstore = NULL;
            if (constrainmodel) constrainstore = constrainmodel->getstoreinterface();

            PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>>::template read(f, options.MINTOKENS,options.MINLENGTH, options.MAXLENGTH, constrainstore, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS, options.DORESET, options.DEBUG);  
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
            PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>>::write(out); //write PatternStore 
        }

        void write(const std::string filename) {
            std::ofstream * out = new std::ofstream(filename.c_str());
            this->write(out);
            out->close();
            delete out;
        }

        typedef typename PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>>::iterator iterator;
        typedef typename PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>>::const_iterator const_iterator;        


        virtual int maxlength() const { return maxn; };
        virtual int minlength() const { return minn; };

        virtual int occurrencecount(const Pattern & pattern)  { 
            return 0; // we don't do occurrence counts 
        }
        
        virtual PatternFeatureVectorMap<FeatureType> * getdata(const Pattern & pattern, bool makeifnew=false) { 
            typename PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>>::iterator iter = this->find(pattern);
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
            typename PatternMap<PatternFeatureVectorMap<FeatureType>,PatternFeatureVectorHandler<FeatureType>>::iterator iter = this->find(pattern);
            if (iter != this->end()) {
                return &(iter->second); 
            } else if (makeifnew) {
                return &((*this)[pattern]);
            } else {
                return NULL;
            }
        }

        //not really useful in this context
        int types() const { return totaltypes; }
        int tokens() const { return totaltokens; }

        unsigned char type() const { return model_type; }
        unsigned char version() const { return model_version; }


        //(source,target) pair versions of has, getdata
        virtual bool has(const Pattern & pattern, const Pattern & pattern2) {
            return (this->has(pattern) && this->getdata(pattern)->has(pattern2));
        }
        virtual bool has(const PatternPointer & patternpointer, const PatternPointer & patternpointer2) {
            const Pattern pattern2 = Pattern(patternpointer2);
            return (this->has(patternpointer) && this->getdata(patternpointer)->has(pattern2));
        }

        virtual PatternFeatureVector<FeatureType> * getdata(const Pattern & pattern, const Pattern & pattern2, bool makeifnew=false) { 
            PatternFeatureVectorMap<FeatureType> * data = this->getdata(pattern, makeifnew);
            if (data == NULL) return NULL;
            return data->getdata(pattern2, makeifnew);
        }
       
        void add(const Pattern & pattern, const Pattern &pattern2, std::vector<FeatureType> & features) {
            PatternFeatureVector<FeatureType> * d = getdata(pattern,pattern2,true);
            d->clear();
            for (typename std::vector<FeatureType>::iterator iter = features.begin(); iter != features.end(); iter++) {
                d->push_back(*iter);
            }
        }

};


#endif
