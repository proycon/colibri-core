#include "classencoder.h"
#include <fstream>
#include <cmath>
#include <iostream>
#include <map>
#include <unordered_map>
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


unsigned char * inttobytes(unsigned int cls, int & length) {	
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
    unknownclass = 2;
    bosclass = 3;
    eosclass = 4;
    highestclass = 5; //5 and lower are reserved

    this->minlength = minlength;
    this->maxlength = maxlength;
}

ClassEncoder::ClassEncoder(const string & filename,const unsigned int minlength, unsigned const int maxlength) {
       load(filename,minlength,maxlength);
}

void ClassEncoder::load(const string & filename,const unsigned int minlength, unsigned const int maxlength) {
       unknownclass = 2;
       highestclass = 0; 
       bosclass = 3;
       eosclass = 4;
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
                  if (cls == 2) {
                    unknownclass = 0;
                  }
                  if (cls > (unsigned int) highestclass) highestclass = cls;
                  //cerr << "CLASS=" << cls << " WORD=" << word << endl;
              }
              
          }
        }        
        IN.close();  
        
        if (unknownclass == 0) {
            highestclass++;
            unknownclass = highestclass;
            classes["{UNKNOWN}"] = unknownclass;
        } else {        
            classes["{UNKNOWN}"] = unknownclass;
            classes["{BEGIN}"] = bosclass;
            classes["{END}"] = eosclass;
        }
}


void ClassEncoder::processcorpus(const string & filename, unordered_map<string,int> & freqlist) {
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

void ClassEncoder::processcorpus(istream * IN , unordered_map<string,int> & freqlist) {
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
                    if ((minlength > 0) || (maxlength > 0))  {
                        const unsigned int l = (unsigned int) utf8_strlen(word);
                        if (((minlength == 0) || (l >= minlength)) && ((maxlength == 0) || (l <= maxlength))) {
                            freqlist[word]++;
                        }
                    } else {
                        freqlist[word]++;
                    }
              	  }
              	  begin = i+ 1; 
              }
              
          }
        }        
}

#ifdef WITHFOLIA
void ClassEncoder::processfoliacorpus(const string & filename, unordered_map<string,int> & freqlist) {
    folia::Document doc;
    doc.readFromFile(filename);
    
    vector<folia::Word*> words = doc.words();
    for (vector<folia::Word*>::iterator iterw = words.begin(); iterw != words.end(); iterw++) {
        folia::Word * word = *iterw;
        const string wordtext = word->str();
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
#endif

void ClassEncoder::buildclasses(const unordered_map<string,int> & freqlist) {

        //sort by occurrence count  using intermediate representation
        multimap<const int, const string> revfreqlist;
        for (unordered_map<string,int>::const_iterator iter = freqlist.begin(); iter != freqlist.end(); iter++) {
        	revfreqlist.insert( pair<const int,const string>(-1 * iter->second, iter->first) );
        } 
        
        int cls = highestclass;
        for (multimap<const int,const string>::iterator iter = revfreqlist.begin(); iter != revfreqlist.end(); iter++) {
            if (!classes.count(iter->second)) { //check if it doesn't already exist, in case we are expanding on existing classes 
        	    cls++;
        	    classes[iter->second] = cls;
            }
        }
        highestclass = cls;
}

void ClassEncoder::build(const string & filename) {
	    unordered_map<string,int> freqlist;
	    if (filename.rfind(".xml") != string::npos) {
            #ifdef WITHFOLIA
	        processfoliacorpus(filename, freqlist);
            #else
            cerr << "Colibri Core was not compiled with FoLiA support!" << endl;
            exit(2);
            #endif
	    } else {
	        processcorpus(filename, freqlist); //also handles bz2
	    }
        buildclasses(freqlist);
}


void ClassEncoder::build(vector<string> & files, bool quiet) {
	    unordered_map<string,int> freqlist;
	    	    
	    for (vector<string>::iterator iter = files.begin(); iter != files.end(); iter++) {
	        const string filename = *iter;
	        if (!quiet) cerr << "Processing " << filename << endl;
	        if (filename.rfind(".xml") != string::npos) {
                #ifdef WITHFOLIA
                processfoliacorpus(filename, freqlist);
                #else
                cerr << "Colibri Core was not compiled with FoLiA support!" << endl;
                exit(2);
                #endif
	        } else {
	            processcorpus(filename, freqlist); //also handles bz2
	        }
	        
	    } 	    	    
        buildclasses(freqlist);
}

void ClassEncoder::save(const string & filename) {
	ofstream OUT;
	OUT.open( filename.c_str() );
	for (std::unordered_map<std::string,unsigned int>::iterator iter = classes.begin(); iter != classes.end(); iter++) {
	    if (iter->second != unknownclass) OUT << iter->second << '\t' << iter->first << endl;
	}
	OUT.close();
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
                if (word == "{*}") {
                    //variable length skip
                    outputcursor++;
                    continue;
                } else if (word == "{?}") {
                    //single tokenskip
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
          	  	int length = 0;
  	        	const unsigned char * byterep = inttobytes(cls, length);
  	        	if (length == 0) {
  	        		cerr << "INTERNAL ERROR: Error whilst encoding '" << word << "' (class " << cls << "), length==0, not possible!" << endl;
  	        		exit(13);
  	        	} else if (length > 128) {
  	        		cerr << "INTERNAL ERROR: Error whilst encoding '" << word << "' (class " << cls << "), length exceeds 128, not possible!" << endl;
  	        		exit(13);
                }
                outputcursor += length + 1;
  	        	delete [] byterep;
          	  }			 
          }
      }
      return outputcursor; 

}

int ClassEncoder::encodestring(const string & line, unsigned char * outputbuffer, bool allowunknown, bool autoaddunknown) {
	  int outputcursor = 0;
      int begin = 0;      
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
                if (word == "{*}") {
                    //variable length skip
                    outputbuffer[outputcursor++] = 129; //DYNAMICGAP MARKER 
                    continue;
                } else if (word == "{?}") {
                    //single tokenskip
                    outputbuffer[outputcursor++] = 128; //FIXEDGAP MARKER
                    continue;
                } else if ((word.substr(0,2) == "{*")  && (word.substr(word.size() - 2,2) == "*}")) {
                    const int skipcount = atoi(word.substr(2,word.size() - 4).c_str()); 
                    for (int j = 0; j < skipcount; j++) {
                        outputbuffer[outputcursor++] = 128; //FIXEDGAP MARKER                     
                    }                
                    continue;
                } else if (classes.count(word) == 0) {
                    if (autoaddunknown) {
                        cls = ++highestclass;
                        classes[word] = cls;  
                        added[cls] = word;
          	    	} else if (!allowunknown) {	
                        //cerr << "ERROR: Unknown word '" << word << "', does not occur in model. You may want to pass either option -U or option -e to colibri-classencode to deal with unknown words." << endl;
  	        			return 0;         
	  	        	} else {
	  	        		//cerr << "WARNING: Unknown word '" << word << "', does not occur in model. Replacing with placeholder" << endl;
	  	        		cls = unknownclass;	
	  	        	}    	
          	    } else {
          	  		cls = classes[word];
          	  	}
          	  	int length = 0;
  	        	const unsigned char * byterep = inttobytes(cls, length);
  	        	if (length == 0) {
  	        		cerr << "INTERNAL ERROR: Error whilst encoding '" << word << "' (class " << cls << "), length==0, not possible!" << endl;
  	        		exit(13);
  	        	} else if (length > 128) {
  	        		cerr << "INTERNAL ERROR: Error whilst encoding '" << word << "' (class " << cls << "), length exceeds 128, not possible!" << endl;
  	        		exit(13);
                }
                outputbuffer[outputcursor++] = length;
  	        	for (int j = 0; j < length; j++) {
  	        		outputbuffer[outputcursor++] = byterep[j];
  	        	}  	        	
  	        	delete [] byterep;
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


void ClassEncoder::add(std::string s, unsigned int cls) {
    classes[s] = cls;
    if (cls > highestclass) highestclass = cls;
}

void ClassEncoder::encodefile(const std::string & inputfilename, const std::string & outputfilename, bool allowunknown, bool autoaddunknown, bool append, bool quiet) {
	    
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
	            OUT.write(&one, sizeof(char)); //newline          
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
            encodefile((istream*) decompressor, (ostream*) &OUT, allowunknown, autoaddunknown, quiet);
            delete decompressor;
        } else {
            IN.open(inputfilename.c_str());
            if (!IN) {
                cerr << "No such file: " << inputfilename << endl;
                exit(2);
            }
            encodefile((istream*) &IN, (ostream*) &OUT, allowunknown, autoaddunknown, quiet);
        }
	    IN.close();
	    OUT.close();
	}
}

void ClassEncoder::encodefile(istream * IN, ostream * OUT, bool allowunknown, bool autoaddunknown, bool quiet) {
    const char zero = 0;
    size_t outputbuffersize = 65536;
    unsigned char * outputbuffer = new unsigned char[outputbuffersize];
    unsigned int outputsize;
    unsigned int linenum = 0;
    while (IN->good()) {	
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
        outputsize = encodestring(line, outputbuffer, allowunknown, autoaddunknown);
        OUT->write((const char *) outputbuffer, outputsize);                        
        OUT->write(&zero, sizeof(char)); //newline          
    }
    if (!quiet) cerr << "Encoded " << linenum << " lines" << endl;
    delete[] outputbuffer;
}

