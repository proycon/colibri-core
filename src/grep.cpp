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
    cerr << "Syntax: colibri-grep -c classfile pattern1 [pattern2 ..etc]" << endl;

    cerr << "Description: Search for patterns in corpus data (-f) or an indexed pattern model (-i), output optionally with context. If multiple search patterns are specified, the disjunction is taken." << endl;
    cerr << "Search Options:" << endl;
    cerr << "\t-l int   Left context size" << endl;
    cerr << "\t-r int   Right context size" << endl;
    cerr << "\t-f str   corpus data to search in (colibri.dat file)
    cerr << "\t-i str   pattern model to search in (must be indexed)
}

int main( int argc, char *argv[] ) {
    string classfile = "";
    string corpusfile = "";
    string modelfile = "";
    int leftcontextsize = 0;
    int rightcontextsize = 0;
    vector<string> querystrings;
    int n = 3;

    char c;
    while ((c = getopt(argc, argv, "c:hf:i::l:r:s:")) != -1) {
        switch (c) {
        case 'c':
            classfile = optarg;
            break;
        case 'f':
            corpusfile = optarg;
            break;
        case 'i':
            modelfile = optarg;
            break;
        case 'l':
            leftcontextsize = atoi(optarg);
            break;
        case 'r':
            rightcontextsize = atoi(optarg);
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
        querystrings.push_back(tmp);
    }

    if (querystrings.empty()) {
        cerr << "No queries specifies" << endl;
        usage();
        exit(2);
    }

    if (classfile.empty()) {
        cerr << "ERROR: No class file specified! (-c)" << endl;
        usage();
        exit(2);
    }

    ClassDecoder classdecoder = ClassDecoder(classfile);
    ClassEncoder classencoder = ClassEncoder(classfile);

    vector<Pattern> queries;
    for (int i = 0; i < querystrings.size(); i++) {
        queries.push_back( classencoder.buildpattern(querystrings[i]) );
    }


    std::vector<std::pair<PatternPointer,int>> ngrams;
    bool first;


    for (int i = 0; i < datafiles.size(); i++) {
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
