#include "patternmodel.h"
#include "algorithms.h"
#include <limits>
#include <sstream>

using namespace std;


void PatternModel::PatternModel(istream * f, const PatternModelOptions options) {
    char null;
    f.read( (char*) &null, sizeof(char));        
    f.read( (char*) &model_type, sizeof(char));        
    f.read( (char*) &model_version, sizeof(char));        
    if ((null != 0) || ((model_type != UNINDEXEDPATTERNMODEL) && (model_type != INDEXEDPATTERNMODEL) && (model_type != GRAPHPATTERNMODEL)))  {
        cerr << "File is not a colibri model file (or a very old one)"
        throw InternalError()
    }
    if (model_type == GRAPHPATTERNMODEL) {
        cerr << "Model is a graph model, can not be loaded as pattern model"
        throw InternalError()
    }
    f.read( (char*) &totaltokens, sizeof(uint64_t));        
    f.read( (char*) &totaltypes, sizeof(uint64_t)); 

    read(f); //read PatternStore
    postread(options);
}


void PatternModel::write(ostream * out) {
    const char null = 0;
    out.write( (char*) &null, sizeof(char));        
    out.write( (char*) &model_type, sizeof(char));        
    out.write( (char*) &model_version, sizeof(char));        
    out.write( (char*) &totaltokens, sizeof(uint64_t));        
    out.write( (char*) &totaltypes, sizeof(uint64_t)); 
    write(out); //write PatternStore
}

void PatternModel::postread(const PatternModelOptions options) {
    //this function has a specialisation specific to indexed pattern models,
    //this is the generic version
    for (iterator iter = this->begin(); iter != this->end(); iter++) {
        const Pattern p = iter->first;
        const int n = p.n();
        if (n > maxn) maxn = n;
        if (n < minn) minn = n;
    }
}



void PatternModel::train(std::istream * in, const PatternModelOptions options) {
    uint32_t sentence = 0;
    const int BUFFERSIZE = 65536;
    unsigned char line[BUFFERSIZE];
    std::map<int, std::vector< std::vector< std::pair<int,int> > > > gapconf;

    for (int n = 1; n <= options.MAXLENGTH; n++) {
        in->seekg(0);
        cerr << "Counting " << n << "-grams" << endl; 
        sentence++;

        if ((DOFIXEDSKIPGRAMS) && (gapconf[n].empty())) compute_multi_skips(gapconf[n], vector<pair<int,int> >(), n);

        Pattern line = Pattern(in);
        vector<pair<Pattern,int>> ngrams;
        line.ngrams(ngrams, n)


        for (vector<pair<Pattern,int>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
            const Pattern pattern = iter->first;
            if (pattern.category() == NGRAM) {
                const IndexReference ref = CorpusReference(sentence, iter->second);
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
                        reverseindex.insert(ref,pattern);
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
                                    reverseindex.insert(ref,skippattern);
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

int PatternModel::prune(int threshold, int _n) {
    int pruned = 0;
    PatternModel::iterator iter = this->begin(); 
    do {
        const Pattern pattern = iter->first;
        if (( (_n == 0) || (pattern.n() == n) )&& (occurrencecount(pattern) < threshold)) {
            iter = erase(iter); 
            pruned++;
        } else {
            iter++;
        }
    } while(iter != this->end());       

    //prune patterns from reverse index if they don't exist anymore
    multimap<IndexReference,Pattern>::iterator iter2 = reverseindex.begin(); 
    do {
        const Pattern pattern = iter->second;
        if (( (_n == 0) || (pattern.n() == n) ) && (!has(pattern))) {
            iter = reverseindex.erase(iter);
        } else {
            iter++;
        }
    } while (iter != reverseindex.end())

    return pruned;
}

int pruneskipgrams(int threshold, int minskiptypes, int minskiptokens, int _n = 0) {
    return 0; //only works for indexed models
}

std::vector<std::pair<const Pattern, int> > getpatterns(const Pattern & pattern) {
    //get all patterns in pattern
    std::vector<std::pair<const Pattern, int> > v;   
    std::vector<std::pair<const Pattern, int> > ngrams;
    pattern.subngrams(ngrams, minlength(), maxlength());
    for (std::vector<std::pair<const Pattern, int> >::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
        const Pattern p = iter->first;
        if (has(p)) v.push_back(*iter);
        
        //TODO: match with skipgrams
    }
    return v;
}

/////////////////////////////////////////

















void ModelReader::readfile(const string & filename, const bool DEBUG ) {
	int last = 0;
	this->DEBUG = DEBUG;
	//EncNGram lastngram;
	//EncSkipGram lastskipgram;
	
	FOUNDMAXN = 0;
	FOUNDMINN = 99;
	
    ifstream f;
    f.open(filename.c_str(), ios::in | ios::binary);
    if ((!f) || (!f.good())) {
       cerr << "File does not exist: " << filename << endl;
       exit(3);
    }
        
    f.read( (char*) &model_id, sizeof(uint64_t));        
    f.read( (char*) &totaltokens, sizeof(uint64_t));        
    f.read( (char*) &totaltypes, sizeof(uint64_t)); 
    
    if (DEBUG) {
    	cerr << "\tLOADING MODEL " << filename << endl;
    	cerr << "\tMODEL-ID=" << model_id << endl;
    	cerr << "\tTOTALTOKENS=" << totaltokens << endl;
    	cerr << "\tTOTALTYPES=" << totaltypes << endl;
    }
    
    //Determine whether to n-gramsize is signed (0) or unsigned (1):
    int ngramversion = 1;
    if ((model_id > UNINDEXEDPATTERNMODEL) && (model_id < INDEXEDPATTERNMODEL)) {
        if (model_id < UNINDEXEDPATTERNMODEL + 2) { ngramversion = 0; } else { ngramversion = 1; } 
    } 
    if ((model_id > INDEXEDPATTERNMODEL) && (model_id < GRAPHPATTERNMODEL)) {
        if (model_id < INDEXEDPATTERNMODEL + 3) { ngramversion = 0; } else { ngramversion = 1; } 
    }     
    if ((model_id > GRAPHPATTERNMODEL) && (model_id < GRAPHPATTERNMODEL+10)) {
        if (model_id < GRAPHPATTERNMODEL + 2) { ngramversion = 0; } else { ngramversion = 1; } 
    }     
    
    readheader(&f);
    unsigned char check;    
    for (unsigned int i = 0; i < totaltypes; i++) {           
        char gapcount;
        if (DEBUG) cerr << "\t@" << i + 1 << '/' << totaltypes;
        f.read((char*) &check, sizeof(char));
        if (check != 0xff) {
        	cerr << "ERROR processing " + filename + " at construction " << i << " of " << totaltypes << ". Expected check-byte, got " << (int) check << endl;
        	f.read(&gapcount, sizeof(char));
        	cerr << "DEBUG: next byte should be gapcount, value=" << (int) gapcount << endl;
        	if (last == 1) {
        		cerr << "DEBUG: previous construction was a n-gram" << endl;
        	} else if (last == 2) {
        		cerr << "DEBUG: previous construction was a skip-gram" << endl;
        	} 
        	exit(13);        	
        }
        f.read(&gapcount, sizeof(char));
        if (gapcount == 0) {
            if (DEBUG)  cerr << "\tNGRAM: ";
            const EncNGram ngram = EncNGram(&f, ngramversion); //read from file
            const int n = ngram.n();
            if (n > FOUNDMAXN) FOUNDMAXN = n;
            if (n < FOUNDMINN) FOUNDMINN = n;
            readngramdata(&f, ngram);     
            if (DEBUG) ngram.out(); 
            last = 1;
            //lastngram = ngram;    
        } else {
            if (DEBUG)  cerr << "\tSKIPGRAM, " << (int) gapcount << " gaps: ";
            const EncSkipGram skipgram = EncSkipGram( &f, gapcount, ngramversion); //read from file
            const int n = skipgram.n();
            if (n > FOUNDMAXN) FOUNDMAXN = n;
            if (n < FOUNDMINN) FOUNDMINN = n;            
            if (DEBUG) skipgram.out();              
            readskipgramdata(&f, skipgram, ngramversion);
            last = 2;
            //lastskipgram = skipgram;
        }
        if (DEBUG)  cerr << endl;      //DEBUG  
    }
    readfooter(&f);    
    f.close();
    if (DEBUG)  cerr << "\tFOUNDMAXN=" << FOUNDMAXN << endl;
    if (DEBUG)  cerr << "\tFOUNDMINN=" << FOUNDMINN << endl;
}

void ModelWriter::writefile(const string & filename) {
    ofstream f;
    f.open(filename.c_str(), ios::out | ios::binary);
    if ((!f) || (!f.good())) {
       cerr << "File does not exist: " << filename << endl;
       exit(3);
    }
    

    uint64_t _id = this->id();
    f.write( (char*) &_id, sizeof(uint64_t));        
    uint64_t _totaltokens = tokens();
    f.write( (char*) &_totaltokens, sizeof(uint64_t));        
    uint64_t _totaltypes = types();
    f.write( (char*) &_totaltypes, sizeof(uint64_t)); 
        
    writeheader(&f);
    writengrams(&f);
    writeskipgrams(&f);
    writefooter(&f);
    f.close();
}


ModelQuerierBase::ModelQuerierBase() {
	/*for (int n = 1; n <= MAXN; n++) { //removed in favour of on-demand-basis
    	compute_multi_skips(gapconf[n], vector<pair<int,int> >(), n);
    }*/
}

std::vector<pair<const EncAnyGram*, CorpusReference> > ModelQuerierBase::getpatterns(const unsigned char * data, const unsigned char datasize, bool doskipgrams, uint32_t linenum, const int minn, const int maxn) {

	std::vector<pair<const EncAnyGram*, CorpusReference> > patterns;

	//extract all patterns in an input string
	if (maxn > MAXN) {
       	cerr << "FATAL ERROR: Maximum n-gram size " << maxn << " exceeds the internal maximum MAXN="  << (int) MAXN << endl;
       	exit(14);
    }   
	const int l = countwords(data, datasize);
	for (int begin = 0; begin <= l; begin++) {
		for (int length = minn; (length <= maxn) && (begin+length <= l);  length++) {
		    //cerr << "TRYING PATTERN " << begin << " " << length << endl;
		    
		    EncNGram * ngram;
		    try {
			     ngram = getencngram(begin,length, data, datasize);
			} catch (exception &e) {
			    cerr << "ModelQuerierBase::getpattern:: Attempting to continue despite error..." << endl;
			    continue;
			}
			const EncAnyGram * anygram =  ngram;
			
				
			const EncAnyGram * key = getfocuskey(anygram);

			if (key != NULL) {			
			//if (occurrencecount(anygram) > 0) {
			    ///cerr << "FOUND" << endl;
				patterns.push_back( make_pair(key, CorpusReference(linenum, (char) begin) ) ); //stores the actual pointer used by the model
				if (doskipgrams) {
					//TODO: make more efficient for complete models that are guaranteed not to prune sub-parts
				
					//iterate over all gap configurations
					const int n = ngram->n();
					
					if (gapconf[n].empty()) compute_multi_skips(gapconf[n], vector<pair<int,int> >(), n);
					
					for (size_t j = 0; j < gapconf[n].size(); j++) {
						//iterate over all gaps

                        vector<EncNGram*> subngrams;
                        vector<int> skipref;
                        bool initialskip = false;
                        bool finalskip = false;
                        int cursor = 0;
                        bool docount = true;                 

                        
                    	//Iterate over all gaps in this configuration    
                        for (size_t k = 0; k < gapconf[n][j].size(); k++) {                                                        
                            const int begin = gapconf[n][j][k].first;  
                            const int length = gapconf[n][j][k].second;                        
                            //cerr << begin << ';' << length << ';' << n << endl;
                            skipref.push_back( length); 
                            if (k == 0) {
                                initialskip = (begin == 0);
                            }                            
                            if (begin > cursor) {
                                EncNGram * subngram = ngram->slice(cursor,begin-cursor);
                                subngrams.push_back(subngram);                                                               
                                /*oc = count((const EncAnyGram *) subngram);
                                if (oc) oc = ngrams[*subngram].count();
                                if ((((oc == 0)) || ((MINSKIPTOKENS > 1) && (MINSKIPTOKENS >= MINTOKENS) && (oc < MINSKIPTOKENS)) ))    {
                                    docount = false;
                                    break;
                                }*/
                            }
                            cursor = begin + length;
                        }
                        if (cursor < n) {
                            EncNGram * subngram = ngram->slice(cursor,n-cursor);
                            subngrams.push_back(subngram);
                            /*oc = ngrams.count(*subngram);
                            if (oc) oc = ngrams[*subngram].count();
                            if (((oc == 0)) || ((MINSKIPTOKENS > 1) && (MINSKIPTOKENS >= MINTOKENS) && (oc < MINSKIPTOKENS)) )  {
                                docount = false;
                                break;
                            }*/
                        } else {
                            finalskip = true;
                        }
                        if (initialskip && finalskip && skipref.size() <= 1) docount = false; //the whole n-gram is a skip, discard
                        if (docount) {
                            EncSkipGram skipgram = EncSkipGram(subngrams, skipref, initialskip, finalskip);
                            const EncAnyGram * anygram2 = &skipgram;	
                            const EncAnyGram * key2 = getfocuskey(anygram2);		
							//if (occurrencecount(anygram2) > 0) {
							if (key2 != NULL) {
								patterns.push_back( make_pair(key2, CorpusReference(linenum, (char) begin)) ); //stores the actual pointer used by the model
							}
							//}			
						}
						
						for (size_t k = 0; k < subngrams.size(); k++) {
                            delete subngrams[k];
                        }
					}
				}
			} else {
				//if this pattern doesn't occur, we could break because longer patterns won't occur either in most models, but this is not always the case, so we don't break
				//TODO: make more efficient for complete models that are guaranteed not to prune sub-parts								
			}
			delete ngram;
		}
	}
	return patterns;
}

void ModelQuerier::querier(ClassEncoder & encoder, ClassDecoder & decoder, bool exact, bool repeat, const int minn, const int maxn) {
	const bool allowunknown = true;
	unsigned char buffer[65536];
	uint32_t linenum = 0;
    std::string line;
    do {
    	linenum++;
    	cout << linenum << ">> "; 
    	getline(cin,line);    	
    	if (!line.empty()) {
			int buffersize = encoder.encodestring(line, buffer, allowunknown); //last bool is
			if (exact) {
				//TODO
			} else {    	
				vector<pair<const EncAnyGram*, CorpusReference> > patterns = getpatterns(buffer,buffersize, true, linenum,minn,maxn);
				for (vector<pair<const EncAnyGram*, CorpusReference> >::iterator iter = patterns.begin(); iter != patterns.end(); iter++) {
					const EncAnyGram * anygram = iter->first;
					const CorpusReference ref = iter->second;
					outputinstance(anygram, ref, decoder);
				} 
			}
		}
    } while (!cin.eof() && (repeat));		
}


IndexedPatternModel::IndexedPatternModel(const string & corpusfile, int MAXLENGTH, int MINTOKENS, bool DOSKIPGRAMS, int MINSKIPTOKENS,  int MINSKIPTYPES, bool DOINITIALONLYSKIP, bool DOFINALONLYSKIP, bool DOREVERSEINDEX) {
    
    this->MAXLENGTH = MAXLENGTH;
    this->MINTOKENS = MINTOKENS;
    this->DOSKIPGRAMS = DOSKIPGRAMS;
    this->MINSKIPTOKENS = MINSKIPTOKENS;
    this->DOINITIALONLYSKIP = DOINITIALONLYSKIP;
    this->DOFINALONLYSKIP = DOFINALONLYSKIP;
    this->DOREVERSEINDEX = DOREVERSEINDEX;
    
    totaltokens = 0; //includes tokens not covered by the model!

    const int BUFFERSIZE = 65536;
    unsigned char line[BUFFERSIZE];

    cerr << "Token Threshold: " << MINTOKENS << endl;
    sentencesize.push_back(0); //dummy element for index 0 (starts at 1)
    
    if (MAXLENGTH > MAXN) {
       	cerr << "FATAL ERROR: Maximum n-gram size " << MAXLENGTH << " exceeds the internal maximum MAXN=" << MAXN << endl;
       	exit(14);
    }

    for (int n = 1; n <= MAXLENGTH; n++) {
        cerr << "Counting " << n << "-grams" << endl;            
        
        uint32_t sentence = 0;
                
        vector< vector< pair<int,int> > > gaps;
        if (DOSKIPGRAMS) compute_multi_skips(gaps, vector<pair<int,int> >(), n);    
                    
                    
        int foundngrams = 0; //types
        int foundskipgrams = 0; //types;
        
                            
        ifstream *IN =  new ifstream( corpusfile.c_str() );
        if (!IN->good()) {
        	cerr << "ERROR: Unable to open file " << corpusfile << endl;
        	exit(5);
        }    
        vector<unsigned int> words;
        while (IN->good()) {
            const int linesize = readline(IN, line, BUFFERSIZE );            
                    
            sentence++;

            if (sentence % 10000 == 0) {
                cerr << "\t@" << sentence << endl;
            }
                            
            
            const int l = countwords(line, linesize);            
            if (l >= 256) {
                if (n == 1) sentencesize.push_back(0);
                cerr << "WARNING: Sentence " << sentence << " exceeds maximum word-length 256, skipping! (" << linesize << " bytes)" << endl;
                continue;
           } else if (l == 0) {
                if (n == 1) sentencesize.push_back(0);
            	cerr << "WARNING: Sentence " << sentence << " contains no words, skipping! (" << linesize << " bytes)" << endl;
                continue;                
            }
            if (n == 1) sentencesize.push_back((unsigned char) l);
            if (n == 1) totaltokens += l;
            
            if (linesize > 0) //no { on purpose! applies to next for loop
            for (unsigned char i = 0; ((i < l - n + 1) && (i < 256)); i++) {                
                EncNGram * ngram = getencngram(i,n, line, linesize, sentence);  
                                                

                //cout << "NGRAM("<<ngram->n()<<","<<(int)ngram->size() << ")" << endl;
                              
                if (n > 2) {                    
                    EncNGram * subngram1 = ngram->slice(0, n - 1);
                    if (!(ngrams.count(*subngram1))) {
                        delete subngram1;
                        delete ngram;
                        continue; //if subngram does not exist (count==0), no need to count ngram, skip to next
                    }
                    delete subngram1;

                                                             
                    EncNGram * subngram2 = ngram->slice(1, n - 1);
                    if (!(ngrams.count(*subngram2))) {
                        delete subngram2;
                        delete ngram;
                        continue; //if subngram does not exist (count==0), no need to count ngram, skip to next
                    }
                    delete subngram2;                    
                }
                
                
                
                CorpusReference ref = CorpusReference(sentence,i);
                if (ngrams[*ngram].refs.empty()) foundngrams++;
                ngrams[*ngram].refs.insert(ref);            
                //tokencount[n]++;            

                if (DOSKIPGRAMS) {
                	//Iterate over all possible gap configurations
                    for (size_t j = 0; j < gaps.size(); j++) { 

                        if (gaps[j].size() == 1) {
                           if (!DOINITIALONLYSKIP) {
                                if (gaps[j][0].first == 0) continue; 
                           }
                           if (!DOFINALONLYSKIP) {
                               if (gaps[j][0].first + gaps[j][0].second == n) continue; 
                           }
                        }

                        
                        vector<EncNGram*> subngrams;
                        vector<int> skipref;
                        bool initialskip = false;
                        bool finalskip = false;
                        int cursor = 0;
                        bool docount = true;    
                        int oc = 0;             
                        
                           
                        //cerr << "INSTANCE SIZE: " << gaps[j].size() << endl;
                        
                    	//Iterate over all gaps in this configuration    
                    	skipref.reserve(gaps[j].size());
                        for (size_t k = 0; k < gaps[j].size(); k++) {                                                        
                            const int begin = gaps[j][k].first;  
                            const int length = gaps[j][k].second;                        
                            //cerr << begin << ';' << length << ';' << n << endl;
                            skipref.push_back( length); 
                            if (k == 0) {
                                initialskip = (begin == 0);
                            }                            
                            if (begin > cursor) {
                                EncNGram * subngram = ngram->slice(cursor,begin-cursor);
                                subngrams.push_back(subngram);                                                               
                                oc = ngrams.count(*subngram);
                                if (oc) oc = ngrams[*subngram].count();
                                if ((((oc == 0)) || ((MINSKIPTOKENS > 1) && (MINSKIPTOKENS >= MINTOKENS) && (oc < MINSKIPTOKENS)) ))    {
                                    docount = false;
                                    break;
                                }
                            }
                            cursor = begin + length;
                        }   
                        if (cursor < n) {
                            EncNGram * subngram = ngram->slice(cursor,n-cursor);
                            subngrams.push_back(subngram);
                            oc = ngrams.count(*subngram);
                            if (oc) oc = ngrams[*subngram].count();
                            if (((oc == 0)) || ((MINSKIPTOKENS > 1) && (MINSKIPTOKENS >= MINTOKENS) && (oc < MINSKIPTOKENS)) )  {
                                docount = false;
                                break;
                            }
                        } else {
                            finalskip = true;
                        }
                        if (initialskip && finalskip && skipref.size() <= 1) docount = false; //the whole n-gram is a skip, discard
                        if (docount) {
                            EncSkipGram skipgram = EncSkipGram(subngrams, skipref, initialskip, finalskip);                                                    
                            vector<EncNGram*> skipcontent_subngrams;
                            vector<int> skipcontent_skipref;
                            cursor = 0;
                            for (size_t k = 0; k < gaps[j].size(); k++) {
                                const int begin = gaps[j][k].first;  
                                const int length = gaps[j][k].second;
                                EncNGram * subskip = ngram->slice(begin,length);                                
                                skipcontent_subngrams.push_back(subskip);                               
                                oc = ngrams.count(*subskip);
                                if (oc) oc = ngrams[*subskip].count();
                                if ((oc == 0) || (oc < MINTOKENS) || ((MINSKIPTOKENS > 1) && (MINSKIPTOKENS >= MINTOKENS) && (oc < MINSKIPTOKENS) ) )  {
                                	//skip content doesn't occur often enough, abort count
                                	docount = false;
                                	break;
                                }
                                if (cursor > 0) skipcontent_skipref.push_back(begin - cursor);
                                cursor = begin+length;
                            }   
                            if (docount) { //second check, we might have aborted
		                        EncSkipGram skipcontent = EncSkipGram(skipcontent_subngrams, skipcontent_skipref, false, false);                                                        
		                        if (skipgrams[skipgram].count() == 0) foundskipgrams++;                            
		                        skipgrams[skipgram]._count++;
		                        skipgrams[skipgram].skipcontent[skipcontent].refs.insert(ref);
		                        //skiptokencount[n]++;                            
                            }
                            for (size_t k = 0; k < skipcontent_subngrams.size(); k++) {       
		                        delete skipcontent_subngrams[k];
		                    }             
                        }
                        //cleanup
                        for (size_t k = 0; k < subngrams.size(); k++) {
                            delete subngrams[k];
                        }
                    }                    
                }
                
                delete ngram;                 
            }            
            
        };

       cerr << "Found " << foundngrams << " distinct " << n << "-grams"; // (" << tokencount[n] << " tokens)";
       if (DOSKIPGRAMS) {
        cerr << " and " << foundskipgrams << " distinct skipgrams" << endl; // (" << skiptokencount[n] << " tokens)" << endl;
       } else {
        cerr << endl;
       }
    
       if (foundngrams == 0) break;

       //prune n-grams
       int pruned = 0;
       {
            unordered_map<EncNGram,NGramData>::iterator iter = ngrams.begin();
            while (iter != ngrams.end()) {       
                if (iter->first.n() == n) {
                    if (iter->second.count() < (unsigned int) MINTOKENS) {
                        pruned++;
                        iter = ngrams.erase(iter);
                    } else {
                        iter++;
                    }                        
                } else {
                    iter++;
                }           
            }
       }
       
       cerr << "Pruned " << pruned << " " << n << "-grams" << endl; //" (" << tokencount[n] << " tokens)" << endl;
    
       
       if (DOSKIPGRAMS) {       
           //prune skipgrams
           pruned = 0;
           {
               unordered_map<EncSkipGram,SkipGramData>::iterator iter = skipgrams.begin();
               while (iter != skipgrams.end()) {     
                   if (iter->first.n() == n) {                          
                        bool pruneskipgram = false;
                        if ((iter->second.count() < (unsigned int) MINTOKENS) || ((iter->second.skipcontent.size() < (unsigned int) MINSKIPTYPES)))  {
                            pruneskipgram = true;
                        } else {                                    
                            int prunedskiptokens = 0;
                            
                            //prune content with insufficient occurrences:                        
                            {
                                unordered_map<EncSkipGram,NGramData>::iterator iter2 = iter->second.skipcontent.begin();
                                while (iter2 != iter->second.skipcontent.end()) {
                                    if (iter2->second.count() < (unsigned int) MINSKIPTOKENS) {
                                        //prune skip
                                        iter->second._count -= iter2->second.count();
                                        prunedskiptokens += iter2->second.count();
                                        iter2 = iter->second.skipcontent.erase(iter2); 
                                    } else {
                                        iter2++;
                                    }                            
                                }
                            }
                            
                            
                            //prune skipgrams with gaps with not enough different types
                            if (!skipgramvarietycheck(iter->second, MINSKIPTYPES)) pruneskipgram = true;

                            
                            if ( (prunedskiptokens > 0) && ( (iter->second.skipcontent.size() < (unsigned int) MINSKIPTYPES) || (iter->second.count() - prunedskiptokens < (unsigned int) MINTOKENS) ) ) { //reevaluate
                                pruneskipgram = true;
                                //skiptokencount[n] -= prunedskiptokens;
                            } else {
                                //skipgramtokencount += iter->second.count;                            
                            }
                        }
                        if (pruneskipgram) {
                            pruned++;
                            iter = skipgrams.erase(iter);
                        } else {
                            iter++;
                        }
                   } else {
                        iter++;
                   }
               }
           }
           cerr << "Pruned " << pruned << " skipgrams" << endl; // (" << skiptokencount[n] << " tokens)" << endl;
           
        }
        IN->close();
        delete IN;
    }
    cerr << "Computing general statistics..." << endl;
    computestats();
}






IndexedPatternModel::IndexedPatternModel(const string & filename, bool DOREVERSEINDEX, const bool DEBUG) {    
    this->DOREVERSEINDEX = DOREVERSEINDEX;
    totaltokens = 0;  
    
    MAXLENGTH = 0;

    totalngramcount = 0;
    totalskipgramcount = 0;

    for (int n = 0; n < MAXN; n++) {
        ngramcount[n] = 0;
        skipgramcount[n] = 0;
        ngramtypes[n] = 0;
        skipgramtypes[n] = 0;
    }
    
    if (!filename.empty()) {
        readfile(filename, DEBUG);
        computestats();
    }
}

IndexedPatternModel::IndexedPatternModel(const string & corpusfile, IndexedPatternModel & refmodel, int MAXLENGTH, int MINTOKENS, bool DOSKIPGRAMS, int MINSKIPTOKENS,  int MINSKIPTYPES, bool DOINITIALONLYSKIP, bool DOFINALONLYSKIP, bool DOREVERSEINDEX) {    
    if (MAXLENGTH > refmodel.getmaxn()) MAXLENGTH = refmodel.getmaxn();
    if (MINTOKENS < refmodel.getminn()) MINTOKENS = refmodel.getminn(); 

    this->MAXLENGTH = MAXLENGTH;
    this->MINTOKENS = MINTOKENS;
    this->DOSKIPGRAMS = DOSKIPGRAMS;
    this->MINSKIPTOKENS = MINSKIPTOKENS;
    this->DOINITIALONLYSKIP = DOINITIALONLYSKIP;
    this->DOFINALONLYSKIP = DOFINALONLYSKIP;
    this->DOREVERSEINDEX = DOREVERSEINDEX;
    
    
    totaltokens = 0;
    

    int sentence = 0;
    const int BUFFERSIZE = 65536;
    unsigned char line[BUFFERSIZE];
    
    sentencesize.push_back(0); //dummy
    
    ifstream *IN =  new ifstream( corpusfile.c_str() );
    if (!IN->good()) {
    	cerr << "ERROR: Unable to open file " << corpusfile << endl;
    	exit(5);
    }    
    vector<unsigned int> words;
    while (IN->good()) {
        const int linesize = readline(IN, line, BUFFERSIZE );            
                
        sentence++;

        if (sentence % 10000 == 0) {
            cerr << "\t@" << sentence << endl;
        }
                                
        const int l = countwords(line, linesize);            
        if (l >= 256) {
            cerr << "WARNING: Sentence " << sentence << " exceeds maximum word-length 256, skipping! (" << linesize << " bytes)" << endl;
            sentencesize.push_back(0);
            continue;
       } else if (l == 0) {        
        	cerr << "WARNING: Sentence " << sentence << " contains no words, skipping! (" << linesize << " bytes)" << endl;
        	sentencesize.push_back(0);
            continue;                
        }
        sentencesize.push_back((unsigned char) l);
        totaltokens += l;
        
		vector<pair<const EncAnyGram*, CorpusReference> > patterns = refmodel.getpatterns(line,linesize, true, sentence,1,MAXLENGTH);
		//cerr << "   " << patterns.size() << " patterns..." << endl;
		for (vector<pair<const EncAnyGram*, CorpusReference> >::iterator iter = patterns.begin(); iter != patterns.end(); iter++) {		    
			const EncAnyGram * anygram = iter->first;			
			const CorpusReference ref = iter->second;			
			if (getkey(anygram) || refmodel.getkey(anygram)) {		
			    //cerr << "Found pattern" << endl;	
			    if (!anygram->isskipgram()) {
			    	const EncNGram ngram = *( (const EncNGram*) refmodel.getkey(anygram) );
			        ngrams[ngram].refs.insert(ref);  
			    } else {
			        const EncSkipGram skipgram = *( (const EncSkipGram*) refmodel.getkey(anygram) );
			        skipgrams[skipgram]._count++;
			        pair<int,int> wordspos = getwords(line, linesize, skipgram.n(), ref.token);
			        if (wordspos.second == 0) {
			            cerr << "INTERNAL ERROR: Original instantiation not found (length=0)" << endl;
			            cerr << "BEGIN=" << wordspos.first << ";LENGTH=" << wordspos.second << endl;
			            cerr << "SKIPGRAM=";
			            skipgram.out();
			            cerr << endl;
			            cerr << "LINESIZE=" << linesize << endl;
			            cerr << "REQTOKEN=" << (int) ref.token << endl;
			            exit(6);
			        }			        
			        EncNGram ngram = EncNGram(line + wordspos.first, wordspos.second);
			        EncSkipGram skipcontent = skipgram.extractskipcontent(ngram);
                    skipgrams[skipgram].skipcontent[skipcontent].refs.insert(ref);
			    }		
			 }			
		} 
    }
    IN->close();
    delete IN;
}



void IndexedPatternModel::readngramdata(std::istream * f, const EncNGram & ngram, int ngramversion, bool ignore) {
    uint32_t count;
    f->read((char*) &count, sizeof(uint32_t)); //read occurrence count
    for (unsigned int j = 0; j < count; j++) {
        CorpusReference ref = CorpusReference(f); //read from file
        if (!ignore) ngrams[ngram].refs.insert(ref);
        if( (DOREVERSEINDEX) && (!ignore)) {
            const int index = ref.sentence;
            const EncAnyGram * key = getkey((const EncAnyGram *) &ngram);
            bool found = false;
            for (int k = 0; k < reverse_index[index].size(); k++) if (reverse_index[index][k] == key) { found = true; break; };
            if (!found) reverse_index[index].push_back(key);
        }
               
    }
    if (!ignore)  {    
    	const char n = ngram.n();
    	if (n > MAXN) {
      		cerr << "FATAL ERROR: N-gram size " << n << " exceeds the internal maximum MAXN=" << MAXN << endl;
	       	exit(14);
    	}
    	if (n > MAXLENGTH) MAXLENGTH = n;    	
    }    
}



void IndexedPatternModel::readskipgramdata(std::istream * f, const EncSkipGram & skipgram, int ngramversion, bool ignore) {
    uint32_t count;
    f->read((char*) &count, sizeof(uint32_t)); //read occurrence count            
    if (!ignore) {
    	skipgrams[skipgram]._count = count; //assign
    	const char n = skipgram.n();
    	if (n > MAXN) {
      		cerr << "FATAL ERROR: N-gram size " << n << " exceeds the internal maximum MAXN=" << MAXN << endl;
	       	exit(14);
    	}
    }            
    uint32_t skipcontentcount;
    f->read((char*) &skipcontentcount, sizeof(uint32_t));   
    for (unsigned int j = 0; j < skipcontentcount; j++) {                                
        EncSkipGram skipcontent = EncSkipGram(f,-1,ngramversion);  
        f->read((char*) &count, sizeof(uint32_t)); //read occurrence count                
        for (unsigned int k = 0; k < count; k++) {
            CorpusReference ref = CorpusReference(f); //read from file
            if (!ignore) skipgrams[skipgram].skipcontent[skipcontent].refs.insert(ref);
            if( (DOREVERSEINDEX) && (!ignore)) {
                const int index = ref.sentence;
                const EncAnyGram * key = (const EncAnyGram *) &(skipgrams[skipgram]);
                bool found = false;
                for (int k = 0; k < reverse_index[index].size(); k++) if (reverse_index[index][k] == key) { found = true; break; };
                if (!found) reverse_index[index].push_back(key);
            }
        }        
    }    
}

void IndexedPatternModel::readheader(std::istream * in, bool ignore) {
    if ((model_id < INDEXEDPATTERNMODEL) && (!ignore)) {
        cerr << "ERROR: Reading an unindexed pattern model as indexed. Did you forget to pass the -u flag?" << endl;
        exit(2);
    }
}

void IndexedPatternModel::readfooter(std::istream * f, bool ignore) {
    if (!ignore) {
         sentencesize.clear();
         sentencesize.push_back(0); //dummy
    }
    if (model_id >= INDEXEDPATTERNMODEL+1) {
        uint64_t count;
        f->read( (char*) &count, sizeof(uint64_t));
        for (unsigned int i = 1; i <= count; i++) {
            unsigned char slen;
            f->read((char*) &slen, sizeof(unsigned char));
            if (!ignore) sentencesize.push_back(slen);       
        }
    }
}

void IndexedPatternModel::writefooter(std::ostream * f) {
   uint64_t count = sentencesize.size() - 1; //minus 0-index dummy
   f->write( (char*) &count, sizeof(uint64_t));
   for (unsigned int i = 1; i <= count; i++) {
        unsigned char slen = sentencesize[i];
        f->write( (char*) &slen, sizeof(unsigned char));
   }    
}


void IndexedPatternModel::writengramdata(std::ostream * f, const EncNGram & ngram) {
    const uint32_t c = ngrams[ngram].count();
    f->write( (char*) &c, sizeof(uint32_t) ); //occurrence count                                     
    for (set<CorpusReference>::iterator iter = ngrams[ngram].refs.begin(); iter != ngrams[ngram].refs.end(); iter++) {                    
        iter->writeasbinary(f);
    }                    
}


void IndexedPatternModel::writengrams(std::ostream * f) {
	const unsigned char check = 0xff;    
    const char czero = 0;
    for(unordered_map<EncNGram,NGramData>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++ ) {
    	f->write((char*) &check, sizeof(char));        
        f->write(&czero, sizeof(char)); //gapcount, always zero for ngrams
        iter->first.writeasbinary(f);
        writengramdata(f, iter->first);       
    }   
}


void IndexedPatternModel::writeskipgramdata(std::ostream * f, const EncSkipGram & skipgram) {
    const uint32_t c  = skipgrams[skipgram].count();
    f->write( (char*) &c, sizeof(uint32_t) ); //occurrence count                         
    const uint32_t skipcontentcount = (uint32_t) skipgrams[skipgram].skipcontent.size();                        
    f->write( (char*) &skipcontentcount, sizeof(uint32_t) );
    for(unordered_map<EncSkipGram,NGramData>::iterator iter = skipgrams[skipgram].skipcontent.begin(); iter != skipgrams[skipgram].skipcontent.end(); iter++ ) {                    
        iter->first.writeasbinary(f);
        iter->second.writeasbinary(f);
    }
}


void IndexedPatternModel::writeskipgrams(std::ostream * f) {
	const unsigned char check = 0xff;                 
    for(unordered_map<EncSkipGram,SkipGramData>::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++ ) {
    	f->write((char*) &check, sizeof(char));                                
        iter->first.writeasbinary(f);        
        writeskipgramdata(f, iter->first);
    }     
}


bool IndexedPatternModel::exists(const EncAnyGram* key) const {
    if (key == NULL) return NULL;    
    if (key->gapcount() == 0) {
        return (ngrams.count(*( (EncNGram*) key) ) > 0);
    } else {
        return (skipgrams.count(*( (EncSkipGram*) key) ) > 0);
    }
    return false;
}



const EncAnyGram* IndexedPatternModel::getkey(const EncAnyGram* key) {
    if (key == NULL) return NULL;
    if (key->gapcount() == 0) {
        std::unordered_map<EncNGram,NGramData >::iterator iter = ngrams.find(*( (EncNGram*) key) );
        if (iter != ngrams.end()) {
            return &iter->first;
        } else {
            return NULL;
        }
    } else {
        std::unordered_map<EncSkipGram,SkipGramData >::iterator iter = skipgrams.find(*( (EncSkipGram*) key) );
        if (iter != skipgrams.end()) {
            return &iter->first;
        } else {
            return NULL;
        }        
    }
}


const AnyGramData* IndexedPatternModel::getdata(const EncAnyGram* key) {

    if (key->gapcount() == 0) {
        std::unordered_map<EncNGram,NGramData >::iterator iter = ngrams.find(*( (EncNGram*) key) );
        if (iter != ngrams.end()) {
            return &iter->second;
        } else {
            return NULL;
        }
    } else {
        std::unordered_map<EncSkipGram,SkipGramData >::iterator iter = skipgrams.find(*( (EncSkipGram*) key) );
        if (iter != skipgrams.end()) {
            return &iter->second;
        } else {
            return NULL;
        }        
    }
}



int IndexedPatternModel::occurrencecount(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return ngrams[*( (EncNGram*) key) ].count();
    } else {
        if (skipgrams.count(*( (EncSkipGram*) key) ) > 0) return skipgrams[*( (EncSkipGram*) key) ].count();   
    }
    return 0;
}

int IndexedPatternModel::coveragecount(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return ngrams[*( (EncNGram*) key) ].count() * key->n();
    } else {
        if (skipgrams.count(*( (EncSkipGram*) key) ) > 0) return skipgrams[*( (EncSkipGram*) key) ].count() * key->n();   
    }
    return 0;
}

double IndexedPatternModel::coverage(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return ((double) (ngrams[*( (EncNGram*) key) ].count() * key->n())  / tokens());
    } else {
        if (skipgrams.count( *( (EncSkipGram*) key)) > 0) return ((double) (skipgrams[ *( (EncSkipGram*) key)].count() * key->n()) / tokens());
    }
    return 0;
}


/*double IndexedPatternModel::relfreq(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return (double) ngrams[*( (EncNGram*) key) ].count() / tokencount[key->n()];
    } else {
        if (skipgrams.count( *( (EncSkipGram*) key)) > 0) return (double) skipgrams[ *( (EncSkipGram*) key)].count() / skiptokencount[key->n()];
    }
    return 0;
}*/


/*set<int> * IndexedPatternModel::index(const EncAnyGram* key) {
    if (key->gapcount() == 0) {        
        if (ngram_index.count(*( (EncNGram*) key) ) > 0) return &ngram_index[*( (EncNGram*) key) ];
    } else {
        if (skipgram_index.count( *( (EncSkipGram*) key)) > 0) return &skipgram_index[ *( (EncSkipGram*) key)];
    }
}*/


double IndexedPatternModel::pmi(const EncAnyGram * key1, const EncAnyGram * key2) {
    //pointwise mutual information
    double jointcount = 0;
        
    set<int> sentences1 = getsentences(key1);
    set<int> sentences2 = getsentences(key2);
    
    set<int>::const_iterator sourceiter = sentences1.begin();    
    set<int>::const_iterator targetiter = sentences2.begin();
    
    while ((sourceiter !=sentences1.end()) && (targetiter!=sentences2.end())) {
        if (*sourceiter < *targetiter) { 
            sourceiter++;
        } else if (*targetiter < *sourceiter) {
            targetiter++;
        } else {  //equal
            jointcount++;
            sourceiter++;
            targetiter++;
        }
    }
    
    return  log( (double) jointcount / (occurrencecount(key1) * occurrencecount(key2)) );    
}


double IndexedPatternModel::npmi(const EncAnyGram * key1, const EncAnyGram * key2) {
    //normalised pointwise mutual information
    double jointcount = 0;
        
    set<int> sentences1 = getsentences(key1);
    set<int> sentences2 = getsentences(key2);
    
    set<int>::const_iterator sourceiter = sentences1.begin();    
    set<int>::const_iterator targetiter = sentences2.begin();
    
    while ((sourceiter !=sentences1.end()) && (targetiter!=sentences2.end())) {
        if (*sourceiter < *targetiter) { 
            sourceiter++;
        } else if (*targetiter < *sourceiter) {
            targetiter++;
        } else {  //equal
            jointcount++;
            sourceiter++;
            targetiter++;
        }
    }
    
    return  log( (double) jointcount / (occurrencecount(key1) * occurrencecount(key2)) )  / -log((double)jointcount/(double)occurrences());    
}





void IndexedPatternModel::computestats() {
    totalngramcount = 0;
    totalskipgramcount = 0;
    for (int n = 1; n <= MAXN; n++) { ngramcount[n] = 0;  skipgramcount[n] = 0; ngramtypes[n] = 0; skipgramcount[n] = 0; }

    for (unordered_map<const EncNGram,NGramData >::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
        const EncAnyGram * anygram = &iter->first;
        ngramtypes[(int) anygram->n()]++;
        ngramcount[(int) anygram->n()] += iter->second.refs.size();  
        totalngramcount += iter->second.refs.size();   
    }
    for (unordered_map<const EncSkipGram,SkipGramData >::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++) {
        const EncAnyGram * anygram = &iter->first;
        skipgramtypes[(int)  anygram->n()]++;
        for(unordered_map<EncSkipGram,NGramData>::iterator iter2 = iter->second.skipcontent.begin(); iter2 != iter->second.skipcontent.end(); iter2++ ) {
          skipgramcount[(int) anygram->n()] += iter2->second.refs.size();  
          totalskipgramcount += iter2->second.refs.size();                        
        }
    }    
}


void readcorpus( const string & corpusfile, unordered_map<CorpusReference, const EncNGram *> & tokens) {
    const int BUFFERSIZE = 65536;
    unsigned char line[BUFFERSIZE];


    //read a whole corpus in memory
    ifstream *IN =  new ifstream( corpusfile.c_str() );    
    if (!IN->good()) {
    	cerr << "ERROR: Unable to open file " << corpusfile << endl;
    	exit(5);
    }        
    int sentence = 0;
    vector<unsigned int> words;
    while (IN->good()) {
        const int linesize = readline(IN, line, BUFFERSIZE );            
                
        sentence++;

        if (sentence % 10000 == 0) {
            cerr << "\t@" << sentence << endl;
        }
                        
        
        const int l = countwords(line, linesize);
        if (l >= 256) {
            cerr << "WARNING: Sentence " << sentence << " exceeds maximum word-length 256, skipping!" << endl;
            continue;
        } else if (l == 0) {
        	cerr << "WARNING: Sentence " << sentence << " contains no words, skipping!" << endl;
            continue;
        }        
        if (linesize > 0) //no { on purpose! applies to next for loop
        for (unsigned char i = 0; ((i < l) && (i < 256)); i++) {                         
                CorpusReference ref = CorpusReference(sentence, i);                
                EncNGram * ngram = getencngram(i,1, line, linesize, sentence);                
                tokens[ref] = ngram;
        }
    }  
    IN->close();
    delete IN;
}


void IndexedPatternModel::coveragereport(std::ostream *OUT, const string & corpusfile, std::ostream *HTMLOUT, ClassDecoder * decoder, int segmentsize) {

    unordered_map<CorpusReference, const EncNGram *> tokens; //for coverage

    if ((HTMLOUT != NULL) && (decoder != NULL)) {
        readcorpus(corpusfile, tokens);        
        *HTMLOUT << "<html>\n<head>\n <meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\" />\n<title>Colibri: coverage view</title>" << endl;
        *HTMLOUT << "<style type=\"text/css\">" << endl;
        *HTMLOUT << "body { background: #ffffff; padding: 20px; font-family: sans-serif; font-size: 10px; margin-top: 50px; }" << endl;
        *HTMLOUT << "div.s { margin: 2px; border: 1px #dddddd solid; width: 75%; }" << endl;
        *HTMLOUT << "div.s span { padding: 1px; cursor: pointer; border: 1px solid white; }" << endl;
        *HTMLOUT << "div.s span:hover { border: 1px solid green; cursor: pointer; }" << endl;
        *HTMLOUT << "div.s:hover { border: 1px #aaaaaa solid; }" << endl;
        *HTMLOUT << "div#visor { background: #d9d9d9; padding: 10px; position: fixed; opacity: 0.9; border-radius: 0.2; left: 76%;}" << endl;
        *HTMLOUT << "span.c0 { background: #ffffff; }" << endl;
        *HTMLOUT << "span.c1 { background: #ffe5e5; }" << endl;
        *HTMLOUT << "span.c2 { background: #ffcccc; }" << endl;
        *HTMLOUT << "span.c3 { background: #ffb2b2; }" << endl;
        *HTMLOUT << "span.c4 { background: #ff9999; }" << endl;
        *HTMLOUT << "span.c5 { background: #ff7f7f; }" << endl;
        *HTMLOUT << "span.c6 { background: #ff6666; color: white; }" << endl;
        *HTMLOUT << "span.c7 { background: #ff4c4c; color: white; }" << endl;
        *HTMLOUT << "span.c8 { background: #ff3333; color: white; }" << endl;
        *HTMLOUT << "span.c9 { background: #ff1919; color: white; }" << endl;
        *HTMLOUT << "span.c10 { background: #ff0000; color: white; }" << endl;
        *HTMLOUT << "span.e { font-style: italic; }" << endl;
        *HTMLOUT << "span.sg { font-weight: bold; }" << endl;
        *HTMLOUT << "</style>" << endl;    
        *HTMLOUT << "<script type=\"text/javascript\" src=\"http://code.jquery.com/jquery-1.5.1.min.js\"></script>" << endl;
        *HTMLOUT << "<script type=\"text/javascript\">" << endl;
        *HTMLOUT << "function s(hoverlist) { $('#visor').html('<ol>' + hoverlist + '</ol>' ); } " << endl;
        //*HTMLOUT << "function s(hoverlist) { $('#visor').html('<ol>' + hoverlist + '</ol>' ); var o = $(this).offset(); $('#visor').css('left: ' + o.left + '; top: ' + (o.top + 50) + ';'); } " << endl;
        *HTMLOUT << "</script>" << endl;
        *HTMLOUT << "</head>\n<body>" << endl;
        *HTMLOUT << "<div id=\"visor\"></div>" << endl;
    }
    
    unordered_map<CorpusReference, unordered_set<const EncAnyGram *> > reverseindex; //for coverage
    
        
    int totalngramcoverage = 0;
    int totalskipgramcoverage = 0;
    int ngramcoverage[MAXN+1];    
    int skipgramcoverage[MAXN+1];
    int skipgramencapsulation[MAXN+1];
    for (int n = 1; n <= MAXN; n++) { ngramcoverage[n] = 0;  skipgramcoverage[n] = 0; skipgramencapsulation[n] = 0; }

            
    int covered = 0;
    int encapsulated = 0;
    int uncovered = 0;
    int outside = 0; // uncovered + unencapsulated

    for (unsigned int sentence = 1; sentence < sentencesize.size(); sentence++) {
        unsigned char slen = sentencesize[sentence];
        //cerr << "SENTENCE=" << sentence << ";SLEN=" << (int) slen << endl;
        
        if (sentence % segmentsize == 1) {
            if (OUT) *OUT << "Computing reverse index for next segment..." << endl;
            reverseindex.clear();
            for (unordered_map<const EncNGram,NGramData >::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
                const EncAnyGram * anygram = &iter->first;
                for (set<CorpusReference>::iterator iter2 = iter->second.refs.begin() ; iter2 != iter->second.refs.end(); iter2++) {
                    CorpusReference ref = *iter2;
                    if ((ref.sentence >= sentence) && (ref.sentence < sentence+segmentsize)) {
                        reverseindex[ref].insert(anygram);                   
                    }                                         
                }
            }
            for (unordered_map<const EncSkipGram,SkipGramData >::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++) {
                const EncAnyGram * anygram = &iter->first;
                for(unordered_map<EncSkipGram,NGramData>::iterator iter2 = iter->second.skipcontent.begin(); iter2 != iter->second.skipcontent.end(); iter2++ ) {                
                    for (set<CorpusReference>::iterator iter3 = iter2->second.refs.begin() ; iter3 != iter2->second.refs.end(); iter3++) {
                        CorpusReference ref = *iter3;
                        if ((ref.sentence >= sentence) && (ref.sentence < sentence+segmentsize)) {
                            reverseindex[ref].insert(anygram);
                        }                        
                    }                     
                }
            }                                        
        }
        
        
        unordered_map<unsigned char, unordered_set<const EncAnyGram *> > sentencecoverage; //exact coverage for this sentence
        unordered_map<unsigned char, unordered_set<const EncAnyGram *> > sentenceencapsulation; //exact coverage for this sentence
        
        for (unsigned char token = 0; token < slen; token++) {
            CorpusReference ref = CorpusReference( (uint32_t) sentence, token);
            if ( reverseindex.count(ref) ) {
                for (unordered_set<const EncAnyGram *>::const_iterator iter = reverseindex[ref].begin(); iter != reverseindex[ref].end(); iter++) {
                    const EncAnyGram * anygram = *iter;
                    if (anygram->isskipgram()) {
                        //skipgram
                        const EncSkipGram * skipgram = (const EncSkipGram *) anygram;
                        vector<bool> mask;
                        skipgram->mask(mask);
                        int offset = 0;
                        for (vector<bool>::iterator maskiter = mask.begin(); maskiter != mask.end(); maskiter++) {
                            if (*maskiter) {
                                sentencecoverage[token+offset].insert(anygram);
                            } else {
                                sentenceencapsulation[token+offset].insert(anygram);
                            }
                            offset++;
                        }                        
                    } else {
                        //ngram
                        for (unsigned char token2 = token; token2 < token+ anygram->n(); token2++) {
                            sentencecoverage[token2].insert(anygram);
                        }
                    }
                } 
            }
        }
        
        if ((HTMLOUT != NULL) && (decoder != NULL)) *HTMLOUT << "<div class=\"s\">"; 

        for (unsigned char token = 0; token < slen; token++) {
            if ((HTMLOUT != NULL) && (decoder != NULL)) {
                int valency = 0;
                if (sentencecoverage.count(token)) valency = sentencecoverage[token].size();        
                if (valency > 10) valency = 10;
                
                CorpusReference ref = CorpusReference( (uint32_t) sentence, token);
                if (tokens.count(ref)) {
                    bool hasskipgram = false;
                    string hoverlist = "";
                    for (unordered_set<const EncAnyGram *>::const_iterator iter = sentencecoverage[token].begin(); iter != sentencecoverage[token].end(); iter++) {
                        const EncAnyGram * anygram = *iter; 
                        if (anygram->isskipgram()) hasskipgram = true;
                        string s = anygram->decode(*decoder);
                        replaceAll(s,"\"","&quot;");
                        replaceAll(s,"<","&lt;");
                        replaceAll(s,">","&gt;");
                        replaceAll(s,"'","`");   
                        ostringstream convert;
                        convert << occurrencecount(anygram); 
                        hoverlist += "<li>" + s + " &mdash; " + convert.str() + "</li>";
                    }                                         
                    *HTMLOUT << "<span class=\"c" << valency;
                    if (sentenceencapsulation.count(token)) *HTMLOUT << " e";                               
                    if (hasskipgram) *HTMLOUT << " sg";  
                    string s = tokens[ref]->decode(*decoder);
                    replaceAll(s,"\"","&quot;");
                    replaceAll(s,"<","&lt;");
                    replaceAll(s,">","&gt;");                                
                    *HTMLOUT << "\" onmouseover=\"s('" + hoverlist + "');\" >" << s << "</span> ";
                } else {
                    cerr << "MISSING TOKEN: " << sentence << ":" << (int) token << endl;
                }
            }
       
            if ( sentencecoverage.count(token) ) {
                covered++;
                bool foundskipgram = false;
                bool foundngram = false;
                for (int n = 1; n < MAXN; n++) {
                    for (unordered_set<const EncAnyGram *>::const_iterator iter = sentencecoverage[token].begin(); iter != sentencecoverage[token].end(); iter++) {
                        const EncAnyGram * anygram = *iter;
                        if (anygram->n() == n) {
                            if (anygram->isskipgram()) {                                                
                                skipgramcoverage[n]++;
                                foundskipgram = true;
                                break; //only count once per n
                            } else {
                                ngramcoverage[n]++;
                                foundngram = true;
                                break; //only count once per n
                            }
                        }
                    }          
                }
                if (foundskipgram) {
                    totalskipgramcoverage++;
                }
                if (foundngram) {
                    totalngramcoverage++;
                }
            } else {
                uncovered++;
            }

            
            
            
            if ( sentenceencapsulation.count(token)) {
                encapsulated++;    
                for (int n = 1; n < MAXN; n++) {
                    for (unordered_set<const EncAnyGram *>::const_iterator iter = sentencecoverage[token].begin(); iter != sentencecoverage[token].end(); iter++) {
                        const EncAnyGram * anygram = *iter;
                        if (anygram->n() == n) {                                              
                            skipgramencapsulation[n]++;
                            break; //only count once per n
                        }
                    }          
                }
            } else if (!sentencecoverage.count(token)) {
                outside++;
            }            
        }
        if ((HTMLOUT != NULL) && (decoder != NULL)) *HTMLOUT << "</div>\n";
    }
    
    if ((HTMLOUT != NULL) && (decoder != NULL)) *HTMLOUT << "</body>\n</html>";
    
    if (OUT) {
        const int totalcount =  totalngramcount+totalskipgramcount;  
        *OUT << setiosflags(ios::fixed) << setprecision(4) << endl;       
        *OUT << "COVERAGE REPORT" << endl;
        *OUT << "----------------------------------" << endl;
        *OUT << "Total number of tokens:   " << setw(10) << totaltokens << endl << endl;
        *OUT << "                          " << setw(10) << "TOKENS" << setw(10) << "COVERAGE" << setw(7) << "TYPES" << setw(11) << "TTR" << setw(10) << "COUNT" << setw(10) << "FREQUENCY" << endl;    
        *OUT << "Total coverage:           " << setw(10) << covered << setw(10) << (double) covered / totaltokens << setw(7) << ngrams.size() + skipgrams.size()  << setw(11)<< (double)  covered / (ngrams.size() + skipgrams.size()) << setw(10) << totalngramcount+totalskipgramcount << setw(10) << (double) (totalngramcount+totalskipgramcount) / totalcount << endl;
        *OUT << "Uncovered:                " << setw(10) << uncovered << setw(10) << (double) uncovered / totaltokens <<endl << endl;
        
        
        
        *OUT << "N-gram coverage:          " << setw(10) << totalngramcoverage << setw(10) << (double) totalngramcoverage / totaltokens << setw(7) << ngrams.size() <<setw(11) << (double) totalngramcoverage / ngrams.size() << setw(10) << totalngramcount << setw(10) << (double) totalngramcount / totalcount << endl;
        for (int n = 1; n <= MAXN; n++) {
         if (ngramcoverage[n] > 0) {
            int t = 0;
            for (unordered_map<const EncNGram,NGramData >::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) if (iter->first.n() == n) t++;
            *OUT << " " << n << "-gram coverage:         " << setw(10) << ngramcoverage[n] << setw(10) << (double) ngramcoverage[n] / totaltokens << setw(7) << t << setw(11) <<  (double) t / ngramcoverage[n] << setw(10) << ngramcount[n] << setw(10) << (double) ngramcount[n] / totalcount  << endl;
         }
        }
        *OUT << endl;
        *OUT << "Skipgram coverage:        " << setw(10) << totalskipgramcoverage << setw(10) << (double) totalskipgramcoverage / totaltokens << setw(7) << skipgrams.size() << setw(11) << (double) totalskipgramcoverage / skipgrams.size() << setw(10) << totalskipgramcount << setw(10) <<  (double) totalskipgramcount / totalcount <<   endl;
        for (int n = 2; n <= MAXN; n++) {         
         if (skipgramcoverage[n] > 0) {
          int t = 0;
          for (unordered_map<const EncSkipGram,SkipGramData >::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++) if (iter->first.n() == n) t++; 
          *OUT << " " << n << "-skipgram coverage:     " << setw(10) << skipgramcoverage[n] << setw(10) << (double) skipgramcoverage[n] / totaltokens << setw(7) << t << setw(11) <<  (double) t / skipgramcoverage[n] << setw(10) << skipgramcount[n] << setw(10) << (double) skipgramcount[n] / totalcount << endl;
         }
        }
        *OUT << "----------------------------------" << endl;
        *OUT << "                                    " << setw(10) << "TOKENS" << setw(15) << "ENCAPSULATION" << setw(30) << "ENCAPSULATION+COVERAGE" << endl;    
        *OUT << "Total encapsulation:                " << setw(10) << encapsulated << setw(15) << (double) encapsulated / totaltokens << setw(30) <<  (double) (encapsulated + totalskipgramcoverage) / totaltokens << endl;
        *OUT << "Outside (uncovered&unencapsulated): " << setw(10) << outside << setw(15) << (double) outside / totaltokens << endl << setw(30) << "-" << endl;
        for (int n = 2; n <= MAXN; n++) {         
         if (skipgramcoverage[n] > 0) {
          int t = 0;
          for (unordered_map<const EncSkipGram,SkipGramData >::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++) if (iter->first.n() == n) t++; 
          *OUT << " " << n << "-skipgram encapsulation:          " << setw(10) << skipgramencapsulation[n] << setw(15) << (double) skipgramencapsulation[n] / totaltokens << setw(30) << (double) (skipgramencapsulation[n] + skipgramcoverage[n]) / totaltokens << endl;     
         }
        }
        //*OUT << "----------------------------------" << endl;
        //*OUT << "                                  " << setw(10) << "VALENCY" << endl;    
        //*OUT << "Mean token valency:               " << setw(10) << valency;                                                      
    }
    
    
        
        
    
    
}




void IndexedPatternModel::outputinstance(const EncAnyGram * anygram, CorpusReference ref, ClassDecoder & decoder) {
	cout << ref.sentence << ':' << (int) ref.token << "\t" << anygram->decode(decoder) << "\t" << occurrencecount(anygram) << "\t" << setprecision(numeric_limits<double>::digits10 + 1) << coverage(anygram) << endl; 
}



std::set<int> IndexedPatternModel::reverse_index_keys() {
    set<int> keys;
    for (unordered_map<int,vector<const EncAnyGram*> >::iterator iter = reverse_index.begin(); iter != reverse_index.end(); iter++) {
        keys.insert(iter->first);
    }   
    return keys;
}


int IndexedPatternModel::reverse_index_size(const int i) {
    int s = 0;
    if (reverse_index.count(i)) s += reverse_index[i].size();
    return s;
    
}

int IndexedPatternModel::reverse_index_size() {
    return reverse_index.size();
}

bool IndexedPatternModel::reverse_index_haskey(const int i) const {
    return (reverse_index.count(i) > 0);
}

void IndexedPatternModel::testreverseindex() {
    //DEBUG
    for (unordered_map<int, vector<const EncAnyGram *> >::iterator iter = reverse_index.begin(); iter != reverse_index.end(); iter++) {
        for (vector<const EncAnyGram *>::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
            const EncAnyGram * anygram = *iter2;
            anygram->n();
        }
    }
}

vector<const EncAnyGram*> IndexedPatternModel::get_reverse_index(const int i) {
    if (reverse_index.count(i) > 0) {
        return reverse_index[i];
    } else {
        vector<const EncAnyGram*> revindex;
        return revindex;
    }
}


const EncAnyGram* IndexedPatternModel::get_reverse_index_item(const int key, const int i) {
    const int s = reverse_index[key].size();
    if (i < s) {                
        vector<const EncAnyGram *>::iterator iter = reverse_index[key].begin() + i;
        return *iter;
    } else return NULL; 
}


void IndexedPatternModel::decode(ClassDecoder & classdecoder, ostream *OUT, bool outputhash) {
    *OUT << "#TYPES=" << types() << ";TOTALTOKENS=" << tokens() << endl;
    *OUT << "#";
    if (outputhash) *OUT << "HASH\t";
    *OUT << "N\tCLASS\tOCC.COUNT\tTOKENS\tCOVERAGE\tFREQ-ALL\tFREQ-G\tFREQ-N\tSKIPTYPES\tENTROPY\tREFERENCES" << endl;
       

    for(unordered_map<EncNGram,NGramData>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++ ) {
       const double covtokens = iter->second.count() * iter->first.n(); 
       const double cov = (double) covtokens / totaltokens;
    
       const double freqall = (double) iter->second.count() / occurrences();
       const double freqg = (double) iter->second.count() / totalngramcount;
       const double freqn = (double) iter->second.count() / ngramcount[(int) iter->first.n()];
    
       
       const EncNGram ngram = iter->first;
        if (outputhash) *OUT << ngram.hash() << '\t';
        *OUT << (int) ngram.n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << ngram.decode(classdecoder) << '\t' << iter->second.count() << '\t' << covtokens << '\t' <<  cov << '\t' << freqall << '\t' << freqg << '\t' << freqn;
        *OUT << "\t0\t-\t";
        for (set<CorpusReference>::iterator iter2 = iter->second.refs.begin() ; iter2 != iter->second.refs.end(); iter2++) {
            *OUT << iter2->sentence << ':' << (int) iter2->token << ' ';
        }                
        *OUT << endl;
    }
   

   
       for(unordered_map<EncSkipGram,SkipGramData>::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++ ) {
            const double covtokens = iter->second.count() * iter->first.n(); 
            const double cov = (double) covtokens / totaltokens;
           
            const double freqall = (double) iter->second.count() / occurrences();
            const double freqg = (double) iter->second.count() / totalskipgramcount;
            const double freqn = (double) iter->second.count() / skipgramcount[(int) iter->first.n()];
                          
           const EncSkipGram skipgram = iter->first;     
           if (outputhash) *OUT << skipgram.hash() << '\t';                         
           *OUT << (int) skipgram.n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << skipgram.decode(classdecoder) << '\t' << iter->second.count() << '\t' << covtokens << '\t' << cov << '\t' << freqall << '\t' << freqg << '\t' << freqn;
           const int skiptypes = iter->second.skipcontent.size();               
           const double entropy = iter->second.entropy();
           *OUT << skiptypes << '\t' << entropy << '\t';
            for(unordered_map<EncSkipGram,NGramData>::iterator iter2 = iter->second.skipcontent.begin(); iter2 != iter->second.skipcontent.end(); iter2++ ) {
                *OUT << iter2->first.decode(classdecoder) << '|' << iter2->second.count() << '|';
                for (set<CorpusReference>::iterator iter3 = iter2->second.refs.begin() ; iter3 != iter2->second.refs.end(); iter3++) {
                    *OUT << iter3->sentence << ':' << (int) iter3->token;        
                    *OUT << ','; 
                }
            }
           *OUT << endl;
       }
    

}


void IndexedPatternModel::decode(IndexedPatternModel & testmodel, ClassDecoder & classdecoder, std::ostream *OUT, bool outputhash) {
    
    *OUT << "#TYPES=" << types() << ";TOKENS=" << tokens() << endl;
    *OUT << "#";
    if (outputhash) *OUT << "HASH\t";
    *OUT << "N\tTEXT\tOCC.COUNT\tTOKENS\tCOVERAGE\tOCC.COUNT-TEST\tTOKENS-TEST\tCOVERAGE-TEST\tSKIPTYPES\tSKIP-ENTROPY\tSKIPTYPES-TEST\tSKIP-ENTROPY-TEST\tREFERENCES\tREFERENCES-TEST" << endl;

    for(unordered_map<EncNGram,NGramData>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++ ) {
       const int covtokens = iter->second.count() * iter->first.n();
       const double cov = (double) covtokens / totaltokens;
       const EncNGram ngram = iter->first;
        if (outputhash) ngram.hash() << '\t';
        *OUT << (int) ngram.n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << ngram.decode(classdecoder) << '\t' << iter->second.count() << '\t' << covtokens << '\t' << cov;
        
        const EncAnyGram * key = testmodel.getkey(&ngram);
        if (key) {
            *OUT << '\t' << testmodel.occurrencecount(key) << '\t' << testmodel.coveragecount(key) << '\t' << testmodel.coverage(key);
        } else {
            *OUT << "\t0\t0\t0";
        }        
        *OUT << "\t0\t-\t0\t-\t";                
        for (set<CorpusReference>::iterator iter2 = iter->second.refs.begin() ; iter2 != iter->second.refs.end(); iter2++) {
            *OUT << iter2->sentence << ':' << (int) iter2->token << ' ';
        }   
        if (key) {
            for (set<CorpusReference>::iterator iter2 = testmodel.ngrams[ngram].refs.begin() ; iter2 != testmodel.ngrams[ngram].refs.end(); iter2++) {
                *OUT << iter2->sentence << ':' << (int) iter2->token << ' ';
            }
        } else {
            *OUT << "\t-";   
        }             
        *OUT << endl;
    }
   

   
       for(unordered_map<EncSkipGram,SkipGramData>::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++ ) {
           const int covtokens = iter->second.count() * iter->first.n();
           const double cov = (double) covtokens / totaltokens;
           const EncSkipGram skipgram = iter->first;
           if (outputhash) skipgram.hash() << '\t';                              
           *OUT << (int) skipgram.n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << skipgram.decode(classdecoder) << '\t' << iter->second.count() << '\t' << covtokens << '\t' << cov;
            const EncAnyGram * key = testmodel.getkey(&skipgram);
            if (key) {
                *OUT << '\t' << testmodel.occurrencecount(key) << '\t' << testmodel.coveragecount(key) << '\t' << testmodel.coverage(key);
            } else {
                *OUT << "\t0\t0\t0";
            }       
           const int skiptypes = iter->second.skipcontent.size();               
           const double entropy = iter->second.entropy();
           *OUT << '\t' << skiptypes << '\t' << entropy << '\t';
           if (key) { 
                const int skiptypes2 = testmodel.skipgrams[skipgram].skipcontent.size();               
                const double entropy2 = testmodel.skipgrams[skipgram].entropy();
                *OUT << '\t' << skiptypes2 << '\t' << entropy2 << '\t';
           } else {
                *OUT << "\t0\t-\t";
           }
            for(unordered_map<EncSkipGram,NGramData>::iterator iter2 = iter->second.skipcontent.begin(); iter2 != iter->second.skipcontent.end(); iter2++ ) {
                *OUT << iter2->first.decode(classdecoder) << '|' << iter2->second.count() << '|';
                for (set<CorpusReference>::iterator iter3 = iter2->second.refs.begin() ; iter3 != iter2->second.refs.end(); iter3++) {
                    *OUT << iter3->sentence << ':' << (int) iter3->token;        
                    *OUT << ',';
                    //if (iter3 != iter2->second.refs.end() - 1) 
                }
                //MAYBE TODO: output references?
            }
            if (key) {
                for(unordered_map<EncSkipGram,NGramData>::iterator iter2 = testmodel.skipgrams[skipgram].skipcontent.begin(); iter2 != testmodel.skipgrams[skipgram].skipcontent.end(); iter2++ ) {
                    *OUT << iter2->first.decode(classdecoder) << '|' << iter2->second.count() << '|';
                    for (set<CorpusReference>::iterator iter3 = iter2->second.refs.begin() ; iter3 != iter2->second.refs.end(); iter3++) {
                        *OUT << iter3->sentence << ':' << (int) iter3->token;        
                        *OUT << ','; 
                    }
                }            
            } else {
                *OUT << "\t-";
            }
           *OUT << endl;
       }
    
}

void IndexedPatternModel::writeanygram(const EncAnyGram * anygram,std::ostream * out) {
	/*helper function to write an ngram or skipgram, provided it occurs in this model */
	const char zero = 0;
    const EncAnyGram * anygram2 = getkey(anygram); //find the anygram pointer as it appear in the model
    if (!anygram2->isskipgram()) {
    	//ngram
    	out->write( (char*) &zero, sizeof(char)); //for ngrams
    	//((EncNGram*) anygram2)->writeasbinary(out);
    	const EncNGram * tmp = (EncNGram*) anygram2;
    	tmp->writeasbinary(out);    	
    } else {
    	//skipgram
    	const EncSkipGram * tmp = (EncSkipGram*) anygram2;
    	tmp->writeasbinary(out);
    	//((EncSkipGram*) anygram2)->writeasbinary(out);
    }        
}         
        
        


bool IndexedPatternModel::skipgramvarietycheck(SkipGramData & skipgramdata, int mintypecount) {
    //returns whether all gaps satisfy type diversity (true) or not (false). Set earlyabort=true to have the algorithm abort as soon as its clear the diversity is not maintained (in which case container will not be complete)      
        
    map<int,unordered_set<int>> variety; //per (pseudo-)index, store all possible classes
                   
    //count step
    for (unordered_map<EncSkipGram,NGramData>::iterator iter = skipgramdata.skipcontent.begin(); iter != skipgramdata.skipcontent.end(); iter++) {        
        vector<EncNGram*> skipparts;
        iter->first.parts(skipparts);
        int skipnum = 0;
        for (vector<EncNGram*>::iterator iter2 = skipparts.begin(); iter2 != skipparts.end(); iter2++) {
            const EncNGram * part = *iter2;
            const int indexbase = skipnum * 100;
            vector<int> partclassvector;
            part->classvector(partclassvector);
            int index = 0;
            for (vector<int>::iterator iter3 = partclassvector.begin(); iter3 != partclassvector.end(); iter3++) {
                variety[indexbase+index].insert(*iter3);
                index++;
            }
            delete part;
            skipnum++;
        }        
    }
        
    //check
    for (map<int,unordered_set<int>>::iterator iter = variety.begin(); iter != variety.end(); iter++) {
        if (iter->second.size() < (size_t) mintypecount) return false;
    } 
    return true;
}

set<int> IndexedPatternModel::getsentences(const EncAnyGram * anygram) {
    const AnyGramData * data = getdata(anygram);
    set<CorpusReference> indices = data->get_refs();
    set<int> sentences;
    for (set<CorpusReference>::iterator iter = indices.begin(); iter != indices.end(); iter++) {
        sentences.insert((int) iter->sentence);
    }
    return sentences;
}
    

unordered_map<const EncAnyGram*, int> IndexedPatternModel::getcooccurrences(const EncAnyGram * anygram, IndexedPatternModel * targetmodel,  set<int> * sentenceconstraints) {
    if (targetmodel == NULL) targetmodel = this;
        
    //find sourcegram sentence index
    set<int> sentences = getsentences(anygram);
    
    unordered_map<const EncAnyGram *,int> coocgrams;
    for (set<int>::iterator iter = sentences.begin(); iter != sentences.end(); iter++) {
        if ((sentenceconstraints == NULL) || (sentenceconstraints->count(*iter))) {
            vector<const EncAnyGram*> tmp = targetmodel->get_reverse_index(*iter);
            //find keys
            for (vector<const EncAnyGram*>::iterator iter2 = tmp.begin(); iter2 != tmp.end(); iter2++) coocgrams[*iter2]++;
        }
    }
    
    return coocgrams;
}

void IndexedPatternModel::histogram(ostream *OUT) {
    map<int,unsigned int> h_ngrams;
    map<int,unsigned int> h_skipgrams;
    
    for (unordered_map<const EncNGram,NGramData>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
        h_ngrams[iter->second.count()] += 1;
    }  
    for (unordered_map<const EncSkipGram,SkipGramData>::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++) {
        h_skipgrams[iter->second.count()] += 1;
    }
    
    *OUT << "N-Grams Histogram:" << endl;
    for (map<int, unsigned int>::iterator iter = h_ngrams.begin(); iter != h_ngrams.end(); iter++) {
        *OUT << iter->first << "\t" << iter->second << endl;
    }
    *OUT << endl;
    
    if (!skipgrams.empty()) {
        *OUT << "Skipgrams Histogram:" << endl;
        for (map<int, unsigned int>::iterator iter = h_skipgrams.begin(); iter != h_skipgrams.end(); iter++) {
            *OUT << iter->first << "\t" << iter->second << endl;
        }
    }
    
}

void UnindexedPatternModel::histogram(ostream *OUT) {
    map<int,unsigned int> h_ngrams;
    map<int,unsigned int> h_skipgrams;
    
    for (unordered_map<const EncNGram,uint32_t>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
        h_ngrams[iter->second] += 1;
    }  
    for (unordered_map<const EncSkipGram,uint32_t>::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++) {
        h_skipgrams[iter->second] += 1;
    }
    
    *OUT << "N-Grams Histogram:" << endl;
    for (map<int, unsigned int>::iterator iter = h_ngrams.begin(); iter != h_ngrams.end(); iter++) {
        *OUT << iter->first << "\t" << iter->second << endl;
    }
    *OUT << endl;
    
    if (!skipgrams.empty()) {
        *OUT << "Skipgrams Histogram:" << endl;
        for (map<int, unsigned int>::iterator iter = h_skipgrams.begin(); iter != h_skipgrams.end(); iter++) {
            *OUT << iter->first << "\t" << iter->second << endl;
        }
    }
    
}


UnindexedPatternModel::UnindexedPatternModel(const string & corpusfile, int MAXLENGTH, int MINTOKENS, bool DOSKIPGRAMS, int MINSKIPTOKENS,  bool DOINITIALONLYSKIP, bool DOFINALONLYSKIP) {
    
    this->MAXLENGTH = MAXLENGTH;
    this->MINTOKENS = MINTOKENS;
    this->DOSKIPGRAMS = DOSKIPGRAMS;
    this->MINSKIPTOKENS = MINSKIPTOKENS;
    this->DOINITIALONLYSKIP = DOINITIALONLYSKIP;
    this->DOFINALONLYSKIP = DOFINALONLYSKIP;

    totaltokens = 0;

    const int BUFFERSIZE = 65536;
    unsigned char line[BUFFERSIZE];


    for (int n = 1; n <= MAXLENGTH; n++) {
        cerr << "Counting " << n << "-grams" << endl;            
        
        uint32_t sentence = 0;

        int foundngrams = 0;
        int foundskipgrams = 0;
                
        vector< vector< pair<int,int> > > gaps;
        if (DOSKIPGRAMS) compute_multi_skips(gaps, vector<pair<int,int> >(), n);    
        
        unordered_map<const EncSkipGram,size_t> lastskipcontenthash; //0 if not seen yet , 1 if already verified, hash key otherwise
        
        ifstream *IN =  new ifstream( corpusfile.c_str() );
        if (!IN->good()) {
        	cerr << "ERROR: Unable to open file " << corpusfile << endl;
        	exit(5);
        }        
        vector<unsigned int> words;
        while (IN->good()) {
            const int linesize = readline(IN, line, BUFFERSIZE );            
                    
            sentence++;

            if (sentence % 10000 == 0) {
                cerr << "\t@" << sentence << endl;
            }
                            
            
            const int l = countwords(line, linesize);            
            if (l >= 256) {
                cerr << "WARNING: Sentence " << sentence << " exceeds maximum word-length 256, skipping!" << endl;
                continue;
            } else if (l == 0) {
            	cerr << "WARNING: Sentence " << sentence << " contains no words, skipping!" << endl;
                continue;
            }
            
            if (linesize > 0) //no { on purpose! applies to next for loop
            for (unsigned char i = 0; ((i < l - n + 1) && (i < 256)); i++) {                
                EncNGram * ngram = getencngram(i,n, line, linesize, sentence);  
                                                

                //cout << "NGRAM("<<ngram->n()<<","<<(int)ngram->size() << ")" << endl;
                              
                if (n > 2) {                    
                    EncNGram * subngram1 = ngram->slice(0, n - 1);
                    if (!(ngrams.count(*subngram1))) {
                        delete subngram1;
                        delete ngram;
                        continue; //if subngram does not exist (count==0), no need to count ngram, skip to next
                    }
                    delete subngram1;

                                                             
                    EncNGram * subngram2 = ngram->slice(1, n - 1);
                    if (!(ngrams.count(*subngram2))) {
                        delete subngram2;
                        delete ngram;
                        continue; //if subngram does not exist (count==0), no need to count ngram, skip to next
                    }
                    delete subngram2;                    
                }
                
                
                if (ngrams[*ngram] == 0) foundngrams++;
                ngrams[*ngram]++; //increase occurence count                     
                //tokencount[n]++;            

                if (DOSKIPGRAMS) {                	                 
                    for (size_t j = 0; j < gaps.size(); j++) {

                        if (gaps[j].size() == 1) {
                           if (!DOINITIALONLYSKIP) {
                                if (gaps[j][0].first == 0) continue; 
                           }
                           if (!DOFINALONLYSKIP) {
                               if (gaps[j][0].first + gaps[j][0].second == n) continue; 
                           }
                        }

                        
                        vector<EncNGram*> subngrams;
                        vector<int> skipref;
                        bool initialskip = false;
                        bool finalskip = false;
                        int cursor = 0;
                        bool docount = true;    
                        int oc = 0;             
                        
                           
                        //cerr << "INSTANCE SIZE: " << gaps[j].size() << endl;
                        skipref.reserve(gaps[j].size());
                        for (size_t k = 0; k < gaps[j].size(); k++) {                                                        
                            const int begin = gaps[j][k].first;  
                            const int length = gaps[j][k].second;                        
                            //cerr << begin << ';' << length << ';' << n << endl;
                            skipref.push_back( length); 
                            if (k == 0) {
                                initialskip = (begin == 0);
                            }                            
                            if (begin > cursor) {
                                EncNGram * subngram = ngram->slice(cursor,begin-cursor);
                                subngrams.push_back(subngram);                                                               
                                oc = ngrams.count(*subngram);
                                if (oc) oc = ngrams[*subngram];
                                if ((oc == 0) || ((MINSKIPTOKENS > 1) && (MINSKIPTOKENS >= MINTOKENS) && (oc < MINSKIPTOKENS)) )    {
                                    docount = false;
                                    break;
                                }
                            }
                            cursor = begin + length;
                        }   
                        if (cursor < n) {
                            EncNGram * subngram = ngram->slice(cursor,n-cursor);
                            subngrams.push_back(subngram);
                            oc = ngrams.count(*subngram);
                            if (oc) oc = ngrams[*subngram];
                            if ((oc == 0) || ((MINSKIPTOKENS > 1) && (MINSKIPTOKENS >= MINTOKENS) && (oc < MINSKIPTOKENS)) )    {
                                docount = false;
                                break;
                            }
                        } else {
                            finalskip = true;
                        }
                        if (initialskip && finalskip && skipref.size() <= 1) docount = false; //the whole n-gram is a skip, discard
                        if (docount) {                        
                            EncSkipGram skipgram = EncSkipGram(subngrams, skipref, initialskip, finalskip);
                            if (skipgrams[skipgram] == 0) foundskipgrams++;                            
                            skipgrams[skipgram]++;
                            //skiptokencount[n]++;                            
                        }
                        if (docount) {
                            EncSkipGram skipgram = EncSkipGram(subngrams, skipref, initialskip, finalskip);                                                    
                            vector<EncNGram*> skipcontent_subngrams;
                            vector<int> skipcontent_skipref;
                            cursor = 0;
                            //iterate over all gaps in this configuration (to check their occurrence count)
                            for (size_t k = 0; k < gaps[j].size(); k++) {
                                const int begin = gaps[j][k].first;  
                                const int length = gaps[j][k].second;
                                EncNGram * subskip = ngram->slice(begin,length);                                                               
                                oc = ngrams.count(*subskip);
                                if (oc) oc = ngrams[*subskip];                                
                                if (lastskipcontenthash[skipgram] != 1) {
                                	const size_t newskipcontenthash = subskip->hash(); 
									if (newskipcontenthash != lastskipcontenthash[skipgram]) {
        								lastskipcontenthash[skipgram] = 1;
        							} else {
        								lastskipcontenthash[skipgram] = newskipcontenthash;
        							}          	
                                }                             
                                delete subskip;                                     
                                if ((oc == 0) || (oc < MINTOKENS) || ((MINSKIPTOKENS > 1) && (MINSKIPTOKENS >= MINTOKENS) && (oc < MINSKIPTOKENS) ) )  {
                                	//skip content doesn't occur often enough, abort count
                                	docount = false;
                                	break;
                                }
                                cursor = begin+length;                                
                            }   
                            if (docount) { //second check, we might have aborted                                                        
		                        if (skipgrams[skipgram] == 0) foundskipgrams++;                            
		                        skipgrams[skipgram]++; //increase count
		                        //skiptokencount[n]++;                            		                        
                            }
                        }
                        
                        //cleanup
                        for (size_t k = 0; k < subngrams.size(); k++) {
                            delete subngrams[k];
                        }
                    }                    
                }
                
                delete ngram;                 
            }            
            
        };

       cerr << "Found " << foundngrams << " distinct " << n << "-grams"; // (" << tokencount[n] << " tokens)";
       if (DOSKIPGRAMS) {
        cerr << " and " << foundskipgrams << " distinct skipgrams" << endl; // (" << skiptokencount[n] << " tokens)" << endl;
       } else {
        cerr << endl;
       }
       
       if (foundngrams == 0) break;
    
    
       //prune n-grams
       int pruned = 0;
       {
            unordered_map<EncNGram,uint32_t>::iterator iter = ngrams.begin();
            while (iter != ngrams.end()) {       
                if (iter->first.n() == n) {
                    if (iter->second < (unsigned int) MINTOKENS) {
                        pruned++;
                        iter = ngrams.erase(iter);
                    } else {
                        iter++;
                    }                        
                } else {
                    iter++;
                }           
            }
       }
              
       
       
       
       cerr << "Pruned " << pruned << " " << n << "-grams" << endl;  //"(" << tokencount[n] << " tokens)" << endl;
    
       
       if (DOSKIPGRAMS) {       
           //prune skipgrams
           pruned = 0;
           {
                unordered_map<EncSkipGram,uint32_t>::iterator iter = skipgrams.begin();
                while ( iter != skipgrams.end()) {
                   if (iter->first.n() == n) {                          
                        if ( (iter->second < (size_t) MINTOKENS) || (lastskipcontenthash[iter->first] != 1) ) {
                            //skiptokencount[n] -= iter->second;
                            pruned++;
                            iter = skipgrams.erase(iter);
                        } else {
                            iter++;
                        }
                   } else {
                        iter++;
                   }
                }
           }
           cerr << "Pruned " << pruned << " skipgrams" << endl; //(" << skiptokencount[n] << " tokens)" << endl;
           
        }
        
        IN->close();
        delete IN;

    }
    
    computestats();        
}


UnindexedPatternModel::UnindexedPatternModel(const string & filename, const bool DEBUG) {    
    totaltokens = 0;
    MAXLENGTH = 0;
    
    readfile(filename, DEBUG);
    computestats();
}



UnindexedPatternModel::UnindexedPatternModel(const string & corpusfile, UnindexedPatternModel & refmodel, int MAXLENGTH, int MINTOKENS, bool DOSKIPGRAMS, int MINSKIPTOKENS,  int MINSKIPTYPES, bool DOINITIALONLYSKIP, bool DOFINALONLYSKIP) {    
    if (MAXLENGTH > refmodel.getmaxn()) MAXLENGTH = refmodel.getmaxn();
    if (MINTOKENS < refmodel.getminn()) MINTOKENS = refmodel.getminn(); 

    this->MAXLENGTH = MAXLENGTH;
    this->MINTOKENS = MINTOKENS;
    this->DOSKIPGRAMS = DOSKIPGRAMS;
    this->MINSKIPTOKENS = MINSKIPTOKENS;
    this->DOINITIALONLYSKIP = DOINITIALONLYSKIP;
    this->DOFINALONLYSKIP = DOFINALONLYSKIP;
    
    
    totaltokens = 0;
    
    
    int sentence = 0;
    const int BUFFERSIZE = 65536;
    unsigned char line[BUFFERSIZE];
    
    ifstream *IN =  new ifstream( corpusfile.c_str() );
    if (!IN->good()) {
    	cerr << "ERROR: Unable to open file " << corpusfile << endl;
    	exit(5);
    }    
    vector<unsigned int> words;
    while (IN->good()) {
        const int linesize = readline(IN, line, BUFFERSIZE );            
                
        sentence++;

        if (sentence % 10000 == 0) {
            cerr << "\t@" << sentence << endl;
        }
                                
        const int l = countwords(line, linesize);            
        if (l >= 256) {
            cerr << "WARNING: Sentence " << sentence << " exceeds maximum word-length 256, skipping! (" << linesize << " bytes)" << endl;
            continue;
       } else if (l == 0) {
        	cerr << "WARNING: Sentence " << sentence << " contains no words, skipping! (" << linesize << " bytes)" << endl;
            continue;                
        }
        totaltokens += l;
        
		vector<pair<const EncAnyGram*, CorpusReference> > patterns = refmodel.getpatterns(line,linesize, true, sentence,1,MAXLENGTH);
		//cerr << "   " << patterns.size() << " patterns..." << endl;
		for (vector<pair<const EncAnyGram*, CorpusReference> >::iterator iter = patterns.begin(); iter != patterns.end(); iter++) {
			const EncAnyGram * anygram = iter->first;						
			if (getkey(anygram) || refmodel.getkey(anygram)) {		
			    //cerr << "Found pattern" << endl;	
			    if (anygram->isskipgram()) {
			        const EncSkipGram skipgram = *( (const EncSkipGram*) refmodel.getkey(anygram) );
			        skipgrams[skipgram]++;
			    } else {
			        const EncNGram ngram = *( (const EncNGram*) refmodel.getkey(anygram) );
			        ngrams[ngram]++;
			    }
			    //refmodel.outputinstance(anygram, ref, decoder);			
			 }			
		} 
    }
   
   
   
       //prune n-grams
       int pruned = 0;
       {
            unordered_map<EncNGram,uint32_t>::iterator iter = ngrams.begin();
            while (iter != ngrams.end()) {       
                    if (iter->second < (unsigned int) MINTOKENS) {
                        pruned++;
                        iter = ngrams.erase(iter);
                    } else {
                        iter++;
                    }                        
            }
       }
       cerr << "Pruned " << pruned << " " << "n-grams" << endl; 
    
       
       if (DOSKIPGRAMS) {       
           //prune skipgrams
           pruned = 0;
           {
               unordered_map<EncSkipGram,uint32_t>::iterator iter = skipgrams.begin();
               while (iter != skipgrams.end()) {
                        if (iter->second < (unsigned int) MINTOKENS) {
                            pruned++;
                            skipgrams.erase(iter->first);
                        } else {
                            iter++;
                        }
               }
           }
           cerr << "Pruned " << pruned << " skipgrams" << endl;
           
        }
       
        
  
}

void UnindexedPatternModel::readngramdata(std::istream * f, const EncNGram & ngram, int ngramversion, bool ignore ) {
    uint32_t count;
    f->read((char*) &count, sizeof(uint32_t)); //read occurrence count
    if (model_id >= INDEXEDPATTERNMODEL) { //read and ignore references        
        for (unsigned int j = 0; j < count; j++) {
            CorpusReference ref = CorpusReference(f); //read from file (and ignore)
        }
    }
    if (!ignore) {
    	ngrams[ngram] = count;
        if (ngram.n() > MAXLENGTH) MAXLENGTH = ngram.n();
    }    
}



void UnindexedPatternModel::readskipgramdata(std::istream * f, const EncSkipGram & skipgram, int ngramversion, bool ignore) {
    uint32_t count;
    f->read((char*) &count, sizeof(uint32_t)); //read occurrence count
    if (model_id >= INDEXEDPATTERNMODEL) { //read and ignore references
        uint32_t skipcontentcount;
        f->read((char*) &skipcontentcount, sizeof(uint32_t));
        uint32_t occount;   
        for (unsigned int j = 0; j < skipcontentcount; j++) {   
            EncSkipGram skipcontent = EncSkipGram(f, -1, ngramversion);  
            f->read((char*) &occount, sizeof(uint32_t)); //read occurrence count                
            for (unsigned int k = 0; k < occount; k++) {
                CorpusReference ref = CorpusReference(f); //read from file (and ignore)
            }        
        }        
    }
    if (!ignore) {
    	skipgrams[skipgram] = count; //assign
    }            
}

void UnindexedPatternModel::writengramdata(std::ostream * f, const EncNGram & ngram) {
    const uint32_t c = ngrams[ngram];
    f->write( (char*) &c, sizeof(uint32_t) ); //occurrence count                                       
}


void UnindexedPatternModel::writengrams(std::ostream * f) {    
    const unsigned char check = 0xff;
    const char czero = 0;
    for(unordered_map<EncNGram,uint32_t>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++ ) {
    	f->write((char*) &check, sizeof(char)); //check        
        f->write(&czero, sizeof(char)); //gapcount, always zero for ngrams
        iter->first.writeasbinary(f);
        writengramdata(f, iter->first);       
    }   
}


void UnindexedPatternModel::writeskipgramdata(std::ostream * f, const EncSkipGram & skipgram) {
    const uint32_t c  = skipgrams[skipgram];
    f->write( (char*) &c, sizeof(uint32_t) ); //occurrence count                         
}


void UnindexedPatternModel::writeskipgrams(std::ostream * f) {
	const unsigned char check = 0xff;             
    for(unordered_map<EncSkipGram,uint32_t>::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++ ) {
    	f->write((char*) &check, sizeof(char)); //check                               
        iter->first.writeasbinary(f);        
        writeskipgramdata(f, iter->first);
    }     
}

void UnindexedPatternModel::outputinstance(const EncAnyGram * anygram, CorpusReference ref, ClassDecoder & decoder) {
	cout << ref.sentence << ':' << (int) ref.token << "\t" << anygram->decode(decoder) << "\t" << occurrencecount(anygram) << "\t" << setprecision(numeric_limits<double>::digits10 + 1) << coverage(anygram) << endl; 
}



bool UnindexedPatternModel::exists(const EncAnyGram* key) const {    
    if (key->gapcount() == 0) {
        return (ngrams.count(*( (EncNGram*) key) ) > 0);
    } else {
        return (skipgrams.count(*( (EncSkipGram*) key) ) > 0);
    }
    return false;
}

const EncAnyGram* UnindexedPatternModel::getkey(const EncAnyGram* key) {
    if (key->gapcount() == 0) {
        std::unordered_map<EncNGram,uint32_t >::iterator iter = ngrams.find(*( (EncNGram*) key) );
        if (iter != ngrams.end()) {
            return &iter->first;
        } else {
            return NULL;
        }
    } else {
        std::unordered_map<EncSkipGram,uint32_t >::iterator iter = skipgrams.find(*( (EncSkipGram*) key) );
        if (iter != skipgrams.end()) {
            return &iter->first;
        } else {
            return NULL;
        }        
    }
}


int UnindexedPatternModel::occurrencecount(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return ngrams[*( (EncNGram*) key) ];
    } else {
        if (skipgrams.count(*( (EncSkipGram*) key) ) > 0) return skipgrams[*( (EncSkipGram*) key) ];   
    }
    return 0;
}

int UnindexedPatternModel::coveragecount(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return ngrams[*( (EncNGram*) key) ] * key->n();
    } else {
        if (skipgrams.count(*( (EncSkipGram*) key) ) > 0) return skipgrams[*( (EncSkipGram*) key) ] * key->n();   
    }
    return 0;
}



double UnindexedPatternModel::coverage(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return ((double) (ngrams[*( (EncNGram*) key) ] * key->n())  / tokens());
    } else {
        if (skipgrams.count( *( (EncSkipGram*) key)) > 0) return ((double) (skipgrams[ *( (EncSkipGram*) key)] * key->n()) / tokens());
    }
    return 0;
}


/*double UnindexedPatternModel::relfreq(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return (double) ngrams[*( (EncNGram*) key) ] / tokencount[key->n()];
    } else {
        if (skipgrams.count( *( (EncSkipGram*) key)) > 0) return (double) skipgrams[ *( (EncSkipGram*) key)] / skiptokencount[key->n()];
    }
    return 0;
}*/


void UnindexedPatternModel::computestats() {
    totalngramcount = 0;
    totalskipgramcount = 0;
    for (int n = 1; n <= MAXN; n++) { ngramcount[n] = 0;  skipgramcount[n] = 0; ngramtypes[n] = 0; skipgramcount[n] = 0; }

    for (unordered_map<const EncNGram,uint32_t >::const_iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
        const EncAnyGram * anygram = &iter->first;
        ngramtypes[(int) anygram->n()]++;
        ngramcount[(int) anygram->n()] += iter->second;  
        totalngramcount += iter->second;   
    }
    for (unordered_map<const EncSkipGram,uint32_t >::const_iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++) {
        const EncAnyGram * anygram = &iter->first;
        skipgramtypes[(int) anygram->n()]++;
        skipgramcount[(int) anygram->n()] += iter->second;  
        totalskipgramcount += iter->second;
    }    
}

void UnindexedPatternModel::decode(ClassDecoder & classdecoder, ostream *OUT, bool outputhash) {
    //const int grandtotal = ngramtokencount + skipgramtokencount;
    *OUT << "#TYPES=" << types() << ";TOKENS=" << tokens() << endl;
    *OUT << "#";
    if (outputhash) *OUT << "HASH\t"; 
    *OUT << "N\tVALUE\tOCC.COUNT\tTOKENS\tCOVERAGE\tFREQ-ALL\tFREQ-G\fFREQ-N\t" << endl;

    for(unordered_map<EncNGram,uint32_t>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++ ) {
       const int tokens = iter->second * iter->first.n();
       const double cov = (double) tokens / totaltokens;
        
       const double freqall = (double) iter->second / occurrences();
       const double freqg = (double) iter->second / totalngramcount;
       const double freqn = (double) iter->second / ngramcount[(int) iter->first.n()];
       //const double freq1 = (double) iter->second / tokencount[iter->first.n()];       
       //const double freq2 = (double) iter->second / ngramtokencount;
       //const double freq3 = (double) iter->second / grandtotal;
       const EncNGram ngram = iter->first;
        if (outputhash) *OUT << ngram.hash() << '\t';
        *OUT << (int) ngram.n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << ngram.decode(classdecoder) << '\t' << iter->second << '\t' << tokens << '\t' << cov << '\t' << freqall << '\t' << freqg << '\t' << freqn;
        *OUT << endl;
    }
   

   
       for(unordered_map<EncSkipGram,uint32_t>::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++ ) {
            const int tokens = iter->second * iter->first.n();
            const double cov = (double) tokens / totaltokens;
           
           
            const double freqall = (double) iter->second / occurrences();
            const double freqg = (double) iter->second / totalskipgramcount;
            const double freqn = (double) iter->second / skipgramcount[(int) iter->first.n()];
           //const double freq1 = (double) iter->second / skiptokencount[iter->first.n()]; 
           //const double freq2 = (double) iter->second / skipgramtokencount;           
           //const double freq3 = (double) iter->second / grandtotal;                          
           const EncSkipGram skipgram = iter->first; 
           if (outputhash) *OUT << skipgram.hash() << '\t';                             
           *OUT << (int) skipgram.n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << skipgram.decode(classdecoder) << '\t' << iter->second << '\t' << tokens << cov << '\t' << freqall << '\t' << freqg << '\t' << freqn;          
           *OUT << endl;
       }
    

}


void UnindexedPatternModel::decode(UnindexedPatternModel & testmodel,  ClassDecoder & classdecoder, std::ostream *OUT, bool outputhash) {
    //const int grandtotal = ngramtokencount + skipgramtokencount;
    *OUT << "#TYPES=" << types() << ";TOKENS=" << tokens() << ";TYPES(2)=" << testmodel.types() << ";TOKENS(2)=" << testmodel.tokens() << endl;
    *OUT << "#";
    if (outputhash) *OUT << "HASH\t"; 
    *OUT << "N\tVALUE\tOCC.COUNT\tTOKENS\tCOVERAGE\tCOUNT-TESTMODEL\tCOVERAGE-TESTMODEL" << endl;    
    for(unordered_map<EncNGram,uint32_t>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++ ) {
       const double freq = ((double) (iter->second * iter->first.n()) / totaltokens);
       //const double freq1 = (double) iter->second / tokencount[iter->first.n()];       
       //const double freq2 = (double) iter->second / ngramtokencount;
       //const double freq3 = (double) iter->second / grandtotal;
       const EncNGram ngram = iter->first;
        if (outputhash) *OUT << ngram.hash() << '\t';
        *OUT << (int) ngram.n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << ngram.decode(classdecoder) << '\t' << iter->second << '\t' << freq;
        
        const EncAnyGram * key = testmodel.getkey((const EncAnyGram *) &iter->first);
        if (key) {
            *OUT << "\t" << testmodel.occurrencecount(key) << "\t" << testmodel.coveragecount(key)  << testmodel.occurrencecount(key) << testmodel.coverage(key); 
        } else {
            *OUT << "\t0\t0";
        }
        *OUT << endl;
    }
   

   
       for(unordered_map<EncSkipGram,uint32_t>::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++ ) {
           const double freq = ((double) (iter->second * iter->first.n()) / totaltokens);
           //const double freq1 = (double) iter->second / skiptokencount[iter->first.n()]; 
           //const double freq2 = (double) iter->second / skipgramtokencount;           
           //const double freq3 = (double) iter->second / grandtotal;                          
           const EncSkipGram skipgram = iter->first;
           if (outputhash) *OUT << skipgram.hash() << '\t';                              
           *OUT << (int) skipgram.n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << skipgram.decode(classdecoder) << '\t' << iter->second << '\t' << freq;
            const EncAnyGram * key = testmodel.getkey((const EncAnyGram *) &iter->first);
            if (key) {
                *OUT << "\t" << testmodel.occurrencecount(key) << "\t" << testmodel.coveragecount(key)  << testmodel.occurrencecount(key) << testmodel.coverage(key);
            } else {
                *OUT << "\t0\t0";
            }                     
           *OUT << endl;
       }  
}



void IndexedPatternModel::report(std::ostream *OUT) {

    const int totalcount =  totalngramcount+totalskipgramcount;  
    *OUT << setiosflags(ios::fixed) << setprecision(4) << endl;       
    *OUT << "REPORT" << endl;
    *OUT << "----------------------------------" << endl;
    *OUT << "                          " << setw(10) << "TOKENS" << setw(10) << "TYPES" << setw(10) << endl;
    *OUT << "Total:                    " << setw(10) << totalcount << setw(10) << ngrams.size() + skipgrams.size() <<  endl;
    
    
    *OUT << "N-grams:                  " << setw(10) << totalngramcount << setw(10) << ngrams.size() <<  endl;
    for (int n = 1; n <= MAXN; n++) {
        *OUT << " n=" << n << "                       "  << setw(10) << ngramcount[n] << setw(10) << ngramtypes[n] << endl; 
    }
    
    *OUT << "Skipgrams:                " << setw(10) << totalskipgramcount << setw(10) << skipgrams.size() << endl;
    for (int n = 1; n <= MAXN; n++) {
        *OUT << " n=" << n << "                       "  << setw(10) << skipgramcount[n] << setw(10) << skipgramtypes[n] << endl; 
    }                

}

void UnindexedPatternModel::report(std::ostream *OUT) {

    const int totalcount =  totalngramcount+totalskipgramcount;  
    *OUT << setiosflags(ios::fixed) << setprecision(4) << endl;       
    *OUT << "REPORT" << endl;
    *OUT << "----------------------------------" << endl;
    *OUT << "                          " << setw(10) << "TOKENS" << setw(10) << "TYPES" << setw(10) << endl;
    *OUT << "Total:                    " << setw(10) << totalcount << setw(10) << ngrams.size() + skipgrams.size() <<  endl;
    
    
    *OUT << "N-grams:                  " << setw(10) << totalngramcount << setw(10) << ngrams.size() <<  endl;
    for (int n = 1; n <= MAXN; n++) {
        *OUT << " n=" << n << "                       "  << setw(10) << ngramcount[n] << setw(10) << ngramtypes[n] << endl; 
    }
    
    *OUT << "Skipgrams:                " << setw(10) << totalskipgramcount << setw(10) << skipgrams.size() << endl;
    for (int n = 1; n <= MAXN; n++) {
        *OUT << " n=" << n << "                       "  << setw(10) << skipgramcount[n] << setw(10) << skipgramtypes[n] << endl; 
    }                

}

double SkipGramData::entropy() const {
    double entropy = 0;
    for(unordered_map<EncSkipGram,NGramData>::const_iterator iter = skipcontent.begin(); iter != skipcontent.end(); iter++ ) {
      double p = iter->second.count() / (double) _count;
      entropy += p * log2(p);
    }    
    return -1 * entropy;
}


std::set<CorpusReference> SkipGramData::get_refs() const {
    std::set<CorpusReference> s;
    for (std::unordered_map<EncSkipGram,NGramData>::const_iterator iter = skipcontent.begin(); iter != skipcontent.end(); iter++) {
        for (std::set<CorpusReference>::const_iterator iter2 = iter->second.refs.begin(); iter2 != iter->second.refs.end(); iter2++) {
            s.insert(*iter2);
        }
    }
    return s;
}




int intersection( set<CorpusReference> & a, set<CorpusReference> & b) {    
    //Jaccard co-occurrence    
    int intersectioncount = 0;        
    set<CorpusReference>::iterator iter_a = a.begin();    
    set<CorpusReference>::iterator iter_b = b.begin();    
    while ((iter_a !=a.end()) && (iter_b!=b.end())) {
        if (*iter_a < *iter_b) { 
            iter_a++;
        } else if (*iter_b < *iter_a) {
            iter_b++;
        } else {  //equal
            intersectioncount++;
            iter_a++;
            iter_b++;
        }
    }
    return intersectioncount;
}



int intersection( set<ExtCorpusReference> & a, set<ExtCorpusReference> & b) {    
    //Jaccard co-occurrence    
    int intersectioncount = 0;        
    set<ExtCorpusReference>::iterator iter_a = a.begin();    
    set<ExtCorpusReference>::iterator iter_b = b.begin();
    while ((iter_a !=a.end()) && (iter_b!=b.end())) {    
        if (iter_a->contains(*iter_b)) {
            intersectioncount++;
            iter_b++;
        } else if (*iter_b > *iter_a) {
            iter_a++;
        } else if (*iter_b < *iter_a) {
            iter_b++;
        } 
    }
    return intersectioncount;
}


GraphPatternModel::GraphPatternModel(IndexedPatternModel * model, const GraphFilter & filter) {
    this->model = model;
    model->model_id = GRAPHPATTERNMODEL+GRAPHPATTERNMODELVERSION;
    applyfilter(filter);
    DELETEMODEL = false;
    
    unordered_map<uint32_t,multimap<unsigned char,EncAnyGram*> > reverseindex; // sentence => token => anygram
    unordered_map<const EncAnyGram*, int> sentencecount; 
    
    if (DOSUCCESSORS || DOPREDECESSORS || DOCOOCCURRENCE) {
        cerr << "Computing reverse index" << endl;
        int rindex_added = 0;
        for(unordered_map<EncNGram,NGramData >::iterator iter = model->ngrams.begin(); iter != model->ngrams.end(); iter++ ) {
            EncAnyGram * ngram = ( EncAnyGram * ) &(iter->first);
            int prevsentence = 0;
            for (set<CorpusReference>::iterator iter2 = iter->second.refs.begin(); iter2 != iter->second.refs.end(); iter2++) {
                CorpusReference ref = *iter2;                                
                if (ref.sentence != prevsentence) sentencecount[ngram] += 1;
                reverseindex[ref.sentence].insert(pair<unsigned char, EncAnyGram*>(ref.token, ngram) );
                rindex_added++;
                prevsentence = ref.sentence;
            }
        }
        for(unordered_map<EncSkipGram,SkipGramData >::iterator iter = model->skipgrams.begin(); iter != model->skipgrams.end(); iter++ ) {
            EncAnyGram * skipgram = ( EncAnyGram * ) &(iter->first);
            set<CorpusReference> refs = iter->second.get_refs();
            for (set<CorpusReference>::iterator iter2 = refs.begin(); iter2 != refs.end(); iter2++) {
                CorpusReference ref = *iter2;                                
                reverseindex[ref.sentence].insert(pair<unsigned char, EncAnyGram*>(ref.token, skipgram) );
                rindex_added++;
            }
        }        
        cerr << "Reverse index contains " << rindex_added << " patterns in " << reverseindex.size() << " sentences" << endl;
    }
    
    if (DOCOOCCURRENCE) {
        cerr << "Computing co-occurrence relations, threshold " << COOCTHRESHOLD << endl;
        for (unordered_map<uint32_t,multimap<unsigned char, EncAnyGram*> >::iterator iter = reverseindex.begin(); iter != reverseindex.end(); iter++) {
            unordered_map<const EncAnyGram *, unordered_map<const EncAnyGram*, int > > cooc_sentence;
            for (multimap<unsigned char, EncAnyGram *>::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
                const EncAnyGram * pivot = iter2->second;
                if (sentencecount[pivot] > COOCTHRESHOLD) {
                    unsigned char pivottoken = iter2->first;
                    multimap<unsigned char, EncAnyGram *>::iterator begin;
                    if (!BIDIRECTIONALCOOC) {
                        begin = iter->second.lower_bound(pivottoken);
                    } else {
                        begin = iter->second.begin();
                    }
                    for (multimap<unsigned char, EncAnyGram *>::iterator iter3 = begin; iter3 != iter->second.end(); iter3++) {
                        const EncAnyGram * neighbour= iter3->second;
                        int neighbourtoken = iter3->first;
                        if ((pivot != neighbour) && (sentencecount[neighbour] > COOCTHRESHOLD)) {
                            cooc_sentence[pivot][neighbour] += 1;
                        }
                    }
                }
            }
            for (unordered_map<const EncAnyGram *, unordered_map<const EncAnyGram*, int > >::iterator iter2 = cooc_sentence.begin(); iter2 != cooc_sentence.end(); iter2++) {
                const EncAnyGram * pivot = iter2->first;
                for (unordered_map<const EncAnyGram*,int>::iterator iter3 = iter2->second.begin(); iter3 != iter2->second.end(); iter3++) {
                    const EncAnyGram * neighbour = iter3->first;
                    rel_cooccurrences[pivot][neighbour] += iter3->second;
                }
            }
        }
    }

    cerr << "Computing relations on n-grams" << endl;
    for(std::unordered_map<EncNGram,NGramData >::iterator iter = model->ngrams.begin(); iter != model->ngrams.end(); iter++ ) {

        const EncNGram * ngram = &(iter->first);
        vector<EncNGram*> subngrams;
        ngram->subngrams(subngrams);
        //cerr << "DEBUG: n2" << endl;
        for (vector<EncNGram*>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {                
            const EncAnyGram * subngram = model->getkey(*iter2);
            if (subngram != NULL) {
                //subgram exists, add relation:
                if (DOCHILDREN) rel_subsumption_children[ngram].insert(subngram);
                                    
                //reverse:
                if ((DOPARENTS) || (DOXCOUNT)) rel_subsumption_parents[subngram].insert(ngram);
            }
            delete *iter2;
        }   
        //cerr << "DEBUG: n3" << endl;
        //
        //
        
        if ((DOSUCCESSORS) || (DOPREDECESSORS) || (DOCOOCCURRENCE)) {
    
            for (set<CorpusReference>::iterator instanceiter = iter->second.refs.begin() ; instanceiter != iter->second.refs.end() ; instanceiter++) {
                CorpusReference ref = *instanceiter;

                
                if (DOSUCCESSORS) {
                    if (ref.token + ngram->n() <= 255) {
                        const unsigned char begintoken = ref.token + ngram->n();
                        if (reverseindex.count(ref.sentence) > 0) {
                            multimap<unsigned char, EncAnyGram*> * reverseindex_tokens =  &reverseindex[ref.sentence];                    
                            for (multimap<unsigned char, EncAnyGram*>::iterator iter2 = reverseindex_tokens->lower_bound(begintoken); iter2 != reverseindex_tokens->upper_bound(begintoken); iter2++) {
                                const EncAnyGram * successor = iter2->second;
                                rel_successors[(const EncAnyGram *) ngram][(const EncAnyGram *) successor] += 1;
                            }
                        }                
                     }        
                }
                if (DOPREDECESSORS) {            
                    if (ref.token > 0) {
                        //different begin points based on length!
                        for (int l = 1; l <= ref.token && l <= MAXN; l++) {
                            const unsigned char begintoken = ref.token - l;
                            if ((reverseindex.count(ref.sentence) > 0) && (begintoken >=0)) {
                                multimap<unsigned char, EncAnyGram*> * reverseindex_tokens =  &reverseindex[ref.sentence];                    
                                for (multimap<unsigned char, EncAnyGram*>::iterator iter2 = reverseindex_tokens->lower_bound(begintoken); iter2 != reverseindex_tokens->upper_bound(begintoken); iter2++) {
                                    const EncAnyGram * predecessor = iter2->second;
                                    if (predecessor->n() == l) rel_predecessors[(const EncAnyGram *) ngram][(const EncAnyGram *) predecessor] += 1;
                                }
                            }                                             
                        }
                     }        
                }
                
                
            }
        
        }

        
    }
    
    
   
    

    cerr << "Computing relations on skip-grams" << endl;
    
    for(std::unordered_map<EncSkipGram,SkipGramData >::iterator iter = model->skipgrams.begin(); iter != model->skipgrams.end(); iter++ ) {
    	//cerr << "DEBUG: s1" << endl;        
        const EncSkipGram * skipgram = &(iter->first);
        vector<EncNGram*> parts;
        skipgram->parts(parts);
        //cerr << "DEBUG: s1.5" << endl;        
        for (vector<EncNGram*>::iterator iter2 = parts.begin(); iter2 != parts.end(); iter2++) {
            const EncNGram * ngram = *iter2;
            const EncAnyGram * partgram = model->getkey(*iter2);
            if (partgram != NULL) {
                    if (DOCHILDREN) rel_subsumption_children[skipgram].insert(partgram);
                    if ((DOPARENTS) || (DOXCOUNT)) rel_subsumption_parents[partgram].insert(skipgram);
            }
            vector<EncNGram*> subngrams;
            //cerr << "DEBUG: s1.6" << endl;
            ngram->subngrams(subngrams);
            //cerr << "DEBUG: s1.7" << endl;
            for (vector<EncNGram*>::iterator iter3 = subngrams.begin(); iter3 != subngrams.end(); iter3++) {
                //subgram exists, add relation:
                const EncAnyGram * subngram = model->getkey(*iter3);
                if (subngram != NULL) {
                    if (DOCHILDREN) rel_subsumption_children[skipgram].insert(subngram);
                     
                    if ((DOPARENTS) || (DOXCOUNT)) rel_subsumption_parents[subngram].insert(skipgram);
                }
                delete *iter3;
            }
            delete ngram;
        }          
        
        if ((DOSUCCESSORS) || (DOPREDECESSORS) || (DOCOOCCURRENCE)) {
            set<CorpusReference> refs = iter->second.get_refs();
            for (set<CorpusReference>::iterator instanceiter = refs.begin() ; instanceiter != refs.end() ; instanceiter++) {
                CorpusReference ref = *instanceiter;

                if (DOSUCCESSORS) {            
                    if (ref.token + skipgram->n() <= 255) {
                        const unsigned char begintoken = ref.token + skipgram->n();
                        if (reverseindex.count(ref.sentence) > 0) {
                            multimap<unsigned char, EncAnyGram*> * reverseindex_tokens =  &reverseindex[ref.sentence];                    
                            for (multimap<unsigned char, EncAnyGram*>::iterator iter2 = reverseindex_tokens->lower_bound(begintoken); iter2 != reverseindex_tokens->upper_bound(begintoken); iter2++) {
                                const EncAnyGram * successor = iter2->second;
                                rel_successors[(const EncAnyGram *) skipgram][(const EncAnyGram *) successor] += 1;
                            }
                        }                
                     }        
                }
                if (DOPREDECESSORS) {            
                    if (ref.token > 0) {
                        //different begin points based on length!
                        for (int l = 1; l <= ref.token && l <= MAXN; l++) {
                            const unsigned char begintoken = ref.token - l;
                            if ((reverseindex.count(ref.sentence) > 0) && (begintoken >=0)) {
                                multimap<unsigned char, EncAnyGram*> * reverseindex_tokens =  &reverseindex[ref.sentence];                    
                                for (multimap<unsigned char, EncAnyGram*>::iterator iter2 = reverseindex_tokens->lower_bound(begintoken); iter2 != reverseindex_tokens->upper_bound(begintoken); iter2++) {
                                    const EncAnyGram * predecessor = iter2->second;
                                    if (predecessor->n() == l) rel_predecessors[(const EncAnyGram *) skipgram][(const EncAnyGram *) predecessor] += 1;
                                }
                            }                                             
                        }
                     }        
                }
          }
      
        }
        
        
		//cerr << "DEBUG: s2" << endl;
        if ((DOSKIPCONTENT) || (DOSKIPUSAGE) || (DOINSTANCES) || (DOTEMPLATES)) {
		    for (unordered_map<EncSkipGram,NGramData>::iterator iter2 = iter->second.skipcontent.begin(); iter2 != iter->second.skipcontent.end(); iter2++) {
		    	const EncSkipGram * skipgram_skipcontent = &(iter2->first);
		    	vector<EncNGram*> contentparts;
		    	skipgram_skipcontent->parts(contentparts);
		    	if ((DOINSTANCES || DOTEMPLATES) && (contentparts.size() > 0)) {
		    		if (contentparts.size() == (unsigned int) skipgram->skipcount) {
						const EncNGram instancengram = skipgram->instantiate(skipgram_skipcontent, contentparts);					
						const EncAnyGram * instance = model->getkey(&instancengram);
						if (instance != NULL) {
							if (DOINSTANCES) rel_instances[skipgram].insert(instance);
							if (DOTEMPLATES) rel_templates[instance].insert(skipgram);
						}
					} else {
						cerr << "WARNING: unable to reconsole skip content of skipgram " << skipgram->hash() << ", skipping..." << endl;
					}				
				}
		    	
		    	if (DOSKIPCONTENT || DOSKIPUSAGE) {
		    		const EncAnyGram * inverseskipgram = model->getkey(skipgram_skipcontent);
					if (inverseskipgram != NULL) {
						if (DOSKIPCONTENT) rel_skipcontent[skipgram][inverseskipgram]++;
						if (DOSKIPUSAGE) rel_skipusage[inverseskipgram][skipgram]++;
					}		    				
					for (vector<EncNGram*>::iterator iter3 = contentparts.begin(); iter3 != contentparts.end(); iter3++) {
						//subgram exists, add relation:
						const EncAnyGram * subngram = model->getkey(*iter3);
						if (subngram != NULL) {
						    if (DOSKIPCONTENT) rel_skipcontent[skipgram][subngram]++;
						    if (DOSKIPUSAGE) rel_skipusage[subngram][skipgram]++;
						}
						delete *iter3;
					}
				}          
		    }
		    
		    
		    
		}
		//cerr << "DEBUG: s3" <<endl;
        
    }


    if (COOCTHRESHOLD > 0) {
        cerr << "Pruning cooccurrence relations below threshold" << endl;
        int coocpruned = 0;
        t_weightedrelations::iterator iter2 = rel_cooccurrences.begin();
        while (iter2 != rel_cooccurrences.end()) {
            unordered_map<const EncAnyGram *,double>::iterator iter3 = iter2->second.begin();
            while (iter3 != iter2->second.end()) {
                if (iter3->second < COOCTHRESHOLD) { 
                    iter3 = iter2->second.erase(iter3);
                    coocpruned++;
                } else {
                    iter3++;
                }
            }
            if (iter2->second.size() == 0) {
                iter2 = rel_cooccurrences.erase(iter2);
            } else {
                iter2++;
            }
        }
        cerr << "Pruned " << coocpruned << endl;
    }


    //find template<->instance relations between skipgrams, based on templates
    if (DOTEMPLATES) {
        //cerr << rel_templates.size() << endl;
        //int c = 0;
        int found = 0;
        for (unordered_map<const EncAnyGram*,unordered_set<const EncAnyGram*> >::iterator iter2 = rel_templates.begin(); iter2 != rel_templates.end(); iter2++) {
           // c++;
          //  cerr << " @" << c << " " << iter2->second.size() << endl;		            
            const EncAnyGram * instance = iter2->first;
            if (!instance->isskipgram()) {
                int i = 0;		            
                for (unordered_set<const EncAnyGram*>::iterator iter3 = iter2->second.begin(); iter3 !=  iter2->second.end(); iter3++) {
                    const EncAnyGram * tmplate = *iter3;
                    i++;
                    int j = 0;
                    for (unordered_set<const EncAnyGram*>::iterator iter4 = iter2->second.begin(); iter4 !=  iter2->second.end(); iter4++) {
                        j++;		                    
                        if (j > i) { //keep complexity down to O(log n)
                            const EncAnyGram * tmplate2 = *iter4;
                            //how do the two templates relate?
                            int rel = ((const EncSkipGram *) tmplate)->instancetemplaterelation( (const EncSkipGram *) tmplate2 );
                            if (rel == 1) {
                                //tmplate is instance of tmplate2, tmplate2 template of tmplate
                                rel_templates[tmplate].insert(tmplate2);
                                rel_instances[tmplate2].insert(tmplate);
                                found++;                                
                            } else if (rel == -1) {
                                //tmplate2 is instance of tmplate, tmplate template of tmplate2
                                rel_templates[tmplate2].insert(tmplate);
                                rel_instances[tmplate].insert(tmplate2);
                                found++;                            
                            }                    
                        }
                    }	                
                }
            }
        }
        cerr << "FOUND: " << found << endl;
    } else if (DOINSTANCES) {
        //based on instances: TODO
        cerr << "WARNING: If you also want instance relations between skipgrams, enable computation of template relations in the graph.. Will be skipped now!!!" << endl;
    }



    if (DOXCOUNT) {
        cerr << "Computing exclusive count" << endl;
        
        for(std::unordered_map<EncNGram,NGramData >::iterator iter = model->ngrams.begin(); iter != model->ngrams.end(); iter++ ) {
            const EncAnyGram * anygram = (const EncAnyGram *) &(iter->first);
            if (rel_subsumption_parents.count(anygram)) {
                data_xcount[anygram] = computexcount(anygram);
            } else {
                data_xcount[anygram] =  iter->second.count();
            } 
        }
        for(std::unordered_map<EncSkipGram,SkipGramData >::iterator iter = model->skipgrams.begin(); iter != model->skipgrams.end(); iter++ ) {
            const EncAnyGram * anygram = (const EncAnyGram *) &(iter->first);
            if (rel_subsumption_parents.count(anygram)) {
                data_xcount[anygram] = computexcount(anygram);
            } else {
                data_xcount[anygram] =  iter->second.count();
            } 
        }        
        
        /*
        for (std::unordered_map<const EncAnyGram*,std::unordered_set<const EncAnyGram*> >::const_iterator iter = rel_subsumption_parents.begin(); iter != rel_subsumption_parents.end(); iter++) {
            const EncAnyGram* anygram = iter->first;
            data_xcount[anygram] = computexcount(anygram);
        } 
        */       
        if (!DOPARENTS) rel_subsumption_parents.clear();
    }
}




void GraphPatternModel::stats(std::ostream *OUT) {
	cerr << "Graphmodel contains " << model->types() << " types, " << model->tokens() << " tokens" << endl;
    if (DOCHILDREN) *OUT << " Child subsumption relations: " << rel_subsumption_children.size() << endl;
    if (DOPARENTS) *OUT << " Parent subsumption relations: " << rel_subsumption_parents.size() << endl;
    if (DOSUCCESSORS) *OUT << " Successors found for " << rel_successors.size() << " patterns" << endl;
    if (DOPREDECESSORS) *OUT << " Predecessors found for " << rel_predecessors.size() << " patterns" << endl;
    if (DOCOOCCURRENCE) *OUT << " Co-occurrences found for " << rel_cooccurrences.size() << " patterns" << endl;
    if (DOSKIPCONTENT)  *OUT << " Content-relations found for " <<  rel_skipcontent.size() << " skipgrams" << endl;
    if (DOSKIPUSAGE)  *OUT << " Skipgram parents found for " <<  rel_skipusage.size() << " n-grams" << endl;
    if (DOTEMPLATES)  *OUT << " Templates found for " <<  rel_templates.size() << " patterns" << endl;
    if (DOINSTANCES)  *OUT << " Instances found for " <<  rel_instances.size() << " skipgrams" << endl;
    if (DOXCOUNT) *OUT << " Exclusive count: " << data_xcount.size() << endl;
 
}

int transitivereduction(std::unordered_map<const EncAnyGram*,std::unordered_set<const EncAnyGram*> > & relations ) {
	int pruned = 0;
	//compute transitive closure by pruning
	for (std::unordered_map<const EncAnyGram*,std::unordered_set<const EncAnyGram*> >::iterator iter = relations.begin(); iter != relations.end(); iter++) {
		const EncAnyGram * node = iter->first;		
		if (relations.count(node) > 0) {
			for (std::unordered_set<const EncAnyGram*>::iterator parentiter = relations[node].begin(); parentiter != relations[node].end(); parentiter++) {
				const EncAnyGram * parent = *parentiter;
				if (relations.count(parent) > 0) {
					for (std::unordered_set<const EncAnyGram*>::iterator grandparentiter = relations[parent].begin(); grandparentiter != relations[parent].end(); grandparentiter++) {
						const EncAnyGram * grandparent = *grandparentiter;
						if (relations[node].count(grandparent) > 0) {
							relations[node].erase(grandparent);
							pruned++;
						}						
					}
				} 
			}
		}
	}
	return pruned;
}

int GraphPatternModel::transitivereduction() {
	int pruned = 0;
	if (DOCHILDREN) pruned += ::transitivereduction(rel_subsumption_children);
    if (DOPARENTS) pruned += ::transitivereduction(rel_subsumption_parents);
    return pruned;	
}

int GraphPatternModel::computexcount(const EncAnyGram* anygram) {
    const AnyGramData* data = model->getdata(anygram);
    if (data == NULL) throw "xcount: No such anygram";
    
    
    //compute union of all parent references
    set<ExtCorpusReference> allparentrefs;
    for (std::unordered_set<const EncAnyGram*>::iterator iter = rel_subsumption_parents[anygram].begin(); iter != rel_subsumption_parents[anygram].end(); iter++) {
        const EncAnyGram * parentgram = *iter;
        const AnyGramData * parentdata = model->getdata(parentgram);
        if (parentdata != NULL) {
            set<CorpusReference> parentrefs = parentdata->get_refs();
            for (set<CorpusReference>::iterator iter2 = parentrefs.begin(); iter2 != parentrefs.end(); iter2++) {
                CorpusReference r = *iter2;
                allparentrefs.insert( ExtCorpusReference(r,parentgram->n()) );
            }
        }        
    }
   
   
   
   set<CorpusReference> allrefs = data->get_refs();

   
   set<ExtCorpusReference> allrefs_ext;
   for (set<CorpusReference>::iterator iter = allrefs.begin(); iter != allrefs.end(); iter++) {
        allrefs_ext.insert(ExtCorpusReference(*iter,anygram->n()));
        //cerr << "ALLREFS: " <<  iter->sentence << ':' << (int) iter->token << ':' << (int) anygram->n() << endl;
   }
   /*for (set<ExtCorpusReference>::iterator iter = allparentrefs.begin(); iter != allparentrefs.end(); iter++) {
        cerr << "PARREFS: " <<  iter->sentence << ':' << (int) iter->token << ':' << (int) iter->n << endl;
   }*/
   
   //const int i = intersection(allparentrefs,allrefs_ext);
   //cerr << "INTERSECTION: " << i << endl;
   //cerr << "XCOUNT: " << allrefs_ext.size() - i << endl << endl;
   //compute: union - intersection
   return allrefs_ext.size() - intersection(allparentrefs,allrefs_ext);;
}


/*void GraphPatternModel::readrelations(std::istream * in, const EncAnyGram * anygram, std::unordered_map<const EncAnyGram*,std::unordered_set<const EncAnyGram*> > & relationhash, bool ignore) {
    uint32_t count;
    in->read((char*) &count,  sizeof(uint32_t));
    char gapcount;    
    
    const EncAnyGram * key = model->getkey(anygram);  
    if (!key) {
		cerr << "INTERNAL WARNING: Anygram not found in model: ";
		anygram->out();
		cerr << endl;    	
    }
    for (int i = 0; i < count; i++) {                        
       in->read(&gapcount, sizeof(char));
       if (gapcount == 0) {
        EncNGram ngram = EncNGram(in);        
        if ((!ignore) && (secondpass)) {
        	const EncAnyGram * key2 = model->getkey((EncAnyGram*) &ngram);
        	if (!key2) {
				cerr << "INTERNAL WARNING: Relation target ngram not found in model: ";
				ngram.out();
				cerr << endl;    	        		
        	} else if (key) {        	 
        	  relationhash[key].insert(key2);
        	}
        }
       } else {
        EncSkipGram skipgram = EncSkipGram( in, gapcount);
        if ((!ignore) && (secondpass)) {
        	const EncAnyGram * key2 = model->getkey((EncAnyGram*) &skipgram);
        	if (!key2) {
				cerr << "INTERNAL WARNING: Relation target skipgram not found in model: ";
				skipgram.out();
				cerr << endl;    	        		
        	} else if (key) {        	 
        	  relationhash[key].insert(key2);
        	}
        }
       }           
    }    
}*/

void GraphPatternModel::writerelations(std::ostream * out,const EncAnyGram * anygram, t_relations & relationhash) {
	unordered_set<const EncAnyGram*> * relations = &relationhash[model->getkey(anygram)];
		
    uint32_t count = (uint32_t) relations->size();
    out->write((char*) &count, sizeof(uint32_t));
    unsigned int i = 0;                           
    for (unordered_set<const EncAnyGram*>::iterator iter = relations->begin(); iter != relations->end(); iter++) {
    	i++;
		model->writeanygram(*iter, out);
    }
    //sanity check:
    if (i != count) {
    	cerr << "INTERNAL ERROR: GraphPatternModel::writerelations: Sanity check failed, wrote " << i << " constructions instead of expected " << count << ". uint32 overflow?" << endl;
    	cerr << "DEBUG: relations.size() == " << relations->size() << endl;        	
    	exit(13);
    }    
}



void GraphPatternModel::writerelations(std::ostream * out,const EncAnyGram * anygram, t_weightedrelations & relationhash) {
	unordered_map<const EncAnyGram*, double> * relations = &relationhash[model->getkey(anygram)];
		
    uint32_t count = (uint32_t) relations->size();
    out->write((char*) &count, sizeof(uint32_t));
    unsigned int i = 0;                           
    for (unordered_map<const EncAnyGram*, double>::iterator iter = relations->begin(); iter != relations->end(); iter++) {
    	i++;
		model->writeanygram(iter->first, out);
		const double weight = iter->second;
		out->write((char*) &weight, sizeof(double));		
    }
    //sanity check:
    if (i != count) {
    	cerr << "INTERNAL ERROR: GraphPatternModel::writerelations: Sanity check failed, wrote " << i << " constructions instead of expected " << count << ". uint32 overflow?" << endl;
    	cerr << "DEBUG: relations.size() == " << relations->size() << endl;        	
    	exit(13);
    }    
}



void GraphPatternModel::readheader(std::istream * in, bool ignore) {
	if ((model_id >= GRAPHPATTERNMODEL) && (model_id <= GRAPHPATTERNMODEL+GRAPHPATTERNMODELVERSION)) {
		if (DEBUG) cerr << "READING GRAPHMODEL HEADER " << endl; 
    	in->read((char*) &HASPARENTS,  sizeof(bool)); //1 byte, not 1 bit
	    in->read((char*) &HASCHILDREN, sizeof(bool)); //1 byte, not 1 bit
	    in->read((char*) &HASXCOUNT, sizeof(bool)); //1 byte, not 1 bit
		if (model_id >= GRAPHPATTERNMODEL+1 ) {
			if (DEBUG) cerr << "READING EXTENDED GRAPHMODEL HEADER (v1)" << endl;
			in->read((char*) &HASTEMPLATES, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASINSTANCES, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASSKIPUSAGE, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASSKIPCONTENT, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASSUCCESSORS, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASPREDECESSORS, sizeof(bool)); //1 byte, not 1 bit
			if (model_id >= GRAPHPATTERNMODEL+3 ) in->read((char*) &HASCOOCCURRENCE, sizeof(bool)); //1 byte, not 1 bit
    	}
	} else {
		if (DEBUG) cerr << "NOT A GRAPHMODEL HEADER" << endl;
		HASPARENTS = false;
		HASCHILDREN = false;
		HASXCOUNT = false;
		HASTEMPLATES = false;
		HASINSTANCES = false;
		HASSKIPUSAGE = false;
		HASSKIPCONTENT = false;
		HASSUCCESSORS = false;
		HASPREDECESSORS = false;
		HASCOOCCURRENCE = false;
	}	
}

void GraphPatternModel::writeheader(std::ostream * out) {
    out->write((char*) &DOPARENTS,  sizeof(bool)); //1 byte, not 1 bit
    out->write((char*) &DOCHILDREN, sizeof(bool)); //1 byte, not 1 bit
    out->write((char*) &DOXCOUNT, sizeof(bool)); //1 byte, not 1 bit
    out->write((char*) &DOTEMPLATES, sizeof(bool)); //1 byte, not 1 bit
    out->write((char*) &DOINSTANCES, sizeof(bool)); //1 byte, not 1 bit
    out->write((char*) &DOSKIPUSAGE, sizeof(bool)); //1 byte, not 1 bit
    out->write((char*) &DOSKIPCONTENT, sizeof(bool)); //1 byte, not 1 bit
    out->write((char*) &DOSUCCESSORS, sizeof(bool)); //1 byte, not 1 bit
    out->write((char*) &DOPREDECESSORS, sizeof(bool)); //1 byte, not 1 bit
    out->write((char*) &DOCOOCCURRENCE, sizeof(bool)); //1 byte, not 1 bit
}

void GraphPatternModel::readngramdata(std::istream * in, const EncNGram & ngram, int ngramversion, bool ignore) {
	model->readngramdata(in, ngram, ignore || secondpass);
    if (HASXCOUNT) {
        uint32_t _xcount;
        in->read((char*) &_xcount, sizeof(uint32_t));
        if ((DOXCOUNT) && (secondpass) && (!ignore)) {
        	const EncAnyGram * key = model->getkey((const EncAnyGram*) &ngram);
        	if (key) {
        		data_xcount[key] = _xcount; //BUG BUG BUG!!
        	} else {
        		cerr << "INTERNAL WARNING: Ngram key not found in model: ";
        		ngram.out();
        		cerr << " skipping..." << endl;
        	}
        } 
    }
    if (HASPARENTS) readrelations(in, (const EncAnyGram*) &ngram, &rel_subsumption_parents, ngramversion, (ignore || !secondpass || !DOPARENTS));
    if (HASCHILDREN) readrelations(in, (const EncAnyGram*) &ngram, &rel_subsumption_children, ngramversion, (ignore || !secondpass || !DOCHILDREN));
    if (HASTEMPLATES) readrelations(in, (const EncAnyGram*) &ngram, &rel_templates, ngramversion, (ignore || !secondpass || !DOTEMPLATES));
    if (HASINSTANCES) readrelations(in, (const EncAnyGram*) &ngram, &rel_instances, ngramversion, (ignore || !secondpass || !DOINSTANCES));
    if (HASSKIPUSAGE) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_skipusage, ngramversion, (ignore || !secondpass || !DOSKIPUSAGE));
    if (HASSKIPCONTENT) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_skipcontent, ngramversion, (ignore || !secondpass || !DOSKIPCONTENT));
    if (HASSUCCESSORS) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_successors, ngramversion, (ignore || !secondpass || !DOSUCCESSORS));
    if (HASPREDECESSORS) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_predecessors, ngramversion, (ignore || !secondpass || !DOPREDECESSORS));
    if (HASCOOCCURRENCE) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_cooccurrences, ngramversion, (ignore || !secondpass || !DOCOOCCURRENCE));        
}

void GraphPatternModel::readskipgramdata(std::istream * in, const EncSkipGram & skipgram, int ngramversion, bool ignore) {
	model->readskipgramdata(in,skipgram, ngramversion, ignore || secondpass);
    if (HASXCOUNT) {
        uint32_t _xcount;
        in->read((char*) &_xcount, sizeof(uint32_t));
        if ((DOXCOUNT) && (secondpass) && (!ignore)) {
        	const EncAnyGram * key = model->getkey((const EncAnyGram*) &skipgram);
        	if (key) {
        		data_xcount[key] = _xcount; //BUG BUG BUG!!
        	} else {
        		cerr << "INTERNAL WARNING: Skipgram key not found in model: ";
        		skipgram.out();
        		cerr <<  " skipping..." << endl;
        	}
        } 
    }
    if (HASPARENTS) readrelations(in, (const EncAnyGram*) &skipgram, &rel_subsumption_parents, (ignore || !secondpass || !DOPARENTS));
    if (HASCHILDREN) readrelations(in, (const EncAnyGram*) &skipgram, &rel_subsumption_children, (ignore || !secondpass || !DOCHILDREN));
    if (HASTEMPLATES) readrelations(in, (const EncAnyGram*) &skipgram, &rel_templates, (ignore || !secondpass || !DOTEMPLATES));
    if (HASINSTANCES) readrelations(in, (const EncAnyGram*) &skipgram, &rel_instances, (ignore || !secondpass || !DOINSTANCES));
    if (HASSKIPUSAGE) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_skipusage, (ignore || !secondpass || !DOSKIPUSAGE));
    if (HASSKIPCONTENT) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_skipcontent, (ignore || !secondpass || !DOSKIPCONTENT));
    if (HASSUCCESSORS) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_successors, (ignore || !secondpass || !DOSUCCESSORS));
    if (HASPREDECESSORS) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_predecessors, (ignore || !secondpass || !DOPREDECESSORS));
    if (HASCOOCCURRENCE) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_cooccurrences, (ignore || !secondpass || !DOCOOCCURRENCE));        
}


void GraphPatternModel::writengramdata(std::ostream * out, const EncNGram & ngram) {
	model->writengramdata(out,ngram);
    if (DOXCOUNT) {
        uint32_t _xcount = data_xcount[(const EncAnyGram*) &ngram]; //;xcount((const EncAnyGram*) &ngram);
        out->write( (char*) &_xcount, sizeof(uint32_t));
    }
    
    if (DOPARENTS) writerelations(out, (const EncAnyGram*) &ngram, rel_subsumption_parents);
    if (DOCHILDREN) writerelations(out, (const EncAnyGram*) &ngram, rel_subsumption_children);
    if (DOTEMPLATES) writerelations(out, (const EncAnyGram*) &ngram, rel_templates);
    if (DOINSTANCES) writerelations(out, (const EncAnyGram*) &ngram, rel_instances);
    if (DOSKIPUSAGE) writerelations(out, (const EncAnyGram*) &ngram, rel_skipusage);
	if (DOSKIPCONTENT) writerelations(out, (const EncAnyGram*) &ngram, rel_skipcontent);
	if (DOSUCCESSORS) writerelations(out, (const EncAnyGram*) &ngram, rel_successors);
	if (DOPREDECESSORS) writerelations(out, (const EncAnyGram*) &ngram, rel_predecessors);
	if (DOCOOCCURRENCE) writerelations(out, (const EncAnyGram*) &ngram, rel_cooccurrences);                  
}

void GraphPatternModel::writeskipgramdata(std::ostream * out, const EncSkipGram & skipgram) {
	model->writeskipgramdata(out,skipgram);    
    if (DOXCOUNT) {
        uint32_t _xcount = data_xcount[(const EncAnyGram*) &skipgram];// xcount((const EncAnyGram*) &skipgram);
        out->write( (char*) &_xcount, sizeof(uint32_t));
    }
    if (DOPARENTS)  writerelations(out, (const EncAnyGram*) &skipgram, rel_subsumption_parents);        
    if (DOCHILDREN)  writerelations(out, (const EncAnyGram*) &skipgram, rel_subsumption_children);
    if (DOTEMPLATES) writerelations(out, (const EncAnyGram*) &skipgram, rel_templates);
    if (DOINSTANCES) writerelations(out, (const EncAnyGram*) &skipgram, rel_instances);
    if (DOSKIPUSAGE) writerelations(out, (const EncAnyGram*) &skipgram, rel_skipusage);
	if (DOSKIPCONTENT) writerelations(out, (const EncAnyGram*) &skipgram, rel_skipcontent);
	if (DOSUCCESSORS) writerelations(out, (const EncAnyGram*) &skipgram, rel_successors);
	if (DOPREDECESSORS) writerelations(out, (const EncAnyGram*) &skipgram, rel_predecessors);
	if (DOCOOCCURRENCE) writerelations(out, (const EncAnyGram*) &skipgram, rel_cooccurrences);        
}



void GraphPatternModel::writengrams(std::ostream * f) {
	const unsigned char check = 0xff;    
    const char czero = 0;
    for(unordered_map<EncNGram,NGramData>::iterator iter = model->ngrams.begin(); iter !=  model->ngrams.end(); iter++ ) {
    	f->write((char*) &check, sizeof(char)); //check        
        f->write(&czero, sizeof(char)); //gapcount, always zero for ngrams
        iter->first.writeasbinary(f);
        writengramdata(f, iter->first);       
    }   
}


void GraphPatternModel::writeskipgrams(std::ostream * f) {
	const char check = 0xff;                 
    for(unordered_map<EncSkipGram,SkipGramData>::iterator iter = model->skipgrams.begin(); iter !=  model->skipgrams.end(); iter++ ) {
    	f->write(&check, sizeof(char)); //gapcount, always zero for ngrams                                
        iter->first.writeasbinary(f);        
        writeskipgramdata(f, iter->first);
    }     
}

GraphPatternModel::~GraphPatternModel() {
    if (DELETEMODEL) delete model;
}


void GraphPatternModel::decode(ClassDecoder & classdecoder, ostream *OUT, bool dooutputrelations) {

    *OUT << "#N\tVALUE\tOCC.COUNT\tTOKENS\tCOVERAGE";
    if (DOXCOUNT) *OUT << "\tXCOUNT\tXRATIO";
    *OUT << "\tPARENTS\tCHILDREN\tTEMPLATES\tINSTANCES\tSKIPUSAGE\tSKIPCONTENT\tSUCCESSORS\tPREDECESSORS" << endl;

    cerr << "Outputting n-grams" << endl;    
    for(unordered_map<const EncNGram,NGramData>::const_iterator iter = model->ngrams.begin(); iter != model->ngrams.end(); iter++ ) {
       const int covtokens = iter->second.count() * iter->first.n();
       const double cov = (double) covtokens / totaltokens;
       
       
       //const double freq1 = (double) iter->second.count() / model->tokencount[iter->first.n()];
       //const double freq2 = (double) iter->second.count() / model->ngramtokencount;
       //const double freq3 = (double) iter->second.count() / grandtotal;
       const EncAnyGram * ngram = &iter->first;       
        *OUT << (int) ngram->n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << ngram->decode(classdecoder) << '\t' << iter->second.count() << '\t' << covtokens << '\t' << cov;
        if (DOXCOUNT) {            
            if (data_xcount.count(ngram) ) {
                const int xc = data_xcount[ngram];
                const double xratio = (double) xc / iter->second.count();
                *OUT << '\t' << xc << '\t' << xratio << '\t';                
            } else {            
                *OUT << '\t' << iter->second.count() << '\t' << 1.0 << '\t';
            }
        }
        
        if (rel_subsumption_parents.count(ngram)) *OUT << rel_subsumption_parents[ngram].size() << '\t'; else  *OUT << "0\t";
        if (rel_subsumption_children.count(ngram)) *OUT << rel_subsumption_children[ngram].size() << '\t'; else  *OUT << "0\t";
        if (rel_templates.count(ngram)) *OUT << rel_templates[ngram].size() << '\t'; else  *OUT << "0\t";
        if (rel_instances.count(ngram)) *OUT << rel_instances[ngram].size() << '\t'; else  *OUT << "0\t";
        if (rel_skipusage.count(ngram)) *OUT << rel_skipusage[ngram].size() << '\t'; else  *OUT << "0\t";
        if (rel_skipcontent.count(ngram)) *OUT << rel_skipcontent[ngram].size() << '\t'; else  *OUT << "0\t";
        if (rel_successors.count(ngram)) *OUT << rel_successors[ngram].size() << '\t'; else  *OUT << "0\t";
        if (rel_predecessors.count(ngram)) *OUT << rel_predecessors[ngram].size() << '\t'; else  *OUT << "0\t";
        if (rel_cooccurrences.count(ngram)) *OUT << rel_cooccurrences[ngram].size() << '\t'; else  *OUT << "0\t";
        /*for (set<CorpusReference>::iterator iter2 = iter->second.refs.begin() ; iter2 != iter->second.refs.end(); iter2++) {
            *OUT << iter2->sentence << ':' << (int) iter2->token << ' ';
        } */               
        *OUT << endl;
        if (dooutputrelations) {
            outputrelations(classdecoder, OUT, (const EncAnyGram*) ngram);
            *OUT << "-------------------------------------------------------------------" << endl;
        }
        
    }
   

   
       cerr << "Outputting skip-grams" << endl;
       for(unordered_map<const EncSkipGram,SkipGramData>::const_iterator iter =  model->skipgrams.begin(); iter !=  model->skipgrams.end(); iter++ ) {
            const int covtokens = iter->second.count() * iter->first.n();
            const double cov = (double) covtokens / totaltokens;
           //const double freq1 = (double) iter->second.count() / model->skiptokencount[iter->first.n()]; 
           //const double freq2 = (double) iter->second.count() / model->skipgramtokencount;           
           //const double freq3 = (double) iter->second.count() / grandtotal;                          
           const EncAnyGram * skipgram = &iter->first;                              
           *OUT << (int) skipgram->n() << '\t' << setprecision(numeric_limits<double>::digits10 + 1) << skipgram->decode(classdecoder) << '\t' << iter->second.count() << '\t' << covtokens << '\t' << cov << '\t';
            if (DOXCOUNT) {            
                if (data_xcount.count( skipgram ) ) {
                    int xc = data_xcount[skipgram];
                    double xratio = (double) xc / iter->second.count() ;
                    *OUT << '\t' << xc << '\t' << xratio << '\t';                
                } else {            
                    *OUT << '\t' << iter->second.count() << '\t' << 1.0 << '\t';
                }
            }           	
            
            /*
           const int skiptypes = iter->second.skipcontent.size();               
           const double entropy = iter->second.entropy();
           *OUT << skiptypes << '\t' << iter->second.count() << '\t' << entropy << '\t';
            for(unordered_map<const EncSkipGram,NGramData>::const_iterator iter2 = iter->second.skipcontent.begin(); iter2 != iter->second.skipcontent.end(); iter2++ ) {
                *OUT << iter2->first.decode(classdecoder) << '|' << iter2->second.count() << '|';
                for (set<CorpusReference>::iterator iter3 = iter2->second.refs.begin() ; iter3 != iter2->second.refs.end(); iter3++) {
                    *OUT << iter3->sentence << ':' << (int) iter3->token;        
                    *OUT << ',';
                    //if (iter3 != iter2->second.refs.end() - 1) 
                }
                //MAYBE TODO: output references?
            }*/
            
            if (rel_subsumption_parents.count(skipgram)) *OUT << rel_subsumption_parents[skipgram].size() << '\t'; else  *OUT << "0\t";
            if (rel_subsumption_children.count(skipgram)) *OUT << rel_subsumption_children[skipgram].size() << '\t'; else  *OUT << "0\t";
            if (rel_templates.count(skipgram)) *OUT << rel_templates[skipgram].size() << '\t'; else  *OUT << "0\t";
            if (rel_instances.count(skipgram)) *OUT << rel_instances[skipgram].size() << '\t'; else  *OUT << "0\t";
            if (rel_skipusage.count(skipgram)) *OUT << rel_skipusage[skipgram].size() << '\t'; else  *OUT << "0\t";
            if (rel_skipcontent.count(skipgram)) *OUT << rel_skipcontent[skipgram].size() << '\t'; else  *OUT << "0\t";
            if (rel_successors.count(skipgram)) *OUT << rel_successors[skipgram].size() << '\t'; else  *OUT << "0\t";
            if (rel_predecessors.count(skipgram)) *OUT << rel_predecessors[skipgram].size() << '\t'; else  *OUT << "0\t";            
            if (rel_cooccurrences.count(skipgram)) *OUT << rel_cooccurrences[skipgram].size() << '\t'; else  *OUT << "0\t";
            
           *OUT << endl;
           if (dooutputrelations) {
            outputrelations(classdecoder, OUT, (const EncAnyGram*) skipgram);
            *OUT << "-------------------------------------------------------------------" << endl;
           }
       }
    

}




void GraphPatternModel::coveragereport(std::ostream *OUT, int segmentsize, double xratiothreshold) {
    
    unordered_map<CorpusReference, unordered_set<const EncAnyGram *> > reverseindex; //for coverage
    
    int totalngramcoverage = 0;
    int totalskipgramcoverage = 0;
    int totalngramxcoverage = 0;
    int totalskipgramxcoverage = 0;
    int ngramcoverage[MAXN+1];    
    int skipgramcoverage[MAXN+1];
    int ngramxcoverage[MAXN+1];    
    int skipgramxcoverage[MAXN+1];
    int skipgramencapsulation[MAXN+1];
    for (int n = 1; n <= MAXN; n++) { ngramcoverage[n] = 0;  skipgramcoverage[n] = 0; ngramxcoverage[n] = 0;  skipgramxcoverage[n] = 0;  skipgramencapsulation[n] = 0; }

            
    int covered = 0;
    int xcovered = 0;
    int encapsulated = 0;
    int uncovered = 0;
    int outside = 0; // uncovered + unencapsulated

    for (unsigned int sentence = 1; sentence < model->sentencesize.size(); sentence++) {
        unsigned char slen = model->sentencesize[sentence];
        //cerr << "SENTENCE=" << sentence << ";SLEN=" << (int) slen << endl;
        
        if (sentence % segmentsize == 1) {
            if (OUT) *OUT << "Computing reverse index for next segment..." << endl;
            reverseindex.clear();
            for (unordered_map<const EncNGram,NGramData >::iterator iter = model->ngrams.begin(); iter != model->ngrams.end(); iter++) {
                const EncAnyGram * anygram = &iter->first;
                for (set<CorpusReference>::iterator iter2 = iter->second.refs.begin() ; iter2 != iter->second.refs.end(); iter2++) {
                    CorpusReference ref = *iter2;
                    if ((ref.sentence >= sentence) && (ref.sentence < sentence+segmentsize)) {
                        reverseindex[ref].insert(anygram);                   
                    }                     
                }
            }
            for (unordered_map<const EncSkipGram,SkipGramData >::iterator iter = model->skipgrams.begin(); iter != model->skipgrams.end(); iter++) {
                const EncAnyGram * anygram = &iter->first;
                for(unordered_map<EncSkipGram,NGramData>::iterator iter2 = iter->second.skipcontent.begin(); iter2 != iter->second.skipcontent.end(); iter2++ ) {                
                    for (set<CorpusReference>::iterator iter3 = iter2->second.refs.begin() ; iter3 != iter2->second.refs.end(); iter3++) {
                        CorpusReference ref = *iter3;
                        if ((ref.sentence >= sentence) && (ref.sentence < sentence+segmentsize)) {
                            reverseindex[ref].insert(anygram);
                        }                        
                    }                     
                }
            }                                        
        }
        
        
        unordered_map<unsigned char, unordered_set<const EncAnyGram *> > sentencecoverage; //exact coverage for this sentence
        unordered_map<unsigned char, unordered_set<const EncAnyGram *> > sentenceencapsulation; //exact coverage for this sentence
        
        for (unsigned char token = 0; token < slen; token++) {
            CorpusReference ref = CorpusReference( (uint32_t) sentence, token);
            if ( reverseindex.count(ref) ) {
                for (unordered_set<const EncAnyGram *>::const_iterator iter = reverseindex[ref].begin(); iter != reverseindex[ref].end(); iter++) {
                    const EncAnyGram * anygram = *iter;
                    if (anygram->isskipgram()) {
                        //skipgram
                        const EncSkipGram * skipgram = (const EncSkipGram *) anygram;
                        vector<bool> mask;
                        skipgram->mask(mask);
                        int offset = 0;
                        for (vector<bool>::iterator maskiter = mask.begin(); maskiter != mask.end(); maskiter++) {
                            if (*maskiter) {
                                sentencecoverage[token+offset].insert(anygram);
                            } else {
                                sentenceencapsulation[token+offset].insert(anygram);
                            }
                            offset++;
                        }                        
                    } else {
                        //ngram
                        for (unsigned char token2 = token; token2 < token+ anygram->n(); token2++) {
                            sentencecoverage[token2].insert(anygram);
                        }
                    }
                } 
            }
        }
        
                
        
        for (unsigned char token = 0; token < slen; token++) {
            if ( sentencecoverage.count(token) ) {
                covered++;
                bool considerexclusive = false;
                if (xratiothreshold > 0) {
                    for (unordered_set<const EncAnyGram *>::const_iterator iter = sentencecoverage[token].begin(); iter != sentencecoverage[token].end(); iter++) {
                        const EncAnyGram * anygram = *iter;
                        if (data_xcount.count(anygram)) {
                            const double xratio = (double) data_xcount.count(anygram) / model->occurrencecount(anygram);
                            if (xratio > xratiothreshold) {
                                considerexclusive = true;
                                break;
                            }
                        } 
                    }
                    if (considerexclusive) xcovered++;
                }                
                bool foundskipgram = false;
                bool foundngram = false;
                bool foundxskipgram = false;
                bool foundxngram = false;
                for (int n = 1; n < MAXN; n++) {
                    if (xratiothreshold) {
                        for (unordered_set<const EncAnyGram *>::const_iterator iter = sentencecoverage[token].begin(); iter != sentencecoverage[token].end(); iter++) {
                            const EncAnyGram * anygram = *iter;
                            if (anygram->n() == n) {
                                if (data_xcount.count(anygram)) {
                                    const double xratio = (double) data_xcount.count(anygram) / model->occurrencecount(anygram);
                                    if (xratio > xratiothreshold) {                                
                                        if (anygram->isskipgram()) {                                                
                                            skipgramxcoverage[n]++;
                                            foundxskipgram = true;
                                            break; //only count once per n
                                        } else {
                                            ngramxcoverage[n]++;
                                            foundxngram = true;
                                            break; //only count once per n
                                        }                                                
                                    }
                                }
                            }
                        }
                    }

                    for (unordered_set<const EncAnyGram *>::const_iterator iter = sentencecoverage[token].begin(); iter != sentencecoverage[token].end(); iter++) {
                        const EncAnyGram * anygram = *iter;
                        if (anygram->n() == n) {
                            if (anygram->isskipgram()) {                                                
                                skipgramcoverage[n]++;
                                foundskipgram = true;
                                break; //only count once per n
                            } else {
                                ngramcoverage[n]++;
                                foundngram = true;
                                break; //only count once per n
                            }
                        }
                    }  
                         
                }
                if (foundskipgram) totalskipgramcoverage++;
                if (foundngram) totalngramcoverage++;
                if (foundxskipgram) totalskipgramxcoverage++;
                if (foundxngram) totalngramxcoverage++;                
            } else {
                uncovered++;
            }
            
            if ( sentenceencapsulation.count(token)) {
                encapsulated++;    
                for (int n = 1; n < MAXN; n++) {
                    for (unordered_set<const EncAnyGram *>::const_iterator iter = sentencecoverage[token].begin(); iter != sentencecoverage[token].end(); iter++) {
                        const EncAnyGram * anygram = *iter;
                        if (anygram->n() == n) {                                              
                            skipgramencapsulation[n]++;
                            break; //only count once per n
                        }
                    }          
                }
            } else if (sentencecoverage.count(token) == 0) {
                outside++;
            }            
        }
        
    }
    
    
    if (OUT) {
        const int totalcount =  model->tokens();  
        *OUT << setiosflags(ios::fixed) << setprecision(4) << endl;       
        *OUT << "COVERAGE REPORT" << endl;
        *OUT << "----------------------------------" << endl;
        *OUT << "Total number of tokens:   " << setw(10) << model->totaltokens << endl;
        if (xratiothreshold > 0) *OUT << "Exclusivity threshold:    " << setw(10) << xratiothreshold << endl << endl;  
        *OUT << "                          " << setw(10) << "TOKENS" << setw(10) << "COVERAGE" << setw(10) << "XCOVERAGE" << setw(7) << "TYPES" << setw(11) << "TTR" << setw(10) << "COUNT" << setw(10) << "FREQUENCY" << endl;    
        *OUT << "Total coverage:           " << setw(10) << covered << setw(10) << (double) covered / model->totaltokens << setw(10) << (double) xcovered / model->totaltokens << setw(7) << model->ngrams.size() + model->skipgrams.size()  << setw(11)<< (double)  covered / (model->ngrams.size() + model->skipgrams.size()) << setw(10) << model->totalngramcount+model->totalskipgramcount << setw(10) << (double) model->tokens() / totalcount << endl;
        *OUT << "Uncovered:                " << setw(10) << uncovered << setw(10) << (double) uncovered / model->totaltokens <<endl << endl;        
        *OUT << "N-gram coverage:          " << setw(10) << totalngramcoverage << setw(10) << (double) totalngramcoverage / model->totaltokens << setw(10) << (double) totalngramxcoverage / model->totaltokens << setw(7) << model->ngrams.size() <<setw(11) << (double) totalngramcoverage / model->ngrams.size() << setw(10) << model->totalngramcount << setw(10) << (double) model->totalngramcount / totalcount << endl;
        for (int n = 1; n <= MAXN; n++) {
         if (ngramcoverage[n] > 0) {
            int t = 0;
            for (unordered_map<const EncNGram,NGramData >::iterator iter = model->ngrams.begin(); iter != model->ngrams.end(); iter++) if (iter->first.n() == n) t++;
            *OUT << " " << n << "-gram coverage:         " << setw(10) << ngramcoverage[n] << setw(10) << (double) ngramcoverage[n] / model->totaltokens << setw(10) << (double) ngramxcoverage[n] / model->totaltokens << setw(7) << t << setw(11) <<  (double) t / ngramcoverage[n] << setw(10) << model->ngramcount[n] << setw(10) << (double) model->ngramcount[n] / totalcount  << endl;
         }
        }
        *OUT << endl;
        *OUT << "Skipgram coverage:        " << setw(10) << totalskipgramcoverage << setw(10) << (double) totalskipgramcoverage / model->totaltokens << setw(10) << (double) totalskipgramxcoverage / model->totaltokens << setw(7) << model->skipgrams.size() << setw(11) << (double) totalskipgramcoverage / model->skipgrams.size() << setw(10) << model->totalskipgramcount << setw(10) <<  (double) model->totalskipgramcount / totalcount <<   endl;
        for (int n = 2; n <= MAXN; n++) {         
         if (skipgramcoverage[n] > 0) {
          int t = 0;
          for (unordered_map<const EncSkipGram,SkipGramData >::iterator iter = model->skipgrams.begin(); iter != model->skipgrams.end(); iter++) if (iter->first.n() == n) t++; 
          *OUT << " " << n << "-skipgram coverage:     " << setw(10) << skipgramcoverage[n] << setw(10) << (double) skipgramcoverage[n] / model->totaltokens << setw(10) << (double) skipgramxcoverage[n] / model->totaltokens  << setw(7) << t << setw(11) <<  (double) t / skipgramcoverage[n] << setw(10) << model->skipgramcount[n] << setw(10) << (double) model->skipgramcount[n] / totalcount << endl;
         }
        }
        *OUT << "----------------------------------" << endl;
        *OUT << "                          " << setw(10) << "TOKENS" << setw(10) << "ENCAPSULATION";    
        *OUT << "Total encapsulation:           " << setw(10) << encapsulated << setw(10) << (double) encapsulated / model->totaltokens << endl;
        *OUT << "Outside (uncovered+unencapsulated):                " << setw(10) << outside << setw(10) << (double) outside / model->totaltokens << endl << endl;
        for (int n = 2; n <= MAXN; n++) {         
         if (skipgramcoverage[n] > 0) {
          int t = 0;
          for (unordered_map<const EncSkipGram,SkipGramData >::iterator iter = model->skipgrams.begin(); iter != model->skipgrams.end(); iter++) if (iter->first.n() == n) t++; 
          *OUT << " " << n << "-skipgram coverage:     " << setw(10) << skipgramencapsulation[n] << setw(10) << (double) skipgramencapsulation[n] / totaltokens << endl;
         }
        }
                
                
    }

}


void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}




void GraphPatternModel::outputgraph(ClassDecoder & classdecoder, ostream *OUT) {
	*OUT << "digraph G {\n";
	//first pass, output nodes
	for (unordered_map<const EncNGram,NGramData>::const_iterator iter = model->ngrams.begin(); iter != model->ngrams.end(); iter++ ) {
		string label = iter->first.decode(classdecoder);
		replaceAll(label,"\"","\\\"");
		*OUT << "c" << iter->first.hash() << " [label=\"" << label << "\\n" << iter->second.refs.size();
		if ((DOXCOUNT) && (HASXCOUNT)) *OUT << setprecision(3) << fixed  << " " << (double) data_xcount[(const EncAnyGram *) &iter->first] / (double) iter->second.refs.size();				
		*OUT << "\",shape=box];" << endl;				 
	}
	for (unordered_map<const EncSkipGram,SkipGramData>::const_iterator iter = model->skipgrams.begin(); iter != model->skipgrams.end(); iter++ ) {
		string label = iter->first.decode(classdecoder);
		replaceAll(label,"\"","\\\"");
		*OUT << "c" << iter->first.hash() << " [label=\"" << label << "\\n" << iter->second.count();
		if ((DOXCOUNT) && (HASXCOUNT)) *OUT << setprecision(3) << fixed  << " " << (double) data_xcount[(const EncAnyGram *) &iter->first] / (double) iter->second.count();
		*OUT << "\",shape=circle];" << endl;				 
	}
	
		
	//second pass, output edges
	for (unordered_map<const EncNGram,NGramData>::const_iterator iter = model->ngrams.begin(); iter != model->ngrams.end(); iter++ ) {
		const EncAnyGram * anygram = &iter->first;
		if (DOPARENTS) outputgraphvizrelations(anygram, OUT, rel_subsumption_parents, "black");
		if (DOCHILDREN) outputgraphvizrelations(anygram, OUT, rel_subsumption_children, "grey");
		if (DOPREDECESSORS) outputgraphvizrelations(anygram, OUT, rel_predecessors, "yellow");
		if (DOSUCCESSORS) outputgraphvizrelations(anygram, OUT, rel_successors, "green");
		if (DOSKIPCONTENT) outputgraphvizrelations(anygram, OUT, rel_skipcontent, "cyan");
		if (DOSKIPUSAGE) outputgraphvizrelations(anygram, OUT, rel_skipusage, "purple");
		if (DOTEMPLATES) outputgraphvizrelations(anygram, OUT, rel_skipcontent, "blue");
		if (DOINSTANCES) outputgraphvizrelations(anygram, OUT, rel_skipusage, "red");
		if (DOCOOCCURRENCE) outputgraphvizrelations(anygram, OUT, rel_cooccurrences, "orange");		
	}
	

	for (unordered_map<const EncSkipGram,SkipGramData>::const_iterator iter = model->skipgrams.begin(); iter != model->skipgrams.end(); iter++ ) {
		const EncAnyGram * anygram = &iter->first;
		//cerr << "DEBUG: IN1" << endl;
		if (DOPARENTS) outputgraphvizrelations(anygram, OUT, rel_subsumption_parents, "black");
		if (DOCHILDREN) outputgraphvizrelations(anygram, OUT, rel_subsumption_children, "grey");
		if (DOPREDECESSORS) outputgraphvizrelations(anygram, OUT, rel_predecessors, "yellow");
		if (DOSUCCESSORS) outputgraphvizrelations(anygram, OUT, rel_successors, "green");
		if (DOSKIPCONTENT) outputgraphvizrelations(anygram, OUT, rel_skipcontent, "cyan");
		if (DOSKIPUSAGE) outputgraphvizrelations(anygram, OUT, rel_skipusage, "purple");
		if (DOTEMPLATES) outputgraphvizrelations(anygram, OUT, rel_skipcontent, "blue");
		if (DOINSTANCES) outputgraphvizrelations(anygram, OUT, rel_skipusage, "red");
		if (DOCOOCCURRENCE) outputgraphvizrelations(anygram, OUT, rel_cooccurrences, "orange");
	}	

	
	*OUT << "}\n";
}

void GraphPatternModel::findincomingnodes(const EncAnyGram * focus, unordered_set<const EncAnyGram *> & relatednodes) {
	for (unordered_map<const EncNGram,NGramData>::const_iterator iter = model->ngrams.begin(); iter != model->ngrams.end(); iter++ ) {
		const EncAnyGram * anygram = (const EncAnyGram *) &iter->first;
		if (anygram != focus) {
			findincomingnodes(focus, anygram, relatednodes, rel_subsumption_parents);
			findincomingnodes(focus, anygram, relatednodes, rel_subsumption_children);
			findincomingnodes(focus, anygram, relatednodes, rel_skipcontent);
			findincomingnodes(focus, anygram, relatednodes, rel_skipusage);
			findincomingnodes(focus, anygram, relatednodes, rel_successors);
			findincomingnodes(focus, anygram, relatednodes, rel_predecessors);
			findincomingnodes(focus, anygram, relatednodes, rel_templates);
			findincomingnodes(focus, anygram, relatednodes, rel_instances);
			findincomingnodes(focus, anygram, relatednodes, rel_cooccurrences);
		}
	}
	for (unordered_map<const EncSkipGram,SkipGramData>::const_iterator iter = model->skipgrams.begin(); iter != model->skipgrams.end(); iter++ ) {
		const EncAnyGram * anygram = (const EncAnyGram *) &iter->first;
		if (anygram != focus) {
			findincomingnodes(focus, anygram, relatednodes, rel_subsumption_parents);
			findincomingnodes(focus, anygram, relatednodes, rel_subsumption_children);
			findincomingnodes(focus, anygram, relatednodes, rel_skipcontent);
			findincomingnodes(focus, anygram, relatednodes, rel_skipusage);
			findincomingnodes(focus, anygram, relatednodes, rel_successors);
			findincomingnodes(focus, anygram, relatednodes, rel_predecessors);
			findincomingnodes(focus, anygram, relatednodes, rel_templates);
			findincomingnodes(focus, anygram, relatednodes, rel_instances);
			findincomingnodes(focus, anygram, relatednodes, rel_cooccurrences);
		}
	}	
}

void GraphPatternModel::findincomingnodes(const EncAnyGram * focus, const EncAnyGram * anygram, unordered_set<const EncAnyGram *> & relatednodes, t_relations  & relationhash ) {
	unordered_set<const EncAnyGram*> * relations = &relationhash[anygram];
	for (unordered_set<const EncAnyGram*>::iterator iter = relations->begin(); iter != relations->end(); iter++) {
		const EncAnyGram * anygram2  = model->getkey(*iter);
		if (focus == anygram2) {
			relatednodes.insert(anygram);
		}	
	}
}

void GraphPatternModel::findincomingnodes(const EncAnyGram * focus, const EncAnyGram * anygram, unordered_set<const EncAnyGram *> & relatednodes, t_weightedrelations  & relationhash ) {
	unordered_map<const EncAnyGram*, double> * relations = &relationhash[anygram];
	for (unordered_map<const EncAnyGram*, double>::iterator iter = relations->begin(); iter != relations->end(); iter++) {
		const EncAnyGram * anygram2  = model->getkey(iter->first);
		if (focus == anygram2) {
			relatednodes.insert(anygram);
		}	
	}
}


void GraphPatternModel::outputrelations(ClassDecoder & classdecoder, ostream *OUT, const EncAnyGram * focusinput, bool outputqueryword) {
	const EncAnyGram * focus = model->getkey(focusinput);	
	if (focus == NULL) {
		cerr << "Query word not found" << endl;
		return;
	} 
	if (outputqueryword) {
	    *OUT << "#Query:" << endl;
        *OUT << "\t" << focus->decode(classdecoder) << "\t-\t" << model->occurrencecount(focus) << "\t" << model->coveragecount(focus) << "\t" << model->coverage(focus);
	    if ((DOXCOUNT) && (HASXCOUNT)) *OUT << "\t" << data_xcount[focus] << '\t' << (double) data_xcount[focus] / model->occurrencecount(focus);
	    *OUT << endl;
	}	
	if (rel_subsumption_parents[focus].size() > 0) {		
	    *OUT << "#Parent relations - " << rel_subsumption_parents[focus].size() << endl;
	    outputrelations(classdecoder,OUT,  rel_subsumption_parents[focus]);
    }
    if (rel_subsumption_children[focus].size() > 0) {
	    *OUT << "#Child relations - " << rel_subsumption_children[focus].size() << endl;
	    outputrelations(classdecoder,OUT,  rel_subsumption_children[focus]);
	}
    if (rel_predecessors[focus].size() > 0) {
       *OUT << "#Predecessor relations - " << rel_predecessors[focus].size() << endl;
       outputrelations(classdecoder,OUT,  rel_predecessors[focus]);
    }
    if (rel_successors[focus].size() > 0) {    
	    *OUT << "#Successor relations - " << rel_successors[focus].size() << endl;
    	outputrelations(classdecoder,OUT,  rel_successors[focus]);
    }
    if (rel_skipcontent[focus].size() > 0) {    
	    *OUT << "#Skipcontent - " << rel_skipcontent[focus].size() << endl;
	    outputrelations(classdecoder,OUT,  rel_skipcontent[focus]);
	}
	if (rel_skipusage[focus].size() > 0) {
	    *OUT << "#Skipusage - " << rel_skipusage[focus].size() << endl;
    	outputrelations(classdecoder,OUT,  rel_skipusage[focus]);
    }
    if (rel_templates[focus].size() > 0) {
	    *OUT << "Templates - " << rel_templates[focus].size() << endl;
	    outputrelations(classdecoder,OUT,  rel_templates[focus]);
	}
	if (rel_instances[focus].size() > 0) {
	    *OUT << "Instances - " << rel_instances[focus].size() << endl;
	    outputrelations(classdecoder,OUT,  rel_instances[focus]);
	}
	if (rel_cooccurrences[focus].size() > 0) {
	    *OUT << "Co-occurrences - " << rel_cooccurrences[focus].size() << endl;
	    outputcoocrelations(focus, classdecoder,OUT,  rel_cooccurrences[focus]);
	}		
}

void GraphPatternModel::outputrelations(ClassDecoder & classdecoder, ostream *OUT, unordered_set<const EncAnyGram*>   & relations ) {
	for (std::unordered_set<const EncAnyGram*>::iterator iter = relations.begin(); iter != relations.end(); iter++) {
		const EncAnyGram * anygram = *iter;
		*OUT << "\t" << anygram->decode(classdecoder) << "\t-\t" << model->occurrencecount(anygram) << "\t" << model->coveragecount(anygram) << "\t" << model->coverage(anygram);
		if ((DOXCOUNT) && (HASXCOUNT)) *OUT << "\t" << data_xcount[anygram] << "\t" << (double) data_xcount[anygram] / model->occurrencecount(anygram);
		*OUT << endl;		
	}
}



void GraphPatternModel::outputrelations(ClassDecoder & classdecoder, ostream *OUT, unordered_map<const EncAnyGram*, double>   & relations ) {
	for (std::unordered_map<const EncAnyGram*, double>::iterator iter = relations.begin(); iter != relations.end(); iter++) {
		const EncAnyGram * anygram = iter->first;
		*OUT << "\t" << anygram->decode(classdecoder) << "\t" << iter->second << "\t" << model->occurrencecount(anygram) << "\t" << model->coveragecount(anygram) << "\t" << model->coverage(anygram);
		if ((DOXCOUNT) && (HASXCOUNT)) *OUT << "\t" << data_xcount[anygram] << "\t" << (double) data_xcount[anygram] / model->occurrencecount(anygram);
		*OUT << endl;		
	}
}


void GraphPatternModel::outputcoocrelations(const EncAnyGram * pivot, ClassDecoder & classdecoder, ostream *OUT, unordered_map<const EncAnyGram*, double>   & relations ) {
    multimap<double, const EncAnyGram *> sorted;
	for (std::unordered_map<const EncAnyGram*, double>::iterator iter = relations.begin(); iter != relations.end(); iter++) {
		const EncAnyGram * anygram = iter->first;
		
        if ((COOCTHRESHOLD > 0) && (iter->second < COOCTHRESHOLD)) continue;

		double coocvalue = 0;
		if (COOCSTYLE == COOCSTYLE_COUNT) {
		    coocvalue = iter->second;
		} else if (COOCSTYLE == COOCSTYLE_PMI) {
		    coocvalue = pmi(pivot, anygram);
		} else if (COOCSTYLE == COOCSTYLE_NPMI) {
		    coocvalue = npmi(pivot, anygram);
		} else {
		    cerr << "Invalid COOCSTYLE in GraphPatternModel::outputcoocrelations" << endl;
		    throw InternalError();
		}	
        sorted.insert( pair<double, const EncAnyGram*>( coocvalue * -1 , anygram )); 
    }
    
    for (multimap<double,const EncAnyGram *>::iterator iter = sorted.begin(); iter != sorted.end(); iter++) {
        const EncAnyGram * anygram = iter->second;
        const double coocvalue = iter->first * -1;
		*OUT << "\tCOOC\t" << pivot->decode(classdecoder) << "\t" << anygram->decode(classdecoder) << "\t" << coocvalue << "\t" << model->occurrencecount(anygram) << "\t" << model->coveragecount(anygram) << "\t" << model->coverage(anygram);
		if ((DOXCOUNT) && (HASXCOUNT)) *OUT << "\t" << data_xcount[anygram] << "\t" << (double) data_xcount[anygram] / model->occurrencecount(anygram);
		*OUT << endl;		
	}
}

double GraphPatternModel::pmi(const EncAnyGram * key1, const EncAnyGram * key2) {
    if ((rel_cooccurrences.count(key1)) && (rel_cooccurrences[key1].count(key2))) {
        const double jointcount = rel_cooccurrences[key1][key2];
        return  log( (double) jointcount / (model->occurrencecount(key1) * model->occurrencecount(key2)) );    
    } else {
        cerr << "Unable to compute PMI, one of the operands does not exist" << endl;
        throw InternalError();
    }
}

double GraphPatternModel::npmi(const EncAnyGram * key1, const EncAnyGram * key2) {
    if ((rel_cooccurrences.count(key1)) && (rel_cooccurrences[key1].count(key2))) {
        const double jointcount = rel_cooccurrences[key1][key2];
        return (double) log( (double) jointcount / (model->occurrencecount(key1) * model->occurrencecount(key2)) ) / (double) -log((double)jointcount/(double)model->occurrences());    
    } else {
        cerr << "Unable to compute NPMI, one of the operands does not exist" << endl;
        throw InternalError();
    }
}




void GraphPatternModel::outputgraph(ClassDecoder & classdecoder, ostream *OUT, const EncAnyGram * focusinput) {
	const EncAnyGram * focus = model->getkey(focusinput);
	if (focus == NULL) {
		cerr << "Query word not found" << endl;
		return;
	} 
	unordered_set<const EncAnyGram *> relatednodes;
	relatednodes.insert(focus);
	
	
	relatednodes.insert( rel_subsumption_parents[focus].begin(), rel_subsumption_parents[focus].end() );
	relatednodes.insert( rel_subsumption_children[focus].begin(), rel_subsumption_children[focus].end() );	
	relatednodes.insert( rel_templates[focus].begin(), rel_templates[focus].end() );
	relatednodes.insert( rel_instances[focus].begin(), rel_instances[focus].end() );
	
	for (unordered_map<const EncAnyGram *, double>::iterator iter = rel_predecessors[focus].begin(); iter != rel_predecessors[focus].end(); iter++) relatednodes.insert(iter->first);  
	for (unordered_map<const EncAnyGram *, double>::iterator iter = rel_successors[focus].begin(); iter != rel_successors[focus].end(); iter++) relatednodes.insert(iter->first);
	for (unordered_map<const EncAnyGram *, double>::iterator iter = rel_skipcontent[focus].begin(); iter != rel_skipcontent[focus].end(); iter++) relatednodes.insert(iter->first);
	for (unordered_map<const EncAnyGram *, double>::iterator iter = rel_skipusage[focus].begin(); iter != rel_skipusage[focus].end(); iter++) relatednodes.insert(iter->first);
	for (unordered_map<const EncAnyGram *, double>::iterator iter = rel_cooccurrences[focus].begin(); iter != rel_cooccurrences[focus].end(); iter++) relatednodes.insert(iter->first);  

	
	cerr << "  Found " << relatednodes.size() << " nodes (direct relations)" << endl;
	
	findincomingnodes(focus,relatednodes);	 			
	
	cerr << "  Found " << relatednodes.size() << " nodes (considering incoming edges)" << endl;
	
	*OUT << "digraph G {\n";

	string focuslabel = focus->decode(classdecoder);
	replaceAll(focuslabel,"\"","\\\"");

	*OUT << "c" << focus->hash() << " [label=\"" << focuslabel << "\\n" << model->occurrencecount(focus);
	if ((DOXCOUNT) && (HASXCOUNT)) *OUT << " " <<  setprecision(2) << fixed << (double) data_xcount[focus] / (double) model->occurrencecount(focus);// << " " << data_xcount[focus];
	if (focus->isskipgram()) {
		*OUT <<  "\",shape=circle,color=yellow,style=filled];" << endl;
	} else {
		*OUT << "\",shape=box,color=yellow,style=filled];" << endl;
	}
	
	for (unordered_set<const EncAnyGram*>::iterator iter = relatednodes.begin(); iter != relatednodes.end(); iter++) {
		const EncAnyGram * anygram = *iter;
		
		if (anygram != focus) {
			string label = anygram->decode(classdecoder);
			replaceAll(label,"\"","\\\"");
			*OUT << "c" << anygram->hash() << " [label=\"" << label << "\\n" << model->occurrencecount(anygram);
			*OUT << " " << setprecision(2) << fixed << ((double) model->occurrencecount(anygram) / (double) model->occurrencecount(focus)) * 100.0 << "%";
			if ((DOXCOUNT) && (HASXCOUNT)) *OUT << " "<<  setprecision(2) << fixed  << (double) data_xcount[anygram] / (double) model->occurrencecount(anygram);// << " " << data_xcount[anygram];	
			if (anygram->isskipgram()) {
				*OUT << "\",shape=circle];" << endl;
			} else {
				*OUT << "\",shape=box];" << endl;	
			}
		
		}
	}

	if (DOPARENTS) outputgraphvizrelations(relatednodes, OUT, rel_subsumption_parents, "black");
	if (DOCHILDREN) outputgraphvizrelations(relatednodes, OUT, rel_subsumption_children, "grey");
	if (DOPREDECESSORS) outputgraphvizrelations(relatednodes, OUT, rel_predecessors, "yellow");
	if (DOSUCCESSORS) outputgraphvizrelations(relatednodes, OUT, rel_successors, "green");
	if (DOSKIPCONTENT) outputgraphvizrelations(relatednodes, OUT, rel_skipcontent, "cyan");
	if (DOSKIPUSAGE) outputgraphvizrelations(relatednodes, OUT, rel_skipusage, "purple");
	if (DOTEMPLATES) outputgraphvizrelations(relatednodes, OUT, rel_templates, "blue");
	if (DOINSTANCES) outputgraphvizrelations(relatednodes, OUT, rel_instances, "red");
	if (DOCOOCCURRENCE) outputgraphvizrelations(relatednodes, OUT, rel_cooccurrences, "orange");	
	*OUT << "}\n";
}



void GraphPatternModel::outputgraphvizrelations( const EncAnyGram * anygram, ostream *OUT, t_relations & relationhash, const std::string & colour) {
		unordered_set<const EncAnyGram*> * relations = &relationhash[anygram];
	    for (unordered_set<const EncAnyGram*>::iterator iter = relations->begin(); iter != relations->end(); iter++) {
	    	const EncAnyGram * anygram2  = model->getkey(*iter);
	    	*OUT << "c" << anygram->hash() << " -> " << "c" << anygram2->hash() << " [ color=" << colour << " ];" << endl; 
	    }				
}

void GraphPatternModel::outputgraphvizrelations( const EncAnyGram * anygram, ostream *OUT, t_weightedrelations & relationhash, const std::string & colour) {
		unordered_map<const EncAnyGram*, double> * relations = &relationhash[anygram];
	    for (unordered_map<const EncAnyGram*, double>::iterator iter = relations->begin(); iter != relations->end(); iter++) {
	    	const EncAnyGram * anygram2  = model->getkey(iter->first);
	    	*OUT << "c" << anygram->hash() << " -> " << "c" << anygram2->hash() << " [ color=" << colour << ",label=\"" << iter->second << "\" ];" << endl; 
	    }				
}

void GraphPatternModel::outputgraphvizrelations( const unordered_set<const EncAnyGram *> & nodes, ostream *OUT, t_relations & relationhash, const std::string & colour) {	
	for (unordered_set<const EncAnyGram*>::const_iterator iter = nodes.begin(); iter != nodes.end(); iter++) {
		const EncAnyGram * anygram = model->getkey(*iter);
		if (relationhash.count(anygram) > 0) { 
			unordered_set<const EncAnyGram*> * relations = &relationhash[anygram];
			for (unordered_set<const EncAnyGram*>::iterator iter2 = relations->begin(); iter2 != relations->end(); iter2++) {
				const EncAnyGram * anygram2  = model->getkey(*iter2);
				if (nodes.count(anygram2) > 0) {
					*OUT << "c" << anygram->hash() << " -> " << "c" << anygram2->hash() << " [ color=" << colour << " ];" << endl;
				} 
			}
		}				
	}
}


void GraphPatternModel::outputgraphvizrelations( const unordered_set<const EncAnyGram *> & nodes, ostream *OUT, t_weightedrelations & relationhash, const std::string & colour) {	
	for (unordered_set<const EncAnyGram*>::const_iterator iter = nodes.begin(); iter != nodes.end(); iter++) {
		const EncAnyGram * anygram = model->getkey(*iter);
		if (relationhash.count(anygram) > 0) { 
			unordered_map<const EncAnyGram*, double> * relations = &relationhash[anygram];
			for (unordered_map<const EncAnyGram*,double>::iterator iter2 = relations->begin(); iter2 != relations->end(); iter2++) {
				const EncAnyGram * anygram2  = model->getkey(iter2->first);
				if (nodes.count(anygram2) > 0) {
					*OUT << "c" << anygram->hash() << " -> " << "c" << anygram2->hash() << " [ color=" << colour << ",label=\"" << iter2->second << "\" ];" << endl;
				} 
			}
		}				
	}
}


/****************************************************************************************************************************************/

SelectivePatternModel::SelectivePatternModel(const std::string & filename,  const GraphFilter & filter, bool DOFORWARDINDEX,  bool DOREVERSEINDEX, int COUNTTHRESHOLD, double FREQTHRESHOLD, double XCOUNTRATIOTHRESHOLD, int XCOUNTTHRESHOLD, bool DOSKIPGRAMS,  int MINLENGTH, int MAXLENGTH, AlignConstraintInterface * alignconstrain, bool alignconstrainsource, const bool DEBUG) { //read a normal graph pattern model in another way optimised for Cooc alignment
    applyfilter(filter);
	this->DOFORWARDINDEX = DOFORWARDINDEX;
	this->DOREVERSEINDEX = DOREVERSEINDEX;
	this->COUNTTHRESHOLD = COUNTTHRESHOLD;
	this->FREQTHRESHOLD = FREQTHRESHOLD;
	this->XCOUNTRATIOTHRESHOLD = XCOUNTRATIOTHRESHOLD;
	this->XCOUNTTHRESHOLD = XCOUNTTHRESHOLD;
	this->DOSKIPGRAMS = DOSKIPGRAMS;
	this->MINLENGTH = MINLENGTH;
	this->MAXLENGTH = MAXLENGTH;
	this->alignconstrain = alignconstrain;
	this->alignconstrainsource = alignconstrainsource;
	
    if (((XCOUNTRATIOTHRESHOLD > 0) || (XCOUNTTHRESHOLD > 0)) && (!DOXCOUNT)) {
        cerr << "WARNING: Exclusive count not being loaded, but threshold parameters set!" << endl;
    }

	totaltokens = 0;
	ignoredtypes = 0;
	ignoredoccurrences = 0;
	       
    	       
    secondpass = false;      
    if (DEBUG) cerr << "******* SelectivePatternModel FIRST PASS ******" << endl;
	readfile(filename,DEBUG);	
	secondpass = (DOPARENTS || DOCHILDREN || DOXCOUNT || DOTEMPLATES || DOINSTANCES || DOSKIPCONTENT || DOSKIPUSAGE || DOSUCCESSORS || DOPREDECESSORS || DOCOOCCURRENCE);
	if (secondpass) {	
	    if (DEBUG) cerr << "******** SelectivePatternModel SECOND PASS *********" << endl;
		readfile(filename, DEBUG);
	}
	computestats();
}


void SelectivePatternModel::readheader(std::istream * in, bool ignore) {
	if ((model_id >= GRAPHPATTERNMODEL) && (model_id <= GRAPHPATTERNMODEL+GRAPHPATTERNMODELVERSION)) {
		if (DEBUG) cerr << "READING GRAPHMODEL HEADER " << endl; 
    	in->read((char*) &HASPARENTS,  sizeof(bool)); //1 byte, not 1 bit
	    in->read((char*) &HASCHILDREN, sizeof(bool)); //1 byte, not 1 bit
	    in->read((char*) &HASXCOUNT, sizeof(bool)); //1 byte, not 1 bit
		if (model_id >= GRAPHPATTERNMODEL+1 ) {
			if (DEBUG) cerr << "READING EXTENDED GRAPHMODEL HEADER (v1)" << endl;
			in->read((char*) &HASTEMPLATES, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASINSTANCES, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASSKIPUSAGE, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASSKIPCONTENT, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASSUCCESSORS, sizeof(bool)); //1 byte, not 1 bit
			in->read((char*) &HASPREDECESSORS, sizeof(bool)); //1 byte, not 1 bit
			if (model_id >= GRAPHPATTERNMODEL+3 ) {
			    in->read((char*) &HASCOOCCURRENCE, sizeof(bool)); //1 byte, not 1 bit
			}
    	}
	} else {
		if (DEBUG) cerr << "NOT A GRAPHMODEL HEADER" << endl;
		HASPARENTS = false;
		HASCHILDREN = false;
		HASXCOUNT = false;
		HASTEMPLATES = false;
		HASINSTANCES = false;
		HASSKIPUSAGE = false;
		HASSKIPCONTENT = false;
		HASSUCCESSORS = false;
		HASPREDECESSORS = false;
		HASCOOCCURRENCE = false;
	}	
	if (DEBUG) {
	    if (secondpass) {
	        cerr << "STARTING SECOND PASS" << endl;
	    } else {
	        cerr << "STARTING FIRST PASS" << endl;
	    }
	}
	/*if ((model_id == UNINDEXEDPATTERNMODEL) && (DOFORWARDINDEX || DOREVERSEINDEX)) 
	  cerr << "WARNING!!! You opted to load indexes but you the model you are loading is unindexed!" << endl;
	if (!HASXCOUNT && DOXCOUNT) 
	  cerr << "WARNING!!! You opted to load exclusive count data but the model you are loading does not contain this data! (Make sure to load a graphmodel generated with exclusive count data)" << endl;
	if (!HASPARENTS && DOPARENTS) 
	  cerr << "WARNING!!! You opted to load parent relation data but the model you are loading does not contain this data! (Make sure to load a graphmodel generated with parent relation data)" << endl;
	if (!HASCHILDREN && DOCHILDREN) 
	  cerr << "WARNING!!! You opted to load children relation data but the model you are loading does not contain this data! (Make sure to load a graphmodel generated with children relation data)" << endl;*/
}

/*void SelectivePatternModel::readrelations(std::istream * in) {
	//Read and ignore relations (we don't care about them but we do need to read them)	
    uint16_t count;
    in->read((char*) &count,  sizeof(uint16_t));
    char gapcount;
    for (int i = 0; i < count; i++) {   
       in->read(&gapcount, sizeof(char));
       if (gapcount == 0) {
        EncNGram ngram = EncNGram(in);
       } else {
        EncSkipGram skipgram = EncSkipGram( in, gapcount);
       }           
    }    
}*/

int SelectivePatternModel::transitivereduction() {
	int pruned = 0;
	if ((HASCHILDREN) && (DOCHILDREN)) ::transitivereduction(rel_subsumption_children);
    if ((HASPARENTS) && (DOPARENTS)) ::transitivereduction(rel_subsumption_parents);
    return pruned;	
}

/*void SelectivePatternModel::readrelations(std::istream * in, const EncAnyGram * anygram, std::unordered_map<const EncAnyGram*,std::unordered_set<const EncAnyGram*> > * relationhash, bool ignore) {
    uint32_t count;
    in->read((char*) &count,  sizeof(uint32_t));
    char gapcount;        
    for (int i = 0; i < count; i++) {                        
       in->read(&gapcount, sizeof(char));
       if (gapcount == 0) {
        EncNGram ngram = EncNGram(in);
        if ((!ignore) && (secondpass) && (anygram != NULL) && (relationhash != NULL)) {
            const EncAnyGram * key = getkey(anygram);
        	const EncAnyGram * key2 = getkey((EncAnyGram*) &ngram);
        	if (!key2) {
        		cerr << "INTERNAL WARNING: Ngram not found ";
        		ngram.out();
        		cerr << endl;        	
        	} else if (key) {
        		(*relationhash)[key].insert(key2);
			}        		
        }
       } else {
        EncSkipGram skipgram = EncSkipGram( in, gapcount);
        if ((!ignore) && (secondpass) && (anygram != NULL) && (relationhash != NULL)) {
            const EncAnyGram * key = getkey(anygram);
        	const EncAnyGram * key2 = getkey((EncAnyGram*) &skipgram);
        	if (!key2) {
        		cerr << "INTERNAL WARNING: Ngram not found ";
        		skipgram.out();
        		cerr << endl;        	
        	} else if (key) {
        		(*relationhash)[key].insert(key2);
			} 
        }
       }           
    }    
}*/


void SelectivePatternModel::readngramdata(std::istream * in, const EncNGram & ngram,int ngramversion,  bool ignore) {
	//NOTE MAYBE TODO: make sure to update when GraphModel updates!

	if ((ngram.n() < MINLENGTH) || (ngram.n() > MAXLENGTH)) ignore = true;

	//READ STAGE -- nothing permanently stored yet

    uint32_t count;
    vector<uint32_t> index;
    uint32_t xcount = 0;     
    
    in->read((char*) &count, sizeof(uint32_t)); //read occurrence count		
	if (model_id != UNINDEXEDPATTERNMODEL) {	
	    index.reserve(count);
		for (unsigned int j = 0; j < count; j++) {
		    CorpusReference ref = CorpusReference(in); //read from file
		    if (!ignore) index.push_back(ref.sentence);         
		}
	}    
    if (HASXCOUNT) in->read((char*) &xcount, sizeof(uint32_t)); //read, process later
    
    if (secondpass) {    	
        if (&ngram == NULL) {
         cerr << "NULL!" << endl;
        }
    	if (HASPARENTS) readrelations(in, (const EncAnyGram*) &ngram, &rel_subsumption_parents,ngramversion);
    	if (HASCHILDREN) readrelations(in, (const EncAnyGram*) &ngram, &rel_subsumption_children,ngramversion);
    	if (HASTEMPLATES) readrelations(in,  (const EncAnyGram*) &ngram, &rel_templates,ngramversion);
        if (HASINSTANCES) readrelations(in, (const EncAnyGram*) &ngram, &rel_instances,ngramversion);
        if (HASSKIPUSAGE) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_skipusage,ngramversion);
        if (HASSKIPCONTENT) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_skipcontent,ngramversion);
        if (HASSUCCESSORS) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_successors,ngramversion);
        if (HASPREDECESSORS) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_predecessors,ngramversion);
        if (HASCOOCCURRENCE) readweightedrelations(in, (const EncAnyGram*) &ngram, &rel_cooccurrences,ngramversion);
    } else {
    	if (HASPARENTS) readrelations(in); //read and ignore
    	if (HASCHILDREN) readrelations(in);  //read and ignore
    	if (HASTEMPLATES) readrelations(in);
        if (HASINSTANCES) readrelations(in);
        if (HASSKIPUSAGE) readweightedrelations(in);
        if (HASSKIPCONTENT) readweightedrelations(in);
        if (HASSUCCESSORS) readweightedrelations(in);
        if (HASPREDECESSORS) readweightedrelations(in);
        if (HASCOOCCURRENCE) readweightedrelations(in);
    }
    
    //THRESHOLD CHECK STAGE - deciding whether to ignore based on unreached thresholds
    
	if ((count < (unsigned int) COUNTTHRESHOLD) || ((double) count / totaltypes < FREQTHRESHOLD)) ignore = true;
	if ((DOXCOUNT) && (HASXCOUNT) && ((xcount < XCOUNTTHRESHOLD) || (( (double) xcount / count) < XCOUNTRATIOTHRESHOLD) ) ) ignore = true;


	//ALIGNMENT MODEL CONSTRAINTS
	if (alignconstrain != NULL) {
		if (alignconstrainsource) {
			if (alignconstrain->getsourcekey((const EncAnyGram*) &ngram) == NULL) {
				ignore = true;
			}
		} else {
			if (alignconstrain->gettargetkey((const EncAnyGram*) &ngram) == NULL) {
				ignore = true;
			}
		}
	}

    //STORAGE STAGE
    if (!ignore) {
		ngrams[ngram]; //will create the ngram if it does not exist yet in the hash
		std::unordered_map<EncNGram,IndexCountData>::iterator iter = ngrams.find(ngram); //pointer to the ngram in the hash
		const EncAnyGram * anygram = &iter->first;	
		ngrams[ngram].count = count; // 0 if n/a
		ngrams[ngram].xcount = xcount; // 0 if n/a
		if (!index.empty()) {			
			for (vector<uint32_t>::iterator iter = index.begin(); iter != index.end(); iter++) {
				ngrams[ngram].sentences.insert(*iter);
		    	reverseindex[*iter].insert(anygram);
			}
		}	
	} else {
		ignoredtypes++;
		ignoredoccurrences += count;
	}        	        
}



void SelectivePatternModel::readskipgramdata(std::istream * in, const EncSkipGram & skipgram, int ngramversion, bool ignore) {
	//NOTE MAYBE TODO: make sure to update when GraphModel updates!

	if ( (!DOSKIPGRAMS) || (skipgram.n() < MINLENGTH) || (skipgram.n() > MAXLENGTH) ) ignore = true;
	 
	//READ STAGE -- nothing permanently stored yet
	
    uint32_t count;
    uint32_t skipcontentcount;
    uint32_t xcount = 0;
    vector<uint32_t> index;
    
    in->read((char*) &count, sizeof(uint32_t)); //read occurrence count
    
    
    if (model_id != UNINDEXEDPATTERNMODEL) {
		in->read((char*) &skipcontentcount, sizeof(uint32_t));   
		for (unsigned int j = 0; j < skipcontentcount; j++) {                                
		    EncSkipGram skipcontent = EncSkipGram(in);  
		    in->read((char*) &count, sizeof(uint32_t)); //read occurrence count
		    for (unsigned int k = 0; k < count; k++) {
		        CorpusReference ref = CorpusReference(in); //read from file       
		        if (!ignore) index.push_back(ref.sentence);            
		    }        
		}    		
	}
    if (HASXCOUNT) in->read((char*) &xcount, sizeof(uint32_t));     

    if (secondpass) {    		
    	if (HASPARENTS) readrelations(in, (const EncAnyGram*) &skipgram, &rel_subsumption_parents, ngramversion);
    	if (HASCHILDREN) readrelations(in, (const EncAnyGram*) &skipgram, &rel_subsumption_children, ngramversion);
    	if (HASTEMPLATES) readrelations(in,  (const EncAnyGram*) &skipgram, &rel_templates, ngramversion);
        if (HASINSTANCES) readrelations(in, (const EncAnyGram*) &skipgram, &rel_instances, ngramversion);
        if (HASSKIPUSAGE) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_skipusage, ngramversion);
        if (HASSKIPCONTENT) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_skipcontent, ngramversion);
        if (HASSUCCESSORS) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_successors, ngramversion);
        if (HASPREDECESSORS) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_predecessors, ngramversion);
        if (HASCOOCCURRENCE) readweightedrelations(in, (const EncAnyGram*) &skipgram, &rel_cooccurrences, ngramversion);
    } else {
    	if (HASPARENTS) readrelations(in); //read and ignore
    	if (HASCHILDREN) readrelations(in);  //read and ignore
    	if (HASTEMPLATES) readrelations(in);
        if (HASINSTANCES) readrelations(in);
        if (HASSKIPUSAGE) readweightedrelations(in);
        if (HASSKIPCONTENT) readweightedrelations(in);
        if (HASSUCCESSORS) readweightedrelations(in);
        if (HASPREDECESSORS) readweightedrelations(in);
        if (HASCOOCCURRENCE) readweightedrelations(in);    	    	
    }
            
    //THRESHOLD CHECK STAGE - deciding whether to ignore based on unreached thresholds
    
	if ((count < (unsigned int) COUNTTHRESHOLD) || ((double) count / totaltypes < FREQTHRESHOLD)) ignore = true;
	if ((DOXCOUNT) && (HASXCOUNT) && ((xcount < XCOUNTTHRESHOLD) || ((double) (xcount / count) < XCOUNTRATIOTHRESHOLD) ) ) ignore = true;
	
	//ALIGNMENT MODEL CONSTRAINTS
	if (alignconstrain != NULL) {
		if (alignconstrainsource) {
			if (alignconstrain->getsourcekey((const EncAnyGram*) &skipgram) == NULL) {
				ignore = true;
			}
		} else {
			if (alignconstrain->gettargetkey((const EncAnyGram*) &skipgram) == NULL) {
				ignore = true;
			}
		}
	}	
	
     //STORAGE STAGE
    if (!ignore) {
    	skipgrams[skipgram]; //will create the skipgram if it does not exist yet in the hash
		std::unordered_map<EncSkipGram,IndexCountData>::iterator iter = skipgrams.find(skipgram); //pointer to the skipgram in the hash
		const EncAnyGram * anygram = &iter->first;         
    	skipgrams[skipgram].count = count; // 0 if n/a
    	skipgrams[skipgram].xcount = xcount;  // 0 if n/a
		if (!index.empty()) {			
			for (vector<uint32_t>::iterator iter = index.begin(); iter != index.end(); iter++) {
		    	skipgrams[skipgram].sentences.insert(*iter);
		    	reverseindex[*iter].insert(anygram);
			}
		}	    	
    } else {
		ignoredtypes++;
		ignoredoccurrences += count;
	}   
    
        
}


int SelectivePatternModel::occurrencecount(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key)) > 0) return ngrams[*( (EncNGram*) key) ].count;
    } else {
        if (skipgrams.count(*( (EncSkipGram*) key) ) > 0) return skipgrams[*( (EncSkipGram*) key) ].count;   
    }
    return 0;
}

int SelectivePatternModel::coveragecount(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key)) > 0) return ngrams[*( (EncNGram*) key) ].count * key->n();
    } else {
        if (skipgrams.count(*( (EncSkipGram*) key) ) > 0) return skipgrams[*( (EncSkipGram*) key) ].count * key->n();   
    }
    return 0;
}



double SelectivePatternModel::coverage(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return  ((double) (ngrams[*( (EncNGram*) key) ].count * key->n())  / tokens());
    } else {
        if (skipgrams.count( *( (EncSkipGram*) key)) > 0) return ((double) (skipgrams[ *( (EncSkipGram*) key)].count * key->n()) / tokens());
    }
    return 0;
}




int SelectivePatternModel::xcount(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key)) > 0) return ngrams[*( (EncNGram*) key) ].xcount;
    } else {
        if (skipgrams.count(*( (EncSkipGram*) key) ) > 0) return skipgrams[*( (EncSkipGram*) key) ].xcount;   
    }
    return 0;
}

double SelectivePatternModel::xcountratio(const EncAnyGram* key) {    
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key)) > 0) return (double) ngrams[*( (EncNGram*) key) ].xcount / ngrams[*( (EncNGram*) key) ].count;
    } else {
        if (skipgrams.count(*( (EncSkipGram*) key) ) > 0) return (double) skipgrams[*( (EncSkipGram*) key) ].xcount / skipgrams[*( (EncSkipGram*) key) ].count;   
    }
    return 0;
}




int SelectivePatternModel::countforsentence(const EncAnyGram* key, const uint64_t sentence) {
    if (key->gapcount() == 0) {        
        if (ngrams.count(*( (EncNGram*) key) ) > 0) return ngrams[*( (EncNGram*) key) ].sentences.count(sentence) ;
    } else {
        if (skipgrams.count( *( (EncSkipGram*) key)) > 0) return skipgrams[ *( (EncSkipGram*) key)].sentences.count(sentence);
    }
    return 0;
}


/*set<int> * IndexedPatternModel::index(const EncAnyGram* key) {
    if (key->gapcount() == 0) {        
        if (ngram_index.count(*( (EncNGram*) key) ) > 0) return &ngram_index[*( (EncNGram*) key) ];
    } else {
        if (skipgram_index.count( *( (EncSkipGram*) key)) > 0) return &skipgram_index[ *( (EncSkipGram*) key)];
    }
}*/


void SelectivePatternModel::computestats() {
    totalngramcount = 0;
    totalskipgramcount = 0;
    for (int n = 1; n <= MAXN; n++) { ngramcount[n] = 0;  skipgramcount[n] = 0; ngramtypes[n] = 0; skipgramcount[n] = 0; }

    for (unordered_map<const EncNGram,IndexCountData >::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
        const EncAnyGram * anygram = &iter->first;
        ngramtypes[(int) anygram->n()]++;
        ngramcount[(int) anygram->n()] += iter->second.count;  
        totalngramcount += iter->second.count;   
    }
    for (unordered_map<const EncSkipGram,IndexCountData >::iterator iter = skipgrams.begin(); iter != skipgrams.end(); iter++) {
        const EncAnyGram * anygram = &iter->first;
        skipgramtypes[(int) anygram->n()]++;
        skipgramcount[(int) anygram->n()] += iter->second.count;  
        totalskipgramcount += iter->second.count;
    }    
}



void SelectivePatternModel::outputinstance(const EncAnyGram * anygram, CorpusReference ref, ClassDecoder & decoder) {
	cout << ref.sentence << ':' << (int) ref.token << "\t" << anygram->decode(decoder) << "\t" << occurrencecount(anygram) << "\t" << coverage(anygram);
	if (DOXCOUNT) cout << "\t" << xcount(anygram) << "\t" << xcountratio(anygram) << endl;
	cout << endl;	
}

bool SelectivePatternModel::exists(const EncAnyGram* key) const {
    if (key == NULL) return NULL;    
    if (key->gapcount() == 0) {
        return (ngrams.count(*( (const EncNGram*) key) ) > 0);
    } else {
        return (skipgrams.count(*( (const EncSkipGram*) key) ) > 0);
    }
    return false;
}



const EncAnyGram* SelectivePatternModel::getkey(const EncAnyGram* key) {
    if (key == NULL) {
        cerr << "INTERNAL WARNING: SelectivePatternModel::getkey(NULL)!" << endl;
        return NULL;
    }
    if (key->gapcount() == 0) {
        std::unordered_map<const EncNGram,IndexCountData >::iterator iter = ngrams.find(*( (EncNGram*) key) );
        if (iter != ngrams.end()) {
            return &iter->first;
        } else {
            return NULL;
        }
    } else {
        std::unordered_map<const EncSkipGram,IndexCountData >::iterator iter = skipgrams.find(*( (EncSkipGram*) key) );
        if (iter != skipgrams.end()) {
            return &iter->first;
        } else {
            return NULL;
        }        
    }
}

IndexCountData SelectivePatternModel::getdata(const EncAnyGram* key) {
    key = getkey(key);
    if (key == NULL) {
        cerr << "INTERNAL ERROR: SelectivePatternModel::getdata(NULL)!" << endl;
        throw InternalError();
    }
    if (key->gapcount() == 0) {
        return ngrams[*((const EncNGram *) key)];
    } else {
        return skipgrams[*((const EncSkipGram *) key)];
    }    
}


set<int> SelectivePatternModel::getsentences(const EncAnyGram * anygram) {
    const IndexCountData data = getdata(anygram);
    return set<int>(data.sentences.begin(), data.sentences.end());
}
    

unordered_map<const EncAnyGram*, int> SelectivePatternModel::getcooccurrences(const EncAnyGram * anygram, SelectivePatternModel * targetmodel,  set<int> * sentenceconstraints) {
    if (targetmodel == NULL) targetmodel = this;
        
    //find sourcegram sentence index
    set<int> sentences = getsentences(anygram);
    
    unordered_map<const EncAnyGram *,int> coocgrams;
    for (set<int>::iterator iter = sentences.begin(); iter != sentences.end(); iter++) {
        if (((sentenceconstraints == NULL) || (sentenceconstraints->count(*iter))) && (reverseindex.count(*iter))) {
            unordered_set<const EncAnyGram*> * tmp = &targetmodel->reverseindex[*iter];
            //find keys
            for (unordered_set<const EncAnyGram*>::iterator iter2 = tmp->begin(); iter2 != tmp->end(); iter2++) coocgrams[*iter2]++;
        }
    }
    
    return coocgrams;
}


void GraphRelations::readrelations(std::istream * in, const EncAnyGram * anygram, t_relations * relationhash, int ngramversion, bool ignore) {
    uint32_t count;
    in->read((char*) &count,  sizeof(uint32_t));
    char gapcount;        
    for (unsigned int i = 0; i < count; i++) {                        
       in->read(&gapcount, sizeof(char));
       if (gapcount == 0) {
        EncNGram ngram = EncNGram(in, ngramversion);
        if ((!ignore) && (secondpass) && (anygram != NULL) && (relationhash != NULL)) {
            const EncAnyGram * key = getkey(anygram);
        	const EncAnyGram * key2 = getkey((EncAnyGram*) &ngram);
        	if (!key2) {        	    
        	    //no warning, this is common if some types are ignored
        		//cerr << "INTERNAL WARNING: Ngram not found ";
        		//ngram.out();
        		//cerr << endl;        	
        	} else if (key) {
        		(*relationhash)[key].insert(key2);
			}        		
        }
       } else {
        EncSkipGram skipgram = EncSkipGram( in, gapcount, ngramversion);
        if ((!ignore) && (secondpass) && (anygram != NULL) && (relationhash != NULL)) {
            const EncAnyGram * key = getkey(anygram);
        	const EncAnyGram * key2 = getkey((EncAnyGram*) &skipgram);
        	if (!key2) {
        	    //no warning, this is common if some types are ignored
        		//cerr << "INTERNAL WARNING: Ngram not found ";
        		//skipgram.out();
        		//cerr << endl;        	
        	} else if (key) {
        		(*relationhash)[key].insert(key2);
			} 
        }
       }           
    }    
}


void GraphRelations::readweightedrelations(std::istream * in, const EncAnyGram * anygram, t_weightedrelations * relationhash, int ngramversion, bool ignore) {
    uint32_t count;
    double weight = 0;
    in->read((char*) &count,  sizeof(uint32_t));
    char gapcount;        
    for (unsigned int i = 0; i < count; i++) {                        
       in->read(&gapcount, sizeof(char));
       if (gapcount == 0) {
        EncNGram ngram = EncNGram(in, ngramversion);
        if (id() >= GRAPHPATTERNMODEL + 3) {
            in->read((char*) &weight,  sizeof(double));
        } else {
            weight = 0; //no weights supported in older versions
        }
        if ((!ignore) && (secondpass) && (anygram != NULL) && (relationhash != NULL)) {
            const EncAnyGram * key = getkey(anygram);
        	const EncAnyGram * key2 = getkey((EncAnyGram*) &ngram);
        	if (!key2) {        	    
        	    //no warning, this is common if some types are ignored
        		//cerr << "INTERNAL WARNING: Ngram not found ";
        		//ngram.out();
        		//cerr << endl;        	
        	} else if (key) {
        		(*relationhash)[key][key2] = weight;
			}        		
        }
       } else {
        EncSkipGram skipgram = EncSkipGram( in, gapcount, ngramversion);
        if (id() >= GRAPHPATTERNMODEL + 3) {
            in->read((char*) &weight,  sizeof(double));
        } else {
            weight = 0; //no weights supported in older versions
        }
        if ((!ignore) && (secondpass) && (anygram != NULL) && (relationhash != NULL)) {
            const EncAnyGram * key = getkey(anygram);
        	const EncAnyGram * key2 = getkey((EncAnyGram*) &skipgram);
        	if (!key2) {
        	    //no warning, this is common if some types are ignored
        		//cerr << "INTERNAL WARNING: Ngram not found ";
        		//skipgram.out();
        		//cerr << endl;        	
        	} else if (key) {
        		(*relationhash)[key][key2] = weight;
			} 
        }
       }           
    }    
}


void GraphRelations::getrelations(unordered_map<const EncAnyGram*,std::unordered_set<const EncAnyGram*> > & relations, const EncAnyGram * anygram, unordered_set<const EncAnyGram*> & container) {
    if (relations.count(anygram)) {
        for (unordered_set<const EncAnyGram*>::iterator iter = relations[anygram].begin(); iter != relations[anygram].end(); iter++) { 
            const EncAnyGram* parent = *iter;
            container.insert(parent);
            this->getrelations( relations, parent, container);
        }
    } 
}


void GraphPatternModel::extractbiskipgrams(ClassDecoder & classdecoder, std::ostream *OUT) {
    //extract loose bi-skipgrams based on co-occurrence information in the
    //graph, this assumes DOBIDIRECTIONALCOOC == false!
    for (t_weightedrelations::iterator iter = rel_cooccurrences.begin(); iter != rel_cooccurrences.end(); iter++) {
        for (unordered_map<const EncAnyGram*, double>::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++) {
            const EncAnyGram * part1 = model->getkey(iter->first);
            const EncAnyGram * part2 = model->getkey(iter2->first);
            const AnyGramData * data1 = model->getdata(part1);
            const AnyGramData * data2 = model->getdata(part2);
            set<CorpusReference> refs1 = data1->get_refs();
            set<CorpusReference> refs2 = data2->get_refs();
            //TODO
        }
    }
}
