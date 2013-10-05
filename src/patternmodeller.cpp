#include <patternmodel.h>
#include <alignmodel.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithms.h>
#include <common.h>
#include <getopt.h>

using namespace std;



void usage() {
    cerr << "Input/output options:" << endl;
    cerr << "\t-i [modelfile]   Input model" << endl;
    cerr << "\t-o [modelfile]   Output model" << endl;
    cerr << "\t-f [datafile]    Corpus data file" << endl;
    cerr << "\t-c [classfile]   Class file"<< endl;
    cerr << "\t-j [modelfile]   2nd input model (e.g. test data)" << endl;
    cerr << " Building a model:  patternmodeller -o [modelfile] -f [datafile] -c [classfile]"
    cerr << "\t-t <number>      Occurrence threshold: patterns occuring less than this will be pruned (default: 2)" << endl;    
    cerr << "\t-u               Build an unindexed model" << endl;    
    cerr << "\t-l <number>      Maximum pattern length (default: 8)" << endl;
    cerr << "\t-s               Compute fixed-size skip-grams (costs extra memory and time)" << endl;    
    cerr << "\t-T <number>      Skip threshold: only skip content that occurs at least x times will be considered (default: same as -t). Value can never be lower than value for -t. Requires indexed models." << endl;
    cerr << "\t-S <number>      Skip type threshold: only skipgrams with at least x possible types for the skip will be considered, otherwise the skipgram will be pruned  (default: 2, this value is unchangable and fixed to 2 when -u is set). Required indexed models" << endl;
    cerr << " Viewing a model:  patternmodeller -i [modelfile] -c [classfile] -[PRCHQ]
    cerr << "\t-P               Print the entire model" << endl;
    cerr << "\t-R               Generate a simple statistical report" << endl;
    cerr << "\t-C               Generate an extensive coverage report (indexed patternmodels) only)" << endl;
    cerr << "\t-H               Generate a histogram report" << endl;   
    cerr << "\t-Q               Start query mode, allows for pattern lookup against the loaded model" << endl; 
    cerr << "\tOptions -tlTS can be used to further filter the model" << endl;
    cerr << "Editing a model:  patternmodeller -o [modelfile] -i [modelfile] 
    cerr << "\tOptions -tlTS can be used to filter the model, -u can be used to remove the index" << endl;
    cerr << "Run train model on test data:  patternmodeller -o [modelfile] -i [modelfile] -j [modelfile2] --test" << endl;
    cerr << " New model will only contain counts from model2 but only for the patterns also occurring in model1. The total number of tokens equals that of model2 (i.e. the amount uncovered is retained in the model)" << endl;
}


void decode(IndexedPatternModel & model, string classfile) {
    cerr << "Loading class decoder " << classfile << endl;
    ClassDecoder classdecoder = ClassDecoder(classfile);
    
    /*const string ngramoutputfile = outputprefix + ".ngrams";
    ofstream *NGRAMSOUT =  new ofstream( ngramoutputfile.c_str() );      
    const string skipgramoutputfile = outputprefix + ".skipgrams";
    ofstream *SKIPGRAMSOUT = NULL;*/
    //if (DOSKIPGRAMS) SKIPGRAMSOUT = new ofstream( skipgramoutputfile.c_str() );      
    cerr << "Decoding" << endl;
    model.decode(classdecoder, (ostream*) &stdout);   
}


int main( int argc, char *argv[] ) {
    
    string classfile = "";
    string corpusfile = "";
    string outputprefix = "";
    string modelfile = "";
    string modelfile2 = "";
    string covviewfile = "";
    string alignmodelfile = "";
    
    
    
    int MINTOKENS = 2;
    int MINSKIPTOKENS = 2;
    unsigned int MINSKIPTYPES = 2;
    int MAXLENGTH = 8;
    bool DOSKIPGRAMS = false;
    bool DOINDEX = true;
    bool DOINITIALONLYSKIP = true;
    bool DOFINALONLYSKIP = true;
    bool DOQUERIER = false;
    bool DOREPORT = false;
    bool DOCOVERAGE = false;
    bool DOCOVVIEW = false;
    bool DOHISTOGRAM = false;
    bool OUTPUTHASH=false;
    //bool DOCOMPOSITIONALITY = false;
    bool DEBUG = false;
    double alignthreshold = 0.0;
    char c;    
    while ((c = getopt(argc, argv, "c:f:d:t:T:S:l:o:suLhnBEQDJ:CRVA:P:Hi")) != -1)
        switch (c)
        {
        case 'c':
            classfile = optarg;
            break;
        case 'd':
            modelfile = optarg;
            break;
        case 'J':
            modelfile2 = optarg;
            break;
        case 'D':
        	DEBUG = true;
        	break;
        case 'C':
            DOCOVERAGE = true;
            break;
        case 'R':
            DOREPORT = true;
            break;            
        case 'V':
            DOCOVVIEW = true;
            break;
        case 'f':
            corpusfile = optarg;
            break;        
        case 't':
            MINTOKENS = atoi(optarg);
            break;
        case 'T':
            MINSKIPTOKENS = atoi(optarg);            
            break;
        case 'S':
            MINSKIPTYPES = atoi(optarg);            
            break;
        case 'l':
            MAXLENGTH = atoi(optarg);            
            break;
        case 's':
            DOSKIPGRAMS = true;
            break;
        case 'o': 
            outputprefix = optarg;
            break;
        case 'B':
            DOINITIALONLYSKIP = false;
            break;
        case 'E':
            DOFINALONLYSKIP = false;    
            break;
		case 'u':
			DOINDEX = false;    		
			break;
		case 'Q':
			DOQUERIER = true;
			break;
	    case 'A':
	        alignmodelfile = optarg;
	        break;
        case 'P':
            alignthreshold = atof(optarg);
            break;	
        case 'H':
            DOHISTOGRAM = true;
            break;        
        case 'i':
            OUTPUTHASH=true;
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
    
    if (DOQUERIER && classfile.empty()) {
            cerr << "ERROR: To use the query mode (-Q), specify a -c classfile and an existing model (-d)" << endl;
            usage();
            exit(2);    	
    }
    
    if (DOCOVVIEW && (classfile.empty() || modelfile.empty() || corpusfile.empty() )) {
        cerr << "ERROR: When generating a coverage view, you need to specify a class file, model file and corpus file (-c, -d, -f)" << endl;
        usage();
        exit(2);
    }
    /*if (corpusfile.empty() ) {
        if (modelfile.empty() || classfile.empty()) {
            cerr << "ERROR: Need to specify -f corpusfile to compute pattern, or -d modelfile -c classfile to decode an existing model" << endl;
            usage();
            exit(2);
        }
    }*/
    
    if (outputprefix.empty()) {
        outputprefix = corpusfile; //TODO: strip .clsenc. .bin?
        strip_extension(outputprefix,"colibri");
        strip_extension(outputprefix,"bin");
        strip_extension(outputprefix,"clsenc");
        strip_extension(outputprefix,"txt");    
    }

    
    if ((!corpusfile.empty()) && (!modelfile.empty())) {
    
        if (DOCOVVIEW) {
            IndexedPatternModel model = IndexedPatternModel(modelfile, DEBUG);
            ClassDecoder classdecoder = ClassDecoder(classfile);
            model.coveragereport((ostream*) &cerr, corpusfile, (ostream*) &cout, &classdecoder);      
        } else {

            if (DOINDEX) {
                //not implemented yet
                cerr << "Loading reference model" << endl;
		        IndexedPatternModel refmodel = IndexedPatternModel(modelfile, DEBUG);
		        
		        cerr << "Computing model on " << corpusfile << endl;
		        IndexedPatternModel model = IndexedPatternModel(corpusfile, refmodel, MAXLENGTH, MINTOKENS, DOSKIPGRAMS, MINSKIPTOKENS, MINSKIPTYPES, DOINITIALONLYSKIP,DOFINALONLYSKIP);

		        cerr << "Saving " << outputprefix << ".indexedpatternmodel.colibri"  << endl;
		        const string outputfile = outputprefix + ".indexedpatternmodel.colibri";
		        model.save(outputfile);     
            } else {
                cerr << "Loading reference model" << endl;
		        UnindexedPatternModel refmodel = UnindexedPatternModel(modelfile, DEBUG);
		        
		        cerr << "Computing model on " << corpusfile << endl;
		        UnindexedPatternModel model = UnindexedPatternModel(corpusfile, refmodel, MAXLENGTH, MINTOKENS, DOSKIPGRAMS, MINSKIPTOKENS, MINSKIPTYPES, DOINITIALONLYSKIP,DOFINALONLYSKIP);

		        cerr << "Saving " << outputprefix << ".unindexedpatternmodel.colibri"  << endl;
		        const string outputfile = outputprefix + ".unindexedpatternmodel.colibri";
		        model.save(outputfile);       		  
            }
        }
        
    } else if (!corpusfile.empty()) {
    	if (DOINDEX) {
    
		    cerr << "Computing model on " << corpusfile << endl;
		    IndexedPatternModel model = IndexedPatternModel(corpusfile, MAXLENGTH, MINTOKENS, DOSKIPGRAMS, MINSKIPTOKENS, MINSKIPTYPES, DOINITIALONLYSKIP,DOFINALONLYSKIP);
		        
		    cerr << "Saving " << outputprefix << ".indexedpatternmodel.colibri"  << endl;
		    const string outputfile = outputprefix + ".indexedpatternmodel.colibri";
		    model.save(outputfile);            
		    
		    
		    if (!classfile.empty()) {
		        cerr << "Loading class decoder " << classfile << endl;
		        ClassDecoder classdecoder = ClassDecoder(classfile);
		        if (DOQUERIER) {
		        	cerr << "Loading class encoder " << classfile << endl;
		        	ClassEncoder classencoder = ClassEncoder(classfile);
		        	cerr << "Starting query mode:" << endl;
		        	model.querier(classencoder, classdecoder);
		        } else {
		        	cerr << "Decoding" << endl;
		        	model.decode(classdecoder, (ostream*) &cout);
		        }   
		    }
		    
        } else {    
		    cerr << "Computing model on " << corpusfile << endl;
		    UnindexedPatternModel model = UnindexedPatternModel(corpusfile, MAXLENGTH, MINTOKENS, DOSKIPGRAMS, MINSKIPTOKENS ,DOINITIALONLYSKIP,DOFINALONLYSKIP);
		        
		    cerr << "Saving " << outputprefix << ".unindexedpatternmodel.colibri"  << endl;
		    const string outputfile = outputprefix + ".unindexedpatternmodel.colibri";
		    model.save(outputfile);            
		    
		    
		    if (!classfile.empty()) {
		        cerr << "Loading class decoder " << classfile << endl;
		        ClassDecoder classdecoder = ClassDecoder(classfile);
		        if (DOQUERIER) {
		        	cerr << "Loading class encoder " << classfile << endl;
		        	ClassEncoder classencoder = ClassEncoder(classfile);
		        	cerr << "Starting query mode:" << endl;
		        	model.querier(classencoder, classdecoder);
		        } else {
		        	cerr << "Decoding" << endl;
			        model.decode(classdecoder, (ostream*) &cout);
			    }   
		    }
		            	
        }
    } else if ( (!modelfile.empty()) && (!alignmodelfile.empty()) ) {
        if (DOINDEX) {
    	    cerr << "Loading model" << endl;
		    IndexedPatternModel model = IndexedPatternModel(modelfile, DEBUG);
		    cerr << "Loading Alignment Model" << endl;
		    AlignmentModel alignmodel = AlignmentModel(alignmodelfile);
		    unsigned int pruned = alignmodel.prunepatternmodel(model, alignthreshold);		    
            cerr << "pruned " << pruned << endl;
		    
		    if (!outputprefix.empty()) {
		        const string outputfile = outputprefix + ".indexedpatternmodel.colibri";
		        cerr << "Saving " << outputprefix << ".indexedpatternmodel.colibri"  << endl;
		        model.save(outputfile);
		    } else {
		        cerr << "Saving " << modelfile << endl;
		        model.save(modelfile);		    
		    }                 
        } else {
            cerr << "Not implemented yet for unindexed models" << endl;
            exit(2);
        }       
    } else if ( (!modelfile.empty()) && ((!classfile.empty()) || DOCOVERAGE || DOREPORT || DOHISTOGRAM  ) ) {
    	if (DOINDEX) {
    	    cerr << "Loading model" << endl;
		    IndexedPatternModel model = IndexedPatternModel(modelfile, DEBUG);
		    if (!classfile.empty()) {
		        cerr << "Loading class decoder " << classfile << endl;
		        ClassDecoder classdecoder = ClassDecoder(classfile);
		        if (DOQUERIER) {
		        	cerr << "Loading class encoder " << classfile << endl;
		        	ClassEncoder classencoder = ClassEncoder(classfile);
		        	cerr << "Starting query mode:" << endl;
		        	model.querier(classencoder, classdecoder);
		        } else {
		            if (modelfile2.empty()) {		        
    		        	cerr << "Decoding" << endl;
		        	    if (covviewfile.empty()) model.decode(classdecoder, (ostream*) &cout, OUTPUTHASH);
    			     } else {    			
    			        cerr << "Loading test model" << endl;     
    			        IndexedPatternModel testmodel = IndexedPatternModel(modelfile2, DEBUG);
    			        cerr << "Joint decoding" << endl;
    			        if (covviewfile.empty()) model.decode(testmodel, classdecoder, (ostream*) &cout, OUTPUTHASH);
    			     }		        	    
		        }
		    }
            if (DOREPORT) {
   		        model.report((ostream*) &cout);		        
	        }
	       	if (DOCOVERAGE) {
   		        model.coveragereport((ostream*) &cout);		        
	        }
	       	if (DOHISTOGRAM) {
   		        model.histogram((ostream*) &cout);		        
	        }	        

		    
		} else {
		    cerr << "Loading model" << endl;
		    UnindexedPatternModel model = UnindexedPatternModel(modelfile, DEBUG);
		    if (!classfile.empty()) {
		        cerr << "Loading class decoder " << classfile << endl;
		        ClassDecoder classdecoder = ClassDecoder(classfile);
				if (DOQUERIER) {
		        	cerr << "Loading class encoder " << classfile << endl;
		        	ClassEncoder classencoder = ClassEncoder(classfile);
		        	cerr << "Starting query mode:" << endl;
		        	model.querier(classencoder, classdecoder);
		        } else {
		            if (modelfile2.empty()) {
			            cerr << "Decoding" << endl;
    			        model.decode(classdecoder, (ostream*) &cout, OUTPUTHASH);
    			     } else {    			
    			        cerr << "Loading test model" << endl;     
    			        UnindexedPatternModel testmodel = UnindexedPatternModel(modelfile2, DEBUG);
    			        cerr << "Joint decoding" << endl;
    			        model.decode(testmodel, classdecoder, (ostream*) &cout, OUTPUTHASH);
    			     }
			    }   
		    }
            if (DOREPORT) {
   		        model.report((ostream*) &cout);		        
	        }
	       	if (DOCOVERAGE) {
   		        cerr << "ERROR: Coverage report not available for unindexed models" << endl;		        
	        }
	       	if (DOHISTOGRAM) {
   		        model.histogram((ostream*) &cout);		        
	        }	        

		}
        
    } else {
        cerr << "Nothing to do?" << endl;
        usage();
    }


}
