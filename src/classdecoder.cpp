#include "classdecoder.h"
#include <fstream>
#include <cmath>
#include <iostream>
#include <sstream>
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

unsigned int bytestoint(const unsigned char* a, unsigned int * length) {
    unsigned int result = 0;
    unsigned char b;
    unsigned int i = 0;
    if (length != NULL) *length = 0;
    do {
        b = *(a+i);
        if (b >> 7) {
            //high
            result += (b ^ 128)* pow(128,i);
        } else {
            result += b * pow(128,i);
            if (length != NULL) *length = i+1;
            break;
        }
        i++;
    } while (true);
    /*if ((length != NULL) && ((*length == 0)))  {
        cerr << "bytestoint: No class found, length == 0" << endl;
        throw InternalError();
    }*/
    return result;
}

unsigned int bytestoint(istream* IN, const unsigned char version) {
    if (version == 1) {
        unsigned char length;
        IN->read((char*) &length,sizeof(unsigned char));
        unsigned char * buffer = new unsigned char[length];;
        IN->read((char*) buffer, length);
        unsigned int result = bytestoint_v1(buffer, (int) length);
        delete[] buffer;
        return result;
    } else {
        unsigned int result = 0;
        unsigned char b;
        unsigned int i = 0;
        do {
            IN->read((char*) &b,sizeof(unsigned char));
            if (!IN->good()) break;
            if (b >> 7) {
                //high
                result += (b ^ 128)* pow(128,i);
            } else {
                result += b * pow(128,i);
                break;
            }
            i++;
        } while (IN->good());
        return result;
    }
}

unsigned int bytestoint_v1(const unsigned char* a, const int l) {
    int result = 0;
    for (int i = 0; i < l; i++) {
        result += *(a + i) * pow(256,i);
    }
    return result;
}


ClassDecoder::ClassDecoder() {
       highestclass = 0;
       classes[unknownclass] = "{?}";
       classes[skipclass] = "{*}";
       classes[flexclass] = "{**}";
       classes[boundaryclass] = "{|}";
}

ClassDecoder::ClassDecoder(const string & filename) {
       load(filename);
}


void ClassDecoder::load(const string & filename) {
       highestclass = 0;

       classes[unknownclass] = "{?}";
       classes[skipclass] = "{*}";
       classes[flexclass] = "{**}";
       classes[boundaryclass] = "{|}";

       ifstream *IN =  new ifstream( filename.c_str() );
       if (!(*IN)) {
           cerr << "File does not exist: " << filename << endl;
           exit(3);
       }
        while (IN->good()) {
          string line;
          getline(*IN, line);
          for (unsigned int i = 0; i < line.size(); i++) {
              if (line[i] == '\t') {
                  const string cls_s = string(line.begin(), line.begin() + i);
                  unsigned int cls = (unsigned int) atoi(cls_s.c_str());
                  const string word = string(line.begin() + i + 1, line.end());
                  classes[cls] = word;
                  if (cls > highestclass) highestclass = cls;
                  //cerr << "CLASS=" << cls << " WORD=" << word << endl;
                  break;
              }
          }
        }
        IN->close();
        delete IN;
}


vector<string> ClassDecoder::decodeseq(const vector<int> & seq) {
    vector<string> result;
    const int l = seq.size();
    for (int i = 0; i < l; i++)
        result.push_back( classes[seq[i]] );
    return result;
}
/*
string decodestring(const unsigned char * data, unsigned char datasize) {
	string output = "";
    unsigned char buffer[10];
    bool eol = true;
    int n = 0;
	for (int i = 0; i < datasize; i++) {
		unsigned char c = data[i];
		buffer[n] = c;
        if (c == 0) {
            //cout << "N: " << n << endl;
            const unsigned int cls = bytestoint(buffer, n);
            if (cls == 1) {
            	output += "\n";
            	eol = true;
                linenumber++;
            } else if (classes.count(cls)) {
                //cout << cls << ' ';
                if (!eol) output +=  " ";
                output += classes[cls];
                eol = false;
             } else if (cls != 0) {
             	cerr <<  "ERROR: Unknown class, unable to resolve: " << cls << endl;
				exit(5);
            }
            n = 0;
        } else {
            n++;
65500bri.cls}
	}
} */

void ClassDecoder::decodefile(const string & filename,  std::ostream* out , unsigned int start, unsigned int end, bool quiet) {
    ifstream *IN = new ifstream(filename.c_str()); //, ios::in | ios::binary);
    unsigned char version = getdataversion(IN);
    if (version == 1) {
        decodefile_v1(IN,out,start,end,quiet);
        return;
    }
    unsigned int linenumber = 1;
    unsigned int cls;
    bool first = true;
    while (IN->good()) {
        cls = bytestoint(IN,version);
        if (!IN->good()) break;
        if (cls == delimiterclass) { //endmarker
            if (((start == 0) && (end == 0)) || ((linenumber >= start) || (linenumber <= end))) {
                *out << endl;
            }
            first = true;
            linenumber++;
        } else if (((start == 0) && (end == 0)) || ((linenumber >= start) || (linenumber <= end))) {
            if (!first) *out << " ";
            *out << classes[cls];
            first = false;
        }
    }
    IN->close();
    linenumber--;
    if (!quiet) cerr << "Processed " << linenumber  << " lines" << endl;
}

void ClassDecoder::decodefile_v1(ifstream *IN,  std::ostream* out , unsigned int start, unsigned int end, bool quiet) {
    unsigned char buffer[1024]; //bit large, only for one token
    unsigned int linenumber = 1;
    bool first = true;
    unsigned char c;
    while (IN->good()) {
        IN->read( (char* ) &c, sizeof(char));
        if (!IN->good()) break;
        if (c == 0) { //endmarker
            if (((start == 0) && (end == 0)) || ((linenumber >= start) || (linenumber <= end))) {
                *out << endl;
            }
            linenumber++;
            first = true;
        } else if (c < 128) {
            //we have a size, load token
            IN->read( (char*) buffer, c);
            if (((start == 0) && (end == 0)) || ((linenumber >= start) || (linenumber <= end))) {
                int cls = bytestoint_v1(buffer, (int) c);
                if (!first) *out << " ";
                *out << classes[cls];
                first = false;
            }
        } else if (c == 128) {
            if (!first) *out << " ";
            *out << "{*}";
            first = false;
        } else if (c == 129) {
            if (!first) *out << " ";
            *out << "{**}";
            first = false;
        }
    }
    IN->close();
    linenumber--;
    if (!quiet) cerr << "Processed " << linenumber  << " lines" << endl;
}


std::string ClassDecoder::decodefiletostring(const std::string & filename,   unsigned int start, unsigned int end, bool quiet) {
    std::ostringstream ss;
    decodefile(filename, (ostream*) &ss, start, end, quiet);
    return ss.str();
}


void ClassDecoder::add(const unsigned int cls, const std::string & s) {
    classes[cls] = s;
    if (cls > highestclass) highestclass = cls;
}



void ClassDecoder::prune(unsigned int threshold) {
    for (unsigned int i = threshold; i <= highestclass; i++) {
        classes.erase(i);
    }
    highestclass = threshold - 1;
}

unsigned char getdataversion(std::istream * in) {
    unsigned char b;
    unsigned char version = 1;
    version = 0;
    if (!in->good())  {
        cerr << "ERROR: Supplied data file can not be opened. Check whether it exists and whether you have proper permissions..." << endl;
        throw InternalError();
    }
    if (in->tellg() > 0) {
        in->seekg(0);
    }
    in->read((char*) &b,sizeof(unsigned char));
    if (b == 0xa2) {
        //version 2 or higher
        in->read((char*) &version, sizeof(unsigned char));
    } else {
        //no metadata, version 1, first byte is a length marker
        version = 1;
        if (b > 5) {
            cerr << "ERROR: Supplied data file is not a valid Colibri Data file, did you pass plain-text instead perhaps?..." << endl;
            throw InternalError();
        }
        in->seekg(0); //reset
    }
    return version;
}
