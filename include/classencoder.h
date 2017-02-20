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
#include <unordered_set>
#include <string>
#include <vector>
#include <fstream>
#include <pattern.h>
#include <common.h>

#ifdef WITHFOLIA
#include "libfolia/document.h"
#include "libfolia/folia.h"
#endif

/**
 * @file classencoder.h
 * \brief Class for encoding plain-text to binary class-encoded data.
 *
 * @author Maarten van Gompel (proycon) <proycon@anaproy.nl>
 *
 * @section LICENSE
 * Licensed under GPLv3
 *
 * @section DESCRIPTION
 * Class for encoding plain-text to binary class-encoded data
 */

/**
 * \brief Class for encoding plain-text to binary class-encoded data.
 * The ClassEncoder maintains a mapping of words to classes (integers). It allows a corpus
 * to be losslessly compressed by substituting words for classes. The classes
 * are distributed based on word frequency, with frequent words receiving a
 * lower class number that can be represented in fewer bytes, and rare words
 * receiving a higher class number.
 */
class ClassEncoder {
    private:
     std::unordered_map<std::string,unsigned int> classes;
     unsigned int highestclass;
     unsigned int minlength;
     unsigned int maxlength;
    public:
     static const unsigned char delimiterclass = 0;
     static const unsigned char boundaryclass = 1;
     static const unsigned char unknownclass = 2;
     static const unsigned char skipclass = 3;
     static const unsigned char flexclass = 4;
    /**
     * Constructor for an empty ClassEncoder
     * @param minlength Minimum supported length of words (default: 0)
     * @param maxlength Maximum supported length of words (default: 0 = unlimited)
     */
    ClassEncoder(const unsigned int minlength = 0, const unsigned int maxlength = 0);

    /**
     * Constructor for a ClassEncoder read from file
     * @param filename The filename  (*.colibri.cls)
     * @param minlength Minimum supported length of words (default: 0)
     * @param maxlength Maximum supported length of words (default: 0 = unlimited)
     */
    ClassEncoder(const std::string & filename, const unsigned int minlength = 0, const unsigned int maxlength = 0); //load an existing classer

    /**
     * Load a class encoding from file
     * @param filename The filename  (*.colibri.cls)
     * @param minlength Minimum supported length of words (default: 0)
     * @param maxlength Maximum supported length of words (default: 0 = unlimited)
     */
    void load(const std::string & filename, const unsigned int minlength = 0, const unsigned int maxlength = 0); //load an existing classer


    /**
     * Build a class encoding from a plain-text corpus
     * @param filename A plain text corpus with the units of interest (e.g sentences) each on one line
     * @param threshold Occurrence threshold, words occurring less will be pruned
     * @param vocabfile Plain text vocabulary file with one line per word,
     * constrains the classes to these words only
     */
    void build(const std::string & filename, unsigned int threshold=0, const std::string vocabfile = "");

    /**
     * Build a class encoding from multiple plain-text corpus files
     * @param files A list of plain text corpus files with the units of interest (e.g sentences) each on one line
     * @param quiet If true, do not output progress to stderr (default: false)
     * @param threshold Occurrence threshold, words occurring less will be pruned
     * @param vocabfile Plain text vocabulary file with one line per word,
     * constrains the classes to these words only
     */
    void build(std::vector<std::string> & files, bool quiet=false, unsigned int threshold =0, const std::string vocabfile = "");

    //auxiliary functions called by build: first do processcorpus() for each
    //corpus, then call buildclasses() once when done:
    //

    /**
     * Assign classes based on the computed frequency list. This method should
     * only be called once.
     * @param freqlist The data structure that will contain the frequency list
     * @param threshold Occurrence threshold, words occurring less will be pruned
     */
    void buildclasses(const std::unordered_map<std::string,unsigned int> & freqlist, unsigned int threshold =0);

    /**
     * Build classes from a pre-supplied frequency list (per line one word , a
     * tab, and an occurrence count)
     * @param filename The filename
     */
    void buildclasses_freqlist(const std::string & filename, unsigned int threshold = 0);

    void loadvocab(const std::string & filename, std::unordered_set<std::string> & vocab);
    /**
     * Count word frequency in a given plain-text corpus.
     * @param filename The corpus file
     * @param freqlist The resulting frequency list, should be shared between multiple calls to processcorpus()
     */
    void processcorpus(const std::string & filename, std::unordered_map<std::string,unsigned int> & freqlist, std::unordered_set<std::string> * vocab = NULL);
    /**
     * Count word frequency in a given plain-text corpus.
     * @param in The input stream
     * @param freqlist The resulting frequency list, should be shared between multiple calls to processcorpus()
     */
    void processcorpus(std::istream * in, std::unordered_map<std::string,unsigned int> & freqlist, std::unordered_set<std::string> * vocab = NULL);
    #ifdef WITHFOLIA
    /**
     * Count word frequency in a given FoLiA corpus.
     * @param filename The corpus file (FoLiA XML)
     * @param freqlist The resulting frequency list, should be shared between multiple calls to processcorpus()
     */
    void processfoliacorpus(const std::string & filename, std::unordered_map<std::string,unsigned int> & freqlist, std::unordered_set<std::string> * vocab = NULL);
    #endif

    std::unordered_map<unsigned int, std::string> added;


    /**
     * Computes how many bytes the class repesentation for this input line would take
     */
    int outputlength(const std::string & line);

    /**
     * Low-level function to encode a string of words as a binary representation of classes
     * @param line The string you want to turn into a Pattern
     * @param outputbuffer Pointer to the output buffer, must be pre-allocated and have enough space
     * @param allowunknown If the string contains unknown words, represent those using a single unknown class. If set to false, an exception will be raised when unknown words are present. (default: false)
     * @param autoaddunknown If the string contains unknown words, automatically add these words to the class encoding. Note that the class encoding will no longer be optimal if this is used. (default: false)
     * @param nroftokens A pointer to a variable that contains the number of tokens outputted, will be written by this function if not NULL. (default: NULL)
     * @return The number of bytes written to outputbuffer
     */
    int encodestring(const std::string & line, unsigned char * outputbuffer, bool allowunknown, bool autoaddunknown=false, unsigned int * nroftokens = NULL);

    /**
     * Create a class-encoded corpus file from a plain-text corpus file. Each of the units of interest (e.g sentences) should occupy a single line (i.e., \n delimited)
     * @param inputfilename Filename of the input file, a plain-text corpus file
     * @param outputfilename Filename of the output file (binary class-encoded corpus file, *.colibri.dat)
     * @param allowunknown If the string contains unknown words, represent those using a single unknown class. If set to false, an exception will be raised when unknown words are present. (default: false)
     * @param autoaddunknown If the string contains unknown words, automatically add these words to the class encoding. Note that the class encoding will no longer be optimal if this is used. (default: false)
     * @param ignorenewlines Set to true to ignore newlines and have all text as one blob (may still result in several blobs if the text is really long)
     * @param append Set to true if this is not the first file to write to the stream
     * @return The number of bytes written to outputbuffer
     */
    void encodefile(const std::string & inputfilename, const std::string & outputfilename, bool allowunknown, bool autoaddunknown=false,  bool append=false, bool ignorenewlines=false, bool quiet=false);
    /**
     * Create a class-encoded corpus file from a plain-text corpus file. Each of the units of interest (e.g sentences) should occupy a single line (i.e., \n delimited)
     * @param IN Input stream of a plain-text corpus file
     * @param OUT Output stream of a binary class-encoded corpus file (*.colibri.dat)
     * @param allowunknown If the string contains unknown words, represent those using a single unknown class. If set to false, an exception will be raised when unknown words are present. (default: false)
     * @param autoaddunknown If the string contains unknown words, automatically add these words to the class encoding. Note that the class encoding will no longer be optimal if this is used. (default: false)
     * @param quiet Set to true to suppress any output
     * @param ignorenewlines Set to true to ignore newlines and have all text as one blob (may still result in several blobs if the text is really long)
     * @param append Set to true if this is not the first file to write to the stream
     * @return The number of bytes written to outputbuffer
     */
    void encodefile(std::istream * IN, std::ostream * OUT, bool allowunknown, bool autoaddunknown, bool quiet=false, bool append=false, bool ignorenewlines=false);

    std::vector<unsigned int> encodeseq(const std::vector<std::string> & seq);

    /**
     * Build a pattern from a string.
     * **Note:** This function is not thread-safe! Use buildpattern_safe() instead if you need thread safety!
     * @param patternstring The string you want to turn into a Pattern
     * @param allowunknown If the string contains unknown words, represent those using a single unknown class. If set to false, an exception will be raised when unknown words are present. (default: false)
     * @param autoaddunknown If the string contains unknown words, automatically add these words to the class encoding. Note that the class encoding will no longer be optimal if this is used. (default: false)
     * @return a Pattern
     */
    Pattern buildpattern(const std::string & patternstring, bool allowunknown=false, bool autoaddunknown = false);  //not thread-safe
    /**
     * Build a pattern from a string (thread-safe variant, slightly slower due to buffer allocation)
     * @param patternstring The string you want to turn into a Pattern
     * @param allowunknown If the string contains unknown words, represent those using a single unknown class. If set to false, an exception will be raised when unknown words are present. (default: false)
     * @param autoaddunknown If the string contains unknown words, automatically add these words to the class encoding. Note that the class encoding will no longer be optimal if this is used. (default: false)
     * @return a Pattern
     */
    Pattern buildpattern_safe(const std::string &  patternstring, bool allowunknown=false, bool autoaddunknown = false);  //thread-safe


    /**
     * Add the word with the specified class to the class encoding
     */
    void add(const std::string &, const unsigned int cls);

    /**
     * Returns the highest assigned class in the class encoding
     */
    unsigned int gethighestclass() { return highestclass; }

    /**
     * Save the class encoding to file
     */
    void save(const std::string & filename);

    /**
     * Returns the number of classes, i.e. word types
     */
    int size() const {
        return classes.size();
    }

    /**
     * Return the class for the given word
     */
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

unsigned int  inttobytes(unsigned char * buffer, unsigned int cls);
unsigned char * inttobytes_v1(unsigned int, int & length);
int readline(std::istream* IN, unsigned char* buffer, const int);

unsigned char * convert_v1_v2(const unsigned char * olddata, unsigned int & newlength);
unsigned char * convert_v1_v2(std::istream * in, bool ignoreeol, bool debug);

int countwords(const unsigned char* data, const int l);
#endif
