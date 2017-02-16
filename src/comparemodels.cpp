#include <string>
#include <iostream>
#include "getopt.h"
#include "classdecoder.h"
#include "patternmodel.h"
#include <common.h>
#include <config.h>

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
using namespace std;

void usage() {
    cerr << "Colibri Core " << VERSION << " - Class Encoder" << endl;
    cerr << "  by Maarten van Gompel, Language Machines, Centre for Language Studies, Radboud University Nijmegen" << endl;
    cerr << "  https://proycon.github.io/colibri-core" << endl << endl;
    cerr << "Syntax: colibri-comparemodels -c classfile patternmodelfile1 patternmodelfile2 etc..." << endl;

    cerr << "Description: Compares the frequency of patterns between two or more pattern models by computing log likelihood, following the methodology of Rayson and Garside (2000), Comparing corpora using frequency profiling. In proceedings of the workshop on Comparing Corpora, held in conjunction with the 38th annual meeting of the Association for Computational Linguistics (ACL 2000). 1-8 October 2000, Hong Kong, pp. 1 - 6: http://www.comp.lancs.ac.uk/~paul/publications/rg_acl2000.pdf" << endl << endl;
    cerr << "Important notes: - All models should be full models, and best generated with the same occurrence threshold, rather than constrained train/test models!" << endl;
    cerr << "                 - Models must share the exact same class encoding to be comparable!" << endl;
    cerr << "Options:" << endl;
    cerr << "\t-l int   Maximum pattern length (default unlimited)" << endl;
    cerr << "\t-m int   Minimum pattern length (default 1)" << endl;
    //cerr << "\t-C       Base computation on coverage rather than frequency" << endl; //not implemented yet
    cerr << "\t-N       omit ngrams" << endl;
    cerr << "\t-S       omit skipgrams" << endl;
    cerr << "\t-F       omit flexgrams" << endl;
    cerr << "\t-a       Include only patterns that occur in all models" << endl;
    cerr << "\t-d       Output directly, don't build a map, don't sort the output (conserves memory)" << endl;
}

int main( int argc, char *argv[] ) {
    string classfile = "";
    vector<string> modelfiles;
    bool conjunctiononly = false;
    bool directoutput = false;
    PatternModelOptions options = PatternModelOptions();
    string inputfile;
    string outputfile;

    char c;
    while ((c = getopt(argc, argv, "c:hl:m:SFad")) != -1) {
        switch (c)
        {
        case 'c':
            classfile = optarg;
            break;
        case 'l':
            options.MAXLENGTH = atoi(optarg);
            break;
        case 'm':
            options.MINLENGTH = atoi(optarg);
            break;
        case 'N':
            options.DOREMOVENGRAMS = true;
            break;
        case 'S':
            options.DOREMOVESKIPGRAMS = true;
            break;
        case 'F':
            options.DOREMOVEFLEXGRAMS = true;
            break;
        case 'a':
            conjunctiononly = true;
            break;
        case 'd':
            directoutput = true;
            break;
        case 'h':
            usage();
            exit(0);
		default:
            cerr << "ERROR: Unknown option: -" <<  optopt << endl;
            abort ();
        }
    }

    for (int i = optind; i < argc; i++) {
        string tmp = argv[i];
        modelfiles.push_back(tmp);
    }

    if (classfile.empty()) {
        cerr << "ERROR: No class file specified! (-c)" << endl;
        usage();
        exit(2);
    }

    PatternMap<double> llmodel;

    if (!inputfile.empty()) {
        cerr << "Reading log-likelihood patternmap from " << inputfile << endl;
        llmodel.read(inputfile, options.MINTOKENS, options.MINLENGTH, options.MAXLENGTH,NULL, !options.DOREMOVENGRAMS, !options.DOREMOVESKIPGRAMS, !options.DOREMOVEFLEXGRAMS);
    } else if (modelfiles.size() < 2) {
        cerr << "ERROR: Need at least two models" << endl;
    	usage();
    	exit(2);
    }

    ClassDecoder classdecoder = ClassDecoder(classfile);

    vector<PatternModel<uint32_t>* > models; //first model is training model or background model

    for (unsigned int i = 0; i < modelfiles.size(); i++) {
        cerr << "Loading model " << modelfiles[i] << endl;
        PatternModel<uint32_t> * model = new PatternModel<uint32_t>(modelfiles[i], options);
        models.push_back(model);
    }

    cerr << "Computing log-likelihood..." << endl;

    if (directoutput) {
        comparemodels_loglikelihood(models, &llmodel, conjunctiononly, (ostream*) &cout, &classdecoder);
    } else {
        comparemodels_loglikelihood(models, &llmodel, conjunctiononly);

        cerr << "Sorting results..." << endl;
        set<pair<double,Pattern>> results;
        for (PatternMap<double>::iterator iter = llmodel.begin(); iter != llmodel.end(); iter++) {
            results.insert(pair<double,Pattern>(-1 * iter->second, iter->first));
        }

        cerr << "Output:" << endl;
        cout << "PATTERN\tLOGLIKELIHOOD";
        for (unsigned int i = 0; i < modelfiles.size(); i++) {
            cout << "\tOCC_" << i << "\tFREQ_" << i;
        }
        cout << endl;

        for (set<pair<double,Pattern>>::iterator iter = results.begin(); iter != results.end(); iter++) {
            const Pattern pattern = iter->second;
            cout << pattern.tostring(classdecoder) << "\t" << (iter->first * -1);
            for (unsigned int i = 0; i < models.size(); i++) {
                cout << "\t" << models[i]->occurrencecount(pattern) << "\t" << models[i]->frequency(pattern);
            }
            cout << endl;
        }
    }

    for (unsigned int i = 0; i < models.size(); i++) {
        delete models[i];
    }

}
