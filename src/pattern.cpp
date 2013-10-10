#include "pattern.h"
#include "SpookyV2.h" //spooky hash

using namespace std;


unsigned char mainpatternbuffer[MAINPATTERNBUFFERSIZE];

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



const size_t Pattern::n() const {
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
    if (s <= 0) {
        cerr << "INTERNAL ERROR Pattern::write(): Writing pattern with size <= 0! Not possible!" << endl;
        throw InternalError();
    }
    out->write( (char*) data , (int) s );
}

std::string Pattern::tostring(ClassDecoder& classdecoder) const {


    std::string result = ""; 
    int i = 0;
    int gapsize = 0;
    do {
        const unsigned char c = data[i];
        if (c == ENDMARKER) {
            //end marker
            if (gapsize > 0) {
                if (!result.empty()) result += " ";
                result += std::string("{*") + to_string(gapsize) + std::string("*} ");                
                gapsize = 0;
            }
            return result;
        } else if (c < 128) {
            //we have a size
            if (gapsize > 0) {
                if (!result.empty()) result += " ";
                result += std::string("{*") + to_string(gapsize) + std::string("*} ");                
                gapsize = 0;
            }
            unsigned int cls =  bytestoint(data + i +1, c);
            //cerr << (int) c << ":" << (int) *(data+i + 1) << ":" << (int) data[i+1] << ":" << cls << endl;
            i += c + 1;
            if (!result.empty()) result += " ";
            result += classdecoder[cls];
        } else if (c == FIXEDGAP) {
            gapsize++;
            i++;
        } else if (c == DYNAMICGAP) {
            if (result.empty()) {
                result = "{*}";
            } else {
                result = " {*}";
            }
            i++;
        } else {
            //we have another marker
            i++;
        }
    } while (1);
    return result;  
}

bool Pattern::out() const { 
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
        } else if (c == FIXEDGAP) {
            //DYNAMICGAP is counted as 1, the minimum fill
            cerr << "FIXEDGAP" << " ";
            i++;
        } else if (c == DYNAMICGAP) {
            //DYNAMICGAP is counted as 1, the minimum fill
            cerr << "DYNAMICGAP" << " ";
            i++;
        } else {
            //we have another marker
            i++;
        }
    } while (1);
    return false;
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
            in->read( (char*) mainpatternbuffer + i, c);
            i += c;
        } else {
            //other marker
            mainpatternbuffer[i++] = c;
        }
    } while (1);

    //copy from mainpatternbuffer
    data = new unsigned char[i];
    for (int j = 0; j < i; j++) {
        data[j] = mainpatternbuffer[j];
    }

}


Pattern::Pattern(const unsigned char * dataref, const int _size) {

    data = new unsigned char[_size];
    int j = 0;
    for (int i = 0; i < _size; i++) {
        data[j++] = dataref[i];
        unsigned char c = dataref[i];
        if (c == 0) {
            break;
        } else if (c < 128) {
            int end = i + c;
            do {
                i++;
                data[j++] = dataref[i];
            } while (i < end);
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
        } else if ((c == FIXEDGAP) || (c == DYNAMICGAP)) {
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
    for (int i = begin_b; i < length_b; i++) {
        data[j++] = ref.data[i];
    }
    data[j++] = ENDMARKER;
}


Pattern::Pattern(const Pattern& ref) { //copy constructor
    const int s = ref.bytesize();
    data = new unsigned char[s + 1];
    for (int i = 0; i < s; i++) {
        data[i] = ref.data[i];
    }
    data[s] = ENDMARKER;
}

Pattern::~Pattern() {
    delete[] data;
}


bool Pattern::operator==(const Pattern &other) const {
        const int s = bytesize();
        if (bytesize() == other.bytesize()) {
            for (int i = 0; i < s; i++) {
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
    int min_s;
    if (s < s2) {
        min_s = s;
    } else {
        min_s = s2;
    }
    for (int i = 0; i < min_s; i++) {
        if (data[i] < other.data[i]) return true;
    }
    return (s < s2);
}
bool Pattern::operator>(const Pattern & other) const {
    const int s = bytesize();
    const int s2 = other.bytesize();
    int min_s;
    if (s < s2) {
        min_s = s;
    } else {
        min_s = s2;
    }
    for (int i = 0; i < min_s; i++) {
        if (data[i] > other.data[i]) return true;
    }
    return (s > s2);
}

Pattern & Pattern::operator =(Pattern other) { //(note: argument passed by value!
        //delete old data
        if (data != NULL) delete [] data;
        
        //set new data
        const int s = other.bytesize();        
        data = new unsigned char[s];   
        for (int i = 0; i < s; i++) {
            data[i] = other.data[i];
        }  
 
        // by convention, always return *this (for chaining)
        return *this;
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

    
    for (int i = 0; i < (_n - n); i++) {
        Pattern pattern = Pattern(*this,i,n);
        container.push_back( pattern );
        found++;
    }
    return found;
}   


int Pattern::ngrams(vector<pair<Pattern,int>> & container, const int n) const { //return multiple ngrams, also includes skipgrams!
    const int _n = this->n();
    if (n > _n) return 0;

    
    int found = 0;

    
    for (int i = 0; i < (_n - n); i++) {
        Pattern pattern = Pattern(*this,i,n);
        container.push_back( pair<Pattern,int>(pattern,i) );
        found++;
    }
    return found;
}   

int Pattern::subngrams(vector<Pattern> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n-1) maxn = _n-1;
    if (minn > _n-1) return 0;
    int found = 0;
    for (int i = minn; i <= maxn; i++) {
        found += ngrams(container, i);
    }
    return found;
}

int Pattern::subngrams(vector<pair<Pattern,int>> & container, int minn, int maxn) const { //also includes skipgrams!
    const int _n = n();
    if (maxn > _n-1) maxn = _n-1;
    if (minn > _n-1) return 0;
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
        } else if ((c == FIXEDGAP) || (c == DYNAMICGAP)) {        
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
        } else if (((c == DYNAMICGAP) || (c == FIXEDGAP)) && ((i > 0) || ((data[i-1] != DYNAMICGAP) && (data[i-1] != FIXEDGAP))))   {
            //we have a marker
            count++;
            i++;
        }
    } while (1); 
}

int Pattern::gaps(vector<pair<int,int> > & container) const {
    vector<pair<int,int> > partscontainer;
    int found = parts(partscontainer);
    if (found == 1) return 0;

    const int _n = n();

    found = 0;
    //compute inverse:
    int begin = 0;
    for (vector<pair<int,int>>::iterator iter = partscontainer.begin(); iter != partscontainer.end(); iter++) {
        if (iter->first > begin) {
            container.push_back(pair<int,int>(begin,iter->first - begin));
            begin = iter->first + iter->second;
            found++;
        }
    }
    if (begin != _n) {
        container.push_back(pair<int,int>(begin,_n - begin));
    }
    return found;
}

Pattern Pattern::extractskipcontent(Pattern & instance) const {
    if (instance.category() == DYNAMICSKIPGRAM) {
        cerr << "Extractskipcontent not supported on Pattern with dynamic gaps!" << endl;
        throw InternalError();
    }
    if (instance.category() == NGRAM) {
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

    vector<Pattern> subngrams;
    vector<int> skipcontent_skipref;
    int cursor = 0;
    std::vector<std::pair<int,int> >::iterator iter = gapcontainer.begin();
    Pattern subngram = Pattern(instance,iter->first, iter->second);
    Pattern pattern = subngram;
    do {  
        cursor = iter->first + iter->second;
        if (cursor > 0) {
            for (int i = 0; i < iter->first - cursor; i++) {
                pattern = pattern + skip;
            }
        }    
        subngram = Pattern(instance,iter->first, iter->second);
        pattern = pattern + subngram;

    } while (iter != gapcontainer.end());
    return pattern;
}

bool Pattern::instantiates(const Pattern & skipgram) const { 
    //Is this a full instantiation of the skipgram?
    if (category() != NGRAM) return false;
    if (skipgram.category() == NGRAM) return (*this) == skipgram;

    if (skipgram.category() == DYNAMICSKIPGRAM) {
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
            if ((token1 != token2) && (token1.category() != FIXEDSKIPGRAM)) return false;
        }
        return true;
    }

}


Pattern Pattern::replace(int begin, int length, const Pattern & replacement) const {
    const int _n = n();
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

Pattern Pattern::addfixedskips(std::vector<std::pair<int,int> > & gaps) const {
    //Returns a pattern with the specified spans replaced by fixed skips
    Pattern pattern = *this;
    for (vector<pair<int,int>>::iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        pattern = pattern.replace(iter->first, iter->second, FIXEDGAPPATTERN);
    }
    return pattern;
}

Pattern Pattern::adddynamicskips(std::vector<std::pair<int,int> > & gaps) const {
    //Returns a pattern with the specified spans replaced by fixed skips
    Pattern pattern = *this;
    for (vector<pair<int,int>>::iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        pattern = pattern.replace(iter->first, iter->second, DYNAMICGAPPATTERN);
    }
    return pattern;
}
