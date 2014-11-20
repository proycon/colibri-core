#include "pattern.h"
#include "patternstore.h"
#include "SpookyV2.h" //spooky hash
#include <cstring>

/*****************************
* Colibri Core
*   by Maarten van Gompel
*   Centre for Language Studies
*   Radboud University Nijmegen
*
*
*   http://proycon.github.io/colibri-core
*   
*   Licensed under GPLv3
*****************************/

using namespace std;


unsigned char mainpatternbuffer[MAINPATTERNBUFFERSIZE+1];

unsigned char * inttopatterndata(unsigned char * buffer,unsigned int cls) {
    //can be chained, encodes length byte (unlike inttobytes)
	unsigned int cls2 = cls;
	unsigned char length = 0;
	do {
		cls2 = cls2 / 256;
        if (length == 255) {
            cerr << "ERROR: inttopatterndata() LENGTH OVERFLOW" << endl;
            throw InternalError();
        }
		length++;
	} while (cls2 > 0);
	int i = 0;
    buffer[i++] = length;
    do {
    	int r = cls % 256;
    	buffer[i++] = (unsigned char) r;
    	cls = cls / 256;
    } while (cls > 0);
	return buffer + i;    
}

const PatternCategory datacategory(const unsigned char * data, int maxbytes = 0) {
    PatternCategory category = NGRAM;
    int i = 0;
    do {
        if ((maxbytes > 0) && (i >= maxbytes)) return category;
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            return category;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
        } else {
            //we have a marker
            if (c == SKIPMARKER) category = SKIPGRAM;
            if (c == FLEXMARKER) return FLEXGRAM;
            i++;
        }
    } while (1);
}

const PatternCategory Pattern::category() const {
    return datacategory(data);
}

const PatternCategory PatternPointer::category() const {
    return datacategory(data, bytes);
}


const size_t Pattern::bytesize() const {
    //return the size of the pattern (in bytes)

    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker, does not count in bytesize() !
            return i;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
        } else {
            //we have a marker
            i++;
        }
    } while (1);
}

const size_t datasize(unsigned char * data, int maxbytes = 0) {
    //return the size of the pattern (in tokens)
    int i = 0;
    int n = 0;
    do {
        if ((maxbytes > 0) && (maxbytes == i)) {
            return n;
        }
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            return n;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
            n++;
        } else if ((c == SKIPMARKER)  || (c == FLEXMARKER)) {
            //FLEXMARKER is counted as 1, the minimum fill
            i++;
            n++;
        } else {
            //we have another marker
            i++;
        }
    } while (1);
}

const size_t Pattern::n() const {
    return datasize(data);
}

const size_t PatternPointer::n() const {
    return datasize(data, bytes);
}


bool Pattern::isgap(int index) const { //is the word at this position a gap?
    //return the size of the pattern (in tokens)

    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            return false;
        } else if (c < 128) {
            //we have a size
            if (n == index) return false;
            i += c + 1;
            n++;
        } else if ((c == SKIPMARKER)  || (c == FLEXMARKER)) {
            //FLEXMARKER is counted as 1, the minimum fill
            if (n == index) return true;
            i++;
            n++;
        } else {
            //we have another marker
            i++;
        }
    } while (1);
}

Pattern Pattern::toflexgram() const { //converts a fixed skipgram into a dynamic one, ngrams just come out unchanged
    //to be computed in bytes
    int i = 0;
    int j = 0;
    int copybytes = 0;
    bool skipgap = false;
    do {
        const unsigned char c = data[i++];
        if (j >= MAINPATTERNBUFFERSIZE) {
            std::cerr << "ERROR: toflexgram(): Patternbuffer size exceeded" << std::endl;
            throw InternalError();
        }
        if (copybytes) {
            mainpatternbuffer[j++] = c;
            copybytes--;
        } else if (copybytes == 0) {
            if (c == ENDMARKER) {
                mainpatternbuffer[j++] = c;
                break;
            } else if (c < 128) {
                mainpatternbuffer[j++] = c;
                copybytes = c;
                skipgap = false;
            } else if (c == SKIPMARKER) {
                if (!skipgap) {
                    mainpatternbuffer[j++] = 129; //store a DYNAMIC GAP instead
                    skipgap = true; //skip next consecutive gap markers
                }
            } else {
                mainpatternbuffer[j++] = c;
            }
        }
    } while (1);
    
    //copy from mainpatternbuffer
    return Pattern(mainpatternbuffer,j-1);
}

const StructureType Pattern::type() const {
    //return the size of the pattern (in tokens)
    int i = 0;
    do {
        const unsigned char c = data[i];
        if ((c == ENDMARKER) || (c < 128) || (c == SKIPMARKER) || (c == FLEXMARKER)) {
            return STRUCT_PATTERN;
        } else if (c == SENTENCEMARKER) {
            return STRUCT_SENTENCE;
        } else if (c == PARAGRAPHMARKER) {
            return STRUCT_PARAGRAPH;
        } else if (c == BEGINDIVMARKER) {
            return STRUCT_DIV;
        } else if (c == TEXTMARKER) {
            return STRUCT_TEXT;
        } else if (c == HEADERMARKER) {
            return STRUCT_HEADER;
        } else {
            //we have a different marker
            i++;
        }
    } while (1);
}



const size_t Pattern::hash(bool stripmarkers) const {
    bool clean = true;

    int s = 0;
    do {
        const unsigned char c = data[s];
        if (c == ENDMARKER) {
            //end marker
            s++;
            break;
        } else if (c < 128) {
            //we have a size
            s += c + 1;
        } else {
            //we have a marker
            s++;
            clean = false;
        }
    } while (1);

    if ((clean) || (!stripmarkers)) {
        //clean, no markers (we don't want include markers except final \0
        //marker in our hash)
        if (sizeof(size_t) == 8) {
            return SpookyHash::Hash64((const void*) data , s);
        } else if (sizeof(size_t) == 4) {
            return SpookyHash::Hash32((const void*) data , s);
        }
    } else {
        //strip all markers (except final \0 marker) for computation of hash
        unsigned char * buffer = (unsigned char *) malloc(s);
        int buffersize = 0;
        int i = 0;
        do {
            const unsigned char c = data[i];
            if (c == ENDMARKER) {
                //end marker
                i++;
                buffer[buffersize++] = 0;
                break;
            } else if (c < 128) {
                //copy token
                for (int j = i; j < i+c+1; j++) {
                    buffer[buffersize++] = data[i];
                }
            } else {
                //we have a marker
                i++;
                clean = false;
            }
        } while (1);

        size_t h;
        if (sizeof(size_t) == 8) {
            h = SpookyHash::Hash64((const void*) buffer , buffersize);
        } else if (sizeof(size_t) == 4) {
            h = SpookyHash::Hash32((const void*) buffer , buffersize);
        }
        free(buffer);
        return h;
    }

    
}



void Pattern::write(ostream * out) const {
    const int s = bytesize();
    if (s > 0) {
        out->write( (char*) data , (int) s + 1); //+1 to include the \0 marker
    }
}

std::string datatostring(unsigned char * data, const ClassDecoder& classdecoder, int maxbytes = 0) {
    std::string result = ""; 
    int i = 0;
    int gapsize = 0;
    do {
        if ((maxbytes > 0) && (i == maxbytes)) {
            if (gapsize > 0) {
                if (!result.empty()) result += " ";
                result += std::string("{*") + to_string(gapsize) + std::string("*}");                
                gapsize = 0;
            }
            return result;
        }
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            if (gapsize > 0) {
                if (!result.empty()) result += " ";
                result += std::string("{*") + to_string(gapsize) + std::string("*}");                
                gapsize = 0;
            }
            return result;
        } else if (c < 128) {
            //we have a size
            if (gapsize > 0) {
                if (!result.empty()) result += " ";
                result += std::string("{*") + to_string(gapsize) + std::string("*}");                
                gapsize = 0;
            }
            unsigned int cls =  bytestoint(data + i +1, c);
            //cerr << (int) c << ":" << (int) *(data+i + 1) << ":" << (int) data[i+1] << ":" << cls << endl;
            i += c + 1;
            if (!result.empty()) result += " ";
            if (classdecoder.hasclass(cls)) {
                result += classdecoder[cls];
            } else {
                result += "{UNKNOWN}";
            }
        } else if (c == SKIPMARKER) {
            gapsize++;
            i++;
        } else if (c == FLEXMARKER) {
            if (result.empty()) {
                result += "{*}";
            } else {
                result += " {*}";
            }
            i++;
        } else {
            //we have another marker
            i++;
        }
    } while (1);
    return result;  
}


std::string Pattern::tostring(const ClassDecoder& classdecoder) const {
    return datatostring(data, classdecoder);
}


std::string PatternPointer::tostring(const ClassDecoder& classdecoder) const {
    return datatostring(data, classdecoder, bytes);
}

bool dataout(unsigned char * data, int maxbytes = 0) {
    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            cerr << endl;
            return true;
        } else if (c < 128) {
            //we have a size
            cerr << bytestoint(data + i + 1, c) << " ";
            i += c + 1;
        } else if (c == SKIPMARKER) {
            //FLEXMARKER is counted as 1, the minimum fill
            cerr << "SKIPMARKER" << " ";
            i++;
        } else if (c == FLEXMARKER) {
            //FLEXMARKER is counted as 1, the minimum fill
            cerr << "FLEXMARKER" << " ";
            i++;
        } else {
            //we have another marker
            i++;
        }
    } while (1);
    return false;
}

bool Pattern::out() const { 
    return dataout(data);
}

bool PatternPointer::out() const { 
    return dataout(data, bytes);
}

const bool Pattern::unknown() const {
    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            return false;
        } else if (c < 128) {
            //we have a size
            if ((c == 1) && (data[i+1] == 2)) return true; //TODO, unknownclass is hardcoded to 2 here!
            i += c + 1;
        } else {
            //we have another marker
            i++;
        }
    } while (1);
    return false;
}

vector<int> Pattern::tovector() const { 
    vector<int> v;
    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            cerr << endl;
            return v;
        } else if (c < 128) {
            //we have a size
            v.push_back(bytestoint(data + i + 1, c));
            i += c + 1;
        } else if ((c == SKIPMARKER) || (c == FLEXMARKER)) {
            //FLEXMARKER is counted as 1, the minimum fill
            v.push_back(c);
            i++;
        } else {
            //we have another marker
            v.push_back(c);
            i++;
        }
    } while (1);
    return v;
}

void readanddiscardpattern(std::istream * in) {
    unsigned char c;
    do {
        in->read( (char* ) &c, sizeof(char));
        if (c == ENDMARKER) {
            return;
        } else if (c < 128) {
            //we have a size
            const size_t pos = in->tellg();
            in->seekg(pos + c);
        } else {
            //we have a marker
        }
    } while (1);
}



Pattern::Pattern(std::istream * in, bool ignoreeol, bool debug) {
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
            if (c == ENDMARKER) {
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

    //allocate buffer
    if (c == ENDMARKER) {
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
    if ((beginposcheck != beginpos) && (beginposcheck >= 18446744073709551000)) {
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
            if (c == ENDMARKER) {
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

    if (c != ENDMARKER) { //add endmarker
        data[i++] = ENDMARKER;
    }

    if (debug) std::cerr << "DEBUG: DONE READING PATTERN" << std::endl;

    //if this is the end of file, we want the eof bit set already, so we try to
    //read one more byte (and wind back if succesful):
    if (in->good()) {
        if (debug) std::cerr << "DEBUG: (TESTING EOF)" << std::endl;
        in->read( (char* ) &c, sizeof(char));
        if (in->good()) in->unget();
    }
    
}


Pattern::Pattern(std::istream * in, unsigned char * buffer, int maxbuffersize, bool ignoreeol, bool debug) {
    //read pattern using a buffer
    int i = 0;
    int readingdata = 0;
    unsigned char c = 0;
    do {
        if (in->good()) {
            in->read( (char* ) &c, sizeof(char));
            if (debug) std::cerr << "DEBUG read=" << (int) c << endl;
        } else {
            std::cerr << "ERROR: Invalid pattern data, unexpected end of file i=" << i << std::endl;
            throw InternalError();
        }
        if (i >= maxbuffersize) {
            std::cerr << "ERROR: Pattern read would exceed supplied buffer size (" << maxbuffersize << ")! Aborting prior to segmentation fault..." << std::endl;
            throw InternalError();
        }
        buffer[i++] = c;
        if (readingdata) {
            readingdata--;
        } else {
            if (c == ENDMARKER) {
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
    } while (1);

    if (c != ENDMARKER) { //add endmarker
        buffer[i++] = ENDMARKER;
    }


    if (debug) std::cerr << "DEBUG: Copying from buffer" << std::endl;

    data  = new unsigned char[i];
    for (int j = 0; j < i; j++) {
        data[j] = buffer[j];
    }
    
    if (debug) std::cerr << "DEBUG: DONE READING PATTERN" << std::endl;
}

Pattern::Pattern(const unsigned char * dataref, const int _size) {
    data = new unsigned char[_size+1];
    for (int i = 0; i < _size; i++) {
        data[i] = dataref[i];
    }
    data[_size] = ENDMARKER;
}


void Pattern::set(const unsigned char * dataref, const int _size) {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
    data = new unsigned char[_size+1];
    for (int i = 0; i < _size; i++) {
        data[i] = dataref[i];
    }
    data[_size] = ENDMARKER;
}


Pattern::Pattern(const Pattern& ref, int begin, int length) { //slice constructor
    //to be computed in bytes
    int begin_b = 0;
    int length_b = 0;

    int i = 0;
    int n = 0;
    do {
        const unsigned char c = ref.data[i];
        
        if ((n - begin == length) || (c == ENDMARKER)) {
            length_b = i - begin_b;
            break;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
            n++;
            if (n == begin) begin_b = i;
        } else if ((c == SKIPMARKER) || (c == FLEXMARKER)) {
            i++;
            n++;
            if (n == begin) begin_b = i;
        } else {
            //we have another marker
            i++;
        }
    } while (1);

    const unsigned char _size = length_b + 1;
    data = new unsigned char[_size];
    int j = 0;
    for (int i = begin_b; i < begin_b + length_b; i++) {
        data[j++] = ref.data[i];
    }
    data[j++] = ENDMARKER;
}


PatternPointer::PatternPointer(const Pattern& ref, int begin, int length) { //slice constructor
    //to be computed in bytes
    int begin_b = 0;
    int length_b = 0;

    int i = 0;
    int n = 0;
    do {
        const unsigned char c = ref.data[i];
        
        if ((n - begin == length) || (c == ENDMARKER)) {
            length_b = i - begin_b;
            break;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
            n++;
            if (n == begin) begin_b = i;
        } else if ((c == SKIPMARKER) || (c == FLEXMARKER)) {
            i++;
            n++;
            if (n == begin) begin_b = i;
        } else {
            //we have another marker
            i++;
        }
    } while (1);

    data = ref.data + begin_b;
    if (length_b > 255) {
        std::cerr << "ERROR: Pattern too long for pattern pointer" << std::endl;
        throw InternalError();
    }
    bytes = length_b;
    
}



PatternPointer::PatternPointer(const PatternPointer& ref, int begin, int length) { //slice constructor
    //to be computed in bytes
    int begin_b = 0;
    int length_b = 0;

    int i = 0;
    int n = 0;
    do {
        if (i >= ref.bytes) {
            length_b = i - begin_b;
            break;
        }
        const unsigned char c = ref.data[i];
        
        if ((n - begin == length) || (c == ENDMARKER)) {
            length_b = i - begin_b;
            break;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
            n++;
            if (n == begin) begin_b = i;
        } else if ((c == SKIPMARKER) || (c == FLEXMARKER)) {
            i++;
            n++;
            if (n == begin) begin_b = i;
        } else {
            //we have another marker
            i++;
        }
    } while (1);

    data = ref.data + begin_b;
    bytes = length_b;
}

Pattern::Pattern(const Pattern& ref) { //copy constructor
    const int s = ref.bytesize();
    data = new unsigned char[s + 1];
    for (int i = 0; i < s; i++) {
        data[i] = ref.data[i];
    }
    data[s] = ENDMARKER;
}

Pattern::Pattern(const PatternPointer& ref) { //constructor from patternpointer
    if (ref.bytesize() > 255) {
        std::cerr << "ERROR: Pattern too long for pattern pointer" << std::endl;
        throw InternalError();
    }
    data = new unsigned char[ref.bytesize() + 1];
    for (unsigned int i = 0; i < ref.bytesize(); i++) {
        data[i] = ref.data[i];
    }
    data[ref.bytesize()] = ENDMARKER;
}

Pattern::~Pattern() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
}


bool Pattern::operator==(const Pattern &other) const {
        const int s = bytesize();
        if (bytesize() == other.bytesize()) {
            for (int i = 0; i < s+1; i++) { //+1 for endmarker
                if (data[i] != other.data[i]) return false;
            }
            return true;
        } else {
            return false;
        }        
}

bool Pattern::operator!=(const Pattern &other) const {
    return !(*this == other);
}

bool Pattern::operator<(const Pattern & other) const {
    const int s = bytesize();
    const int s2 = other.bytesize();
    for (int i = 0; (i <= s && i <= s2); i++) {
        if (data[i] < other.data[i]) {
            return true;
        } else if (data[i] > other.data[i]) {
            return false;
        }
    }
    return (s < s2);
}

bool Pattern::operator>(const Pattern & other) const {
    return (other < *this);
}

void Pattern::operator =(const Pattern & other) { 
    //delete old data
    if (data != NULL) {
        delete[] data;
        data = NULL;
    } else if (data == other.data) {
        //nothing to do
        return;
    }
    
    //set new data
    const int s = other.bytesize();        
    data = new unsigned char[s+1];   
    for (int i = 0; i < s; i++) {
        data[i] = other.data[i];
    }  
    data[s] = ENDMARKER;
}

Pattern Pattern::operator +(const Pattern & other) const {
    const int s = bytesize();
    const int s2 = other.bytesize();
    unsigned char buffer[s+s2]; 
    for (int i = 0; i < s; i++) {
        buffer[i] = data[i];
    }
    for (int i = 0; i < s2; i++) {
        buffer[s+i] = other.data[i];
    }
    return Pattern(buffer, s+s2);
}


int Pattern::find(const Pattern & pattern) const { //returns the index, -1 if not fount 
    const int s = bytesize();
    const int s2 = pattern.bytesize();
    if (s2 > s) return -1;
    
    for (int i = 0; i < s; i++) {
        if (data[i] == pattern.data[0]) {
            bool match = true;
            for (int j = 0; j < s2; j++) {
                if (data[i+j] != pattern.data[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return i;
        }
    }
    return -1;
}

bool Pattern::contains(const Pattern & pattern) const {
    return (find(pattern) != -1);
}

int Pattern::ngrams(vector<Pattern> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    int found = 0;
    for (int i = 0; i < (_n - n) + 1; i++) {
        container.push_back( Pattern(*this,i,n));
        found++;
    }
    return found;
}   

int Pattern::ngrams(vector<PatternPointer> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    int found = 0;
    for (int i = 0; i < (_n - n) + 1; i++) {
        container.push_back(  PatternPointer(*this,i,n));
        found++;
    }
    return found;
}   

int PatternPointer::ngrams(vector<PatternPointer> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    int found = 0;
    for (int i = 0; i < (_n - n) + 1; i++) {
        container.push_back(  PatternPointer(*this,i,n));
        found++;
    }
    return found;
}   

int Pattern::ngrams(vector<pair<Pattern,int>> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    
    int found = 0;
    for (int i = 0; i < (_n - n)+1; i++) {
        container.push_back( pair<Pattern,int>(Pattern(*this,i,n),i) );
        found++;
    }
    return found;
}   

int Pattern::ngrams(vector<pair<PatternPointer,int>> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    
    int found = 0;
    for (int i = 0; i < (_n - n)+1; i++) {
        container.push_back( pair<PatternPointer,int>(PatternPointer(*this,i,n),i) );
        found++;
    }
    return found;
}   


int PatternPointer::ngrams(vector<pair<PatternPointer,int>> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;
    
    int found = 0;
    for (int i = 0; i < (_n - n)+1; i++) {
        container.push_back( pair<PatternPointer,int>(PatternPointer(*this,i,n),i) );
        found++;
    }
    return found;
}   

int Pattern::subngrams(vector<Pattern> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int Pattern::subngrams(vector<PatternPointer> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int PatternPointer::subngrams(vector<PatternPointer> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int Pattern::subngrams(vector<pair<Pattern,int>> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int Pattern::subngrams(vector<pair<PatternPointer,int>> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int PatternPointer::subngrams(vector<pair<PatternPointer,int>> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n) maxn = _n;
    if (minn > _n) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int Pattern::parts(vector<pair<int,int>> & container) const {
    //to be computed in bytes
    int partbegin = 0;
    int partlength = 0;

    int found = 0;
    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        
        if (c == ENDMARKER) {
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(pair<int,int>(partbegin,partlength));
                found++;
            }
            break;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
            n++;
        } else if ((c == SKIPMARKER) || (c == FLEXMARKER)) {        
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(pair<int,int>(partbegin,partlength));
                found++;
            }
            i++;
            n++; 
            partbegin = n; //for next part
        } else {
            //we have another marker
            i++;
        }
    } while (1);
    return found;
}

int Pattern::parts(vector<Pattern> & container) const {
    vector<pair<int,int>> partoffsets; 
    int found = parts(partoffsets);

    for (vector<pair<int,int>>::iterator iter = partoffsets.begin(); iter != partoffsets.end(); iter++) {
        const int begin = iter->first;
        const int length = iter->second;
        Pattern pattern = Pattern(*this,begin, length);
        container.push_back( pattern );
    }
    return found;
}

const unsigned int Pattern::skipcount() const {
    int count = 0;
    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            return count;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
        } else if (((c == FLEXMARKER) || (c == SKIPMARKER)) && ((i > 0) || ((data[i-1] != FLEXMARKER) && (data[i-1] != SKIPMARKER))))   {
            //we have a marker
            count++;
            i++;
        }
    } while (1); 
}

int Pattern::gaps(vector<pair<int,int> > & container) const {
    vector<pair<int,int> > partscontainer;
    parts(partscontainer);

    const int _n = n();
    const int bs = bytesize();
    

    //compute inverse:
    int begin = 0;
    for (vector<pair<int,int>>::iterator iter = partscontainer.begin(); iter != partscontainer.end(); iter++) {
        if (iter->first > begin) {
            container.push_back(pair<int,int>(begin,iter->first - begin));
        }
        begin = iter->first + iter->second;
    }
    if (begin != _n) {
        container.push_back(pair<int,int>(begin,_n - begin));
    }
    
    int endskip = 0;
    for (int i = bs; i > 0; i--) {
        if ((data[i] == SKIPMARKER) || (data[i] == FLEXMARKER)) {
            endskip++;
        } else {
            break;
        }
    } 

    if (endskip) container.push_back(pair<int,int>(_n - endskip,endskip));
    return container.size();
}

Pattern Pattern::extractskipcontent(Pattern & instance) const {
    if (this->category() == FLEXGRAM) {
        cerr << "Extractskipcontent not supported on Pattern with dynamic gaps!" << endl;
        throw InternalError();
    }
    if (this->category() == NGRAM) {
        cerr << "Extractskipcontent not supported on Pattern without gaps!" << endl;
        throw InternalError();
    }
    if (instance.n() != n()) {
        cerr << "WARNING: Extractskipcontent(): instance.n() != skipgram.n(), " << (int) instance.n() << " != " << (int) n() << endl;
        cerr << "INSTANCE: " << instance.out() << endl;
        cerr << "SKIPGRAM: " << out() << endl;
        throw InternalError();
    }
    std::vector<std::pair<int,int> > gapcontainer;
    gaps(gapcontainer);
    
    unsigned char a = 128;
    const Pattern skip = Pattern(&a,1);

    std::vector<std::pair<int,int> >::iterator iter = gapcontainer.begin();
    Pattern subngram = Pattern(instance,iter->first, iter->second);
    Pattern pattern = subngram;
    int cursor = iter->first + iter->second;
    iter++;
    while (iter != gapcontainer.end()) {  
        if (cursor > 0) {
            const int skipsize = iter->first - cursor;
            for (int i = 0; i < skipsize; i++) {
                pattern = pattern + skip;
            }
        }    
        subngram = Pattern(instance,iter->first, iter->second);
        pattern = pattern + subngram;
        cursor = iter->first + iter->second;
        iter++;
    }
    return pattern;
}

bool Pattern::instanceof(const Pattern & skipgram) const { 
    //Is this an instantiation of the skipgram?
    //Instantiation is not necessarily full, aka: A ? B C is also an instantiation
    //of A ? ? C
    if (this->category() == FLEXGRAM) return false;
    if (skipgram.category() == NGRAM) return (*this) == skipgram;

    if (skipgram.category() == FLEXGRAM) {
        //DYNAMIC SKIPGRAM
        //TODO: NOT IMPLEMENTED YET!!
       return false;
    } else {
        //FIXED SKIPGRAM
        const unsigned int _n = n();
        if (skipgram.n() != _n) return false;

        for (unsigned int i = 0; i < _n; i++) {
            const Pattern token1 = Pattern(skipgram, i, 1);
            const Pattern token2 = Pattern(*this, i, 1);
            if ((token1 != token2) && (token1.category() != SKIPGRAM)) return false;
        }
        return true;
    }

}


Pattern Pattern::replace(int begin, int length, const Pattern & replacement) const {
    const int _n = n();
    if (begin + length > _n) {
        cerr << "ERROR: Replacing slice " << begin << "," << length << " in a pattern of length " << _n << "! Out of bounds!" << endl;
        throw InternalError();
    }

    if (begin > 0) {
        Pattern p = Pattern(*this,0,begin) + replacement;
        if (begin+length != _n) {
            return p + Pattern(*this,begin+length,_n - (begin+length));
        } else {
            return p;
        }
    } else {
        Pattern p = replacement;
        if (begin+length != _n) {
            return p + Pattern(*this,begin+length,_n - (begin+length));
        } else {
            return p;
        }
    }
}


Pattern Pattern::addskip(std::pair<int,int> gap) const {
    //Returns a pattern with the specified span replaced by a fixed skip
    const unsigned int _n = n();
    Pattern pattern = *this;
    const Pattern replacement = Pattern(gap.second);
    pattern = pattern.replace(gap.first, gap.second, replacement);
    if (pattern.n() != _n) {
        std::cerr << "ERROR: addskip(): Pattern length changed from " << _n << " to " << pattern.n() << " after substituting slice (" << gap.first << "," <<gap.second << ")" << std::endl;
        throw InternalError();
    }
    return pattern;
}

Pattern Pattern::addskips(std::vector<std::pair<int,int> > & gaps) const {
    //Returns a pattern with the specified spans replaced by fixed skips
    const unsigned int _n = n();
    Pattern pattern = *this;
    for (vector<pair<int,int>>::iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        const Pattern replacement = Pattern(iter->second);
        pattern = pattern.replace(iter->first, iter->second, replacement);
        if (pattern.n() != _n) {
            std::cerr << "ERROR: addskip(): Pattern length changed from " << _n << " to " << pattern.n() << " after substituting slice (" << iter->first << "," <<iter->second << ")" << std::endl;
            throw InternalError();
        }
    }
    return pattern;
}

Pattern Pattern::addflexgaps(std::vector<std::pair<int,int> > & gaps) const {
    //Returns a pattern with the specified spans replaced by fixed skips
    Pattern pattern = *this;
    for (vector<pair<int,int>>::iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        pattern = pattern.replace(iter->first, iter->second, FLEXPATTERN);
    }
    return pattern;
}


Pattern Pattern::reverse() const {
    const unsigned char _size = bytesize() + 1;
    unsigned char * newdata = new unsigned char[_size];

    //set endmarker
    newdata[_size - 1] = ENDMARKER;

    //we fill the newdata from right to left
    unsigned char cursor = _size - 1;

    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            break;
        } else if (c < 128) {
            //we have a size
            cursor = cursor - c - 1; //move newdata cursor to the left (place of insertion)
            strncpy((char*) newdata + cursor, (char*) data + i, c + 1);
            i += c + 1;
        } else {
            //we have another marker
            newdata[--cursor] = c;
            i++;
        }
    } while (1);

    Pattern rev = Pattern(newdata, _size);
    delete[] newdata;
    return rev;
}


IndexedCorpus::IndexedCorpus(std::istream *in, bool debug){
    this->load(in, debug);
}

IndexedCorpus::IndexedCorpus(std::string filename, bool debug){
    this->load(filename, debug);
}


void IndexedCorpus::load(std::istream *in, bool debug) {
    int sentence = 0;
    while (in->good()) {
        sentence++;
        Pattern line = Pattern(in);
        int linesize = line.size();
        for (int i = 0; i < linesize; i++) {
            const Pattern unigram = line[i];
            const IndexReference ref = IndexReference(sentence,i);
            data.push_back(IndexPattern(ref,unigram));
        }
    }
    if (debug) cerr << "Loaded " << sentence << " sentences" << endl;
    data.shrink_to_fit();
}


void IndexedCorpus::load(std::string filename, bool debug) {
    std::ifstream * in = new std::ifstream(filename.c_str());
    if (!in->good()) {
        std::cerr << "ERROR: Unable to load file " << filename << std::endl;
        throw InternalError();
    }
    this->load( (std::istream *) in, debug);
    in->close();
    delete in;
}

Pattern IndexedCorpus::getpattern(const IndexReference & begin, int length) const {
    //warning: will segfault if mainpatternbuffer overflows!!
    //length in tokens
    //
    //std::cerr << "getting pattern " << begin.sentence << ":" << begin.token << " length " << length << std::endl;
    const_iterator iter = this->find(begin);
    unsigned char * buffer = mainpatternbuffer;
    int i = 0;
    while (i < length) {
        if ((iter == this->end()) || (iter->ref != begin + i)) {
            //std::cerr << "ERROR: Specified index " << (begin + i).tostring() << " (pivot " << begin.tostring() << ", offset " << i << ") does not exist"<< std::endl;
            throw KeyError();
        }
        buffer = inttopatterndata(buffer, iter->cls);
        i++;
        iter++;
    }
    if (buffer == mainpatternbuffer) {
        //std::cerr << "ERROR: Specified index " << begin.tostring() << " does not exist"<< std::endl;
        throw KeyError();
    }
    int buffersize = buffer - mainpatternbuffer; //pointer arithmetic
    return Pattern(mainpatternbuffer, buffersize);
}

std::vector<IndexReference> IndexedCorpus::findpattern(const Pattern & pattern, int maxmatches) {
    //far more inefficient than a pattrn model obviously
    std::vector<IndexReference> result;
    const int _n = pattern.size();
    if (_n == 0) return result;

    IndexReference ref;
    int i = 0;
    bool moved = false;
    Pattern matchunigram = pattern[i];
    for (iterator iter = this->begin(); iter != this->end(); iter++) {
        Pattern unigram = iter->pattern(); 
        iter->pattern().out();
        if (matchunigram == unigram) {
            if (i ==0) ref = iter->ref;
            i++;
            if (i == _n) {
                result.push_back(ref);
                if ((maxmatches != 0) && (result.size() == (unsigned int) maxmatches)) break;
            }
            matchunigram = pattern[i];
            moved = true;
        } else {
            i = 0;
            if (moved) matchunigram = pattern[i];
            moved = false;
        }
    }
    return result;
}

int IndexedCorpus::sentencelength(int sentence) const {
    IndexReference ref = IndexReference(sentence, 0);
    int length = 0;
    for (const_iterator iter = this->find(ref); iter != this->end(); iter++) {
        if (iter->ref.sentence != (unsigned int) sentence) return length;
        length++;
    }
    return length;
}

unsigned int IndexedCorpus::sentences() const {
    unsigned int max = 0;
    for (const_iterator iter = this->begin(); iter != this->end(); iter++) {
        if (iter->ref.sentence > max) max = iter->ref.sentence;
    }
    return max;
}

Pattern IndexedCorpus::getsentence(int sentence) const { 
    return getpattern(IndexReference(sentence,0), sentencelength(sentence));
}


Pattern patternfromfile(const std::string & filename) {//helper function to read pattern from file, mostly for Cython
    std::ifstream * in = new std::ifstream(filename.c_str());
    if (!in->good()) {
        std::cerr << "ERROR: Unable to load file " << filename << std::endl;
        throw InternalError();
    }
    Pattern p = Pattern( (std::istream *) in, true);
    in->close();
    delete in;
    return p;
}
