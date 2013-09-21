#include "SpookyV2.h" //spooky hash

using namespace std;


const PatternCategory Pattern::category() const {
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
            i++;
        }
    } while (1);
}



const unsigned int Pattern::size() const {
    //return the size of the pattern (in bytes)

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
            i++;
        }
    } while (1);
}



const unsigned int Pattern::n() const {
    //return the size of the pattern (in tokens)

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
        } else if ((c == FIXEDGAP)  || (c == DYNAMICGAP)) {
            //DYNAMICGAP is counted as 1, the minimum fill
            i++;
            n++;
        } else {
            //we have another marker
            i++;
        }
    } while (1);
}


const StructureType Pattern::type() const {
    //return the size of the pattern (in tokens)
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
    const int s = size();
    if (s <= 0) {
        cerr << "INTERNAL ERROR Pattern::write(): Writing pattern with size <= 0! Not possible!" << endl;
        throw InternalError();
    }
    out->write( (char*) data , (int) s );
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
            i++;
        }
    } while (1);
}



Pattern::Pattern(std::istream * in) {
    int i = 0;
    unsigned char c;
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
            //other marker
            mainpatternbuffer[i++] = c;
        }
    } while (1);

    //copy from mainpatternbuffer
    data = new unsigned char[i]
    for (int j = 0; j < i; j++) {
        data[j] = mainpatternbuffer[j];
    }

}


Pattern::Pattern(const unsigned char* dataref, const int _size) {

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
        } else if (c == FIXEDGAP) || (c == DYNAMICGAP) {
            i++;
            n++;
            if (n == begin) begin_b = i;
        } else {
            //we have another marker
            i++;
        }
    } while (1);

    const unsigned char _size = length_b + 1
    data = new unsigned char[_size]
    int j = 0;
    for (int i = begin_b; i < length_b; i++) {
        data[j++] = ref.data[i];
    }
    data[j++] = ENDMARKER;
}

Pattern::~Pattern() {
    delete[] data;
}


bool Pattern::operator==(const Pattern &other) const {
        const int s = size();
        if (size() == other.size()) {
            for (int i = 0; i < s; i++) {
                if (data[i] != other.data[i]) return false;
            }
            return true;
        } else {
            return false;
        }        
}

bool Pattern::operator!=(const EncAnyGram &other) const {
    return !(*this == other);
}


Pattern & Pattern::operator =(Pattern other) { //(note: argument passed by value!
        //delete old data
        if (data != NULL) delete [] data;
        
        //set new data
        const int s = other.size();        
        data = new unsigned char[s];   
        for (int i = 0; i < s; i++) {
            data[i] = other.data[i];
        }  
 
        // by convention, always return *this (for chaining)
        return *this;
}

Pattern Pattern::operator +(const Pattern & other) const {
    const int s = size();
    const int s2 = other.size();
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
    const int s = size();
    const int s2 = pattern.size();
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

int Pattern::ngrams(vector<const Pattern> & container, const int n) const { //return multiple ngrams
    const int _n = n();
    if (n > _n) return 0;

    
    int found = 0;

    
    for (int i = 0; i < (_n - n); i++) {
        container.append( Pattern(*this, i, n) );
        found++;
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
        const unsigned char c = ref.data[i];
        
        if (c == ENDMARKER) {
            partlength = n - partbegin;
            if (partlength > 0) {
                container.append(pair<int,int>(partbegin,partlength);
                found++;
            }
            break;
        } else if (c < 128) {
            //we have a size
            i += c + 1;
            n++;
        } else if (c == FIXEDGAP) || (c == DYNAMICGAP) {        
            partlength = n - partbegin;
            if (partlength > 0) {
                container.append(pair<int,int>(partbegin,partlength);
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

int Pattern::parts(vector<const Pattern> & container) const {
    vector<pair<int,int>> partoffsets; 
    found = parts(partoffsets);

    for (vector<pair<int,int>>::iterator iter = partoffsets.begin(); iter != partoffsets.end(); iter++) {
        const int begin = iter->first;
        const int length = iter->second;
        container.append( Pattern(*this, begin, length) );
    }
    return found;
}

