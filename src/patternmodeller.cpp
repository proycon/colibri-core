#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithms.h>
#include <common.h>
#include <getopt.h>
#include <patternmodel.h>

using namespace std;



void usage() {
    cerr << "Input/output options:" << endl;
    cerr << "\t-i [modelfile]   Input model" << endl;
    cerr << "\t-o [modelfile]   Output model" << endl;
    cerr << "\t-f [datafile]    Corpus data file" << endl;
    cerr << "\t-c [classfile]   Class file"<< endl;
    cerr << "\t-j [modelfile]   Joined input model. Result will be the *intersection* of this (training) model and the input model or constructed model." << endl;
    cerr << " Building a model:  patternmodeller -o [modelfile] -f [datafile] -c [classfile]" << endl;
    cerr << "\t-t <number>      Occurrence threshold: patterns occuring less than this will be pruned (default: 2)" << endl;    
    cerr << "\t-u               Build an unindexed model (default is indexed)" << endl;    
    cerr << "\t-l <number>      Maximum pattern length (default: 100)" << endl;
    cerr << "\t-s               Compute fixed-size skip-grams (costs extra memory and time)" << endl;    
    cerr << "\t-T <number>      Skip type threshold (for use with -s): only fixed skipgrams with at least x possible types for the skip will be considered, otherwise the skipgram will be pruned  (default: 2, unindexed models always act as if fixed to 1). Also note that only types that occur above the occurrent threshold (-t) are counted here! Requires indexed models" << endl;
    cerr << "\t-S S             Compute dynamic-size skip-grams by abstracting over fixed-size skipgrams (implies -s)." << endl; 
    cerr << "\t-S <number>      Compute dynamic-size skip-grams (of type X {*} Y only) by using co-occurrence information. The number is the normalised pointwise information threshold above which to form skipgrams. Only for indexed models." << endl; 
    cerr << " Viewing a model:  patternmodeller -i [modelfile] -c [classfile] -[PRHQ]" << endl;
    cerr << "\t-P               Print the entire model" << endl;
    cerr << "\t-R               Generate a (statistical/coverage) report" << endl;
    cerr << "\t-H               Generate a histogram" << endl;   
    cerr << "\t-Q               Start interactive query mode, allows for pattern lookup against the loaded model (input from standard input)" << endl; 
    cerr << "\t-q               Query a pattern (may be specified multiple times!)" << endl; 
    cerr << "\t-r               Compute and show relationships for the specified patterns (use with -q or -Q). Relationships are: subsumptions, neigbours, skipcontent. Only for indexed models." << endl; 
    cerr << "\t-C <threshold>   Compute and show co-occurrence counts above the specified threshold [-1,1] (normalised pointwise mutual information). Only for indexed models." << endl;
    //cerr << "\t-G               Output relationship graph in graphviz format (use with -q)" << endl; 
    cerr << "\tOptions -tlT can be used to further filter the model" << endl;
    cerr << "Editing a model:  patternmodeller -o [modelfile] -i [modelfile]" << endl;
    cerr << "\t-x               Delete all fixed-size skipgrams from the model" << endl;    
    cerr << "\t-X               Delete all dynamic-size skipgrams from the model" << endl;    
    cerr << "\t-N               Delete all ngrams from the model" << endl;    
    cerr << "\tOptions -tlT can be used to filter the model, -u can be used to remove the index, -j can be used to take the intersection with another model, -S to compute and add dynamic width-skipgrams" << endl;
    cerr << "Building a model constrained by another model:  patternmodeller -o [modelfile] -j [trainingmodel] -f [datafile] -c [classfile]" << endl;
}

template<class ModelType = IndexedPatternModel<>>
void processquerypattern(ModelType & model, ClassDecoder * classdecoder, const Pattern & pattern, bool dorelations) {
    if (!model.has(pattern)) {
        cout << "PATTERN \"" << pattern.tostring(*classdecoder) << "\" NOT FOUND IN MODEL" << endl;
    } else {
        model.print(&cout, *classdecoder, pattern);
        if (dorelations) model.outputrelations(pattern, *classdecoder, &cout);
    }
}

template<class ModelType = IndexedPatternModel<>>
void processquerypatterns(ModelType & model, ClassEncoder * classencoder, ClassDecoder * classdecoder, vector<string> & querypatterns, bool dorelations) {
    const bool allowunknown = true;
    unsigned char buffer[65536];
    for (vector<string>::iterator iter = querypatterns.begin(); iter != querypatterns.end(); iter++) {
       const string s = *iter;
       const int buffersize = classencoder->encodestring(s, buffer, allowunknown); 
       const Pattern pattern = Pattern(buffer, buffersize);
       processquerypattern<ModelType>(model,classdecoder,pattern, dorelations);
    }
}


template<class ModelType = IndexedPatternModel<>>
void querymodel(ModelType & model, ClassEncoder * classencoder, ClassDecoder * classdecoder, bool dorelations, bool repeat = true) {
    const bool allowunknown = true;
    unsigned char buffer[65536];
    uint32_t linenum = 0;
    std::string line;
    cerr << "Colibri Patternmodeller -- Interactive query mode." << endl;
    cerr << "  Type ctrl-D to quit, type X to switch between exact mode and extensive mode (default: extensive mode)." << endl;
    bool exact = false;
    do {
            linenum++;
            cerr << linenum << ">> "; 
            getline(cin,line);            
            if ((line == "X") || (line == "X\n")) {
                exact = !exact;
                if (exact) {
                    cerr << "Switched to Exact mode - Only exact matches will be shown now" << endl;
                } else {
                    cerr << "Switched to Extensive mode - Input will be scanned for all matching patterns" << endl;
                }
            } else if (!line.empty()) {
                const int buffersize = classencoder->encodestring(line, buffer, allowunknown); 
                Pattern linepattern = Pattern(buffer, buffersize);
                if (exact) { 
                    processquerypattern<ModelType>(model,classdecoder, linepattern, dorelations);
                } else {
                    vector<pair<Pattern, int> > patterns = model.getpatterns(linepattern);
                    if (model.has(linepattern)) {
                        const IndexReference ref = IndexReference(linenum,0);

                        //process and output instance
                        cout << ref.sentence << ':' << (int) ref.token << "\t";
                        processquerypattern<ModelType>(model, classdecoder, linepattern, dorelations);                                
                    }
                    for (vector<pair<Pattern,int> >::iterator iter = patterns.begin(); iter != patterns.end(); iter++) {
                            const Pattern pattern = iter->first;
                            const IndexReference ref = IndexReference(linenum,iter->second);

                            //process and output instance
                            cout << ref.sentence << ':' << (int) ref.token << "\t";
                            processquerypattern<ModelType>(model, classdecoder, pattern, dorelations);                                
                    } 
                }
            }
    } while (!cin.eof() && (repeat)); 
}



template<class ModelType = IndexedPatternModel<>>
bool viewmodel(ModelType & model, ClassDecoder * classdecoder,  ClassEncoder * classencoder, bool print, bool report,  bool histogram , bool query, bool relations) {
    if (print) {
        if (classdecoder == NULL) {
            cerr << "ERROR: Unable to print model, no class file specified (-c)" << endl;
        } else {
            model.print(&cout, *classdecoder);
        }
    }
    if (report) {
        model.report(&cout);
    }
    if (histogram) {
        model.histogram(&cout);
    }
    if (query) {
        if (classencoder == NULL) {
            cerr << "ERROR: Unable to query model, no class encoder specified (-c)" << endl;
        } else {
            querymodel<ModelType>(model, classencoder, classdecoder, relations); 
        }
    }
    return (print || report || histogram || query);
}

template<class A,class B,class C>
void prunebymodel(PatternModel<A,B,C> & inputmodel, const std::string & inputmodelfile2, int inputmodeltype2, PatternModelOptions options) {
    if (inputmodeltype2 == UNINDEXEDPATTERNMODEL) {
        PatternModel<uint32_t> inputmodel2 = PatternModel<uint32_t>(inputmodelfile2, options);
        inputmodel.prunebymodel(inputmodel2);
    } else if (inputmodeltype2 == INDEXEDPATTERNMODEL) {
        IndexedPatternModel<> inputmodel2 = IndexedPatternModel<>(inputmodelfile2, options);
        inputmodel.prunebymodel(inputmodel2);
    }
}

int main( int argc, char *argv[] ) {
    
    string classfile = "";
    string inputmodelfile = "";
    string inputmodelfile2 = "";
    string outputmodelfile = "";
    string corpusfile = "";
    

    string modelfile = "";
    string modelfile2 = "";
    string covviewfile = ""; //not used yet
   
    vector<string> querypatterns;
    
    
    PatternModelOptions options;
    options.DOREVERSEINDEX = true;  //TODO: make configurable?

    int outputmodeltype = INDEXEDPATTERNMODEL;
    bool DOQUERIER = false;
    bool DOREPORT = false;
    bool DOHISTOGRAM = false;
    bool DOPRINT = false;
    bool DORELATIONS = false;
    bool DEBUG = false;
    bool DODYNSKIPFROMFIXED = false;
    bool DODYNSKIPFROMCOOC = false;
    double COOCTHRESHOLD = 0;
    bool DOCOOC = false;
    bool DOREMOVEFIXEDSKIPGRAMS = false;
    bool DOREMOVEDYNAMICSKIPGRAMS = false;
    bool DOREMOVENGRAMS = false;
    char c;    
    while ((c = getopt(argc, argv, "hc:i:j:o:f:t:ul:sT:PRHQDhq:rGS:xXNC:")) != -1)
        switch (c)
        {
        case 'c':
            classfile = optarg;
            break;
        case 'i':
            inputmodelfile = optarg;
            break;
        case 'j':
            inputmodelfile2 = optarg;
            break;
        case 'D':
        	DEBUG = true;
        	break;
        case 'R':
            DOREPORT = true;
            break;            
        case 'f':
            corpusfile = optarg;
            break;        
        case 't':
            options.MINTOKENS = atoi(optarg);
            break;
        case 'T':
            options.MINSKIPTYPES = atoi(optarg);            
            break;
        case 'l':
            options.MAXLENGTH = atoi(optarg);            
            break;
        case 's':
            options.DOFIXEDSKIPGRAMS = true;
            break;
        case 'o': 
            outputmodelfile = optarg;
            break;
		case 'u':
            outputmodeltype = UNINDEXEDPATTERNMODEL;
			break;
		case 'r':
            DORELATIONS = true;
			break;
		case 'Q':
			DOQUERIER = true;
			break;
        case 'q':
            querypatterns.push_back(optarg);
            break;
        case 'H':
            DOHISTOGRAM = true;
            break;        
        case 'P':
            DOPRINT = true;
            break;        
        case 'S':
            if (string(optarg) == "S") {
                DODYNSKIPFROMFIXED = true;
                options.DOFIXEDSKIPGRAMS = true;
            } else {
                DODYNSKIPFROMCOOC = true;
                COOCTHRESHOLD = atof(optarg);
            }
            break;
        case 'C':
            DOCOOC = true;
            COOCTHRESHOLD = atof(optarg);
            break;
        case 'x':
            DOREMOVEFIXEDSKIPGRAMS = true;
            break;
        case 'X':
            DOREMOVEDYNAMICSKIPGRAMS = true;
            break;
        case 'N':
            DOREMOVENGRAMS = true;
            break;
        case 'G':
            cerr << "Option -G NOT IMPLEMENTED YET!" << endl;
            exit(2);
        case 'h':
            usage();
            exit(0);
        case '?':
            if (optopt == 'c') {
                cerr <<  "Option -" << optopt << " requires an argument." << endl;
            } else {
                cerr << "Unknown option: -" <<  optopt << endl;
            }
            
            return 1;
        default:
            cerr << "Unknown option: -" <<  optopt << endl;
            abort ();
        }
  
    bool didsomething = false;
    if ((inputmodelfile.empty()) && (corpusfile.empty())) {
        cerr << "ERROR: No input model (-i) or corpus data file specified (-f), specify at least one." << classfile << endl;
        exit(2);
    }


    ClassDecoder * classdecoder = NULL;
    ClassEncoder * classencoder = NULL;

    if (!classfile.empty()) {
        cerr << "Loading class decoder from file " << classfile << endl;
        classdecoder = new ClassDecoder(classfile);
        if ((DOQUERIER) || (!querypatterns.empty())) {
            cerr << "Loading class encoder from file " << classfile << endl;
            classencoder = new ClassEncoder(classfile);
        }
    }

    int inputmodeltype = 0;
    if (!inputmodelfile.empty()) {
        inputmodeltype = getmodeltype(inputmodelfile);
        if ((inputmodeltype == INDEXEDPATTERNMODEL) && (outputmodeltype == UNINDEXEDPATTERNMODEL)) {
            cerr << "Indexed input model will be read as unindexed because -u was set" << endl;
            inputmodeltype = UNINDEXEDPATTERNMODEL; //will read indexed models as unindexed automatically
        }
    }
    
    int inputmodeltype2 = 0;
    if (!inputmodelfile2.empty()) {
        inputmodeltype2 = getmodeltype(inputmodelfile2);
        if ((inputmodeltype2 == INDEXEDPATTERNMODEL) && (outputmodeltype == UNINDEXEDPATTERNMODEL)) {
            cerr << "Indexed joint model will be read as unindexed because -u was set" << endl;
            inputmodeltype2 = UNINDEXEDPATTERNMODEL; //will read indexed models as unindexed automatically
        }
    }



    if (inputmodeltype == INDEXEDPATTERNMODEL) {
        cerr << "Loading indexed pattern model " << inputmodelfile << " as input model..."<<endl;
        IndexedPatternModel<> inputmodel = IndexedPatternModel<>(inputmodelfile, options);
        if (!inputmodelfile2.empty()) prunebymodel(inputmodel, inputmodelfile2, inputmodeltype2, options);
        inputmodel.pruneskipgrams(options.MINTOKENS, options.MINSKIPTYPES);


        
        if (!outputmodelfile.empty()) {
            didsomething = true;
            inputmodel.write(outputmodelfile);
        }
        didsomething = viewmodel<IndexedPatternModel<>>(inputmodel, classdecoder, classencoder, DOPRINT, DOREPORT, DOHISTOGRAM, DOQUERIER, DORELATIONS) || didsomething; 
        if (!querypatterns.empty()) {
            didsomething = true;
            processquerypatterns<IndexedPatternModel<>>(inputmodel,  classencoder, classdecoder, querypatterns, DORELATIONS);
        }
        
        
    } else if (inputmodeltype == UNINDEXEDPATTERNMODEL) {
        cerr << "Loading unindexed pattern model " << inputmodelfile << " as input model..."<<endl;
        PatternModel<uint32_t> inputmodel = PatternModel<uint32_t>(inputmodelfile, options);

        if (!inputmodelfile2.empty()) prunebymodel(inputmodel, inputmodelfile2, inputmodeltype2, options);

        if (!outputmodelfile.empty()) {
            didsomething = true;
            inputmodel.write(outputmodelfile);
        }
        didsomething = viewmodel<PatternModel<uint32_t>>(inputmodel, classdecoder, classencoder, DOPRINT, DOREPORT, DOHISTOGRAM, DOQUERIER, DORELATIONS) || didsomething; 
        if (!querypatterns.empty()) {
            didsomething = true;
            processquerypatterns<PatternModel<uint32_t>>(inputmodel,  classencoder, classdecoder, querypatterns, DORELATIONS);
        }

    } else if (!inputmodelfile.empty()) {
        cerr << "ERROR: Input model is not a valid colibri pattern model" << endl;
        exit(2);
    } else {
        //operations without input model
        PatternModelInterface * constrainbymodel = NULL;
        if (!inputmodeltype2 == INDEXEDPATTERNMODEL) {
            constrainbymodel = new IndexedPatternModel<>(inputmodelfile2, options);
        } else if (!inputmodeltype2 == INDEXEDPATTERNMODEL) {
            constrainbymodel = new PatternModel<uint32_t>(inputmodelfile2, options);
        }


        if (outputmodeltype == INDEXEDPATTERNMODEL) {
            IndexedPatternModel<> outputmodel = IndexedPatternModel<>();
            if (!corpusfile.empty()) {
                //build new model from corpus
                cerr << "Building new indexed model from  " << corpusfile <<endl;
                outputmodel.train(corpusfile, options, constrainbymodel);
            }

            if (!outputmodelfile.empty()) {
                cerr << "Saving indexed pattern model to " << outputmodelfile <<endl;
                outputmodel.write(outputmodelfile);
                didsomething = true;
            }
            didsomething = viewmodel<IndexedPatternModel<>>(outputmodel, classdecoder, classencoder, DOPRINT, DOREPORT, DOHISTOGRAM, DOQUERIER, DORELATIONS) || didsomething; 
            if (!querypatterns.empty()) {
                didsomething = true;
                processquerypatterns<IndexedPatternModel<>>(outputmodel,  classencoder, classdecoder, querypatterns, DORELATIONS);
            }
        } else if (outputmodeltype == UNINDEXEDPATTERNMODEL) {
            PatternModel<uint32_t> outputmodel = PatternModel<uint32_t>();
            if (!corpusfile.empty()) {
                //build new model from corpus
                cerr << "Building new unindexed model from  " << corpusfile <<endl;
                outputmodel.train(corpusfile, options, constrainbymodel);
            }
            if (!outputmodelfile.empty()) {
                cerr << "Saving unindexed pattern model to " << outputmodelfile <<endl;
                outputmodel.write(outputmodelfile);
                didsomething = true;
            }
            didsomething = viewmodel<PatternModel<uint32_t>>(outputmodel, classdecoder, classencoder, DOPRINT, DOREPORT, DOHISTOGRAM, DOQUERIER, DORELATIONS) || didsomething; 
            if (!querypatterns.empty()) {
                didsomething = true;
                processquerypatterns<PatternModel<uint32_t>>(outputmodel,  classencoder, classdecoder, querypatterns, DORELATIONS);
            }

        }

    }


    if (!didsomething)  {
        cerr << "Ooops... You didn't really give me anything to do...that can't be right.. Please study the usage options (-h) again! Did you perhaps forget a -P or -o? " << endl;
        exit(1);
    }
}





    



