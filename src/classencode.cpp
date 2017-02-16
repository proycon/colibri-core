#include <string>
#include <iostream>
#include "classencoder.h"
#include "getopt.h"
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
    cerr << "Syntax: colibri-classencode [ -c classmodel ] corpus [corpus2 etc..]" << endl;
    cerr << "Description: Encodes a corpus. If used with -c, encodes a corpus according to the specified pre-existing class model" << endl << endl;
    cerr << "The corpus file should be in one of the following formats:" << endl;
    cerr << " - plain text, preferably tokenised (tokens space delimited)" << endl;
    cerr << "   one sentence per line, unix newlines, encoding agnostic." << endl;
    cerr << " - as above, but bzip2 compressed (bz2 extension)" << endl;
#ifdef WITHFO
    cerr << " - FoLiA XML (xml extension)" << endl << endl;
#endif
    cerr << "Options: -o    outputprefix for class file" << endl;
    cerr << "         -d    output directory, including trailing slash" << endl;
    cerr << "         -l    read input filenames from list-file (one filename per line)" << endl;
    cerr << "         -u    produce one unified encoded corpus (in case multiple corpora are specified)" << endl;
    cerr << "         -e    extend specified class file with unseen classes" << endl;
    cerr << "         -n    ignore newlines in input, treat everything as one text blob" << endl;
    cerr << "         -U    encode all unseen classes using one special unknown class" << endl;
    cerr << "         -t    word occurrence threshold (default: 1)" << endl;
    cerr << "         -F    Import frequency list (word \\t frequency, per line)" << endl;
    cerr << "         -V    Import vocabulary file (one word per line)" << endl;
}

int main( int argc, char *argv[] ) {
    string classfile = "";
    string corpusfile = "";
    string outputprefix = "";
    string outputdirectoryprefix = "";
    string freqlistfile = "";
    string vocabfile = "";
    vector<string> corpusfiles;
    bool unified = false;
    bool extend = false;
    bool allowunknown = false;
    bool ignorenewlines = false;
    int threshold = 0;
    ifstream listin;
    string tmpfilename;

    char c;
    while ((c = getopt(argc, argv, "f:hc:o:d:ul:eUt:F:V:n")) != -1) {
        switch (c) {
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
        case 'd':
        	outputdirectoryprefix = optarg;
        	break;
        case 'u':
            unified = true;
            break;
        case 'n':
            ignorenewlines = true;
            break;
        case 'e':
            extend = true;
            break;
        case 'U':
            allowunknown = true;
            break;
        case 't':
            threshold = atoi(optarg);
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
        case 'F':
            freqlistfile = optarg;
            break;
        case 'V':
            vocabfile = optarg;
            break;
        case 'h':
            usage();
            exit(0);
		default:
            cerr << "Unknown option: -" <<  optopt << endl;
			exit(2);
        }
    }

    for (int i = optind; i < argc; i++) {
        string tmp = argv[i];
        corpusfiles.push_back(tmp);
    }

    if (corpusfiles.empty() && (freqlistfile.empty())) {
    	usage();
    	exit(2);
    }

	if (!corpusfiles.empty()) {
        corpusfile = corpusfiles[0]; //only for extension determination
    }


    if (outputprefix.empty()) {
        if (corpusfile.find_last_of("/") == string::npos) {
            outputprefix = corpusfile;
        } else {
            outputprefix = corpusfile.substr(corpusfile.find_last_of("/")+1);
        }
        strip_extension(outputprefix,"bz2");
        strip_extension(outputprefix,"xml");
        strip_extension(outputprefix,"txt");
    }

	if ((!freqlistfile.empty()) && (outputprefix.empty())) {
        if (freqlistfile.find_last_of("/") == string::npos) {
            outputprefix = freqlistfile;
        } else {
            outputprefix = freqlistfile.substr(freqlistfile.find_last_of("/")+1);
        }
        strip_extension(outputprefix,"bz2");
        strip_extension(outputprefix,"xml");
        strip_extension(outputprefix,"txt");
	}

    ClassEncoder classencoder;

    string prefixedoutputprefix = outputdirectoryprefix + outputprefix;
    if (!classfile.empty()) {
        cerr << "Loading classes from file" << endl;
        classencoder = ClassEncoder(classfile);
        if (extend) {
            cerr << "Building classes from corpus (extending existing classes)" << endl;
            classencoder.build(corpusfiles, false, threshold, vocabfile);
            classencoder.save(prefixedoutputprefix + ".colibri.cls");
            cerr << "Built " << prefixedoutputprefix << ".colibri.cls , extending " << classfile << endl;
        }
    } else if (!freqlistfile.empty()) {
        cerr << "Building classes from imported frequency list or vocabulary file" << endl;
        classencoder = ClassEncoder();
        classencoder.buildclasses_freqlist(freqlistfile);
        classencoder.save(prefixedoutputprefix + ".colibri.cls");
        cerr << "Built " << prefixedoutputprefix << ".colibri.cls" << endl;
    } else {
        cerr << "Building classes from corpus" << endl;
        classencoder = ClassEncoder();
        classencoder.build(corpusfiles, false, threshold, vocabfile);
        classencoder.save(prefixedoutputprefix + ".colibri.cls");
        cerr << "Built " << prefixedoutputprefix << ".colibri.cls" << endl;
    }

    unsigned int highestclass = classencoder.gethighestclass();
    for (size_t i = 0; i < corpusfiles.size(); i++) {
        string outfile = corpusfiles[i];
        if (outfile.find_last_of("/") != string::npos) {
            outfile = outfile.substr(outfile.find_last_of("/")+1);
        }
        if (unified) outfile = outputprefix;
        strip_extension(outfile,"bz2");
        strip_extension(outfile,"txt");
        strip_extension(outfile,"xml");

        if(outputdirectoryprefix.compare("")) // output directory given
        {
            outfile = outputdirectoryprefix + "/" + outfile;
        }

        cerr << "Encoding corpus " << corpusfiles[i] << " to " << outfile << ".colibri.dat" << endl;
        classencoder.encodefile(corpusfiles[i], outfile + ".colibri.dat", allowunknown, extend, (unified && i>0), ignorenewlines);
        cerr << "...Done" << endl;
    }

    if (classencoder.gethighestclass() > highestclass) {
        if (extend) {
            classencoder.save(outputprefix + ".colibri.cls");
            cerr << "Built " << outputprefix << ".colibri.cls" << endl;
        } else {
            cerr << "WARNING: classes were added but the result was ignored! Use -e!" << endl;
        }
    }

}
