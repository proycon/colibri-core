#ifndef CLASSENCODER_H
#define CLASSENCODER_H

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
     unsigned int minlength;
     unsigned int maxlength;
    public:
    ClassEncoder(const unsigned int minlength = 0, const unsigned int maxlength = 0);
    ClassEncoder(const std::string &, const unsigned int minlength = 0, const unsigned int maxlength = 0); //load an existing classer
    void load(const std::string &, const unsigned int minlength = 0, const unsigned int maxlength = 0); //load an existing classer
    void build(const std::string & filename); //build a class from this dataset
    void build(std::vector<std::string> & files, bool quiet=false); //build a class from this dataset
    
    //auxiliary functions called by build: first do processcorpus() for each
    //corpus, then call buildclasses() once when done:
    void buildclasses(std::unordered_map<std::string,int> & freqlist);
    void processcorpus(const std::string & filename, std::unordered_map<std::string,int> & freqlist);
    void processcorpus(std::istream * , std::unordered_map<std::string,int> & freqlist);
    #ifdef WITHFOLIA
    void processfoliacorpus(const std::string & filename, std::unordered_map<std::string,int> & freqlist);
    #endif

    std::unordered_map<unsigned int, std::string> added;
    

    int outputlength(const std::string & line);
    int encodestring(const std::string & line, unsigned char * outputbuffer, bool allowunknown, bool autoaddunknown=false);
    void encodefile(const std::string &, const std::string &, bool allowunknown, bool autoaddunknown=false, bool append=false, bool quiet=false);
    void encodefile(std::istream * IN, std::ostream * OUT, bool allowunknown, bool autoaddunknown, bool quiet=false);

    std::vector<unsigned int> encodeseq(const std::vector<std::string> & seq);
    
    Pattern buildpattern(const std::string, bool allowunknown=false, bool autoaddunknown = false);  //not thread-safe
    Pattern buildpattern_safe(const std::string, bool allowunknown=false, bool autoaddunknown = false);  //thread-safe

 
    void add(std::string, unsigned int cls);
    
    unsigned int gethighestclass() { return highestclass; }
    
    void save(const std::string & filename);
    
    int size() const {
        return classes.size();
    }
    
    unsigned int operator[](const std::string & key) {
         return classes[key];
    }

    typedef std::unordered_map<std::string, unsigned int>::const_iterator const_iterator;

    const_iterator begin() const {
        return classes.begin();
    }
    const_iterator end() const {
        return classes.end();
    }
};    

unsigned char * inttobytes(unsigned int, int & length);
int readline(std::istream* IN, unsigned char* buffer, const int);

const int countwords(const unsigned char* data, const int l);
#endif
