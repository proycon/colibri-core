#include <string>
#include <iostream>
#include "classencoder.h"
#include "getopt.h"
#include <common.h>

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
    cerr << "Syntax: colibri-modelcompare -c classfile trainmodel testmodel [testmodel2 etc..]" << endl;

    cerr << "Description: Compares the occurence of patterns in a model by computing log likelihood" << endl;
    cerr << "Important note: Test models should be full models rather than constrained models! Unless based on coverage (-C)" << endl;
    cerr << "Options:" << endl;
    cerr << "\t-l   Maximum pattern length (default unlimited)" << endl;
    cerr << "\t-m   Minimum pattern length (default 1)" << endl;
    cerr << "\t-C   Base computation on coverage rather than frequency" << endl;
    cerr << "\t-N   omit ngrams" << endl;
    cerr << "\t-S   omit skipgrams" << endl;
    cerr << "\t-F   omit flexgrams" << endl;
}

int main( int argc, char *argv[] ) {    
    string classfile = "";
    vector<string> modelfiles;
    bool coveragebased = false;
    bool dongrams = true;
    bool doskipgrams = true;
    bool doflexgrams = true;
    
    char c;    
    while ((c = getopt(argc, argv, "c:hl:m:CSF")) != -1) {
        switch (c)
        {
        case 'c':
            classfile = optarg;
            break;   
        case 'l':
            maxlength = atoi(optarg); 
            break;
        case 'm':
            minlength = atoi(optarg); 
            break;
        case 'C':
            coveragebased = true;
            break;
        case 'h':
            usage();
            exit(0);  
		default:
            cerr << "Unknown option: -" <<  optopt << endl;
            abort ();
        }
    }
    
    for (int i = optind; i < argc; i++) {
        string tmp = argv[i];
        modelfiles.push_back(tmp);
    }

    if (modelfiles.size() < 2) {
    	usage();
    	exit(2);
    }



}

