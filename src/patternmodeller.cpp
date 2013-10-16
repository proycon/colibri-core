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
    cerr << "\t-j [modelfile]   2nd input model (e.g. test data)" << endl;
    cerr << " Building a model:  patternmodeller -o [modelfile] -f [datafile] -c [classfile]" << endl;
    cerr << "\t-t <number>      Occurrence threshold: patterns occuring less than this will be pruned (default: 2)" << endl;    
    cerr << "\t-u               Build an unindexed model" << endl;    
    cerr << "\t-l <number>      Maximum pattern length (default: 8)" << endl;
    cerr << "\t-s               Compute fixed-size skip-grams (costs extra memory and time)" << endl;    
    cerr << "\t-T <number>      Skip type threshold: only skipgrams with at least x possible types for the skip will be considered, otherwise the skipgram will be pruned  (default: 2, this value is unchangable and fixed to 2 when -u is set). Requires indexed models" << endl;
    cerr << " Viewing a model:  patternmodeller -i [modelfile] -c [classfile] -[PRHQ]" << endl;
    cerr << "\t-P               Print the entire model" << endl;
    cerr << "\t-R               Generate a (statistical/coverage) report" << endl;
    cerr << "\t-H               Generate a histogram" << endl;   
    cerr << "\t-Q               Start interactive query mode, allows for pattern lookup against the loaded model (input from standard input)" << endl; 
    cerr << "\t-q               Query a pattern (may be specified multiple times!)" << endl; 
    cerr << "\t-r               Compute relationships for the specified patterns (use with -q or -Q). Relationships are: subsumptions, neigbours, skipcontent " << endl; 
    cerr << "\t-G               Output relationship graph in graphviz format (use with -q)" << endl; 
    cerr << "\tOptions -tlT can be used to further filter the model" << endl;
    cerr << "Editing a model:  patternmodeller -o [modelfile] -i [modelfile]" << endl;
    cerr << "\tOptions -tlT can be used to filter the model, -u can be used to remove the index" << endl;
    cerr << "Run train model on test data:  patternmodeller -o [modelfile] -i [modelfile] -j [modelfile2] --test" << endl;
    cerr << " New model will only contain counts from model2 but only for the patterns also occurring in model1. The total number of tokens equals that of model2 (i.e. the amount uncovered is retained in the model)" << endl;
}

template<class ModelType = IndexedPatternModel<>>
void processquerypattern(ModelType & model, ClassDecoder * classdecoder, const Pattern & pattern, bool dorelations) {
    if (!model.has(pattern)) {
        cout << "PATTERN NOT FOUND IN MODEL" << endl;
    } else {
        cout << pattern.tostring(*classdecoder) << "\t" << model.occurrencecount(pattern) << "\t" << setprecision(numeric_limits<double>::digits10 + 1) << model.coverage(pattern) << endl; 
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
void viewmodel(ModelType & model, ClassDecoder * classdecoder,  ClassEncoder * classencoder, bool print, bool report,  bool histogram , bool query, bool relations) {
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
    char c;    
    while ((c = getopt(argc, argv, "c:i:j:o:f:t:ul:sT:PRHQDhq:r")) != -1)
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
            outputmodelfile = UNINDEXEDPATTERNMODEL;
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
    }

    if (inputmodeltype == INDEXEDPATTERNMODEL) {
        cerr << "Loading indexed pattern model " << inputmodelfile << " as input model..."<<endl;
        IndexedPatternModel<> inputmodel = IndexedPatternModel<>(inputmodelfile, options);
        inputmodel.pruneskipgrams(options.MINTOKENS, options.MINSKIPTYPES);
        
        if (!outputmodelfile.empty()) {
            inputmodel.write(outputmodelfile);
        }
        viewmodel<IndexedPatternModel<>>(inputmodel, classdecoder, classencoder, DOPRINT, DOREPORT, DOHISTOGRAM, DOQUERIER, DORELATIONS); 
        if (!querypatterns.empty()) processquerypatterns<IndexedPatternModel<>>(inputmodel,  classencoder, classdecoder, querypatterns, DORELATIONS);
        
    } else if (inputmodeltype == UNINDEXEDPATTERNMODEL) {
        cerr << "Loading unindexed pattern model " << inputmodelfile << " as input model..."<<endl;
        PatternModel<uint32_t> inputmodel = PatternModel<uint32_t>(inputmodelfile, options);

        if (!outputmodelfile.empty()) {
            inputmodel.write(outputmodelfile);
        }
        viewmodel<PatternModel<uint32_t>>(inputmodel, classdecoder, classencoder, DOPRINT, DOREPORT, DOHISTOGRAM, DOQUERIER, DORELATIONS); 
        if (!querypatterns.empty()) processquerypatterns<PatternModel<uint32_t>>(inputmodel,  classencoder, classdecoder, querypatterns, DORELATIONS);

    } else if (!inputmodelfile.empty()) {
        cerr << "ERROR: Input model is not a valid colibri pattern model" << endl;
        exit(2);
    } else {
        //operations without input model

        if (outputmodeltype == INDEXEDPATTERNMODEL) {
            IndexedPatternModel<> outputmodel = IndexedPatternModel<>();
            if (!corpusfile.empty()) {
                //build new model from corpus
                cerr << "Building new indexed model from  " << corpusfile <<endl;
                outputmodel.train(corpusfile, options);
            }

            if (!outputmodelfile.empty()) {
                cerr << "Saving indexed pattern model to " << outputmodelfile <<endl;
                outputmodel.write(outputmodelfile);
            }
        } else if (outputmodeltype == UNINDEXEDPATTERNMODEL) {
            PatternModel<uint32_t> outputmodel = PatternModel<uint32_t>();
            if (!corpusfile.empty()) {
                //build new model from corpus
                cerr << "Building new unindexed model from  " << corpusfile <<endl;
                outputmodel.train(corpusfile, options);
            }
            if (!outputmodelfile.empty()) {
                cerr << "Saving unindexed pattern model to " << outputmodelfile <<endl;
                outputmodel.write(outputmodelfile);
            }

        }

    }
}





    



