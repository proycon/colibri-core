#include "classencoder.h"
#include <fstream>
#include <cmath>
#include <iostream>
#include <map>
#include <unordered_map>
#include <climits>
#include "bz2stream.h"

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

unsigned int inttobytes(unsigned char * buffer, unsigned int cls) {
	unsigned int cls2 = cls;
	unsigned int length = 0;
	do {
		cls2 = cls2 / 128;
		length++;
	} while (cls2 > 0);
    if (buffer != NULL) {
        unsigned int i = 0;
        do {
            unsigned int r = cls % 128;
            if (i != length - 1) {
                buffer[i++] = (unsigned char) r | 128; //high
            } else {
                buffer[i++] = (unsigned char) r; //low
            }
            cls = cls / 128;
        } while (cls > 0);
    }
	return length;
}

unsigned char * inttobytes_v1(unsigned int cls, int & length) {
	//compute length of byte array
	unsigned int cls2 = cls;
	length = 0;
	do {
		cls2 = cls2 / 256;
		length++;
	} while (cls2 > 0);
	unsigned char * byterep = new unsigned char[length];
	int i = 0;
    do {
    	int r = cls % 256;
    	byterep[i++] = (unsigned char) r;
    	cls = cls / 256;
    } while (cls > 0);
	return byterep;
}

//from http://www.zedwood.com/article/cpp-utf8-strlen-function
int utf8_strlen(const string& str)
{
    int c,i,ix,q;
    for (q=0, i=0, ix=str.length(); i < ix; i++, q++)
    {
        c = (unsigned char) str[i];
        if      (c>=0   && c<=127) i+=0;
        else if ((c & 0xE0) == 0xC0) i+=1;
        else if ((c & 0xF0) == 0xE0) i+=2;
        else if ((c & 0xF8) == 0xF0) i+=3;
        //else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return 0;//invalid utf8
    }
    return q;
}


ClassEncoder::ClassEncoder(const unsigned int minlength, unsigned const int maxlength) {
    highestclass = 5; //5 and lower are reserved

    this->minlength = minlength;
    this->maxlength = maxlength;
}

ClassEncoder::ClassEncoder(const string & filename,const unsigned int minlength, unsigned const int maxlength) {
       load(filename,minlength,maxlength);
}

void ClassEncoder::load(const string & filename,const unsigned int minlength, unsigned const int maxlength) {
       highestclass = 0;
       this->minlength = minlength;
       this->maxlength = maxlength;

	   ifstream IN;
	   IN.open( filename.c_str() );
       if (!(IN)) {
           cerr << "ERROR: File does not exist: " << filename << endl;
           exit(3);
       }
        while (IN.good()) {
          string line;
          getline(IN, line);
          const int s = line.size();
          for (int i = 0; i < s; i++) {
              if (line[i] == '\t') {
                  const string cls_s = string(line.begin(), line.begin() + i);
                  unsigned int cls = (unsigned int) atoi(cls_s.c_str());
                  const string word = string(line.begin() + i + 1, line.end());
                  if ((minlength > 0) || (maxlength > 0))  {
                    const unsigned int l = (unsigned int) utf8_strlen(word);
                    if (((minlength > 0) && (l < minlength)) || ((maxlength > 0) && (l > maxlength))) continue;
                  }
                  classes[word] = cls;
                  if (cls > (unsigned int) highestclass) highestclass = cls;
                  //cerr << "CLASS=" << cls << " WORD=" << word << endl;
				  break;
              }

          }
        }
        IN.close();

        classes["{?}"] = unknownclass;
        classes["{*}"] = skipclass;
        classes["{**}"] = flexclass;
        classes["{|}"] = boundaryclass;
}


void ClassEncoder::processcorpus(const string & filename, unordered_map<string,unsigned int> & freqlist, unordered_set<string> * ) {
	   //compute frequency list of all words
       ifstream IN;
	   if (filename.rfind(".bz2") != string::npos) {
            IN.open( filename.c_str(), ios::in | ios::binary );
            if (!(IN)) {
                cerr << "ERROR: File does not exist: " << filename << endl;
                exit(3);
            }
            bz2istream * decompressor = new bz2istream(IN.rdbuf());
            processcorpus((istream*) decompressor,freqlist);
       } else {
            IN.open( filename.c_str() );
            if (!(IN)) {
                cerr << "ERROR: File does not exist: " << filename << endl;
                exit(3);
            }
            processcorpus(&IN, freqlist);
       }
       IN.close();
}

void ClassEncoder::processcorpus(istream * IN , unordered_map<string,unsigned int> & freqlist, unordered_set<string> * vocab) {
        while (IN->good()) {
          string line;
          getline(*IN, line);
          int begin = 0;
          const int s = line.size();
          for (int i = 0; i < s; i++) {
              if ((line[i] == ' ') || (i == s - 1)) {
              	  int offset = 0;
              	  if (i == s - 1) offset = 1;
              	  string word = string(line.begin() + begin, line.begin() + i + offset);
              	  if ((word.length() > 0) && (word != "\r") && (word != "\t") && (word != " ")) {
              	    word = trim(word, " \t\n\r"); //trim whitespace, control characters
                    if ((vocab == NULL) || ((vocab != NULL) && (vocab->find(word) != vocab->end()) ) ) {
                        if ((minlength > 0) || (maxlength > 0))  {
                            const unsigned int l = (unsigned int) utf8_strlen(word);
                            if (((minlength == 0) || (l >= minlength)) && ((maxlength == 0) || (l <= maxlength))) {
                                freqlist[word]++;
                            }
                        } else {
                            freqlist[word]++;
                        }
                    }
              	  }
              	  begin = i+ 1;
              }

          }
        }
}

#ifdef WITHFOLIA
void ClassEncoder::processfoliacorpus(const string & filename, unordered_map<string,unsigned int> & freqlist, unordered_set<string> * vocab) {
    folia::Document doc;
    doc.readFromFile(filename);

    vector<folia::Word*> words = doc.words();
    for (vector<folia::Word*>::iterator iterw = words.begin(); iterw != words.end(); iterw++) {
        folia::Word * word = *iterw;
        const string wordtext = word->str();
        if ((vocab == NULL) || ((vocab != NULL) && (vocab->find(word) != vocab->end()) ) ) {
            if ((minlength > 0) || (maxlength > 0))  {
                const int l = utf8_strlen(wordtext);
                if (((minlength == 0) || (l >= minlength)) && ((maxlength == 0) || (l <= maxlength))) {
                    freqlist[wordtext]++;
                }
            } else {
                freqlist[wordtext]++;
            }
        }
    }

}
#endif

void ClassEncoder::buildclasses(const unordered_map<string,unsigned int> & freqlist, unsigned int threshold) {

        //sort by occurrence count  using intermediate representation
        multimap<const unsigned int, const string> revfreqlist;
        for (unordered_map<string,unsigned int>::const_iterator iter = freqlist.begin(); iter != freqlist.end(); iter++) {
            if (iter->second >= threshold) revfreqlist.insert( pair<const unsigned int,const string>(-1 * iter->second, iter->first) );
        }

        int cls = highestclass;
        for (multimap<const unsigned int,const string>::const_iterator iter = revfreqlist.begin(); iter != revfreqlist.end(); iter++) {
            if (!classes.count(iter->second)) { //check if it doesn't already exist, in case we are expanding on existing classes
        	    cls++;
        	    classes[iter->second] = cls;
            }
        }
        highestclass = cls;
}

void ClassEncoder::build(const string & filename, unsigned int threshold, const string vocabfile) {
	    unordered_map<string,unsigned int> freqlist;
        unordered_set<string> vocab;
        if (!vocabfile.empty()) loadvocab(vocabfile, vocab);
	    if (filename.rfind(".xml") != string::npos) {
            #ifdef WITHFOLIA
	        processfoliacorpus(filename, freqlist, &vocab);
            #else
            cerr << "Colibri Core was not compiled with FoLiA support!" << endl;
            exit(2);
            #endif
	    } else {
	        processcorpus(filename, freqlist, &vocab); //also handles bz2
	    }
        buildclasses(freqlist, threshold);
}


void ClassEncoder::build(vector<string> & files, bool quiet, unsigned int threshold, const string vocabfile) {
	    unordered_map<string,unsigned int> freqlist;
        unordered_set<string> vocab;
        if (!vocabfile.empty()) loadvocab(vocabfile, vocab);
	    for (vector<string>::iterator iter = files.begin(); iter != files.end(); iter++) {
	        const string filename = *iter;
	        if (!quiet) cerr << "Processing " << filename << endl;
	        if (filename.rfind(".xml") != string::npos) {
                #ifdef WITHFOLIA
                processfoliacorpus(filename, freqlist, &vocab);
                #else
                cerr << "Colibri Core was not compiled with FoLiA support!" << endl;
                exit(2);
                #endif
	        } else {
	            processcorpus(filename, freqlist, &vocab); //also handles bz2
	        }

	    }
        buildclasses(freqlist, threshold);
}

void ClassEncoder::save(const string & filename) {
	ofstream OUT;
	OUT.open( filename.c_str() );
	for (std::unordered_map<std::string,unsigned int>::iterator iter = classes.begin(); iter != classes.end(); iter++) {
	    if (iter->second != unknownclass) OUT << iter->second << '\t' << iter->first << endl;
	}
	OUT.close();
}

void ClassEncoder::loadvocab(const string & filename, unordered_set<string> & vocab) {
    cerr << "Loading vocabulary file" << endl;
    ifstream IN(filename);
    while (IN.good()) {
        string line;
        getline(IN, line);
        line = trim(line, " \t\n\r"); //trim whitespace, control characters
        const int s = line.size();
		if (s > 0) {
            vocab.insert(line);
		}
    }
}

void ClassEncoder::buildclasses_freqlist(const string & filename, unsigned int threshold) {
	unordered_map<string,unsigned int> freqlist;
    ifstream IN(filename);
    while (IN.good()) {
        string line;
        getline(IN, line);
        line = trim(line, " \t\n\r"); //trim whitespace, control characters
        const int s = line.size();
		if (s > 0) {
            for (int i = 0; i < s; i++) {
                if (line[i] == '\t') {
                    const string word = string(line.begin(), line.begin() + i);
                    const string freq_s = string(line.begin() + i + 1, line.end());
                    unsigned int freq = (unsigned int) atoi(freq_s.c_str());
                    freqlist[word] = freq;
                    break;
                }
            }
		}
    }
    buildclasses(freqlist, threshold);
}

vector<unsigned int> ClassEncoder::encodeseq(const vector<string> & seq) {
    vector<unsigned int> result;
    const int l = seq.size();
    for (int i = 0; i < l; i++)
        result.push_back( classes[seq[i]] );
    return result;
}

int ClassEncoder::outputlength(const string & line) {
	  int outputcursor = 0;
      int begin = 0;
      int tmphighestclass = highestclass;
      unsigned int classlength;
      const int l = line.length();
      for (int i = 0; i < l; i++) {
      	  if ((line[i] == ' ') || (i == l - 1)) {
          	  string word;
          	  if (line[i] == ' ') {
          	  	word  = string(line.begin() + begin, line.begin() + i);
          	  } else {
			   	word  = string(line.begin() + begin, line.begin() + i + 1);
          	  }
              word = trim(word, " \t\n\r\b"); //trim whitespace, control characters
          	  begin = i+1;
          	  if ((word.length() > 0) && (word != "\r") && (word != "\t") && (word != " ")) {
          	    unsigned int cls;
                if ((word == "{*}") || (word == "{**}")) {
                    //length skip
                    outputcursor++;
                    continue;
                } else if (word == "{?}") {
                    //unknown word
                    outputcursor++;
                    continue;
                } else if ((word.substr(0,2) == "{*")  && (word.substr(word.size() - 2,2) == "*}")) {
                    const int skipcount = atoi(word.substr(2,word.size() - 4).c_str());
                    for (int j = 0; j < skipcount; j++) {
                        outputcursor++;
                    }
                    continue;
                } else if (classes.count(word) == 0) {
                    cls = ++tmphighestclass; //as if autoaddunknown
          	    } else {
          	  		cls = classes[word];
          	  	}
          	  	classlength = inttobytes(NULL, cls);
                outputcursor += classlength;
          	  }
          }
      }
      return outputcursor;

}

int ClassEncoder::encodestring(const string & line, unsigned char * outputbuffer, bool allowunknown, bool autoaddunknown, unsigned int * nroftokens) {
	  int outputcursor = 0;
      int begin = 0;
      const int l = line.length();
      unsigned int classlength;
      for (int i = 0; i < l; i++) {
      	  if ((line[i] == ' ') || (i == l - 1)) {
          	  string word;
          	  if (line[i] == ' ') {
          	  	word  = string(line.begin() + begin, line.begin() + i);
          	  } else {
			   	word  = string(line.begin() + begin, line.begin() + i + 1);
          	  }
              word = trim(word, " \t\n\r\b"); //trim whitespace, control characters
          	  begin = i+1;
          	  if ((word.length() > 0) && (word != "\r") && (word != "\t") && (word != " ")) {
          	    unsigned int cls;
                if (word == "{*}") {
                    //fixed length skip
                    outputbuffer[outputcursor++] = skipclass;
                    if (nroftokens != NULL) (*nroftokens)++;
                    continue;
                } else if (word == "{**}") {
                    //variable length skip
                    outputbuffer[outputcursor++] = flexclass;
                    if (nroftokens != NULL) (*nroftokens)++;
                    continue;
                } else if (word == "{?}") {
                    //unknown word
                    outputbuffer[outputcursor++] = unknownclass;
                    if (nroftokens != NULL) (*nroftokens)++;
                    continue;
                } else if ((word.substr(0,2) == "{*")  && (word.substr(word.size() - 2,2) == "*}")) {
                    const int skipcount = atoi(word.substr(2,word.size() - 4).c_str());
                    for (int j = 0; j < skipcount; j++) {
                        outputbuffer[outputcursor++] = skipclass; //FIXEDGAP MARKER
                        if (nroftokens != NULL) (*nroftokens)++;
                    }
                    continue;
                } else if (classes.find(word) == classes.end()) {
                    if (autoaddunknown) {
                        cls = ++highestclass;
                        classes[word] = cls;
                        added[cls] = word;
          	    	} else if (!allowunknown) {
                        //cerr << "ERROR: Unknown word '" << word << "', does not occur in model. You may want to pass either option -U or option -e to colibri-classencode to deal with unknown words." << endl;
                        throw UnknownTokenError();
	  	        	} else {
	  	        		//cerr << "WARNING: Unknown word '" << word << "', does not occur in model. Replacing with placeholder" << endl;
	  	        		cls = unknownclass;
	  	        	}
          	    } else {
          	  		cls = classes[word];
          	  	}
  	        	classlength = inttobytes(outputbuffer + outputcursor, cls);
                outputcursor += classlength;
                if (nroftokens != NULL) (*nroftokens)++;
          	  }
          }
      }
      return outputcursor;
}


const int buildbuffersize = 65536;
unsigned char buildbuffer[buildbuffersize];
Pattern ClassEncoder::buildpattern(const std::string & patternstring, bool allowunknown,  bool autoaddunknown) { //not thread-safe
	int buffersize = encodestring(patternstring, buildbuffer, allowunknown, autoaddunknown);
    if (buffersize > buildbuffersize) {
        cerr << "INTERNAL ERROR: Exceeded buildpattern buffer size" << endl;
        exit(2);
    }
    Pattern pattern = Pattern(buildbuffer,buffersize);
	return pattern;
}



Pattern ClassEncoder::buildpattern_safe(const std::string & patternstring, bool allowunknown,  bool autoaddunknown) { //thread-safe
    unsigned char buffer[buildbuffersize];
	int buffersize = encodestring(patternstring, buffer, allowunknown, autoaddunknown);
    if (buffersize > buildbuffersize) {
        cerr << "INTERNAL ERROR: Exceeded buildpattern buffer size" << endl;
        exit(2);
    }
    Pattern pattern = Pattern(buffer,buffersize);
	return pattern;
}


void ClassEncoder::add(const std::string & s, const unsigned int cls) {
    classes[s] = cls;
    if (cls > highestclass) highestclass = cls;
}

void ClassEncoder::encodefile(const std::string & inputfilename, const std::string & outputfilename, bool allowunknown, bool autoaddunknown, bool append, bool ignorenewlines, bool quiet) {

    if ((inputfilename.rfind(".xml") != string::npos) ||  (inputfilename.rfind(".xml.bz2") != string::npos) ||  (inputfilename.rfind(".xml.gz") != string::npos)) {
        #ifdef WITHFOLIA
        const char zero = 0;
        const char one = 1;
        //FoLiA
        folia::Document doc;
        doc.readFromFile(inputfilename);

	    ofstream OUT;
	    if (append) {
	        OUT.open(outputfilename.c_str(), ios::app | ios::binary);
	        if (OUT.tellp() > 0) {
          	    OUT.write(&zero, sizeof(char)); //write separator
	        }
	    } else {
	        OUT.open(outputfilename.c_str(), ios::out | ios::binary);
	    }
	    unsigned char outputbuffer[65536];
	    int outputsize = 0;
	    unsigned int linenum = 1;
	    vector<folia::Word*> words = doc.words();
	    const size_t wl = words.size();
        folia::FoliaElement * prevparent = NULL;
	    string line = "";
	    for (size_t i = 0; i < wl; i++) {
            folia::Word * word = words[i];
	        if ((!line.empty()) && (word->parent() != prevparent)) {
	            outputsize = encodestring(line, outputbuffer, allowunknown, autoaddunknown);
	            if (outputsize > 0) OUT.write((const char *) outputbuffer, outputsize);
          	    OUT.write(&zero, sizeof(char)); //newline
          	    linenum++;
          	    line = "";
	        }
            prevparent = word->parent();
        	if (line.empty()) {
                line += word->str();
            } else {
                line += " " + word->str();
            }
        }
        if (!line.empty()) {
            outputsize = encodestring(line, outputbuffer, allowunknown, autoaddunknown);
	        if (outputsize > 0) {
                OUT.write((const char *) outputbuffer, outputsize);
          	    OUT.write(&zero, sizeof(char)); //newline
            }
        }
	    if (!quiet) cerr << "Encoded " << linenum << " lines" << endl;
	    OUT.close();
        #else
        cerr << "Colibri Core was not compiled with FoLiA support!" << endl;
        #endif
    } else {
	    ofstream OUT;
	    ifstream IN;
	    if (append) {
	        OUT.open(outputfilename.c_str(), ios::app | ios::binary);
	    } else {
	        OUT.open(outputfilename.c_str(), ios::out | ios::binary);
	    }
        if (inputfilename.rfind(".bz2") != string::npos) {
            IN.open(inputfilename.c_str(), ios::in | ios::binary);
            if (!IN) {
                cerr << "No such file: " << inputfilename << endl;
                exit(2);
            }
            bz2istream * decompressor = new bz2istream(IN.rdbuf());
            encodefile((istream*) decompressor, (ostream*) &OUT, allowunknown, autoaddunknown, quiet, append, ignorenewlines);
            delete decompressor;
        } else {
            IN.open(inputfilename.c_str());
            if (!IN) {
                cerr << "No such file: " << inputfilename << endl;
                exit(2);
            }
            encodefile((istream*) &IN, (ostream*) &OUT, allowunknown, autoaddunknown, quiet, append, ignorenewlines);
        }
	    IN.close();
	    OUT.close();
	}
}

void ClassEncoder::encodefile(istream * IN, ostream * OUT, bool allowunknown, bool autoaddunknown, bool quiet, bool append, bool ignorenewlines) {
    if (!append) {
        const char mark = 0xa2;
        const char version = 2;
        OUT->write(&mark,1);
        OUT->write(&version,1);
    }
    const char zero = 0;
    size_t outputbuffersize = 65536;
    unsigned char * outputbuffer = new unsigned char[outputbuffersize];
    unsigned int outputsize;
    unsigned int linenum = 0;
    unsigned int totalnroftokens = 0;
    unsigned int nroftokens;
    while (IN->good()) {
        nroftokens = 0;
        string line = "";
        getline(*IN, line);
        if (!IN->good()) break;
        linenum++;
        if (line.length() > outputbuffersize) { //heuristic to check if we need to compute the length, string will be longer than encoded representation
            outputsize = outputlength(line);
            if (outputsize > outputbuffersize) {
                delete[] outputbuffer;
                outputbuffersize = outputsize+1;
                outputbuffer = new unsigned char[outputbuffersize];
            }
        }
        outputsize = encodestring(line, outputbuffer, allowunknown, autoaddunknown, &nroftokens);
        if (ignorenewlines) {
            if (totalnroftokens + nroftokens >= 65536) { //max token count (uint16)
                if (totalnroftokens == 0) {
                    cerr << "ERROR: Each input line may not contain more than 65536 tokens/words. Limit exceeded on line " << linenum << " (" << nroftokens << " tokens)" << endl;
                    throw InternalError();
                }
                OUT->write(&zero, sizeof(char)); //newline
                totalnroftokens = 0; //reset for next block
            }
            totalnroftokens += nroftokens;
        }
        OUT->write((const char *) outputbuffer, outputsize);
        if (!ignorenewlines) OUT->write(&zero, sizeof(char)); //newline
    }
    if (ignorenewlines) OUT->write(&zero, sizeof(char)); //force newline at end of file even if ignorenewlines is set
    if (!quiet) cerr << "Encoded " << linenum << " lines" << endl;
    delete[] outputbuffer;
}

unsigned char * convert_v1_v2(const unsigned char * olddata, unsigned int & newlength) {

	std::vector<unsigned int> classes;

    //get new length
	newlength = 0;
    int i = 0;
    do {
        const unsigned char c = olddata[i];
        if (c == 0) {
			classes.push_back(ClassEncoder::delimiterclass);
		    break;
        } else if (c < 128) {
            //we have a size
			unsigned int cls = bytestoint_v1(olddata + i +1, c);
			classes.push_back(cls);
            newlength += inttobytes( NULL, cls);
            i += c + 1;
        } else if (c == 128) { //SKIPMARKER (v1)
			newlength++;
			classes.push_back(ClassEncoder::skipclass);
            i++;
        } else if (c == 129) { //FLEXMARKER (v1)
			newlength++;
			classes.push_back(ClassEncoder::flexclass);
            i++;
        } else {
            //we have another marker
            i++;
        }
    } while (1);

	//allocate new data
    //cerr<<"DEBUG: Newlength=" << newlength << endl;
	unsigned char * data  = new unsigned char[newlength+1];
	unsigned char * datacursor = data;
    unsigned int classlength;
	for (std::vector<unsigned int>::iterator iter = classes.begin(); iter != classes.end(); iter++) {
		classlength = inttobytes(datacursor, *iter);
		datacursor += classlength;
	}
    return data;
}

unsigned char * convert_v1_v2(istream * in, bool ignoreeol, bool debug) {
    int readingdata = 0;
    unsigned char c = 0;

    std::streampos beginpos = 0;
    bool gotbeginpos = false;

    //stage 1 -- get length
    int length = 0;
    readingdata = 0;
    do {
        if (in->good()) {
            if (!gotbeginpos) {
                beginpos = in->tellg();
                gotbeginpos = true;
            }
            in->read( (char* ) &c, sizeof(char));
            if (debug) std::cerr << "DEBUG read1=" << (int) c << endl;
        } else {
            if (ignoreeol) {
                break;
            } else {
                std::cerr << "WARNING: Unexpected end of file (stage 1, length=" << length << "), no EOS marker found (adding and continuing)" << std::endl;
                in->clear(); //clear error bits
                break;
            }
        }
        length++;
        if (readingdata) {
            readingdata--;
        } else {
            if (c == 0) {
                if (!ignoreeol) break;
            } else if (c < 128) {
                //we have a size
                if (c == 0) {
                    std::cerr << "ERROR: Pattern length is zero according to input stream.. not possible! (stage 1)" << std::endl;
                    throw InternalError();
                } else {
                    readingdata = c;
                }
            }
        }
    } while (1);

    if (length == 0) {
        std::cerr << "ERROR: Attempting to read pattern from file, but file is empty?" << std::endl;
        throw InternalError();
    }

    unsigned char * data;
    //allocate buffer
    if (c == 0) {
        data  = new unsigned char[length];
    } else {
        data  = new unsigned char[length+1];
    }

    //stage 2 -- read buffer
    int i = 0;
    readingdata = 0;
    if (debug) std::cerr << "STARTING STAGE 2: BEGINPOS=" << beginpos << ", LENGTH=" << length << std::endl;
    if (!gotbeginpos) {
        std::cerr << "ERROR: Invalid position in input stream whilst Reading pattern" << std::endl;
        throw InternalError();
    }
    in->seekg(beginpos, ios::beg);
    std::streampos beginposcheck = in->tellg();
    if ((beginposcheck != beginpos)
	&& (beginposcheck >= numeric_limits<std::streampos>::max() )) {
      std::cerr << "ERROR: Resetting read pointer for stage 2 failed! (" << (unsigned long) beginposcheck << " != " << (unsigned long) beginpos << ")" << std::endl;
      throw InternalError();
    } else if (!in->good()) {
        std::cerr << "ERROR: After resetting readpointer for stage 2, istream is not 'good': eof=" << (int) in->eof() << ", fail=" << (int) in->fail() << ", badbit=" << (int) in->bad() << std::endl;
        throw InternalError();
    }
    while (i < length) {
        if (in->good()) {
            in->read( (char* ) &c, sizeof(char));
            if (debug) std::cerr << "DEBUG read2=" << (int) c << endl;
        } else {
            std::cerr << "ERROR: Invalid pattern data, unexpected end of file (stage 2,i=" << i << ",length=" << length << ",beginpos=" << beginpos << ",eof=" << (int) in->eof() << ",fail=" << (int) in->fail() << ",badbit=" << (int) in->bad() << ")" << std::endl;
            throw InternalError();
        }
        data[i++] = c;
        if (readingdata) {
            readingdata--;
        } else {
            if (c == 0) {
                if (!ignoreeol) break;
            } else if (c < 128) {
                //we have a size
                if (c == 0) {
                    std::cerr << "ERROR: Pattern length is zero according to input stream.. not possible! (stage 2)" << std::endl;
                    throw InternalError();
                } else {
                    readingdata = c;
                }
            }
        }
    }

    if (c != 0) { //add endmarker
        data[i++] = 0;
    }

    if (debug) std::cerr << "DEBUG: DONE READING PATTERN" << std::endl;

    //if this is the end of file, we want the eof bit set already, so we try to
    //read one more byte (and wind back if succesful):
    if (in->good()) {
        if (debug) std::cerr << "DEBUG: (TESTING EOF)" << std::endl;
        in->read( (char* ) &c, sizeof(char));
        if (in->good()) in->unget();
    }

    unsigned int newlength;
    unsigned char * newdata = convert_v1_v2(data, newlength);
    delete[] data;
    return newdata;
}
