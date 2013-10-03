#include "patternmodel.h"
#include "algorithms.h"
#include <limits>
#include <sstream>

using namespace std;


template<class ValueType, class ValueHandler, class MapType> 
PatternModel<ValueType,ValueHandler,MapType>::PatternModel(istream * f, const PatternModelOptions options) {
    char null;
    f->read( (char*) &null, sizeof(char));        
    f->read( (char*) &model_type, sizeof(char));        
    f->read( (char*) &model_version, sizeof(char));        
    if ((null != 0) || ((model_type != UNINDEXEDPATTERNMODEL) && (model_type != INDEXEDPATTERNMODEL) && (model_type != GRAPHPATTERNMODEL)))  {
        cerr << "File is not a colibri model file (or a very old one)" << endl;
        throw InternalError();
    }
    if (model_type == GRAPHPATTERNMODEL) {
        cerr << "Model is a graph model, can not be loaded as pattern model" << endl;
        throw InternalError();
    }
    f->read( (char*) &totaltokens, sizeof(uint64_t));        
    f->read( (char*) &totaltypes, sizeof(uint64_t)); 

    this->read(f); //read PatternStore
    postread(options);
}


template<class ValueType, class ValueHandler, class MapType> 
void PatternModel<ValueType,ValueHandler,MapType>::write(ostream * out) {
    const char null = 0;
    out->write( (char*) &null, sizeof(char));        
    out->write( (char*) &model_type, sizeof(char));        
    out->write( (char*) &model_version, sizeof(char));        
    out->write( (char*) &totaltokens, sizeof(uint64_t));        
    out->write( (char*) &totaltypes, sizeof(uint64_t)); 
    write(out); //write PatternStore
}


template<class ValueType, class ValueHandler, class MapType> 
void PatternModel<ValueType,ValueHandler,MapType>::postread(const PatternModelOptions options) {
    //this function has a specialisation specific to indexed pattern models,
    //this is the generic version
    for (iterator iter = this->begin(); iter != this->end(); iter++) {
        const Pattern p = iter->first;
        const int n = p.n();
        if (n > maxn) maxn = n;
        if (n < minn) minn = n;
    }
}



template<class ValueType, class ValueHandler, class MapType> 
void PatternModel<ValueType,ValueHandler,MapType>::train(std::istream * in, const PatternModelOptions options) {
    uint32_t sentence = 0;
    const int BUFFERSIZE = 65536;
    unsigned char line[BUFFERSIZE];
    std::map<int, std::vector< std::vector< std::pair<int,int> > > > gapconf;

    for (int n = 1; n <= options.MAXLENGTH; n++) {
        in->seekg(0);
        cerr << "Counting " << n << "-grams" << endl; 
        sentence++;

        if ((options.DOFIXEDSKIPGRAMS) && (gapconf[n].empty())) compute_multi_skips(gapconf[n], vector<pair<int,int> >(), n);

        Pattern line = Pattern(in);
        vector<pair<Pattern,int>> ngrams;
        line.ngrams(ngrams, n);


        for (vector<pair<Pattern,int>>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
            const Pattern pattern = iter->first;
            if (pattern.category() == NGRAM) {
                const IndexReference ref = IndexReference(sentence, iter->second);
                bool found = true;
                if (n > 1) {
                    //check if sub-parts were counted
                    vector<Pattern> subngrams;
                    pattern.ngrams(subngrams,n-1);
                    for (vector<Pattern>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {
                        const Pattern subpattern = *iter2;
                        if ((subpattern.category() == NGRAM) && (!this->has(subpattern))) {
                            found = false;
                            break;
                        }
                    }
                }
                if (found) {
                    add(pattern, &((*this)[pattern]), ref );
                    if (options.DOREVERSEINDEX) {
                        reverseindex.insert(pair<IndexReference,Pattern>(ref,pattern));
                    }
                }                
                if (options.DOFIXEDSKIPGRAMS) {
                    //loop over all possible gap configurations
                    for (vector<vector<pair<int,int>>>::iterator iter =  gapconf[n].begin(); iter != gapconf[n].end(); iter++) {
                        for (vector<pair<int,int>>::iterator iter2 =  iter->begin(); iter2 != iter->end(); iter2++) {
                            vector<pair<int,int>> * gapconfiguration = iter2;
                            //add skips
                            const Pattern skippattern = pattern.addfixedskips(*gapconfiguration);                            


                            //test whether parts occur in model, otherwise skip
                            //can't occur either and we can discard it
                            bool skippattern_valid = true;
                            vector<Pattern> parts;
                            skippattern.parts(parts);
                            for (vector<Pattern>::iterator iter3 = parts.begin(); iter3 != parts.end(); iter3++) {
                                const Pattern part = *iter3;
                                if (!this->has(part)) {
                                    skippattern_valid = false;
                                    break;
                                }
                            }

                            if (skippattern_valid) {
                                add(skippattern, &((*this)[skippattern]), ref );
                                if (options.DOREVERSEINDEX) {
                                    reverseindex.insert(pair<IndexReference,Pattern>(ref,skippattern));
                                }
                            }
                        }
                    }
                }

            }            
        }

        cerr << "Pruning..." << endl;
        prune(options.MINTOKENS,n);
        pruneskipgrams(options.MINTOKENS, options.MINSKIPTYPES, options.MINSKIPTOKENS, n);
    }
}

template<class ValueType, class ValueHandler, class MapType> 
int PatternModel<ValueType,ValueHandler,MapType>::prune(int threshold, int _n) {
    int pruned = 0;
    PatternModel::iterator iter = this->begin(); 
    do {
        const Pattern pattern = iter->first;
        if (( (_n == 0) || (pattern.n() == _n) )&& (occurrencecount(pattern) < threshold)) {
            iter = erase(iter); 
            pruned++;
        } else {
            iter++;
        }
    } while(iter != this->end());       

    if (pruned) prunereverseindex();
    return pruned;
}

template<class ValueType, class ValueHandler, class MapType> 
int PatternModel<ValueType,ValueHandler,MapType>::prunereverseindex(int _n) {
    //prune patterns from reverse index if they don't exist anymore
    int pruned = 0;
    multimap<IndexReference,Pattern>::iterator iter = reverseindex.begin(); 
    do {
        const Pattern pattern = iter->second;
        if (( (_n == 0) || (pattern.n() == _n) ) && (!this->has(pattern))) {
            iter = reverseindex.erase(iter);
            pruned++;
        } else {
            iter++;
        }
    } while (iter != reverseindex.end());
    return pruned;
}


template<class ValueType, class ValueHandler, class MapType> 
int PatternModel<ValueType,ValueHandler,MapType>::pruneskipgrams(int threshold, int minskiptypes, int minskiptokens, int _n ) {
    return 0; //only works for indexed models
}

template<class ValueType, class ValueHandler, class MapType> 
std::vector<std::pair<const Pattern, int> > PatternModel<ValueType,ValueHandler,MapType>::getpatterns(const Pattern & pattern) {
    //get all patterns in pattern
    std::vector<std::pair<const Pattern, int> > v;   
    std::vector<std::pair<const Pattern, int> > ngrams;
    pattern.subngrams(ngrams, minlength(), maxlength());
    for (std::vector<std::pair<const Pattern, int> >::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
        const Pattern p = iter->first;
        if (this->has(p)) v.push_back(*iter);
        
        //TODO: match with skipgrams
    }
    return v;
}




