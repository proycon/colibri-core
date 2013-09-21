#include <string>
#include <iostream>
#include "classencoder.h"
#include "getopt.h"
#include <common.h>

using namespace std;

void usage() {
    cerr << "Syntax: classencoder [ -c classmodel ] corpus [corpus2 etc..]" << endl;
    cerr << "Description: Encodes a corpus. If used with -c, encodes a corpus according to the specified pre-existing class model" << endl;
    cerr << "Options: -o    outputprefix for class file" << endl;
    cerr << "         -l    read input filenames from list-file (one filename per line)" << endl;
    cerr << "         -u    produce one unified encoded corpus (in case multiple corpora are specified)" << endl;
}

int main( int argc, char *argv[] ) {    
    string classfile = "";
    string corpusfile = "";
    string outputprefix = "";
    vector<string> corpusfiles;
    bool unified = false;
    
    ifstream listin;
    string tmpfilename;
    
    char c;    
    while ((c = getopt(argc, argv, "f:h:c:o:ul:")) != -1) {
        switch (c)
        {
        case 'f': //keep for backward compatibility
            corpusfile = optarg;
            corpusfiles.push_back(corpusfile);
            break;
        case 'c':
            classfile = optarg;
            break;   
        case 'o':
            outputprefix = optarg;
            break;
        case 'u':
            unified = true;
            break;
        case 'l':
            listin.open(optarg);
            if (listin.good()) {
                while (!listin.eof()) {
                    listin >> tmpfilename;
                    corpusfiles.push_back(tmpfilename);
                }
            } else {
                cerr << "Can't read " << optarg << endl;
                abort();
            }
            listin.close();
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
        corpusfiles.push_back(tmp);
    }
    
    if (corpusfiles.empty()) {
    	usage();
    	exit(2);
    } else {
        corpusfile = corpusfiles[0]; //only for extension determination
    }
        
    if (outputprefix.empty()) {
        outputprefix = corpusfile;
        strip_extension(outputprefix,"xml");     
        strip_extension(outputprefix,"txt");    
    }


    ClassEncoder classencoder;
    
    bool allowunknown = false;
    
    if (!classfile.empty()) {
        cerr << "Loading classes from file" << endl;
        classencoder = ClassEncoder(classfile);
        allowunknown = true;
        cerr << "Building classes from corpus (extending existing classes)" << endl;
        classencoder.build(corpusfiles);
        classencoder.save(outputprefix + ".cls");
        cerr << "Built " << outputprefix << ".cls , extending " << classfile << endl;          
    } else {
        cerr << "Building classes from corpus" << endl;
        classencoder = ClassEncoder();
        classencoder.build(corpusfiles);
        classencoder.save(outputprefix + ".cls");
        cerr << "Built " << outputprefix << ".cls" << endl;            
    }   
    
    for (int i = 0; i < corpusfiles.size(); i++) {
        string outfile = corpusfiles[i];
        if (unified) {
            outfile = outputprefix;
        } else {
            strip_extension(outfile,"txt");
            strip_extension(outfile,"xml");
        }       
        classencoder.encodefile(corpusfiles[i], outfile + ".clsenc", allowunknown, false, unified);
        cerr << "Encoded corpus " << corpusfiles[i] << " in " << outfile << ".clsenc" << endl;
    }

    
}
