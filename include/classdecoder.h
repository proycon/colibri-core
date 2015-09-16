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

/**
 * @file classdecoder.h
 * \brief Class for decoding binary class-encoded data back to plain-text.
 *
 * @author Maarten van Gompel (proycon) <proycon@anaproy.nl>
 * 
 * @section LICENSE
 * Licensed under GPLv3
 *
 * @section DESCRIPTION
 * Class for decoding binary class-encoded data back to plain-text
 *
 */

/**
 * \brief Class for decoding binary class-encoded data back to plain-text.
 * The ClassDecoder maintains a mapping of classes (integers) to words. It allows decoding of a corpus
 * that was losslessly compressed by substituting words for classes. The classes
 * are distributed based on word frequency, with frequent words receiving a
 * lower class number that can be represented in fewer bytes, and rare words
 * receiving a higher class number.
 */
class ClassDecoder {
    private:
     std::unordered_map<unsigned int,std::string> classes;
     unsigned int unknownclass;
     unsigned int bosclass;
     unsigned int eosclass;
     unsigned int highestclass;
    public:
    
    /**
     * Constructor for an empty class decoder 
     */
    ClassDecoder();

    /**
     * Constructor for a class decoder loading a class encoding from file
     */
    ClassDecoder(const std::string & filename);

    /**
     * Load a class encoding from file
     */
    void load(const std::string & filename);
    
    std::vector<std::string> decodeseq(const std::vector<int> & seq);
    
    /**
     * Create a plain-text corpus file from a class-encoded corpus file (*.colibri.dat)
     * @param inputfilename Filename of the input file, a plain-text corpus file
     * @param out Output stream for the plain-text corpus data, units (e.g sentences) are delimited with newlines
     * @param start Start decoding at the specified line (corresponds to sentences or whatever other unit the data employs)
     * @param end End decoding at the specified line (this line will be included) (corresponds to sentences or whatever other unit the data employs)
     * @param quiet Do not report decoding problems to stderr
     */
    void decodefile(const std::string & filename, std::ostream*,  unsigned int start = 0, unsigned int end = 0, bool quiet=false);

    /**
     * Create a plain-text corpus file from a class-encoded corpus file (*.colibri.dat)
     * @param inputfilename Filename of the input file, a plain-text corpus file
     * @param start Start decoding at the specified line (corresponds to sentences or whatever other unit the data employs)
     * @param end End decoding at the specified line (this line will be included) (corresponds to sentences or whatever other unit the data employs)
     * @param quiet Do not report decoding problems to stderr
     * @return A string with the plain-text corpus data, units (e.g sentences) are delimited with newlines
     */
    std::string decodefiletostring(const std::string & filename,  unsigned int start = 0, unsigned int end = 0, bool quiet=true);
    
    /**
     * Return the number of classes, i.e. word types, in the class encoding
     */
    int size() const {
        return classes.size();
    }
    
    /**
     * Return the word pertaining to the given class. Unknown classes will be
     * decoded as {UNKNOWN}.
     */
    std::string operator[](unsigned int key) const {
         std::unordered_map<unsigned int, std::string>::const_iterator it = classes.find(key);
         if (it != classes.end()) {
             return it->second;
         } else {
             return "{UNKNOWN}";
         }
    }
    
    /**
     * Add the class with the given word string to the class encoding
     */
    void add( unsigned int, std::string); 

    /**
     * Return the highest class in the class encoding
     */
    unsigned int gethighestclass() { return highestclass; }

    /**
     * Test if the specified class exists in this class encoding
     */
    bool hasclass(unsigned int key) const { return (classes.count(key) > 0); }
    
    /**
     * Return a new class, not yet assigned
     */
    unsigned int newclass(); 
    
    /**
     * Retain only the specified number of most frequent classes, prune the remainder
     */
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
