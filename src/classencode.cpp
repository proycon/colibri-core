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
    cerr << "         -e    extend specified class file with unseen classes" << endl;
    cerr << "         -U    encode all unseen classes using one special unknown class" << endl;
}

int main( int argc, char *argv[] ) {    
    string classfile = "";
    string corpusfile = "";
    string outputprefix = "";
    vector<string> corpusfiles;
    bool unified = false;
    bool extend = false;
    bool allowunknown = false;
    ifstream listin;
    string tmpfilename;
    
    char c;    
    while ((c = getopt(argc, argv, "f:h:c:o:ul:eU")) != -1) {
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
        case 'e':
            extend = true;
            break;
        case 'U':
            allowunknown = true;
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
    
    
    if (!classfile.empty()) {
        cerr << "Loading classes from file" << endl;
        classencoder = ClassEncoder(classfile);
        if (extend) {
            cerr << "Building classes from corpus (extending existing classes)" << endl;
            classencoder.build(corpusfiles);
            classencoder.save(outputprefix + ".colibri.cls");
            cerr << "Built " << outputprefix << ".colibri.cls , extending " << classfile << endl;          
        }
    } else {
        cerr << "Building classes from corpus" << endl;
        classencoder = ClassEncoder();
        classencoder.build(corpusfiles);
        classencoder.save(outputprefix + ".colibri.cls");
        cerr << "Built " << outputprefix << ".colibri.cls" << endl;            
    }   
    
    int highestclass = classencoder.gethighestclass();
    for (size_t i = 0; i < corpusfiles.size(); i++) {
        string outfile = corpusfiles[i];
        if (unified) {
            outfile = outputprefix;
        } else {
            strip_extension(outfile,"txt");
            strip_extension(outfile,"xml");
        }       
        classencoder.encodefile(corpusfiles[i], outfile + ".colibri.dat", allowunknown, extend, unified);
        cerr << "Encoded corpus " << corpusfiles[i] << " in " << outfile << ".colibri.dat" << endl;
    }

    if ((classencoder.gethighestclass() > highestclass)) {
        if (extend) {
            classencoder.save(outputprefix + ".colibri.cls");
            cerr << "Built " << outputprefix << ".colibri.cls" << endl;            
        } else {
            cerr << "WARNING: classes were added but the result was ignored! Use -e!" << endl;
        }
    }
    
}
