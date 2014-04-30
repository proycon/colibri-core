#include "classdecoder.h"
#include <fstream>
#include <cmath>
#include <iostream>
#include <sstream>

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

unsigned int bytestoint(const unsigned char* a, const int l) {
    int result = 0;
    for (int i = 0; i < l; i++) {
        result += *(a + i) * pow(256,i);
    }
    return result;
}


ClassDecoder::ClassDecoder() {
       unknownclass = 2;
       highestclass = 0;
       bosclass = 3;
       eosclass = 4;
       classes[unknownclass] = "{UNKNOWN}";
       classes[bosclass] = "{BEGIN}";
       classes[eosclass] = "{END}";
}

ClassDecoder::ClassDecoder(const string & filename) {
       load(filename);
}


void ClassDecoder::load(const string & filename) {
       unknownclass = 2;
       highestclass = 0;
       bosclass = 3;
       eosclass = 4;

       classes[unknownclass] = "{UNKNOWN}";
       classes[bosclass] = "{BEGIN}";
       classes[eosclass] = "{END}";

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
    unsigned char buffer[1024]; //bit large, only for one token
    ifstream *IN = new ifstream(filename.c_str()); //, ios::in | ios::binary);
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
                int cls = bytestoint(buffer, (int) c);
                if (!first) *out << " ";
                *out << classes[cls];
                first = false;
            }
        } else if (c == 128) {
            if (!first) *out << " ";
            *out << "{?}";
            first = false;
        } else if (c == 129) {
            if (!first) *out << " ";
            *out << "{*}";
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

const int countwords(const unsigned char* data, const int l) {
	if (l == 0) return 0;	
    int words = 1; 
    bool empty = true;
    for (int i = 0; i < l; i++) {
        if ( (data[i] == 0) && (i > 0) && (i < l - 1) ) words++;
        if (data[i] != 0) empty = false;
    }
    if (empty) {
     	return 0;
    } else {
     	return words;
    }
}

void ClassDecoder::add(unsigned int cls, std::string s) {
    classes[cls] = s;
    if (cls > highestclass) highestclass = cls;
}


pair<int,int> getwords(const unsigned char* data, const int datasize, const int n, const int begin) {
    //cerr << "IN DATASIZE=" << datasize << " N=" << n << " BEGIN=" << begin << endl;
    int words = 0; 
    int beginsize = 0;
    for (int i = 0; i < datasize; i++) {
        if (data[i] == 0) {
            words++;
            if (words == begin) {
                beginsize = i+1;
                //cerr << "BEGINSIZE: " << beginsize << endl;
            }
            //cerr << words << " vs " << beginsize+n << endl;
            if (words == begin+n) {                
                //cerr << "BEGIN: "<<  beginsize << endl;
                //cerr << "LENGTH: "<<  i - beginsize << endl;
                return pair<int,int>(beginsize,i - beginsize );
            }
        } 
    }
    words++;
    if (words == begin+n) {
        //cerr << "BEGIN: "<<  beginsize << endl;
        //cerr << "LENGTH: "<<  datasize - beginsize << endl;
        return pair<int,int>(beginsize, datasize - beginsize);
    } else {
        //cerr << "WARNING: getwords not found" << endl;
        //cerr << "DEBUG WORDS=" << words << endl;;
        return pair<int,int>(0,0); //fragment too small
    }
}

void ClassDecoder::prune(unsigned int threshold) {
    for (unsigned int i = threshold; i <= highestclass; i++) {
        classes.erase(i);
    } 
    highestclass = threshold - 1;
}
