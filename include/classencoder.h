#ifndef CLASSENCODER_H
#define CLASSENCODER_H

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <pattern.h>
#include <common.h>

#ifdef WITHFOLIA
#include "libfolia/document.h"
#include "libfolia/folia.h"
#endif

class ClassEncoder {
    private:
     std::unordered_map<std::string,unsigned int> classes;
     unsigned int unknownclass;
     unsigned int bosclass;
     unsigned int eosclass;
     unsigned int highestclass;
     void buildclasses(std::unordered_map<std::string,int> freqlist);
     void processcorpus(const std::string & filename, std::unordered_map<std::string,int> & freqlist);
     #ifdef WITHFOLIA
     void processfoliacorpus(const std::string & filename, std::unordered_map<std::string,int> & freqlist);
     #endif
    public:
    ClassEncoder();
    ClassEncoder(const std::string &); //load an existing classer
    void build(const std::string & filename); //build a class from this dataset
    void build(std::vector<std::string> & files); //build a class from this dataset
    
    std::unordered_map<unsigned int, std::string> added;
    

    int encodestring(const std::string & line, unsigned char * outputbuffer, bool allowunknown, bool autoaddunknown=false);
    void encodefile(const std::string &, const std::string &, bool allowunknown, bool autoaddunknown=false, bool append=false);
    std::vector<unsigned int> encodeseq(const std::vector<std::string> & seq);

    Pattern input2pattern(const std::string &, bool allowunknown, bool autoaddunknown = false); 
 
    void add(std::string, unsigned int cls);
    
    unsigned int gethighestclass() { return highestclass; }
    
    void save(const std::string & filename);
    
    int size() const {
        return classes.size();
    }
    
    unsigned int operator[](const std::string & key) {
         return classes[key];
    }
};    

unsigned char * inttobytes(unsigned int, int & length);
int readline(std::istream* IN, unsigned char* buffer, const int);

const int countwords(const unsigned char* data, const int l);
#endif
