#include "SpookyV2.h" //spooky hash

using namespace std;


const unsigned char Pattern::category() const {
    //return props >> 5; //get first three bits
    
    //return the size of the pattern (in bytes)
    if (iskey()) return sizeof(size_t)+1;

    PatternCategory category = NGRAM;

    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            return category;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
        } else {
            //we have a marker
            if ((c == FIXEDGAP) && (category < FIXEDSKIPGRAM)) category = FIXEDSKIPGRAM;
            if (c == DYNAMICGAP) return DYNAMICSKIPGRAM;
            i = reader_passmarker(i);
        }
    } while (1);
}


//const bool Pattern::iscopy() const {
//    return (bool) (props & 16); //get fourth bit
//}


//void Pattern::setprops(PatternCategory category) {
//    props = category << 5; //set first three bits
    //if (copy) props |= 16; //set fourth bit
//}


int Pattern::reader_passmarker(int i) const {
    //read marker from memory
    const unsigned char c = data[i];
    if ((c >= FACTOR1) && (C <= FACTOR9)) {
        i += data[i+1] //size of the factor
    }
    return i+1;
}




int reader_passmarker(const unsigned char c, std::istream * in) const {
    //read marker from file and skip
    if ((c >= FACTOR1) && (C <= FACTOR9)) {
        //read size of the factor
        unsigned char size;
        in->read((char *) &size, sizeof(char));
        //advance pointer (skip)
        const size_t pos = in->tellg();
        in->seekg(pos + size);
    }
    return i+1;
}



void Pattern::reader_marker(unsigned char * _data, std::istream * in) {
    //read marker from file into data
    if ((c >= FACTOR1) && (C <= FACTOR9)) {
        unsigned char size;
        in->read((char *) _data, sizeof(char));
        in->read((char *) _data + 1, _data[0]);
    }
}



const unsigned int Pattern::size() const {
    //return the size of the pattern (in bytes)
    if (iskey()) return sizeof(size_t);

    int i = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            return i + 1;
        } else if (c < 128) {
            //we have a size
            i += c + 1
        } else {
            //we have a marker
            i = reader_passmarker(i);
        }
    } while (1);
}



const unsigned int Pattern::n() const {
    //return the size of the pattern (in tokens)
    if (iskey()) return 0;

    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            return n;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
            n++;
        } else if (c == FIXEDGAP) {
            i++;
            n++;
        } else if (c == DYNAMICGAP) {
            return 0;
        } else {
            //we have a marker
            i = reader_passmarker(i);
        }
    } while (1);
}


const StructureType Pattern::type() const {
    //return the size of the pattern (in tokens)
    if (iskey()) return 0;



    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        if ((c == ENDMARKER) || (c < 128) || (c == FIXEDGAP) || (c == DYNAMICGAP)) {
            return STRUCT_PATTERN;
        } else if (c == SENTENCEMARKER) {
            return STRUCT_SENTENCE;
        } else if (c == PARAGRAPHMARKER) {
            return STRUCT_PARAGRAPH;
        } else if (c == BEGINDIVMARKER) {
            return STRUCT_DIV;
        } else if (c == BEGINTEXT) {
            return STRUCT_TEXT;
        } else if (c == HEADERMARKER) {
            return STRUCT_HEADER;
        } else {
            //we have a different marker
            i = reader_passmarker(i);
        }
    } while (1);
}



const set<int> Pattern::factors() const {
    //return the size of the pattern (in tokens)
    if (iskey()) return 0;

    set<char> factors;
    int i = 0;
    int n = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            return factors;
        } else if (c < 128) {
            i += c + 1;
        } else if (c == FACTOR1) {
            factors.insert(1);
        } else if (c == FACTOR2) {
            factors.insert(2);
        } else if (c == FACTOR3) {
            factors.insert(3);
        } else if (c == FACTOR4) {
            factors.insert(4);
        } else if (c == FACTOR5) {
            factors.insert(5);
        } else if (c == FACTOR6) {
            factors.insert(6);
        } else if (c == FACTOR7) {
            factors.insert(7);
        } else if (c == FACTOR8) {
            factors.insert(8);
        } else if (c == FACTOR9) {
            factors.insert(9);
        } else {
            //we have a marker
            i = reader_passmarker(i);
        }
    } while (1);
}


const size_t Pattern::hash(bool stripmarkers) const {
    if (iskey()) return (size_t) *(data+1);

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
            s = reader_passmarker(s);
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
                i = reader_passmarker(i);
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
    if (iskey()) {
        cerr << "INTERNAL ERROR Pattern::write(): Attempting to write unresolved PatternKey! Not possible!" << endl;
        throw InternalError();
    }
    const int _size = size();
    if (_size <= 0) {
        cerr << "INTERNAL ERROR Pattern::write(): Writing pattern with size <= 0! Not possible!" << endl;
        throw InternalError();
    }
    out->write( (char*) &_size, sizeof(unsigned char) ); //data length
    out->write( (char*) data , (int) _size ); //data
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
            in->seekg(pos + size);
        } else {
            //we have a marker
            reader_passmarker(c);
        }
    } while (1);
}

Pattern::Pattern(std::istream * in) {
    int i = 0;
    unsigned char c;
    PatternCategory category = NGRAM;
    do {
        in->read( (char* ) &c, sizeof(char));
        mainpatternbuffer[i++] = c;
        if (c == ENDMARKER) {
            mainpatternbuffer[i++] = ENDMARKER;
            break;
        } else if (c < 128) {
            //we have a size, load token
            in->read( mainpatternbuffer + i, c)
            i += c;
        } else {
            //we have a marker
            reader_marker(mainpatternbuffer + i, in);
        }
    } while (1);

    //copy from mainpatternbuffer
    data = new unsigned char[i]
    for (int j = 0; j < i; j++) {
        data[j] = mainpatternbuffer[j];
    }

    //setprops(category, true);
}


Pattern::Pattern(const unsigned char* dataref, const int _size, const set<unsigned char> * markers = NULL) {

    data = new unsigned char[_size]
    int j = 0;
    for (int i = 0; i < _size; i++) {
        data[j++] = dataref[i];
        unsigned char c = dataref[i];
        if (c == 0) {
            break;
        } else if (c < 128) {
            int end = i + c;
            for (i; i < end; i++) {
                data[j++] = dataref[i];
            }
        }


    }
    data[j++] = ENDMARKER;

}

Pattern::~Pattern() {
    if (iskey() || copy()) delete[] data;
}

