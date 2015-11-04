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

const PatternCategory datacategory(const unsigned char * data, int maxbytes = 0) {
    PatternCategory category = NGRAM;
    int i = 0;
    unsigned int length;
    do {
        if ((maxbytes > 0) && (i >= maxbytes)) return category;
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            //end marker
            return category;
        } else if (cls == ClassDecoder::skipclass) {
            return SKIPGRAM;
        } else if (cls == ClassDecoder::flexclass) {
            return FLEXGRAM;
        }
    } while (1);
}

const PatternCategory Pattern::category() const {
    return datacategory(data);
}

const PatternCategory PatternPointer::category() const {
    if (mask == 0) {
        return datacategory(data, bytes);
    } else if (mask > B32 / 2) {
        return FLEXGRAM;
    } else {
        return SKIPGRAM;
    }
}


const size_t Pattern::bytesize() const {
    //return the size of the pattern (in bytes)
    unsigned int i = 0;
    bool prevhigh = false;
    do {
        if ((!prevhigh) && (data[i] == ClassDecoder::delimiterclass)) { //end marker
            return i;
        }
        prevhigh = (data[i] >= 128);
        i++;
    } while (1);
}

const size_t datasize(unsigned char * data, int maxbytes = 0) {
    //return the size of the pattern (in tokens)
    int i = 0;
    int n = 0;
    bool prevhigh = false;
    do {
        if ((maxbytes > 0) && (i >= maxbytes)) {
            return n;
        }
        if ((!prevhigh) && (data[i] == ClassDecoder::delimiterclass)) {
            return n;
        } else if (data[i] < 128) {
            n++;
        }
        prevhigh = (data[i] >= 128);
        i++;
    } while (1);
}

const size_t Pattern::n() const {
    return datasize(data);
}

const size_t PatternPointer::n() const {
    return datasize(data, bytes);
}


bool Pattern::isgap(int index) const { //is the word at this position a gap? 
    int i = 0;
    int n = 0;
    bool prevhigh = false;
    do {
        const unsigned char c = data[i];
        if ((!prevhigh) && (c == ClassDecoder::delimiterclass)) {
            //end marker
            return false;
        } else if ((!prevhigh) && ((c == ClassDecoder::skipclass)  || (c == ClassDecoder::flexclass))) {
            //FLEXMARKER is counted as 1, the minimum fill
            if (n == index) return true;
            i++;
            n++;
            prevhigh = false;
        } else if (c < 128) {
            //we have a size
            if (n == index) return false;
            i++;
            n++;
            prevhigh = false;
        } else {
            prevhigh = true;
            i++;
        }
    } while (1);
}

bool PatternPointer::isgap(int index) const { //is the word at this position a gap? 
    if ((mask == 0) || (index > 30)) return false;
    return (mask & bitmask[index]);
}

Pattern Pattern::toflexgram() const { //converts a fixed skipgram into a dynamic one, ngrams just come out unchanged
    //to be computed in bytes
    int i = 0;
    int j = 0;
    int copybytes = 0;
    bool skipgap = false;
    bool prevhigh = false;
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
            if ((!prevhigh) && (c == ClassDecoder::delimiterclass)) {
                mainpatternbuffer[j++] = c;
                break;
            } else if ((!prevhigh) && (c == ClassDecoder::skipclass)) {
                if (!skipgap) {
                    mainpatternbuffer[j++] = ClassDecoder::flexclass; //store a DYNAMIC GAP instead
                    skipgap = true; //skip next consecutive gap markers
                }
            } else {
                mainpatternbuffer[j++] = c;
                skipgap = false;
            }
        }
        prevhigh = (c >= 128);
    } while (1);
    
    //copy from mainpatternbuffer
    return Pattern(mainpatternbuffer,j-1);
}

PatternPointer PatternPointer::toflexgram() const { //converts a fixed skipgram into a flexgram, ngrams just come out unchanged
    PatternPointer copy = *this;
    if (mask != 0) copy.mask = mask | (1<<31);
    return copy;
}

uint32_t PatternPointer::computemask() const {
    uint32_t mask = 0;
    unsigned int n = 0;
    bool isflex = false;
    for (unsigned int i = 0; (i < bytes) && (n < 31); i++) {
        if (data[i] < 128) {
            if ((i == 0) || (data[i-1] < 128)) {
                if (data[i] == ClassDecoder::flexclass){
                    isflex = true;
                    mask |= (1 << n);
                } else if ((data[i] == ClassDecoder::skipclass)) {
                    mask |= (1 << n);
                }
            }
            n++;
        }
    }
    if (isflex) mask |= (1 << 31);
    return mask;
}




const size_t Pattern::hash() const {
    return SpookyHash::Hash64((const void*) data , bytesize());
}

const size_t PatternPointer::hash() const {
    if (mask == 0) {
        return SpookyHash::Hash64((const void*) data , bytesize());
	} else if (isflexgram()) {
        //hashing skipgrams/flexgrams is a bit more expensive cause we need to process the mask before we can compute the hash:
        unsigned char datacopy[bytes+1];
		int size = flexcollapse(datacopy);
        datacopy[size] = ClassDecoder::delimiterclass;
        return SpookyHash::Hash64((const void*) datacopy , size+1);
    } else {
        //hashing skipgrams/flexgrams is a bit more expensive cause we need to process the mask before we can compute the hash:
        unsigned char datacopy[bytes+1];
        memcpy(datacopy, data, bytes);
        datacopy[bytes] = ClassDecoder::delimiterclass;
        unsigned int n = 0;
		for (unsigned int i = 0; i < bytes; i++) {
            if (datacopy[i] < 128) {
                if (isgap(n)) datacopy[i] = ClassDecoder::skipclass;
                n++;
            }
		}
        return SpookyHash::Hash64((const void*) datacopy , bytes);
    }
}


void Pattern::write(ostream * out, const unsigned char * corpusstart) const {
    //corpusstart is not used must does need to be present so we have the same signature as PatternPointer
    const int s = bytesize();
    if (s > 0) {
        out->write( (char*) data , (int) s + 1); //+1 to include the \0 marker
    }
}

void PatternPointer::write(ostream * out, const unsigned char * corpusstart) const {
    if (corpusstart != NULL) {
        const unsigned int offset = data - corpusstart;
        out->write((char*) &offset, sizeof(unsigned int));
        out->write((char*) &bytes, sizeof(uint32_t));
        out->write((char*) &mask, sizeof(uint32_t));
    } else {
        const int s = bytesize();
        if (s > 0) {
            out->write( (char*) data , (int) s + 1); //+1 to include the \0 marker
        }
    }
}

std::string datatostring(unsigned char * data, const ClassDecoder& classdecoder, int maxbytes = 0) {
    std::string result = ""; 
    int i = 0;
    unsigned int length; 
    do {
        if ((maxbytes > 0) && (i >= maxbytes)) {
            return result;
        }
        const unsigned int cls = bytestoint(data + i, &length);
        if (length == 0) {
            cerr << "ERROR: Class length==0, shouldn't happen" << endl;
            throw InternalError();
        }
        i += length;
        if ((maxbytes == 0) && (cls == ClassDecoder::delimiterclass)) {
            return result;
        } else if (classdecoder.hasclass(cls)) {
            if (!result.empty()) result += " ";
            result += classdecoder[cls];
        } else {
            if (!result.empty()) result += " ";
            result += "{?}";
        }
    } while (1);
    return result;  
}


std::string Pattern::tostring(const ClassDecoder& classdecoder) const {
    return datatostring(data, classdecoder);
}


std::string PatternPointer::tostring(const ClassDecoder& classdecoder) const {
    std::string result = ""; 
    unsigned int i = 0;
    unsigned int n =0;
    unsigned int length; 
    unsigned int cls;
    bool flex = false;
    if (mask != 0) flex = isflexgram();
    do {
        if ((bytes > 0) && (i >= bytes)) {
            return result;
        }
        cls = bytestoint(data + i, &length);
        if ((mask != 0) && (isgap(n)))  {
            if (flex) {
                cls = ClassDecoder::flexclass;
            } else {
                cls = ClassDecoder::skipclass;
            }
        }
        n++;
        if (length == 0) {
            cerr << "ERROR: Class length==0, shouldn't happen" << endl;
            throw InternalError();
        }
        i += length;
        if ((bytes == 0) && (cls == ClassDecoder::delimiterclass)) {
            return result;
        } else if (classdecoder.hasclass(cls)) {
            if (!result.empty()) result += " ";
            result += classdecoder[cls];
        } else {
            if (!result.empty()) result += " ";
            result += "{?}";
        }
    } while (1);
    return result;  
}

bool dataout(unsigned char * data, int maxbytes = 0) {
    int i = 0;
    unsigned int length;
    do {
        if ((maxbytes > 0) && (i >= maxbytes)) {
            return true;
        }
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            cerr << endl;
            return true;
        } else {
            cerr << cls << " ";
        }
    } while (1);
    return false;
}

bool Pattern::out() const { 
    return dataout(data);
}

bool PatternPointer::out() const { 
    return dataout(data, bytesize());
}

const bool Pattern::unknown() const {
    int i = 0;
    unsigned int length;
    do {
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            //end marker
            return false;
        } else if (cls == ClassDecoder::unknownclass) {
            return true;
        }
    } while (1);
}

vector<unsigned int> Pattern::tovector() const { 
    vector<unsigned int> v;
    int i = 0;
    unsigned int length;
    do {
        const unsigned int cls = bytestoint(data + i, &length);
        i += length;
        if (cls == ClassDecoder::delimiterclass) {
            return v;
        } else {
            v.push_back(cls);
        }
    } while (1);
}

void readanddiscardpattern(std::istream * in, bool pointerformat) {
    unsigned char c;
    if (pointerformat) {
        in->read( (char*) &c, sizeof(unsigned int)); 
        in->read( (char*) &c, sizeof(uint32_t));
        in->read( (char*) &c, sizeof(uint32_t));
    } else {
        bool prevhigh = false;
        do {
            in->read( (char*) &c, sizeof(char));
            if ((!prevhigh) && (c == ClassDecoder::delimiterclass)) {
                return;
            }
            prevhigh = (c >= 128);
        } while (1);
    }
}

void readanddiscardpattern_v1(std::istream * in) {
    unsigned char c;
    do {
        in->read( (char*) &c, sizeof(char));
        if (c == 0) {
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



Pattern::Pattern(std::istream * in, bool ignoreeol, const unsigned char version, const unsigned char * corpusstart, bool debug) {
    if (version == 2) {
        //stage 1 -- get length
        unsigned char c = 0;

        std::streampos beginpos = 0;
        bool gotbeginpos = false;
        bool prevhigh = false;

        //stage 1 -- get length
        int length = 0;
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
            if ((!prevhigh) && (c == ClassDecoder::delimiterclass)) {
                if (!ignoreeol) break;
            }
            prevhigh = (c >= 128);
        } while (1);

        if (length == 0) {
            std::cerr << "ERROR: Attempting to read pattern from file, but file is empty?" << std::endl;
            throw InternalError();
        }

        //allocate buffer
        if (c == 0) {
            data  = new unsigned char[length];
        } else {
            data  = new unsigned char[length+1];
        }

        //stage 2 -- read buffer
        int i = 0;
        prevhigh = false;
        if (debug) std::cerr << "STARTING STAGE 2: BEGINPOS=" << beginpos << ", LENGTH=" << length << std::endl;
        if (!gotbeginpos) {
            std::cerr << "ERROR: Invalid position in input stream whilst Reading pattern" << std::endl;
            throw InternalError();
        }
        in->seekg(beginpos, ios::beg);
        std::streampos beginposcheck = in->tellg();
        if ((beginposcheck != beginpos) && (beginposcheck >= 0xffffffffffffffff)) {
            std::cerr << "ERROR: Resetting read pointer for stage 2 failed! (" << (unsigned long) beginposcheck << " != " << (unsigned long) beginpos << ")" << std::endl;
            throw InternalError();
        } else if (!in->good()) {
            std::cerr << "ERROR: After resetting readpointer for stage 2, istream is not 'good': eof=" << (int) in->eof() << ", fail=" << (int) in->fail() << ", badbit=" << (int) in->bad() << std::endl;
            throw InternalError();
        }
        while (i < length) { //TODO: read multiple bytes in one go
            if (in->good()) {
                in->read( (char* ) &c, sizeof(char));
                if (debug) std::cerr << "DEBUG read2=" << (int) c << endl;
            } else {
                std::cerr << "ERROR: Invalid pattern data, unexpected end of file (stage 2,i=" << i << ",length=" << length << ",beginpos=" << beginpos << ",eof=" << (int) in->eof() << ",fail=" << (int) in->fail() << ",badbit=" << (int) in->bad() << ")" << std::endl;
                throw InternalError();
            }
            data[i++] = c;
            //if ((c == 0) && (!ignoreeol)) break; //not needed, already computed length
        }

        if (c != ClassDecoder::delimiterclass) { //add endmarker
            data[i++] = ClassDecoder::delimiterclass;
        }

        if (debug) std::cerr << "DEBUG: DONE READING PATTERN" << std::endl;

        //if this is the end of file, we want the eof bit set already, so we try to
        //read one more byte (and wind back if succesful):
        if (in->good()) {
            if (debug) std::cerr << "DEBUG: (TESTING EOF)" << std::endl;
            in->read( (char* ) &c, sizeof(char));
            if (in->good()) in->unget();
        }

    } else if (version == 1) { 
        data = convert_v1_v2(in, ignoreeol, debug);
    } else {
        std::cerr << "ERROR: Unknown version " << (int) version << std::endl;
        throw InternalError();
    }
    
}


PatternPointer::PatternPointer(std::istream * in, bool ignoreeol, const unsigned char version , unsigned char * corpusstart, bool debug) {
    if (corpusstart == NULL) {
        std::cerr << "ERROR: Can not read PatternPointer, no corpusstart passed!" << std::endl;
        throw InternalError();
    } else {
        unsigned int corpusoffset;
        in->read( (char* ) &corpusoffset, sizeof(unsigned int));
        data = corpusstart + corpusoffset;
        in->read( (char* ) &bytes, sizeof(uint32_t));
        in->read( (char* ) &mask, sizeof(uint32_t));
        if (debug) std::cerr << "DEBUG read patternpointer @corpusoffset=" << (size_t) data << " bytes=" << (int) bytes << " mask=" << (int) mask << std::endl;
    }
}
/*
Pattern::Pattern(std::istream * in, unsigned char * buffer, int maxbuffersize, bool ignoreeol, const unsigned char version, bool debug) {
    //read pattern using a buffer
    if (version == 2) {
    } else if (version == 1){ 
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
    } else {
        std::cerr << "ERROR: Unknown version " << (int) version << std::endl;
        throw InternalError();
    }

}
*/

Pattern::Pattern(const unsigned char * dataref, const int _size) {
    data = new unsigned char[_size+1];
    memcpy(data, dataref, _size);
    data[_size] = ClassEncoder::delimiterclass;
}


void Pattern::set(const unsigned char * dataref, const int _size) {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
    data = new unsigned char[_size+1];
    memcpy(data, dataref, _size);
    data[_size] = ClassEncoder::delimiterclass;
}


Pattern::Pattern(const Pattern& ref, unsigned int begin, unsigned int length) { //slice constructor
    //to be computed in bytes
    unsigned int begin_b = 0;
    unsigned int length_b = 0;
    bool prevhigh = false;

    unsigned int i = 0;
    unsigned int n = 0;
    do {
        const unsigned char c = ref.data[i];
        if (c < 128) {
            //we have a token
            n++; 
            if (n - begin == length) {
                length_b = (i + 1) - begin_b;
                break;
            } else if ((c == ClassDecoder::delimiterclass) && (!prevhigh) ) {
                length_b = i - begin_b;
                break;
            }
            i++;
            if (n == begin) begin_b = i;
            prevhigh = false;
        } else {
            prevhigh = true;
            i++;
        }
    } while (1);

    const unsigned char _size = length_b + 1;
    data = new unsigned char[_size];
    memcpy(data, ref.data + begin_b, begin_b+length_b);
    data[length_b] = ClassDecoder::delimiterclass;
}

PatternPointer::PatternPointer(unsigned char * ref, unsigned int begin, unsigned int length) { //slice constructor
    //to be computed in bytes
    unsigned int begin_b = 0;
    unsigned int length_b = 0;
    bool prevhigh = false;

    unsigned int i = 0;
    unsigned int n = 0;
    unsigned char c;
    do {
        c = ref[i];
        if (c < 128) {
            //we have a token
            n++; 
            if (n - begin == length) {
                length_b = (i + 1) - begin_b;
                break;
            } else if ((c == ClassDecoder::delimiterclass) && (!prevhigh) ) {
                length_b = i - begin_b;
                break;
            }
            i++;
            if (n ==  begin) begin_b = i;
            prevhigh = false;
        } else {
            prevhigh = true;
            i++;
        }
    } while (1);

    data = ref + begin_b;
    bytes = length_b;
    mask = computemask();
}

PatternPointer::PatternPointer(const Pattern& ref, unsigned int begin, int unsigned length) { //slice constructor
    //to be computed in bytes
    int begin_b = 0;
    int length_b = 0;
    bool prevhigh = false;

    unsigned int i = 0;
    unsigned int n = 0;
    unsigned char c;
    do {
        c = ref.data[i];
        if (c < 128) {
            //we have a token
            n++;
            if (n - begin == length) {
                length_b = (i + 1) - begin_b;
                break;
            } else if ((c == ClassDecoder::delimiterclass) && (!prevhigh) ) {
                length_b = i - begin_b;
                break;
            }
            i++;
            if (n == begin) begin_b = i;
            prevhigh = false;
        } else {
            prevhigh = true;
            i++;
        }
    } while (1);

    data = ref.data + begin_b;
    /*if (length_b >= B32) {
        std::cerr << "ERROR: Pattern too long for pattern pointer [length_b=" << length_b << ",begin=" << begin << ",length=" << length << ", reference_length_b=" << ref.bytesize() << "]  (did you set MAXLENGTH (-l)?)" << std::endl;
        std::cerr << "Reference=";
        ref.out();
        std::cerr << std::endl;
        throw InternalError();
    }*/
    bytes = length_b;
    mask = computemask();
}



PatternPointer::PatternPointer(const PatternPointer& ref, unsigned int begin, unsigned int length) { //slice constructor
    //to be computed in bytes
    unsigned int begin_b = 0;
    unsigned int length_b = 0;
    bool prevhigh = false;

    unsigned int i = 0;
    unsigned int n = 0;
    unsigned char c;
    do {
        if (i == ref.bytes) {
            length_b = i - begin_b;
            break;
        }
        c = ref.data[i];
        
        if (c < 128) {
            //we have a token
            n++; 
            if (n - begin == length) {
                length_b = (i + 1) - begin_b;
                break;
            } else if ((c == ClassDecoder::delimiterclass) && (!prevhigh) ) {
                length_b = i - begin_b;
                break;
            }
            i++;
            if (n == begin) begin_b = i;
            prevhigh = false;
        } else {
            prevhigh = true;
            i++;
        }
    } while (1);

    data = ref.data + begin_b;
    bytes = length_b;
    mask = computemask();
    
    /*std::cerr << "Created patternpointer: b=" << bytes << " n=" << this->n() << " (begin="<<begin<<",length="<<length<<")" <<endl; 
    this->out();
    std::cerr << std::endl;*/
}

Pattern::Pattern(const Pattern& ref) { //copy constructor
    const int s = ref.bytesize();
    data = new unsigned char[s + 1];
    memcpy(data, ref.data, s);
    data[s] = ClassDecoder::delimiterclass;
}

Pattern::Pattern(const PatternPointer& ref) { //constructor from patternpointer
	if ((ref.mask != 0) && (ref.isflexgram())) {
		unsigned char tmpdata[ref.bytesize()+1];
		int size = ref.flexcollapse(tmpdata);
		data = new unsigned char[size+1];
		memcpy(data, tmpdata, size);
		data[size] = ClassDecoder::delimiterclass;
	} else {
		data = new unsigned char[ref.bytesize() + 1];
		memcpy(data, ref.data, ref.bytesize());
		data[ref.bytesize()] = ClassDecoder::delimiterclass;

		if (ref.mask != 0) { 
			unsigned int n = 0;
			for (unsigned int i = 0; i < ref.bytesize(); i++) {
				if (ref.data[i] < 128) {
					if (ref.isgap(n)) {
						data[i] = ClassDecoder::skipclass;
					}
					n++;
				}
			}
		}
	}
}

Pattern::Pattern(const PatternPointer& ref, unsigned int begin, unsigned int length) { //slice constructor from patternpointer
    //to be computed in bytes
    unsigned int begin_b = 0;
    unsigned int length_b = 0;
    bool prevhigh = false;

    unsigned int i = 0;
    unsigned int n = 0;
    do {
        const unsigned char c = ref.data[i];
         if (c < 128) {
            //we have a token
            n++; 
            if (n - begin == length) {
                length_b = (i + 1) - begin_b;
                break;
            } else if ((c == ClassDecoder::delimiterclass) && (!prevhigh) ) {
                length_b = i - begin_b;
                break;
            }
            i++;
            if (n == begin) begin_b = i;
            prevhigh = false;
        } else {
            prevhigh = true;
            i++;
        }
    } while (1);

    const unsigned char _size = length_b + 1;
    data = new unsigned char[_size];
    memcpy(data, ref.data + begin_b, begin_b+length_b);
    data[length_b] = ClassDecoder::delimiterclass;

    if (ref.mask != 0) { 
        const bool flex = ref.isflexgram();
        i = 0;
        n = 0;
        do {
            if (ref.data[i] < 128) {
                if ((i >= begin_b) && (i < begin_b + length_b)) {
                    if (ref.isgap(n)) {
                        if (flex) {
                            data[i-begin_b] = ClassDecoder::flexclass;
                        } else {
                            data[i-begin_b] = ClassDecoder::skipclass;
                        }
                    }
                }
                n++;
            }
            i++;
        } while (i < ref.bytesize()); 
    }

}

Pattern::~Pattern() {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
}


bool Pattern::operator==(const Pattern &other) const {
    unsigned int i = 0;
    do {
        if (data[i] != other.data[i]) return false;
        if ((data[i] == 0) || (other.data[i] == 0)) return data[i] == other.data[i];
        i++;
    } while (true);
}
bool Pattern::operator==(const PatternPointer &other) const {
    return other == *this;
}

bool PatternPointer::operator==(const Pattern &other) const {
    unsigned int i = 0;
    unsigned int n = 0;
    if ((mask != 0) && (isflexgram())) {
		if (!other.isflexgram()) return false;
		unsigned char data1[bytesize()];
		const unsigned int size1 = flexcollapse(data1);
		if (size1 != other.bytesize()) return false;
		return (memcmp(data1,other.data,size1) == 0);
    }
    while (i<bytes){
	    if ((i>0) && (other.data[i-1] >= 128) && (other.data[i] == 0)) return false;
        if ((mask != 0) && (data[i] < 128))  {
            if (isgap(n)) {
                if (other.data[i] != ClassDecoder::skipclass)  return false;
            } else if (data[i] != other.data[i]) return false;
            n++;
        } else if (data[i] != other.data[i]) return false;
        i++;
    }
    return other.data[i] == ClassDecoder::delimiterclass;

}

int PatternPointer::flexcollapse(unsigned char * collapseddata) const {
	//collapse data
	bool prevgap = false;
	unsigned int j = 0;
	unsigned int n = 0;
	for (unsigned int i = 0; i < bytes; i++) {
		if (data[i] < 128) {
			if (isgap(n)) {
				if (!prevgap) {
					collapseddata[j++] = ClassDecoder::flexclass; 
					prevgap = true;
				}
			} else {
				collapseddata[j++] = data[i];
				prevgap = false;
			}
			n++;
		} else {
			collapseddata[j++] = data[i];
		}
	}
	return j;
}


bool PatternPointer::operator==(const PatternPointer & other) const {
    if (bytes == other.bytes) {
		if ((mask != 0) && (isflexgram())) {
			if ((other.mask == 0) || (!other.isflexgram())) return false;  
			unsigned char data1[bytes];
			int size1 = flexcollapse(data1);
			unsigned char data2[other.bytesize()];
			int size2 = other.flexcollapse(data2);
			if (size1 != size2) return false;
			return (memcmp(data1,data2,size1) == 0);
		} else if (mask != other.mask) {
			return false;
		} else {
			if (data == other.data) return true; //shortcut
			unsigned int i = 0;
			unsigned int n = 0;
			while (i<bytes) {
				if ((mask != 0) && (data[i] < 128))  {
					if (isgap(n)) {
						if (!other.isgap(n)) return false;
					} else if (data[i] != other.data[i]) return false;
					n++;
				} else if (data[i] != other.data[i]) return false;
				i++;
			}
			return true;
		}
    }
    return false;
}

bool Pattern::operator!=(const Pattern &other) const {
    return !(*this == other);
}
bool Pattern::operator!=(const PatternPointer &other) const {
    return !(other == *this);
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
    memcpy(data, other.data, s);
    data[s] = ClassDecoder::delimiterclass;
}

Pattern Pattern::operator +(const Pattern & other) const {
    const int s = bytesize();
    const int s2 = other.bytesize();
    unsigned char buffer[s+s2]; 
    memcpy(buffer, data, s);
    for (int i = 0; i < s2; i++) {
        buffer[s+i] = other.data[i];
    }
    return Pattern(buffer, s+s2);
}


PatternPointer& PatternPointer::operator++() {
	const size_t _n = n();
	unsigned char * cursor  = data;
	unsigned char * newdata  = NULL;
	unsigned int newn = 0;
	do {
		if (*cursor < 128) {
			if (newdata == NULL) {
				newdata = cursor + 1;
			} else {
				newn++;
			}
		}
		cursor++;	
	} while (newn < _n);
	data = newdata;
	bytes = cursor-newdata;
	return *this;
}

int Pattern::find(const Pattern & pattern) const { //returns the index, -1 if not found 
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
        container.push_back(PatternPointer(*this,i,n));
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
    int partbegin = 0;
    int partlength = 0;
    bool prevhigh = false;

    int found = 0;
    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        if ((!prevhigh) && (c == ClassDecoder::delimiterclass)) {
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(pair<int,int>(partbegin,partlength));
                found++;
            }
            break;
        } else if ((!prevhigh) &&  ((c == ClassDecoder::skipclass) || (c == ClassDecoder::flexclass))) {        
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(pair<int,int>(partbegin,partlength));
                found++;
            }
            i++;
            n++; 
            partbegin = n; //for next part
        } else if (c < 128) {
            //low byte, end of token
            i++;
            n++;
        } else {
            //high byte
            i++;
        }
        prevhigh = (c >= 128);
    } while (1);
    return found;
}

int Pattern::parts(vector<Pattern> & container) const { 
    int partbegin = 0;
    int partlength = 0;
    bool prevhigh = false;

    int found = 0;
    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        if ((!prevhigh) && (c == ClassDecoder::delimiterclass)) {
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(Pattern(*this,partbegin,partlength));
                found++;
            }
            break;
        } else if ((!prevhigh) &&  ((c == ClassDecoder::skipclass) || (c == ClassDecoder::flexclass))) {        
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(Pattern(*this,partbegin,partlength));
                found++;
            }
            i++;
            n++; 
            partbegin = n; //for next part
        } else if (c < 128) {
            //low byte, end of token
            i++;
            n++;
        } else {
            //high byte
            i++;
        }
        prevhigh = (c >= 128);
    } while (1);
    return found;
}

int Pattern::parts(vector<PatternPointer> & container) const { 
    int partbegin = 0;
    int partlength = 0;
    bool prevhigh = false;

    int found = 0;
    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        if ((!prevhigh) && (c == ClassDecoder::delimiterclass)) {
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(PatternPointer(*this,partbegin,partlength));
                found++;
            }
            break;
        } else if ((!prevhigh) &&  ((c == ClassDecoder::skipclass) || (c == ClassDecoder::flexclass))) {        
            partlength = n - partbegin;
            if (partlength > 0) {
                container.push_back(PatternPointer(*this,partbegin,partlength));
                found++;
            }
            i++;
            n++; 
            partbegin = n; //for next part
        } else if (c < 128) {
            //low byte, end of token
            i++;
            n++;
        } else {
            //high byte
            i++;
        }
        prevhigh = (c >= 128);
    } while (1);
    return found;
}

int PatternPointer::parts(vector<pair<int,int>> & container) const { 
    int partbegin = 0;
    int partlength = 0;

    int found = 0;
    unsigned int n = 0;
    for (unsigned int i = 0; (i<bytes) && (i<31); i++) {
        const unsigned char c = data[i];
        if (c < 128) {
            //low byte, end of token
            if ((mask & bitmask[n]) == 0) {
                partlength = n - partbegin;
                if (partlength > 0) {
                    container.push_back(pair<int,int>(partbegin,partlength));
                    found++;
                }
                n++;
                partbegin = n;
            } else {
                n++;
            }
        } else {
            //high byte
        }
    } 
    partlength = n - partbegin;
    if (partlength > 0) {
        container.push_back(pair<int,int>(partbegin,partlength));
        found++;
    }
    return found;
}


int PatternPointer::parts(vector<PatternPointer> & container) const { 
    int partbegin = 0;
    int partlength = 0;

    int found = 0;
    unsigned int n = 0;
    for (unsigned int i = 0; (i<bytes) && (i<31); i++) {
        const unsigned char c = data[i];
        if (c < 128) {
            if (isgap(n)) {
                partlength = n - partbegin;
                if (partlength > 0) {
                    container.push_back(PatternPointer(*this,partbegin,partlength));
                    found++;
                }
                partbegin = n+1;
            }
            //low byte, end of token
            n++;
        }
    } 
    partlength = n - partbegin;
    if (partlength > 0) {
        container.push_back(PatternPointer(*this,partbegin,partlength));
        found++;
    }
    return found;
}

const unsigned int Pattern::skipcount() const { 
    int count = 0;
    int i = 0;
    bool prevhigh = false;
    do {
        const unsigned char c = data[i];
        if ((!prevhigh) && (c == ClassDecoder::delimiterclass)) {
            //end marker
            return count;
        } else if (!prevhigh && (((c == ClassDecoder::skipclass) || (c == ClassDecoder::flexclass)) && ((i > 0) || ((data[i-1] != ClassDecoder::skipclass) && (data[i-1] != ClassDecoder::flexclass)))))   {
            //we have a marker
            count++;
            i++;
        } else {
            i++;
        }
        prevhigh = (c >= 128);
    } while (1); 
}

const unsigned int PatternPointer::skipcount() const { 
    if (mask == 0) return 0;
    unsigned int skipcount = 0;
    unsigned int i = 0;
    unsigned int n = 0;
    bool prevskip = false;
    do {
        if (data[i] < 128) {
            if (isgap(n)) {
                if (!prevskip) skipcount++;
                prevskip = true;
            } else {
                prevskip = false;
            }
            n++;
        }
        i++;
    } while (i < bytes); 
    return skipcount;
}

int PatternPointer::gaps(vector<pair<int,int> > & container) const { 
    if (mask == 0) return 0;
    int i = 0;
    int n = 0;
    int beginskip = -1;
    int skiplength = 0;
    do {
        if (data[i] < 128) {
            if (isgap(n)) {
                if (beginskip > 0) { 
                    skiplength++;
                } else {
                    beginskip = i;
                    skiplength = 1;
                }
            } else {
                if (beginskip > -1) container.push_back(pair<int,int>(beginskip,skiplength));
                beginskip = -1;
            }
            n++;
        }
        i++;
    } while ((unsigned int) i < bytes); 
    if (beginskip > -1) container.push_back(pair<int,int>(beginskip,skiplength));
    return container.size();
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
        if (((i == 0) || (data[i-1] >= 128) )&&  ((data[i] == ClassDecoder::skipclass) || (data[i] == ClassDecoder::flexclass))) {
            endskip++;
        } else {
            break;
        }
    } 

    if (endskip) container.push_back(pair<int,int>(_n - endskip,endskip));
    return container.size();
}

Pattern Pattern::extractskipcontent(const Pattern & instance) const {
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
    
    unsigned char a = ClassDecoder::skipclass;
    const Pattern skip = Pattern(&a,1);

    std::vector<std::pair<int,int> >::iterator iter = gapcontainer.begin();
    Pattern pattern = Pattern(instance,iter->first, iter->second);
    int cursor = iter->first + iter->second;
    iter++;
    while (iter != gapcontainer.end()) {  
        if (cursor > 0) {
            const int skipsize = iter->first - cursor;
            for (int i = 0; i < skipsize; i++) {
                pattern = pattern + skip;
            }
        }    
        Pattern subngram = Pattern(instance,iter->first, iter->second);
        pattern = pattern + subngram;
        cursor = iter->first + iter->second;
        iter++;
    }
    return pattern;
}

bool Pattern::instanceof(const Pattern & skipgram) const {  //TODO: refactor for v2
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

bool PatternPointer::instanceof(const Pattern & skipgram) const {  //TODO: refactor for v2
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
            const PatternPointer token2 = PatternPointer(*this, i, 1);
            if ((token2 != token1) && (token1.category() != SKIPGRAM)) return false;
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


Pattern Pattern::addskip(const std::pair<int,int> & gap) const {
    //Returns a pattern with the specified span replaced by a fixed skip
    const unsigned int _n = n();
    const Pattern replacement = Pattern(gap.second);
    Pattern pattern = replace(gap.first, gap.second, replacement);
    if (pattern.n() != _n) {
        std::cerr << "ERROR: addskip(): Pattern length changed from " << _n << " to " << pattern.n() << " after substituting slice (" << gap.first << "," <<gap.second << ")" << std::endl;
        throw InternalError();
    }
    return pattern;
}

Pattern Pattern::addskips(const std::vector<std::pair<int,int> > & gaps) const {
    //Returns a pattern with the specified spans replaced by fixed skips
    const unsigned int _n = n();
    Pattern pattern = *this; //needless copy?
    for (vector<pair<int,int> >::const_iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        const Pattern replacement = Pattern(iter->second);
        pattern = pattern.replace(iter->first, iter->second, replacement);
        if (pattern.n() != _n) {
            std::cerr << "ERROR: addskip(): Pattern length changed from " << _n << " to " << pattern.n() << " after substituting slice (" << iter->first << "," <<iter->second << ")" << std::endl;
            throw InternalError();
        }
    }
    return pattern;
}

PatternPointer PatternPointer::addskip(const std::pair<int,int> & gap) const {
    //Returns a patternpointer with the specified span replaced by a fixed skip
    PatternPointer copy = *this;
    for (int i = gap.first; i < (gap.first + gap.second) && (i < 31); i++ ) {
        copy.mask |= bitmask[i];
    }
    return copy;
}

PatternPointer PatternPointer::addskips(const std::vector<std::pair<int,int> > & gaps) const {
    //Returns a patternpointer with the specified spans replaced by fixed skips
    PatternPointer copy = *this;
    for (vector<pair<int,int> >::const_iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        for (int i = iter->first; i < (iter->first + iter->second) && (i < 31); i++ ) {
            copy.mask |= bitmask[i];
        }
    }
    return copy;
}
Pattern Pattern::addflexgaps(const std::vector<std::pair<int,int> > & gaps) const {
    //Returns a pattern with the specified spans replaced by fixed skips
    Pattern pattern = *this; //needless copy? 
    for (vector<pair<int,int> >::const_iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        pattern = pattern.replace(iter->first, iter->second, FLEXPATTERN);
    }
    return pattern;
}


Pattern Pattern::reverse() const { 
    const unsigned char _size = bytesize() + 1;
    unsigned char * newdata = new unsigned char[_size];

    //set endmarker
    newdata[_size - 1] = ClassDecoder::delimiterclass;

    //we fill the newdata from right to left
    unsigned char cursor = _size - 1;

    bool prevhigh = false;
	unsigned int high = 0;
    int i = 0;
    do {
        const unsigned char c = data[i];
        if ((!prevhigh) && (c == ClassDecoder::delimiterclass)) {
            //end marker
            break;
        } else if (c < 128) {
		    cursor = cursor - high - 1;	
            //we have a size
            strncpy((char*) newdata + cursor, (char*) data + i, high + 1);
            i += high + 1;
			high = 0;
            prevhigh = false;
        } else {
            prevhigh = true;
			high++;
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
    unsigned char version = getdataversion(in);
    if (version == 2) {
        in->seekg(0,ios_base::end);
        corpussize = in->tellg();
        in->seekg(2);
        corpussize = corpussize - 2;
        corpus = new unsigned char[corpussize];
        in->read((char*) corpus,sizeof(unsigned char) * corpussize);
    } else {
        //old version
        in->seekg(0);
        corpus = convert_v1_v2(in,true,false);
    }

    //constructing sentence index
    uint32_t sentence = 1;
    unsigned char * cursor = corpus;
    bool prevdelimiter = true;
    bool prevhigh = false;
    while (cursor < corpus + corpussize) {
        if (prevdelimiter) {
            sentenceindex.insert(pair<uint32_t,unsigned char *>(sentence,cursor));
            sentence++;
            prevdelimiter = false;
        }
        if ((!prevhigh) && (*cursor == ClassDecoder::delimiterclass)) {
            prevdelimiter = true;
        }
        prevhigh = (*cursor >= 128);
        cursor++;
    }
    if (debug) cerr << "Loaded " << sentenceindex.size() << " sentences" << endl;
	patternpointer = new PatternPointer(corpus,corpussize);
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

unsigned char * IndexedCorpus::getpointer(const IndexReference & begin) const {
    if (corpus == NULL) {
        std::cerr << "ERROR: No corpus data loaded! (in IndexedCorpus::getpattern)" << std::endl;
        throw InternalError();
    }
    std::map<uint32_t,unsigned char*>::const_iterator iter = sentenceindex.find(begin.sentence);
    if (iter == sentenceindex.end()) {
        return NULL;
    }
    unsigned char * i = iter->second;
    unsigned int n = 0;
    bool prevhigh = false;
    do {
        if (begin.token == n) return i;
        if ((!prevhigh) && (*i == ClassDecoder::delimiterclass)) {
            break;
        } else if (*i < 128) {
            n++;
            prevhigh = false;
        } else {
            prevhigh = true;
        }
        i++;
    } while (i < corpus + corpussize);
    return NULL;
}



PatternPointer IndexedCorpus::getpattern(const IndexReference & begin, int length) const { 
    if (corpus == NULL) {
        std::cerr << "ERROR: No corpus data loaded! (in IndexedCorpus::getpattern)" << std::endl;
        throw InternalError();
    }
    unsigned char * data = getpointer(begin);
    if (data == NULL) throw KeyError();
    return PatternPointer(data,false,length);
}


void IndexedCorpus::findpattern(std::vector<IndexReference> & result, const Pattern & pattern,  uint32_t sentence, const PatternPointer & sentencedata, int maxmatches) {
    //TODO: no flexgrams supported yet
    const int _n = pattern.size();
    vector<pair<PatternPointer,int>> ngrams;
    sentencedata.ngrams(ngrams,_n);
    for (vector<pair<PatternPointer,int>>::iterator iter = ngrams.begin(); iter != ngrams.end(); iter++) { 
        if ((iter->first == pattern) || ((pattern.category() > NGRAM) && iter->first.instanceof(pattern))) {
            result.push_back(IndexReference(sentence,iter->second));
        }
    }
}

std::vector<IndexReference> IndexedCorpus::findpattern(const Pattern & pattern, uint32_t sentence, int maxmatches) {
    //far more inefficient than a pattern model obviously
    std::vector<IndexReference> result;
    const int _n = pattern.size();
    if (_n == 0) return result;
 
    IndexReference ref;
    if (sentence == 0) {
        for (std::map<uint32_t,unsigned char*>::iterator iter = sentenceindex.begin(); iter != sentenceindex.end(); iter++) {
            const PatternPointer sentence_pp = getsentence(iter->second);
            findpattern(result, pattern, iter->first, sentence_pp,maxmatches);
        }
    } else {
        const PatternPointer sentence_pp = getsentence(sentence);
        findpattern(result, pattern, sentence, sentence_pp,maxmatches);
    }
    return result;
}

int IndexedCorpus::sentencelength(int sentence) const {
    return sentencelength(getpointer(IndexReference(sentence,0)));
}

int IndexedCorpus::sentencelength(unsigned char * cursor) const {
    unsigned int n = 0;
    bool prevhigh = false;
    do {
        if ((!prevhigh) && (*cursor == ClassDecoder::delimiterclass)) return n;
        if (*cursor < 128) {
            n++;
            prevhigh = false;
        } else {
            prevhigh = true;
        }
        cursor++;
    } while (cursor < corpus + corpussize);
    return n;
}

PatternPointer IndexedCorpus::getsentence(int sentence) const { 
    return getpattern(IndexReference(sentence,0), sentencelength(sentence));
}

PatternPointer IndexedCorpus::getsentence(unsigned char * sentencedata) const { 
	return PatternPointer(sentencedata,sentencelength(sentencedata));
}


PatternPointer Pattern::getpointer() const {
    return PatternPointer(*this);
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
