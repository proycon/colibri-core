#ifndef CLASSDECODER_H
#define CLASSDECODER_H
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

#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>

class ClassDecoder {
    private:
     std::unordered_map<unsigned int,std::string> classes;
     unsigned int unknownclass;
     unsigned int bosclass;
     unsigned int eosclass;
     unsigned int highestclass;
    public:
    
    ClassDecoder();
    ClassDecoder(const std::string & filename);
    void load(const std::string & filename);
    
    std::vector<std::string> decodeseq(const std::vector<int> & seq);
    
    void decodefile(const std::string & filename, std::ostream*,  unsigned int start = 0, unsigned int end = 0, bool quiet=false);
    std::string decodefiletostring(const std::string & filename,  unsigned int start = 0, unsigned int end = 0, bool quiet=true);
    
    int size() const {
        return classes.size();
    }
    
    std::string operator[](unsigned int key) const {
         std::unordered_map<unsigned int, std::string>::const_iterator it = classes.find(key);
         if (it != classes.end()) {
             return it->second;
         } else {
             return "{UNKNOWN}";
         }
    }
    
    void add( unsigned int, std::string); 
    unsigned int gethighestclass() { return highestclass; }
    bool hasclass(unsigned int key) const { return (classes.count(key) > 0); }
    
    unsigned int newclass(); 
    
    void prune(unsigned int threshold);    


    typedef std::unordered_map<unsigned int, std::string>::const_iterator const_iterator;

    const_iterator begin() const {
        return classes.begin();
    }
    const_iterator end() const {
        return classes.end();
    }

};

unsigned int bytestoint(const unsigned char* a, const int l);
int readline(std::istream* IN, unsigned char* buffer, const int);

const int countwords(const unsigned char* data, const int l);
std::pair<int,int> getwords(const unsigned char* data, const int datasize, const int n, const int begin = 0);
#endif
