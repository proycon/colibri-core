#include <string>
#include <iostream>
#include "classdecoder.h"
#include "getopt.h"

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
    cerr << "Syntax: colibri-classdecode -f encoded-corpus -c class-file" << endl;
    cerr << "Descriptions: Decodes an encoded corpus" << endl;
    cerr << "Options:" << endl;
    cerr << "\t-s 	start line number (default: 0)" << endl;
    cerr << "\t-e 	end line number (default: infinite)" << endl;
}

int main( int argc, char *argv[] ) {    
    string classfile = "";
    string corpusfile = "";
    unsigned int start = 0;
    unsigned int end = 0; 
     
    char c;    
    while ((c = getopt(argc, argv, "c:f:hs:e:")) != -1)
        switch (c)
        {
        case 'c':
            classfile = optarg;
            break;
        case 'f':
            corpusfile = optarg;
            break;  
        case 's':    
        	start = atoi(optarg);
        	break;
       	case 'e':    
        	end = atoi(optarg);
        	break;        	  
        case 'h':
            usage();
            exit(0);            
		default:
            cerr << "Unknown option: -" <<  optopt << endl;
            abort ();
        }
        
    if (classfile.empty() || corpusfile.empty()) {
    	usage();
    	exit(2);
    }
    
    ClassDecoder classdecoder = ClassDecoder(classfile);
    classdecoder.decodefile(corpusfile, (ostream*) &cout, start, end);   
}
