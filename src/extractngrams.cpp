#include <string>
#include <iostream>
#include "getopt.h"
#include "classdecoder.h"
#include "patternmodel.h"
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
    cerr << "Syntax: colibri-extractngrams -n [size] -c classfile datafile datafile2 ..." << endl;

    cerr << "Description: Simple tool that just extracts n-grams from corpus data (class encoded), i.e. it moves a simple sliding window over the data, and outputs n-grams in the order found, it does not generate any pattern models so is very low in memory" << endl << endl;
    cerr << "Options:" << endl;
    cerr << "\t-n int   N-gram size" << endl;
    cerr << "\t-c str   Class file for decoding" << endl;
}

int main( int argc, char *argv[] ) {
    string classfile = "";
    vector<string> datafiles;
    int n = 3;

    char c;
    while ((c = getopt(argc, argv, "c:hn:")) != -1) {
        switch (c) {
        case 'c':
            classfile = optarg;
            break;
        case 'n':
            n = atoi(optarg);
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
        datafiles.push_back(tmp);
    }

    if (classfile.empty()) {
        cerr << "ERROR: No class file specified! (-c)" << endl;
        usage();
        exit(2);
    }

    ClassDecoder classdecoder = ClassDecoder(classfile);

    std::vector<std::pair<PatternPointer,int>> ngrams;


    for (unsigned int i = 0; i < datafiles.size(); i++) {
        std::ifstream * in = new std::ifstream(datafiles[i].c_str(), std::ios::in|std::ios::binary);
        while (!in->eof()) {
            //read line
            Pattern line = Pattern(in);
            ngrams.clear();
            line.ngrams(ngrams, n);

            for (std::vector<std::pair<PatternPointer,int>>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) {
                cout << iter->first.tostring(classdecoder) << endl;
            }
        }
        delete in;
    }



}
