#include "pattern.h"
#include "SpookyV2.h" //spooky hash

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
            if (c == SKIPMARKER) category = SKIPGRAM;
            if (c == FLEXMARKER) return FLEXGRAM;
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
            result += classdecoder[cls];
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



Pattern::Pattern(std::istream * in) {
    int i = 0;
    int readingdata = 0;
    unsigned char c;
    do {
        if (in->good()) {
            in->read( (char* ) &c, sizeof(char));
        } else {
            std::cerr << "ERROR: Invalid pattern data, unexpected end of file" << std::endl;
            throw InternalError();
        }
        if (i >= MAINPATTERNBUFFERSIZE) {
            std::cerr << "ERROR: Pattern(): Patternbuffer size exceeded, exceptionally large pattern, must be invalid" << std::endl;
            throw InternalError();
        }
        mainpatternbuffer[i++] = c;
        if (readingdata) {
            readingdata--;
        } else {
            if (c == ENDMARKER) {
                break;
            } else if (c < 128) {
                //we have a size
                readingdata = c;
            }
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


void Pattern::set(const unsigned char * dataref, const int _size) {
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
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


Pattern::Pattern(const Pattern& ref) { //copy constructor
    const int s = ref.bytesize();
    data = new unsigned char[s + 1];
    for (int i = 0; i < s; i++) {
        data[i] = ref.data[i];
    }
    data[s] = ENDMARKER;
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

Pattern & Pattern::operator =(const Pattern other) { //(note: argument passed by value!)
    //delete old data
    if (data != NULL) {
        delete[] data;
        data = NULL;
    }
    
    //set new data
    const int s = other.bytesize();        
    data = new unsigned char[s+1];   
    for (int i = 0; i < s; i++) {
        data[i] = other.data[i];
    }  
    data[s] = ENDMARKER;

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

    
    for (int i = 0; i < (_n - n) + 1; i++) {
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


Pattern Pattern::addskips(std::vector<std::pair<int,int> > & gaps) const {
    //Returns a pattern with the specified spans replaced by fixed skips
    Pattern pattern = *this;
    for (vector<pair<int,int>>::iterator iter = gaps.begin(); iter != gaps.end(); iter++) {
        const Pattern replacement = Pattern(iter->second);
        pattern = pattern.replace(iter->first, iter->second, replacement);
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

IndexedCorpus::IndexedCorpus(std::istream *in){
    this->load(in);
}

IndexedCorpus::IndexedCorpus(std::string filename){
    this->load(filename);
}


void IndexedCorpus::load(std::istream *in) {
    int sentence = 0;
    while (!in->eof()) {
        Pattern line = Pattern(in);
        sentence++;
        if (in->eof()) break;
        int linesize = line.size();
        for (int i = 0; i < linesize; i++) {
            const Pattern unigram = line[i];
            const IndexReference ref = IndexReference(sentence,i);
            data[ref] = unigram;
        }
    }
}


void IndexedCorpus::load(std::string filename) {
    std::ifstream * in = new std::ifstream(filename.c_str());
    if (!in->good()) {
        std::cerr << "ERROR: Unable to load file " << filename << std::endl;
        throw InternalError();
    }
    this->load( (std::istream *) in);
    in->close();
    delete in;
}

Pattern IndexedCorpus::getpattern(IndexReference begin, int length) { 
    Pattern pattern;
    for (int i = 0; i < length; i++) {
        IndexReference ref = begin + i;
        iterator iter = data.find(ref);
        if (iter != data.end()) {
            const Pattern unigram = iter->second;
            pattern  = pattern + unigram;
        } else {
            std::cerr << "ERROR: Specified index does not exist"<< std::endl;
            throw InternalError();
        }
    }
    return pattern;
}

std::vector<IndexReference> IndexedCorpus::findmatches(const Pattern & pattern, int maxmatches) {
    //far more inefficient than a pattrn model obviously
    std::vector<IndexReference> result;
    const int _n = pattern.size();
    if (_n == 0) return result;

    IndexReference ref;
    int i = 0;
    Pattern matchunigram = pattern[i];
    for (iterator iter = data.begin(); iter != data.end(); iter++) {
        Pattern unigram = iter->second;
        if (matchunigram == unigram) {
            if (i ==0) ref = iter->first;
            i++;
            if (i == _n) {
                result.push_back(ref);
                if ((maxmatches != 0) && (result.size() == maxmatches)) break;
            }
            matchunigram = pattern[i];
        }
    }
    return result;
}

int IndexedCorpus::sentencelength(int sentence)  {
    IndexReference ref = IndexReference(sentence, 0);
    int length = 0;
    for (iterator iter = data.find(ref); iter != data.end(); iter++) {
        length++;
    }
    return length;
}
