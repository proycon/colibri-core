#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <unistd.h>
#include <classencoder.cpp>
#include <classdecoder.cpp>
#include <pattern.h>
#include <patternmodel.h>
#include <alignmodel.h>
#include <ctime>


using namespace std;

void test(bool r) {
    if (r) {
        cerr << string( ".... \33[1;32mok\33[m") << endl;
    } else {
        cerr << ".... \33[1;31mFAILED!\33[m" << endl;
        exit(2);
    }
}

void test(unsigned int value , unsigned int ref) {
    if (value != ref) {
        cerr << " " << value << " .... \33[1;31mFAILED!\33[m expected " << ref << endl;
        exit(2);
    } else {
        cerr << " " << value << " .... \33[1;32mok\33[m" << endl;
    }
}

void testgt(unsigned int value , unsigned int ref) {
    if (value <= ref) {
        cerr << " " << value << " .... \33[1;31mFAILED!\33[m expected " << ref << endl;
        exit(2);
    } else {
        cerr << " " <<value << " .... \33[1;32mok\33[m" << endl;
    }
}
void test(string value , string ref) {
    if (value != ref) {
        cerr << " " << value << " .... \33[1;31mFAILED!\33[m expected " << ref << endl;
        exit(2);
    } else {
        cerr << " " << value << " .... \33[1;32mok\33[m" << endl;
    }
}

int main( int argc, char *argv[] ) {


	//string model = argv[1];
	//string classfile = argv[1];
    int i;
    bool verbose = false;

    if (argc == 2) {
		string option = argv[1];
		if (option == "-v") verbose = true;
	}

    const char * poem =
    "To be or not to be , that is the question ;\n"
    "Whether 'tis nobler in the mind to suffer\n"
    "The slings and arrows of outrageous fortune ,\n"
    "Or to take arms against a sea of troubles ,\n"
    "And by opposing , end them . To die , to sleep ;\n"
    "No more ; and by a sleep to say we end\n"
    "The heart-ache and the thousand natural shocks\n"
    "That flesh is heir to — 'tis a consummation\n"
    "Devoutly to be wish'd . To die , to sleep ;\n"
    "To sleep , perchance to dream . Ay , there's the rub ,\n"
    "For in that sleep of death what dreams may come ,\n"
    "When we have shuffled off this mortal coil ,\n"
    "Must give us pause . There's the respect\n"
    "That makes calamity of so long life ,\n"
    "For who would bear the whips and scorns of time,\n"
    "Th'oppressor's wrong , the proud man 's contumely ,\n"
    "The pangs of despised love , the law 's delay ,\n"
    "The insolence of office , and the spurns\n"
    "That patient merit of th' unworthy takes ,\n"
    "When he himself might his quietus make\n"
    "With a bare bodkin ? who would fardels bear ,\n"
    "To grunt and sweat under a weary life ,\n"
    "But that the dread of something after death ,\n"
    "The undiscovered country from whose bourn\n"
    "No traveller returns , puzzles the will ,\n"
    "And makes us rather bear those ills we have\n"
    "Than fly to others that we know not of ?\n"
    "Thus conscience does make cowards of us all ,\n"
    "And thus the native hue of resolution\n"
    "Is sicklied o'er with the pale cast of thought ,\n"
    "And enterprises of great pitch and moment\n"
    "With this regard their currents turn awry ,\n"
    "And lose the name of action .\n"
    "Soft you now ! The fair Ophelia ! Nymph ,\n"
    "in thy orisons be all my sins remember'd .\n"
    "To flee or not to flee .\n" //additions to test skipgrams
    "To flee or not to flee .\n"
    "To see or not to see .\n"
    "To see or not to see .\n"
    "To pee or not to pee .\n"; //See that Shakespeare? I could be a poet too!

    {
       /*
        {
            //DEBUG ZONE

            string corpusfile = "tmppage.colibri.dat";
            string classfile = "tmppage.colibri.cls";

            ClassDecoder classdecoder = ClassDecoder(classfile);
            IndexedCorpus corpusdata = IndexedCorpus(corpusfile);

            cerr << "Iterating over sentences " << corpusfile << endl;
            int sentencecount = corpusdata.sentences();
            cerr << "Number of sentences: "  <<  sentencecount << endl;

            for (int i = 1; i <= sentencecount; i++) {
                Pattern sentence = corpusdata.getsentence(i);
                cerr << "SENTENCE #" << i << ": " << sentence.tostring(classdecoder) << endl;
                for (int n = 1; n <= 5; n++) {
                    vector<Pattern> container;
                    sentence.ngrams(container,n);
                }
            }


            exit(0);

        }
        */


        chdir("/tmp");

        string classfile = "/tmp/colibritest";
        ofstream f;
        f.open(classfile.c_str(), ios::out);
        f << "5\tbe\n6\tTo\n7\tto\n8\tor\n9\tnot\n73477272\tblah\n131\ttest\n";
        f.close();



        cerr << "Loading class decoder" << endl;
        ClassDecoder classdecoder(classfile);
        cerr << "  Number of classes: "; test(classdecoder.size(),11); //including reserved classes

        cerr << "Loading class encoder" << endl;
        ClassEncoder encoder(classfile);

        cerr << endl << "************************** Ngram tests  ***************************************" << endl << endl;

        cerr << "Encoding n-gram from string input" << endl;
        string querystring = "To be or not to be";
        Pattern ngram = encoder.buildpattern(querystring, true);
        cerr << "(Pattern built)" << endl;


        cerr << "Ngram #1:" << endl;
        cerr << "Bytesize: "; test(ngram.bytesize(),6);
        cerr << "Size (n): "; test(ngram.n(), 6);
        cerr << "Category==ngram: "; test(ngram.category(), NGRAM);
        cerr << "Hash: "; testgt(ngram.hash(),0);
        cerr << "Raw: " << endl;
        ngram.out();
        cerr << "Decoded: "; test(ngram.decode(classdecoder),"To be or not to be");
		cerr << endl;



        cerr << "Ngram with high classes:" << endl;
        querystring = "blah or blah";
        Pattern ngramhigh = encoder.buildpattern(querystring, true);
        cerr << "Bytesize: "; test(ngramhigh.bytesize(),9);
        cerr << "Size (n): "; test(ngramhigh.n(), 3);
        cerr << "Category==ngram: "; test(ngramhigh.category(), NGRAM);
        cerr << "Hash: "; testgt(ngramhigh.hash(),0);
        cerr << "Raw: " << endl;
        ngramhigh.out();
        cerr << "Decoded: "; test(ngramhigh.decode(classdecoder),"blah or blah");

        cerr << "Ngram with high classes (2):" << endl;
        querystring = "to test or not to test";
        Pattern ngramhigh2 = encoder.buildpattern(querystring, true);
        cerr << "Bytesize: "; test(ngramhigh2.bytesize(),8);
        cerr << "Size (n): "; test(ngramhigh2.n(), 6);
        cerr << "Category==ngram: "; test(ngramhigh2.category(), NGRAM);
        cerr << "Hash: "; testgt(ngramhigh2.hash(),0);
        cerr << "Raw: " << endl;
        ngramhigh2.out();
        cerr << "Decoded: "; test(ngramhigh2.decode(classdecoder),"to test or not to test");

        cerr << "Slice constructor ngram #1, specific subngram (low)" << endl;
        Pattern ngram2 = Pattern(ngram, 2, 2);

        cerr << "Ngram: "; test( ngram2.decode(classdecoder),"or not");
        cerr << "N: "; test(ngram2.n(),2);
        cerr << "Bytesize: ", test(ngram2.bytesize(),2);
        cerr << "Raw: " << endl;
        ngram2.out();
        cerr << endl;

        cerr << "Slice constructor, specific subngram (high)" << endl;
        Pattern ngramhighslice = Pattern(ngramhigh, 1, 2);
        cerr << "Ngram: "; test( ngramhighslice.decode(classdecoder),"or blah");
        cerr << "N: "; test(ngramhighslice.n(),2);
        cerr << "Raw: " << endl;
        ngramhighslice.out();
        cerr << endl;

        cerr << "Slice constructor (2), specific subngram (high)" << endl;
        Pattern ngramhighslice2 = Pattern(ngramhigh2, 0, 2);
        cerr << "Ngram: "; test( ngramhighslice2.decode(classdecoder),"to test");
        cerr << "N: "; test(ngramhighslice2.n(),2);
        cerr << "Bytesize: "; test(ngramhighslice2.bytesize(),3);
        cerr << "Raw: " << endl;
        ngramhighslice2.out();
        cerr << endl;

        {
            ClassDecoder classdecoder2 = ClassDecoder();

            cerr << "Decoding non-existing classes (should return {?}):"; test(ngram.tostring(classdecoder2),"{?} {?} {?} {?} {?} {?}");

        }
        cerr << "----------------------------------------------------" << endl;
        cerr << "Copy constructor" << endl;
        Pattern ngramcopy(ngram);
		cerr << "Testing equality "; test(ngram == ngramcopy);
        cerr << endl;



        cerr << "Empty/null pattern" << endl;
        Pattern emptypattern;
        cerr << "N: "; test(emptypattern.n(),0);
        cerr << "Bytesize: "; test(emptypattern.bytesize(),0);


        cerr << "----------------------------------------------------" << endl;
        cerr << "Tokens of ngram #1: " << endl;


        vector<Pattern> tokens;
        ngram.ngrams(tokens,1);
        cerr << "Testing correct size "; test(tokens.size() == 6);
        i = 0;
        vector<string> tokenref = {"To","be","or","not","to","be"};
        for (vector<Pattern>::iterator iter2 = tokens.begin(); iter2 != tokens.end(); iter2++) {
            const Pattern subngram = *iter2;
            cerr << "#" << i << " -- "; test(subngram.decode(classdecoder), tokenref[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,6);

        cerr << "----------------------------------------------------" << endl;
        cerr << "Tokens of ngram \"to test or not to test\": " << endl;

        tokens.clear();
        ngramhigh2.ngrams(tokens,1);
        cerr << "Testing correct size "; test(tokens.size() == 6);
        i = 0;
        vector<string> tokenref2 = {"to","test","or","not","to","test"};
        for (vector<Pattern>::iterator iter2 = tokens.begin(); iter2 != tokens.end(); iter2++) {
            const Pattern subngram = *iter2;
            cerr << "#" << i << " -- "; test(subngram.decode(classdecoder), tokenref2[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,6);

        cerr << "----------------------------------------------------" << endl;
        cerr << "Subgrams of ngram #1: " << endl;


        vector<Pattern> subngrams;
        ngram.subngrams(subngrams);
        i = 0;
        vector <string> subngramref = {
            "To",
			"be",
			"or",
			"not",
			"to",
			"be",
			"To be",
			"be or",
			"or not",
			"not to",
			"to be",
			"To be or",
			"be or not",
			"or not to",
			"not to be",
			"To be or not",
			"be or not to",
			"or not to be",
			"To be or not to",
			"be or not to be",
			"To be or not to be"};
        for (vector<Pattern>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {
            const Pattern subngram = *iter2;
            cerr << "#" << i << " -- "; test(subngram.decode(classdecoder), subngramref[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,21);


        string substring = "or not";
        Pattern subngram = encoder.buildpattern(substring, true);
        cerr << "Testing occurrence of substring 'or not'? "; test(ngram.contains(subngram) == 1);

        string substring2 = "to not";
        Pattern subngram2 = encoder.buildpattern(substring2, true);
        cerr << "Testing non-occurrence of substring 'to not'? "; test(ngram.contains(subngram2) == 0);

        string substring3 = "to be";
        Pattern subngram3 = encoder.buildpattern(substring3, true);
        cerr << "Testing occurrence of substring 'to be'? "; test(ngram.contains(subngram3) == 1);

        string substring4 = "or";
        Pattern subngram4 = encoder.buildpattern(substring4, true);
        cerr << "Testing occurrence of substring 'or'? "; test(ngram.contains(subngram4) == 1);


        string substring5 = "to";
        Pattern subngram5 = encoder.buildpattern(substring5, true);

        cerr << "----------------------------------------------------" << endl;
        cerr << "aeverse of ngram #1: " << endl;

        Pattern revngram = ngram.reverse();
        cerr << "Reverse ngram: "; test(revngram.decode(classdecoder), "be to not or be To");
        cerr << "N: "; test(revngram.n(),6);

        cerr << "----------------------------------------------------" << endl;
        cerr << "Reverse of ngram with high bytes: " << endl;

        string histring = "to blah blah or not";
        Pattern hingram = encoder.buildpattern(histring);
        Pattern revhingram = hingram.reverse();
        cerr << "Reverse ngram: "; test(revhingram.decode(classdecoder), "not or blah blah to");
        cerr << "N: "; test(revhingram.n(),5);


        cerr << "----------------------------------------------------" << endl;
        cerr << "Pattern Pointer tests" << endl;
        PatternPointer pngram(&ngram);
        cerr << "Testing equivalence between pointer and pattern"; test(ngram == pngram);
        cerr << "Testing equivalence between pointer and pattern (rev)"; test(pngram == ngram);
        cerr << "Testing hash equivalence between pointer and pattern"; test(ngram.hash() == pngram.hash());
        Pattern derefngram(pngram);
        cerr << "Testing equivalence after pointer construction and dereference"; test(ngram == derefngram);
        cerr << "Testing equivalence after pointer construction and dereference (rev)"; test(derefngram == ngram);
        cerr << "Testing equivalence between pointer from pattern and pointer"; test(PatternPointer(ngram) == pngram);
        cerr << "Testing equivalence between pointer from pattern and pointer (rev)"; test(pngram == PatternPointer(ngram));

        cerr << "Bytesize: "; test(pngram.bytesize(),6);
        cerr << "Size (n): "; test(pngram.n(), 6);
        cerr << "Category==ngram: "; test(pngram.category(), NGRAM);
        cerr << "Raw: " << endl;
        ngram.out();
        cerr << "Decoded: "; test(pngram.decode(classdecoder),"To be or not to be");

        cerr << "Slice constructor, specific subngram (low only)" << endl;
        PatternPointer pngramslice = PatternPointer(pngram, 2, 2);
        cerr << "Ngram: "; test( pngramslice.decode(classdecoder),"or not");
        cerr << "N: "; test(pngramslice.n(),2);
        cerr << "Bytesize: ", test(pngramslice.bytesize(),2);
        cerr << "Raw: " << endl;
        pngram.out();
        cerr << endl;


        cerr << "Ngram (pointer) with high classes:" << endl;
        PatternPointer pngramhigh = PatternPointer(ngramhigh); //blah or blah
        cerr << "Bytesize: "; test(pngramhigh.bytesize(),9);
        cerr << "Size (n): "; test(pngramhigh.n(), 3);
        cerr << "Category==ngram: "; test(pngramhigh.category(), NGRAM);
        cerr << "Hash: "; testgt(pngramhigh.hash(),0);
        cerr << "Raw: " << endl;
        pngramhigh.out();
        cerr << "Decoded: "; test(pngramhigh.decode(classdecoder),"blah or blah");


        cerr << "Slice constructor, specific subngram (high)" << endl;
        Pattern pngramhighslice = PatternPointer(pngramhigh, 1, 2);
        cerr << "Ngram: "; test( pngramhighslice.decode(classdecoder),"or blah");
        cerr << "N: "; test(pngramhighslice.n(),2);
        cerr << "Raw: " << endl;
        pngramhighslice.out();
        cerr << endl;

        cerr << "Ngram (pointer) with high classes (2):" << endl;
        Pattern pngramhigh2 = encoder.buildpattern(querystring, true);
        cerr << "Bytesize: "; test(pngramhigh2.bytesize(),8);
        cerr << "Size (n): "; test(pngramhigh2.n(), 6);
        cerr << "Category==ngram: "; test(pngramhigh2.category(), NGRAM);
        cerr << "Hash: "; testgt(pngramhigh2.hash(),0);
        cerr << "Raw: " << endl;
        pngramhigh2.out();
        cerr << "Decoded: "; test(pngramhigh2.decode(classdecoder),"to test or not to test");

        cerr << "Slice constructor, specific subngram (high 2)" << endl;
        PatternPointer pngramhighslice2 = PatternPointer(pngramhigh2, 0, 2);
        cerr << "Ngram: "; test( pngramhighslice2.decode(classdecoder),"to test");
        cerr << "N: "; test(pngramhighslice2.n(),2);
        cerr << "Bytesize: "; test(pngramhighslice2.bytesize(),3);
        cerr << "Raw: " << endl;
        pngramhighslice2.out();
        cerr << endl;

        cerr << "Slice constructor, specific subngram (high 3)" << endl;
        Pattern ngramhighslice3 = Pattern(ngramhigh2, 0, 2);
        cerr << "Ngram: "; test( ngramhighslice3.decode(classdecoder),"to test");
        cerr << "N: "; test(ngramhighslice3.n(),2);
        cerr << "Bytesize: "; test(ngramhighslice3.bytesize(),3);
        cerr << "Raw: " << endl;
        ngramhighslice3.out();
        cerr << endl;


        cerr << "Tokens of ngram #1 (as patternpointers): " << endl;
        vector<PatternPointer> ptokens;
        ngram.ngrams(ptokens,1);
        cerr << "Testing correct size "; test(ptokens.size() == 6);
		i = 0;
        for (vector<PatternPointer>::iterator iter2 = ptokens.begin(); iter2 != ptokens.end(); iter2++) {
            const PatternPointer subngram = *iter2;
            cerr << "#" << i << " -- "; test(subngram.decode(classdecoder), tokenref[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,6);

        cerr << "Subgrams of ngram #1 (as patternpointers, from patternpointer): " << endl;
        vector<PatternPointer> psubngrams;
        pngram.subngrams(psubngrams);
		i = 0;
        for (vector<PatternPointer>::iterator iter2 = psubngrams.begin(); iter2 != psubngrams.end(); iter2++) {
            const PatternPointer psubngram = *iter2;
            cerr << "#" << i << " -- "; test(psubngram.decode(classdecoder), subngramref[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,21);



        cerr << "Subgrams of ngram #1 (as patternpointers, from pattern): " << endl;
        vector<PatternPointer> psubngrams2;
        ngram.subngrams(psubngrams2);
		i = 0;
        for (vector<PatternPointer>::iterator iter2 = psubngrams2.begin(); iter2 != psubngrams2.end(); iter2++) {
            const PatternPointer psubngram2 = *iter2;
            cerr << "#" << i << " -- "; test(psubngram2.decode(classdecoder), subngramref[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,21);


        cerr << "----------------------------------------------------" << endl;
        cerr << "Ngram addition: " << endl;
        const Pattern ngrambegin = Pattern(ngram,0,2);
        Pattern ngramconc = ngrambegin;
        ngramconc = ngramconc + subngram;
        ngramconc = ngramconc + subngram3;
        cerr << "Ngram: "; test(ngramconc.decode(classdecoder),"To be or not to be");
        cerr << "N: "; test(ngramconc.n(),6);
        cerr << "Bytesize: "; test(ngramconc.bytesize());
        cerr << "Raw: " << endl;
        ngramconc.out();
        cerr << "----------------------------------------------------" << endl;
        emptypattern = emptypattern + ngramconc;
        cerr << "Adding to empty? "; test(emptypattern == ngramconc);
        cerr << "----------------------------------------------------" << endl;
        cerr << "Ngram comparison: " << endl;
        cerr << "Equality? " ; test(ngramconc == ngram);
        cerr << "Non-equality? " ; test(ngramconc != ngrambegin);
        cerr << "greater than? " ; test(subngram2 > subngram3) ;
        cerr << "greater than? " ; test(subngram4 > subngram3) ;
        cerr << "less than? " ; test(subngram3 < subngram2);
        cerr << "less than? " ; test(subngram5 < subngram3);
        cerr << "Begin is less? " ; test(ngrambegin < ngramconc);
        cerr << "equivalence under less? " ; test(!(subngram < subngram));

        cerr << "----------------------------------------------------" << endl;

        cerr << "Testing handling of unknown tokens" << endl;

        string unkstring = "to sdfdsf ksd32 or not";

        cerr << "Raise exception when allowunknown=false: ";
        bool caught = false;
        try {
            Pattern unkgram = encoder.buildpattern(unkstring);
        } catch (const UnknownTokenError &e) {
            caught = true;
        }
        test(caught,true);

        Pattern unkgram2 = encoder.buildpattern(unkstring, true);
        cerr << "Encode as unknown when allowunknown=true"; test(unkgram2.decode(classdecoder),"to {?} {?} or not");

        cerr << endl << "************************** Skipgram tests  ***************************************" << endl << endl;

        cerr << "Encoding skip-gram from string input" << endl;
        string querystring2 = "To {*1*} or {*1*} to be";

        Pattern skipgram = encoder.buildpattern(querystring2, true);

        cerr << "Skipgram decoded: "; test(skipgram.decode(classdecoder),"To {*} or {*} to be");
        cerr << "N: "; test(skipgram.n(),6);
        cerr << "Bytesize: "; test(skipgram.bytesize(),6);
        cerr << "Category==skipgram? " ; test(skipgram.category() == SKIPGRAM) ;
        cerr << "Skipcount==2? " ; test(skipgram.skipcount(), 2) ;

        cerr << "Parts: " << endl;
        vector<Pattern> parts;
        skipgram.parts(parts);
		vector<string> partsref = {"To","or","to be"};
		i = 0;
        for (vector<Pattern>::iterator iter2 = parts.begin(); iter2 != parts.end(); iter2++) {
            const Pattern part = *iter2;
            cerr << "#" << i << " -- "; test(part.decode(classdecoder), partsref[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,3);


        cerr << "Gaps: " << endl;
        std::vector<std::pair<int,int> > gapcontainer;
        skipgram.gaps(gapcontainer);
        skipgram.parts(parts);
		vector<int> gaprefbegin = {1,3};
		vector<int> gapreflength = {1,1};
		i = 0;
        for (vector<std::pair<int,int>>::iterator iter2 = gapcontainer.begin(); iter2 != gapcontainer.end(); iter2++) {
            cerr << "#" << i << " -- begin "; test(iter2->first, gaprefbegin[i]);
            cerr << "#" << i << " -- length "; test(iter2->second, gapreflength[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,2);
		cerr << "Instanceof test (positive) "; test( ngram.instanceof(skipgram) );

        string querystring2_1gap = "To be or {*1*} to be";
        Pattern skipgram_1gap = encoder.buildpattern(querystring2_1gap, true);
		cerr << "Instanceof test with less abstract skipgram "; test( skipgram_1gap.instanceof(skipgram) );
		cerr << "Instanceof test (ngram equality) "; test( ngram.instanceof(ngram) );

        Pattern revskipgram = skipgram.reverse();
        cerr << "Reverse skipgram: "; test(revskipgram.decode(classdecoder),"be to {*} or {*} To");
        cerr << "N: "; test(revskipgram.n(),6);
		cerr << "Instanceof test (negative) "; test( !ngram.instanceof(revskipgram) );



        cerr << "--- Skipgram as pattern pointer ---" << endl;

        PatternPointer pskipgram = PatternPointer(skipgram);;

        Pattern skipgramcopy = encoder.buildpattern(querystring2, true);
        PatternPointer pskipgram2 = PatternPointer(skipgramcopy);

        cerr << "Skipgram decoded: "; test(pskipgram.decode(classdecoder),"To {*} or {*} to be");
        cerr << "N: "; test(pskipgram.n(),6);
        cerr << "Bytesize: "; test(pskipgram.bytesize(),6);
        cerr << "Category==skipgram? " ; test(pskipgram.category() == SKIPGRAM) ;
        cerr << "Skipcount==2? " ; test(pskipgram.skipcount(),2) ;
        cerr << "Testing equivalence between pointer and different-source pointer"; test(pskipgram == pskipgram2);
        cerr << "Testing equivalence between pointer and pattern"; test(skipgram == pskipgram);
        cerr << "Testing equivalence between pointer and pattern (rev)"; test(pskipgram == skipgram);
        cerr << "Testing hash equivalence between pointer and pattern"; test(skipgram.hash() == pskipgram.hash());
        Pattern derefskipgram = Pattern(pskipgram);
        cerr << "Testing equivalence after pointer construction and dereference"; test(skipgram == derefskipgram);
        cerr << "Testing equivalence after pointer construction and dereference (rev)"; test(derefskipgram==skipgram);
        cerr << "Testing equivalence between pointer from pattern and pointer"; test(PatternPointer(skipgram) == pskipgram);
        cerr << "Testing equivalence between pointer from pattern and pointer (rev)"; test(pskipgram == PatternPointer(skipgram));

        cerr << "Parts: " << endl;
        vector<PatternPointer> pparts;
        pskipgram.parts(pparts);
		i = 0;
        for (vector<PatternPointer>::iterator iter2 = pparts.begin(); iter2 != pparts.end(); iter2++) {
            const Pattern part = *iter2;
            cerr << "#" << i << " -- "; test(part.decode(classdecoder), partsref[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,3);


        cerr << "Gaps: " << endl;
        std::vector<std::pair<int,int> > pgapcontainer;
        pskipgram.gaps(pgapcontainer);
		i = 0;
        for (vector<std::pair<int,int>>::iterator iter2 = pgapcontainer.begin(); iter2 != pgapcontainer.end(); iter2++) {
            cerr << "#" << i << " -- begin "; test(iter2->first, gaprefbegin[i]);
            cerr << "#" << i << " -- length "; test(iter2->second, gapreflength[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,2);
		cerr << "Instanceof test (positive) "; test( ngram.instanceof(pskipgram) );

        cerr << "----------------------------------------------------" << endl;

        cerr << "Extracting skip content based on skip gram and full instance" << endl;
        Pattern skipcontent = skipgram.extractskipcontent(ngram);

        cerr << "Skipcontent: "; test(skipcontent.decode(classdecoder),"be {*} not");
        cerr << "N: "; test(skipcontent.n(),3);
        cerr << "Bytesize: "; test(skipcontent.bytesize(),3);

        cerr << "----------------------------------------------------" << endl;

        vector<Pattern> parts2;
    /*
        cerr << "Encoding skip-gram from string input" << endl;
        string querystring3 = "{*1*} be {*1*} not {*2*}";

        EncSkipGram skipgraminv = encoder.input2skipgram(querystring3);

        cerr << "Skipgram: " << skipgraminv.decode(classdecoder) << endl;
        cerr << "N: " << (int) skipgraminv.n() << endl;
        cerr << "Size: " << (int) skipgraminv.size() << endl;

        cerr << "Parts: " << endl;

        for (vector<EncNGram*>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {
            const EncAnyGram * subngram = *iter2;
            cerr << "'" << subngram->decode(classdecoder) << "'" << endl;
        }
    */
        cerr << "----------------------------------------------------" << endl;

        cerr << "Encoding skip-gram from string input" << endl;
        string querystring4 = "be {*} not";

        Pattern skipgraminv2 = encoder.buildpattern(querystring4, true);

        cerr << "Skipgram: "; test(skipgraminv2.decode(classdecoder),"be {*} not");
        cerr << "N: "; test(skipgraminv2.n(),3);
        cerr << "Bytesize: "; test(skipgraminv2.bytesize(),3);

        cerr << "Parts: " << endl;
        parts2.clear();
        skipgraminv2.parts(parts2);
		vector<string> partsref2 = {"be","not"};
		i = 0;
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {
            const Pattern part = *iter2;
            cerr << "#" << i << " -- "; test(part.decode(classdecoder), partsref2[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,2);
        cerr << "----------------------------------------------------" << endl;
        /*cerr << "Re-instantiating skipgram with skipcontent" << endl;
        Pattern rengram = skipgram.instantiate(&skipgraminv2, parts2);

        cerr << "Ngram: " << rengram.decode(classdecoder) << endl;
        cerr << "N: " << (int) rengram.n() << endl;
        cerr << "Size: " << (int) rengram.size() << endl;*/

        cerr << "----------------------------------------------------" << endl;
        string querystring5 = "be {*1*} not {*2*} be";
        Pattern skipgram5 = encoder.buildpattern(querystring5, true);
        cerr << "Skipgram: "; test(skipgram5.decode(classdecoder),"be {*} not {*} {*} be");

        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram5.parts(parts2);
		vector<string> skipgram5partsref = {"be","not","be"};
     	i = 0;
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {
            const Pattern part = *iter2;
            cerr << "#" << i << " -- "; test(part.decode(classdecoder), skipgram5partsref[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,3);

        cerr << "getgaps: " << endl;
        vector<pair<int,int> > gaps;
        skipgram5.gaps(gaps);
		vector<int> skipgram5gaprefbegin = {1,3};
		vector<int> skipgram5gapreflength = {1,2};
		i = 0;
        for (vector<pair<int,int >>::iterator iter2 = gaps.begin(); iter2 != gaps.end(); iter2++) {
            cerr << "#" << i << " -- begin "; test(iter2->first, skipgram5gaprefbegin[i]);
            cerr << "#" << i << " -- length "; test(iter2->second, skipgram5gapreflength[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,2);

        cerr << "getparts: " << endl;
        vector<pair<int,int> > p;
        skipgram5.parts(p);
		vector<int> skipgram5partrefbegin = {0,2,5};
		vector<int> skipgram5partreflength = {1,1,1};
		i = 0;
        for (vector<pair<int,int >>::iterator iter2 = p.begin(); iter2 != p.end(); iter2++) {
            cerr << "#" << i << " -- begin "; test(iter2->first, skipgram5partrefbegin[i]);
            cerr << "#" << i << " -- length "; test(iter2->second, skipgram5partreflength[i]);
			i++;
        }
		cerr << "Count check "; test(i,3);
        /*cerr << "mask: " << endl;
        vector<bool> m;
        skipgram5.mask(m);
        for (vector<bool>::iterator iter2 = m.begin(); iter2 != m.end(); iter2++) {
            if (*iter2)  {
                cerr << "1";
            } else {
                cerr << "0";
            }
        }*/
        cerr << endl;

        cerr << "slice(5,1): ";
        Pattern token = Pattern(skipgram5,5,1);
		test(token.decode(classdecoder),"be");



        cerr << "slice(2,4): ";
        Pattern s5slice2 = Pattern(skipgram5,2,4);
		test(s5slice2.decode(classdecoder),"not {*} {*} be");

        cerr << "slice(1,3): " << endl;
        Pattern s5slice3 = Pattern(skipgram5,1,3);
		test(s5slice3.decode(classdecoder),"{*} not {*}");


        cerr << "----------------------------------------------------" << endl;
        string querystring6 = "be {*1*} not";
        Pattern skipgram6 = encoder.buildpattern(querystring6, true);
        cerr << "Skipgram: ";
		test(skipgram6.decode(classdecoder),"be {*} not");
        skipgram6.out();

        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram6.parts(parts2);
		i = 0;
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {
            const Pattern part = *iter2;
            cerr << "#" << i << " -- "; test(part.decode(classdecoder), partsref2[i]);
            i += 1;
        }
		cerr << "Count check "; test(i,2);



        cerr << "----------------------------------------------------" << endl;
        string querystring7 = "blah {*1*} or {*2*} blah";
        Pattern skipgram7 = encoder.buildpattern(querystring7, true); //allowunknown=true
        cerr <<  "Skipgram with low-frequency/high-byte words: ";
	    test(skipgram7.decode(classdecoder),"blah {*} or {*} {*} blah");
        skipgram7.out();

		cerr << "Size: "; test(skipgram7.n(),6);
		cerr << "Bytesize: "; test(skipgram7.bytesize(),12);

        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram7.parts(parts2);
		cerr << "(computed)" << endl;
		vector<string> skipgram7partsref = {"blah","or","blah"};
		i = 0;
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {
            const Pattern part = *iter2;
            cerr << "#" << i << " -- "; test(part.decode(classdecoder), skipgram7partsref[i]);
			i += 1;
        }
		cerr << "Count check "; test(i,3);

        cerr << "gaps: " << endl;
        gaps.clear();
        skipgram7.gaps(gaps);
        vector<int> skipgram7gaprefbegin = {1,3};
        vector<int> skipgram7gapreflength = {1,2};
        i = 0;
        for (vector<pair<int,int >>::iterator iter2 = gaps.begin(); iter2 != gaps.end(); iter2++) {
            cerr << "#" << i << " -- begin "; test(iter2->first, skipgram7gaprefbegin[i]);
            cerr << "#" << i << " -- length "; test(iter2->second, skipgram7gapreflength[i]);
			i++;
        }
		cerr << "Count check "; test(i,2);

        /*cerr << "mask: " << endl;
        m.clear();
        skipgram7.mask(m);
        for (vector<bool>::iterator iter2 = m.begin(); iter2 != m.end(); iter2++) {
            if (*iter2)  {
                cerr << "1";
            } else {
                cerr << "0";
            }
        }		*/
        cerr << endl;

        cerr << "slice(5,1): ";
        token = Pattern(skipgram7,5,1);
        test(token.decode(classdecoder),"blah");

        cerr << "----------------------------------------------------" << endl;
        string querystring8 = "{*1*} or blah {*2*}";
        Pattern skipgram8 = encoder.buildpattern(querystring8, true);
        cerr << "Skipgram: ";
        test(skipgram8.decode(classdecoder),"{*} or blah {*} {*}");


        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram8.parts(parts2);
        vector<string> skipgram8partsref = {"or blah"};
        i = 0;
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {
            const Pattern part = *iter2;
            cerr << "#" << i << " -- "; test(part.decode(classdecoder), skipgram8partsref[i]);
            i++;
        }
		cerr << "Count check "; test(i,1);

        cerr << "gaps: " << endl;
        gaps.clear();
        skipgram8.gaps(gaps); //TODO: FIX!!
        vector<int> skipgram8gaprefbegin= {0,3};
        vector<int> skipgram8gapreflength= {1,2};
        i = 0;
        for (vector<pair<int,int >>::iterator iter2 = gaps.begin(); iter2 != gaps.end(); iter2++) {
            cerr << "#" << i << " -- begin "; test(iter2->first, skipgram8gaprefbegin[i]);
            cerr << "#" << i << " -- length "; test(iter2->second, skipgram8gapreflength[i]);
            i++;
        }
		cerr << "Count check "; test(i,2);

        /*cerr << "mask: " << endl;
        m.clear();
        skipgram8.mask(m);
        for (vector<bool>::iterator iter2 = m.begin(); iter2 != m.end(); iter2++) {
            if (*iter2)  {
                cerr << "1";
            } else {
                cerr << "0";
            }
        }*/
        cerr << endl;

        cerr << "slice(2,1): ";
        token = Pattern(skipgram8,2,1);
        test(token.decode(classdecoder),"blah");

        cerr << "slice(0,4): ";
        Pattern s8slice1 = Pattern(skipgram8,0,4);
        test(s8slice1.decode(classdecoder),"{*} or blah {*}");
        cerr << endl;

        cerr << "slice(1,2): ";
        Pattern s8slice2 = Pattern(skipgram8,1,2);
        test(s8slice2.decode(classdecoder),"or blah");

        cerr << "slice(1,4): ";
        Pattern s8slice3 = Pattern(skipgram8,1,4);
        test(s8slice3.decode(classdecoder),"or blah {*} {*}");


        cerr << endl <<"************************** Flexgram tests  ***************************************" << endl << endl;

        Pattern flexgram5 = skipgram5.toflexgram();
        cerr << "Converting skipgram '" << querystring5 << "' to flexgram:" << endl;
        cerr << flexgram5.decode(classdecoder) << endl;
        cerr << "Size (n): "; test(flexgram5.n(),5); //== size()
        cerr << "Bytesize: "; test(flexgram5.bytesize(),5);
        cerr << "Category==flexgram: "; test(flexgram5.category() == FLEXGRAM);
        cerr << "Raw" << endl;
        flexgram5.out();
        cerr << "Parts: " << endl;
        vector<Pattern> flexparts;
        flexgram5.parts(flexparts);

        vector<string> flexrefparts = {"be","not","be"};
        i = 0;
        for (vector<Pattern>::iterator iter2 = flexparts.begin(); iter2 != flexparts.end(); iter2++) {
            const Pattern part = *iter2;
            cerr << "#" << i << " -- "; test(part.decode(classdecoder), flexrefparts[i]);
            i++;
        }
		cerr << "Count check "; test(i,3);

        cerr << endl << "************************** Serialisation and low-level map test  ***************************************" << endl << endl;

        Pattern empty = Pattern();

        cerr << "Writing patterns to file: " << endl;
        ofstream * out = new ofstream("/tmp/patterns.tmp");
        ngram.write(out);
        ngramhigh.write(out);
        empty.write(out);
        skipgram.write(out);
        out->close();

        cerr << "Reading from file" << endl;
        ifstream * in = new ifstream("/tmp/patterns.tmp");
        Pattern ngram_read = Pattern(in);
        Pattern ngramhigh_read = Pattern(in);
        Pattern empty_read = Pattern(in);
        Pattern skipgram_read = Pattern(in);
        in->close();
        cerr << "Integrity check for ngram? " ; test(ngram == ngram_read) ;
        cerr << "Integrity check for ngram (high)? " ; test(ngramhigh == ngramhigh_read) ;
        cerr << "Integrity check for empty pattern? " ; test(empty == empty_read);
        cerr << "Integrity check for skipgram? " ; test(skipgram == skipgram_read) ;



        PatternMap<uint32_t> map1;
        map1[ngram] = 1;
        map1[ngramhigh] = 1;
        map1[empty] = 1;
        map1[flexgram5] = 2;
        cerr << "Integrity for PatternMap" ; test(map1.size() , 4);
        cerr << "Integrity for PatternMap" ; test(map1[ngram], 1);
        cerr << "Integrity for PatternMap" ; test(map1[ngramhigh], 1);
        cerr << "Integrity for PatternMap" ; test(map1[empty], 1);
        cerr << "Integrity for PatternMap" ; test(map1[flexgram5] , 2);
        cerr << "Saving patternmap" << endl;
        map1.write("/tmp/patternmap.tmp");


        PatternMap<uint32_t> map2;
        cerr << "Loading patternmap" << endl;
        map2.read("/tmp/patternmap.tmp");
        cerr << "Integrity for PatternMap" ; test(map2.size() , 4);
        cerr << "Integrity for PatternMap" ; test(map2[ngram] , 1);
        cerr << "Integrity for PatternMap" ; test(map2[ngramhigh] , 1);
        cerr << "Integrity for PatternMap" ; test(map2[empty] , 1);
        cerr << "Integrity for PatternMap" ; test(map2[flexgram5] , 2);

        AlignedPatternMap<uint32_t> amap1;
        amap1[ngram][ngram] = 1;
        amap1[flexgram5][flexgram5] = 2;
        cerr << "Integrity for AlignedPatternMap" ; test(amap1.size(), 2);
        cerr << "Integrity for AlignedPatternMap" ; test(amap1[ngram][ngram], 1);
        cerr << "Integrity for AlignedPatternMap" ; test(amap1[flexgram5][flexgram5], 2);
        cerr << "Saving AlignedPatternMap" << endl;
        amap1.write("/tmp/alignedpatternmap.tmp");


        AlignedPatternMap<uint32_t> amap2;
        cerr << "Loading alignedpatternmap" << endl;
        amap2.read("/tmp/alignedpatternmap.tmp");
        cerr << "Integrity for AlignedPatternMap" ; test(amap2.size() , 2);
        cerr << "Integrity for AlignedPatternMap" ; test(amap2[ngram][ngram] , 1);
        cerr << "Integrity for AlignedPatternMap" ; test(amap2[flexgram5][flexgram5] , 2);

        PatternPointerMap<uint32_t> ppmap;
        ppmap[pngram] = 1;
        ppmap[pskipgram] = 3;
        cerr << "Integrity for PatternPointerMap" ; test(ppmap.size() , 2);
        cerr << "Integrity for PatternPointerMap" ; test(ppmap[pngram], 1);
        cerr << "Integrity for PatternPointerMap" ; test(ppmap[pskipgram], 3);
        cerr << "Querying PatternPointerMap with Pattern (ngram)" ; test(ppmap[ngram], 1);
        cerr << "Querying PatternPointerMap with Pattern (skipgram)" ; test(ppmap[skipgram], 3);
        cerr << "Querying PatternPointerMap for non-existing pattern" ; test(ppmap[flexgram5], 0);
        cerr << "Saving patternpointermap" << endl;
        ppmap.write("/tmp/patternpointermap.tmp");

        }
        {
        cerr << endl << "************************** Low-level skipgram tests ***************************************" << endl << endl;

        const string classfile = "/tmp/colibritest";
        ClassEncoder encoder = ClassEncoder(classfile);
        ClassDecoder decoder = ClassDecoder(classfile);

        string querystring = "to be or not to be";
        Pattern ngram = encoder.buildpattern(querystring, true);

        vector<uint32_t> masks;

        masks = compute_skip_configurations(3,3);
        cerr << "Computing possible gaps in 3-grams: "; test(masks.size(),1);

        masks = compute_skip_configurations(4,4);
        cerr << "Computing possible gaps in 4-grams: "; test(masks.size(),3);

        masks.clear();

        masks = compute_skip_configurations(6,6);
        cerr << "Computing possible gaps in 6-grams: " << endl;
        int j = 0;
        for (vector<uint32_t>::iterator iter = masks.begin(); iter != masks.end(); iter++) {
            vector<pair<int,int>> gapconfig = mask2vector(*iter,6);
            int data[6] = {0,0,0,0,0,0};
            for (vector<pair<int,int>>::iterator iter2 = gapconfig.begin(); iter2 != gapconfig.end(); iter2++) {
                for (int i = iter2->first; i < iter2->first + iter2->second; i++) {
                    data[i] = 1;
                }
            }
            for (int i = 0; i < 6; i++) {
                cerr << data[i] << " ";
            }
            cerr << endl;

            Pattern skipgram = ngram.addskips(gapconfig);
            cerr << "   skipgram (pattern): " << skipgram.tostring(decoder) << endl;
            PatternPointer pskipgram = PatternPointer(ngram);
            pskipgram.mask = *iter;
            cerr << "   skipgram (patternpointer): " << pskipgram.tostring(decoder) << endl;
            PatternPointer pskipgram2 = PatternPointer(skipgram);
            cerr << "   skipgram (patternpointer from pattern): " << pskipgram2.tostring(decoder) << endl;
            cerr << "   pattern vs patternpointer equivalence: "; test(skipgram == pskipgram);
            cerr << "   patternpointer vs patternpointer equivalence: "; test(pskipgram == pskipgram2);
            cerr << "   pattern from patternpointer equivalence: "; test(Pattern(pskipgram) == skipgram);
            j++;
        }
        cerr << "Count check: "; test(masks.size(),15);


        }
        {
        cerr << endl << "************************** Unindexed PatternModel Tests ***************************************" << endl << endl;

        string rawcorpusfile = "/tmp/hamlet.txt";
        ofstream * out = new ofstream(rawcorpusfile);
        *out << string(poem);
        out->close();

        cerr << "Class encoding corpus..." << endl;
        system("colibri-classencode /tmp/hamlet.txt");



        cerr << "Class decoding corpus..." << endl;
        system("colibri-classdecode -c /tmp/hamlet.colibri.cls -f /tmp/hamlet.colibri.dat > /tmp/hamlet.decoded.txt");
        int r = system("diff /tmp/hamlet.txt /tmp/hamlet.decoded.txt");
        cerr << "Checking equivalence after decoding: "; test(r,0);



        cerr << "Loading class decoders/encoders" << endl;
        const string classfile = "/tmp/hamlet.colibri.cls";
        ClassDecoder classdecoder = ClassDecoder(classfile);
        ClassEncoder classencoder = ClassEncoder(classfile);

        cerr << "Loading corpus as IndexedCorpus" << endl;
        IndexedCorpus corpus("/tmp/hamlet.colibri.dat");
        cerr << "Total number of tokens: " << corpus.size() << endl;
        Pattern firstword = corpus.getpattern(IndexReference(1,0),1);
        cerr << "First word:  "; test(firstword.tostring(classdecoder),"To");
        Pattern needle = classencoder.buildpattern(string("fair Ophelia"));
        cerr << "Finding pattern: ";
        vector<std::pair<IndexReference,PatternPointer>> matches = corpus.findpattern(needle);
        test(matches.size(), 1);

        std::string infilename = "/tmp/hamlet.colibri.dat";
        std::string outputfilename = "/tmp/data.colibri.patternmodel";

		cerr << endl << " --- unindexed model without skipgrams ---" << endl << endl;
		PatternModelOptions options;

		cerr << "Building unindexed model (from preloaded corpus, no skipgrams)" << endl;
		PatternModel<uint32_t> unindexedmodelNSR(&corpus);
		unindexedmodelNSR.train(infilename, options);
		cerr << "Patterns"; test(unindexedmodelNSR.size(),111);
		cerr << "Types"; test(unindexedmodelNSR.types(),186);
		cerr << "Tokens"; test(unindexedmodelNSR.tokens(),354);

		cerr << "Unigram Types"; test(unindexedmodelNSR.totalwordtypesingroup(0,1),45);

		cerr << "Building unindexed model (without preloaded corpus, no skipgrams)" << endl;
		PatternModel<uint32_t> unindexedmodelNS;

		unindexedmodelNS.train(infilename, options);
		cerr << "Patterns"; test(unindexedmodelNS.size(),111);
		cerr << "Types"; test(unindexedmodelNS.types(),186);
		cerr << "Tokens"; test(unindexedmodelNS.tokens(),354);


		unindexedmodelNS.print(&std::cerr, classdecoder);
		cerr << endl;
		unindexedmodelNS.report(&std::cerr);
		cerr << endl;

		Pattern ngramNS = classencoder.buildpattern(string("or not to"), true);
		cerr << "Testing unindexedmodel.has()"; test( unindexedmodelNS.has(ngramNS) );
		Pattern ngramNS_ne = classencoder.buildpattern("give us fortune", true);
		cerr << "Testing !unindexedmodel.has()"; test( !unindexedmodelNS.has(ngramNS_ne) );
		cerr << "Testing unindexedmodel.occurencecount()"; test( unindexedmodelNS.occurrencecount(ngramNS),  6);


		cerr << endl << " --- extracting skipgrams from single ngram ---" << endl << endl;
		Pattern queryngram = classencoder.buildpattern(string("to be or not to be"), true);
		vector<PatternPointer> skipgrams;
		int i = 0;
		unindexedmodelNS.computeskipgrams(queryngram,1,NULL,NULL,NULL,&skipgrams,true,3);
		for (vector<PatternPointer>::iterator iter = skipgrams.begin(); iter != skipgrams.end();iter++) {
			cerr << iter->tostring(classdecoder) << endl;
			i++;
		}
		cerr << "Count check "; test(i,15);







        cerr << endl << " --- unindexed model with skipgrams ---" << endl << endl;
        options.DOSKIPGRAMS_EXHAUSTIVE = true;
        options.DOSKIPGRAMS = false ;


        cerr << "Building unindexed model (from preloaded corpus, with skipgrams)" << endl;
        PatternModel<uint32_t> unindexedmodelR(&corpus);
        unindexedmodelR.train(infilename, options);
        cerr << "Patterns"; test(unindexedmodelR.size(),385);
        cerr << "Types"; test(unindexedmodelR.types(),186);
        cerr << "Tokens"; test(unindexedmodelR.tokens(),354);

        cerr << "Building unindexed model (without preloaded corpus, with skipgrams)" << endl;
        PatternModel<uint32_t> unindexedmodel;

        unindexedmodel.train(infilename, options);
        cerr << "Patterns"; test(unindexedmodel.size(),385);
        cerr << "Types"; test(unindexedmodel.types(),186);
        cerr << "Tokens"; test(unindexedmodel.tokens(),354);


        unindexedmodel.print(&std::cerr, classdecoder);
        cerr << endl;
        unindexedmodel.report(&std::cerr);
        cerr << endl;
        unindexedmodel.histogram(&std::cerr);





        Pattern ngram = classencoder.buildpattern(string("or not to"), true);
        cerr << "Testing unindexedmodel.has()"; test( unindexedmodel.has(ngram) );
        Pattern ngram_ne = classencoder.buildpattern("give us fortune", true);
        cerr << "Testing !unindexedmodel.has()"; test( !unindexedmodel.has(ngram_ne) );
        cerr << "Testing unindexedmodel.occurencecount()"; test( unindexedmodel.occurrencecount(ngram),  6);


        cerr << endl;
        cerr << "Writing unindexed pattern model to file" << endl;
        unindexedmodel.write(outputfilename);
        //options.DEBUG = true;
        cerr << "Reading unindexed pattern modelfrom file" << endl;
        PatternModel<uint32_t> unindexedmodel2 = PatternModel<uint32_t>(outputfilename, options);
        //options.DEBUG = false;
        cerr << "Outputting report again" << endl;
        unindexedmodel2.report(&std::cerr);
        cerr << "Equal tokens? " ; test(unindexedmodel.tokens() == unindexedmodel2.tokens() );
        cerr << "Equal types? " ; test(unindexedmodel.types() == unindexedmodel2.types() );
        cerr << "Equal size? " ; test(unindexedmodel.size() == unindexedmodel2.size() );
        cerr << "Testing unindexedmodel2.has()"; test( unindexedmodel2.has(ngram) );
        cerr << "Testing unindexedmodel2.occurrencecount()"; test( unindexedmodel2.occurrencecount(ngram),6 );



        cerr << endl << "************************** Indexed PatternModel & Relations Tests ***************************************" << endl << endl;
        options.DOSKIPGRAMS_EXHAUSTIVE = false;
        options.DOSKIPGRAMS = true ;
        cerr << "Building indexed model" << endl;
        IndexedPatternModel<> indexedmodel(&corpus);
        indexedmodel.train(infilename, options);
		cerr << "Size test"; test(indexedmodel.size(), 133 );
		cerr << "Size test (2) "; test(indexedmodel.size(), unindexedmodelNS.size() + indexedmodel.totalpatternsingroup(SKIPGRAM,0) );
        cerr << "Equal tokens? " ; test(unindexedmodel.tokens() == indexedmodel.tokens() );
        cerr << "Equal types? " ; test(unindexedmodel.types() == indexedmodel.types() );
        cerr << "Testing indexedmodel.has()"; test( indexedmodel.has(ngram) );
        cerr << "Testing indexedmodel.occurrencecount()"; test( indexedmodel.occurrencecount(ngram),6 );
        indexedmodel.print(&std::cerr, classdecoder);
        cerr << endl;
        indexedmodel.report(&std::cerr);
        cerr << endl;
        indexedmodel.histogram(&std::cerr);

		cerr << "Unigram Types"; test(indexedmodel.totalwordtypesingroup(0,1),45);


        cerr << "Writing indexed model to file" << endl;
        indexedmodel.write(outputfilename);
        cerr << "Reading indexed model from file" << endl;
        options.DEBUG = verbose;
        IndexedPatternModel<> indexedmodel2 = IndexedPatternModel<>(outputfilename, options);
        options.DEBUG = false;
        cerr << "Equal size? " ; test(indexedmodel2.size(), indexedmodel.size() );
        cerr << "Equal tokens? " ; test(indexedmodel2.tokens(), indexedmodel.tokens() );
        cerr << "Equal types? " ; test(indexedmodel2.types() , indexedmodel2.types() );


        cerr << "Reading indexed model as unindexed" << endl;
        PatternModel<uint32_t> indexedasunindexedmodel = PatternModel<uint32_t>(outputfilename, options);
        cerr << "Equal size? " ; test(indexedasunindexedmodel.size(), indexedmodel.size() );
        cerr << "Equal tokens? " ; test(indexedasunindexedmodel.tokens(), indexedmodel.tokens() );
        cerr << "Equal types? " ; test(indexedasunindexedmodel.types() , indexedmodel2.types() );

        string querystring  = "To die , to sleep";
        Pattern patterndiesleep = classencoder.buildpattern(querystring, true);
        cerr << "Extracting subsumption relations for " << querystring << endl;

        t_relationmap relations = indexedmodel.getsubchildren(patterndiesleep);
        indexedmodel.outputrelations(ngram, relations, classdecoder, &cerr,"SUBSUMES");


        string querystring2  = "not";
        cerr << "Extracting subsumption relations for " << querystring2 << endl;
        Pattern patternnot = classencoder.buildpattern(querystring2, true);
        t_relationmap relations2 = indexedmodel.getsubparents(patternnot);
        indexedmodel.outputrelations(patternnot, relations2, classdecoder, &cerr,"SUBSUMED-BY");


        cerr << "Extracting neighbour relations for " << querystring2 << endl;
        t_relationmap relations3 = indexedmodel.getleftneighbours(patternnot);
        indexedmodel.outputrelations(patternnot, relations3, classdecoder, &cerr,"RIGHT-OF");

        t_relationmap relations4 = indexedmodel.getrightneighbours(patternnot);
        indexedmodel.outputrelations(patternnot, relations4, classdecoder, &cerr,"LEFT-OF");


        string querystring3 = "To {*3*} to";
        Pattern skipgram = classencoder.buildpattern(querystring3, true);
        cerr << "Extracting subsumption relations for " << querystring3 << endl;

        t_relationmap relations5 = indexedmodel.getsubchildren(skipgram);
        indexedmodel.outputrelations(skipgram, relations5, classdecoder, &cerr,"SUBSUMES");

        cerr << "Extracting skipcontent relations for " << querystring3 << endl;

        t_relationmap relations6 = indexedmodel.getskipcontent(skipgram);
        indexedmodel.outputrelations(skipgram, relations6, classdecoder, &cerr,"SKIPCONTENT");
        for (t_relationmap::iterator iter = relations6.begin(); iter != relations6.end(); iter++) {
            cerr << " length check: "; test(iter->first.n(), 3);
            cerr << " type check: "; test(iter->first.category(), NGRAM);
        }
        cerr << "Verify count: "; test(relations6.size(), 2); //the other 2 are below occurrence threshold


        Pattern skipgram2 = classencoder.buildpattern("To {*} or {*} to {*} ."); //this is a pattern actually in the model (important!)
        cerr << "Extracting instances for " << skipgram2.tostring(classdecoder) << endl;
        t_relationmap relations7 = indexedmodel.getinstances(skipgram2);
        indexedmodel.outputrelations(skipgram2, relations7, classdecoder, &cerr,"INSTANTIATED-BY");
        for (t_relationmap::iterator iter = relations7.begin(); iter != relations7.end(); iter++) {
            cerr << " length check: "; test(iter->first.n(), 7);
            cerr << " type check: "; test(iter->first.category(), NGRAM);
        }
        cerr << "Verify count: "; test(relations7.size(), 2);  //other two are below threshold


        Pattern instance = classencoder.buildpattern("To flee or not to flee ."); //this is a pattern actually in the model (important!)
        cerr << "Extracting templates for " << instance.tostring(classdecoder) << endl;
        t_relationmap relations8 = indexedmodel.gettemplates(instance);
        for (t_relationmap::iterator iter = relations8.begin(); iter != relations8.end(); iter++) {
            cerr << " " << iter->first.tostring(classdecoder);
            cerr << " length check: "; test(iter->first.n(), 7);
            cerr << " type check: "; test(iter->first.category(), SKIPGRAM);  //no flexgrams in model at this point
        }
        cerr << "Verify count: "; test(relations8.size(), 8);  //other two are below threshold



        cerr << "All relations for  " << querystring3 << " in one go" << endl;
        indexedmodel.outputrelations(skipgram, classdecoder, &cerr);

        cerr << endl;


        cerr << "Computing flexgrams out of skipgrams";
        int foundflex = indexedmodel.computeflexgrams_fromskipgrams();
		test(foundflex, 22);
		test(indexedmodel.size(), 155);
		//cerr << "Size test (2) "; test(indexedmodel.size(), unindexedmodelNS.size() + indexedmodel.totalpatternsingroup(SKIPGRAM,0) + indexedmodel.totalpatternsingroup(FLEXGRAM,0) ) ;


        Pattern flexgram = skipgram.toflexgram();
        cerr << "Extracting skipcontent relations for flexgram " << flexgram.tostring(classdecoder) << endl;
        t_relationmap relations62 = indexedmodel.getskipcontent(flexgram);
        indexedmodel.outputrelations(flexgram, relations62, classdecoder, &cerr,"SKIPCONTENT");
        for (t_relationmap::iterator iter = relations62.begin(); iter != relations62.end(); iter++) {
            cerr << " type check: "; test(iter->first.category(), NGRAM);
        }
        cerr << "Verify count: "; test(relations62.size(), 2);  //other two are below threshold

        Pattern flexgram2 = skipgram2.toflexgram(); //this is a pattern actually in the model (important!)
        t_relationmap relations72 = indexedmodel.getinstances(flexgram2);
        cerr << "Extracting instances for flexgram " << flexgram2.tostring(classdecoder) << endl;
        indexedmodel.outputrelations(flexgram2, relations72, classdecoder, &cerr,"INSTANTIATED-BY");
        for (t_relationmap::iterator iter = relations72.begin(); iter != relations72.end(); iter++) {
            cerr << " length check: "; test(iter->first.n(), 7);
            cerr << " type check: "; test(iter->first.category(), NGRAM);
        }
        cerr << "Verify count: "; test(relations72.size(), 2);  //other two are below threshold


        if (verbose) {
            cerr << "Iterating over all patterns and testing (non-)equivalence" << endl;
            int i = 0;
            for (IndexedPatternModel<>::iterator iter = indexedmodel.begin(); iter != indexedmodel.end(); iter++) {
                const Pattern pattern = iter->first;
                const PatternPointer pp = PatternPointer(pattern);
                const IndexedData data = iter->second;
                int j = 0;
                cerr << pattern.tostring(classdecoder) << endl;
                for (IndexedPatternModel<>::iterator iter2 = indexedmodel.begin(); iter2 != indexedmodel.end(); iter2++) {
                    const Pattern pattern2 = iter2->first;
                    const PatternPointer pp2 = PatternPointer(pattern2);
                    cerr << "\t" << pp2.tostring(classdecoder) << endl;
                    cerr << "\t\t...p vs p "; test(pattern == pattern2, i == j);
                    cerr << "\t\t...pp vs pp "; test(pp == pp2, i == j);
                    cerr << "\t\t...p vs pp "; test(pattern == pp2, i == j);
                    cerr << "\t\t...pp vs p "; test(pp == pattern2, i == j);
                    j++;
                }
                i++;
            }
        }

        cerr << "outputting all" << endl;
        indexedmodel.print(&std::cerr, classdecoder);
        cerr << "Outputting report again, now with flexgrams" << endl;
        indexedmodel.report(&std::cerr);



        cerr << "Building indexed model with mintokens=1" << endl;
        options.MINTOKENS = 1;
        IndexedPatternModel<> indexedmodel1(&corpus);
        indexedmodel1.train(infilename, options);
        cerr << "Found " << indexedmodel1.size() << " patterns, " << indexedmodel1.types() << " types, " << indexedmodel1.tokens() << " tokens" << endl;

        cerr << endl << "************************** HashOrderedPatternMap Test  ***************************************" << endl << endl;

        options.MINTOKENS = 2;
        options.DOSKIPGRAMS_EXHAUSTIVE = true;
        options.DOSKIPGRAMS = false ;
        cerr << "Building unindexed model using ordered patternmap" << endl;
        PatternModel<uint32_t,BaseValueHandler<uint32_t>,HashOrderedPatternMap<uint32_t,BaseValueHandler<uint32_t>>> unindexedmodel3;

        cerr << endl;
        unindexedmodel3.train(infilename, options);
        cerr << "Patterns"; test(unindexedmodel3.size(),385);
        cerr << "Types"; test(unindexedmodel3.types(),186);
        cerr << "Tokens"; test(unindexedmodel3.tokens(),354);

        cerr << endl << "************************** PatternSet Test ***************************************" << endl << endl;

        cerr << "Testing low-level PatternSet...";
        PatternSet<uint32_t> set;
        for (IndexedPatternModel<>::iterator iter = indexedmodel.begin(); iter != indexedmodel.end(); iter++) {
            set.insert(iter->first);
        }

        //double inserts:
        for (IndexedPatternModel<>::iterator iter = indexedmodel.begin(); iter != indexedmodel.end(); iter++) {
            set.insert(iter->first);
        }
        test(set.size(), 155);


    }
    {
        cerr << endl << "************************** IndexCorpus & Reverse Index Iteration Tests ***************************************" << endl << endl;

        const string classfile = "/tmp/hamlet.colibri.cls";
        ClassDecoder classdecoder = ClassDecoder(classfile);
        ClassEncoder classencoder = ClassEncoder(classfile);

        cerr << "Loading corpus as IndexedCorpus" << endl;
        IndexedCorpus corpus("/tmp/hamlet.colibri.dat");

        cerr << "Checking sentence count";
        test(corpus.sentences(),40);

        IndexedCorpus::iterator ri_iter = corpus.begin();
        cerr << "Testing first word (index)";
        test(ri_iter.index()== IndexReference(1,0));
        cerr << "Testing first word (string)";
        test(ri_iter.patternpointer().tostring(classdecoder), "To");
        ri_iter++;
        cerr << "Testing second word (index)";
        test(ri_iter.index() == IndexReference(1,1));
        cerr << "Testing second word (string)";
        test(ri_iter.patternpointer().tostring(classdecoder), "be");
        cerr << "Testing sentence by index: ";
        test(corpus.getsentence(33).tostring(classdecoder), "And lose the name of action .");


        cerr << "Building pattern model, passing corpus as reverse index" << endl;
        PatternModel<uint32_t> model = PatternModel<uint32_t>(&corpus);

        PatternModelOptions options;
        options.MINLENGTH = 3;
        options.MAXLENGTH = 5;
        options.MINTOKENS = 1;

        cerr << "Training model" << endl;
        model.train("/tmp/hamlet.colibri.dat", options);

        cerr << "Iterating over reverse index..." << endl;
        int i = 0;
        for (IndexedCorpus::iterator iter = corpus.begin(); iter != corpus.end(); iter++) {
            cerr << "\tGetting pattern for index " << iter.index().tostring() << " = " << iter.patternpointer().tostring(classdecoder) << endl;
            unordered_set<PatternPointer> patterns = model.getreverseindex(iter.index());
            for (PatternPointer p : patterns) {
                cerr << "\t\t" << p.tostring(classdecoder) << endl;
            }
            i++;
        }
        cerr << "Size check: "; test(corpus.size(), i);




        cerr << "Findpattern test with skipgram" << endl;
        Pattern skipgram = classencoder.buildpattern("that {*} the");
        vector<std::pair<IndexReference,PatternPointer>> matches = corpus.findpattern(skipgram);
        i = 0;
        for ( vector<std::pair<IndexReference,PatternPointer>>::iterator iter = matches.begin(); iter != matches.end(); iter++) {
            //should be only 1
            cerr << "   " << iter->second.tostring(classdecoder) << endl;
            cerr << "   testing match equivalence: "; test(iter->second == skipgram);
            cerr << "   testing reference: "; test(iter->first == IndexReference(1,7));
            i++;
        }
        cerr << "Size check: "; test(1, i);

        cerr << "Findpattern test with flexgram" << endl;
        Pattern flexgram = classencoder.buildpattern("that {**} the");
        matches = corpus.findpattern(flexgram);
        i = 0;
        for ( vector<std::pair<IndexReference,PatternPointer>>::iterator iter = matches.begin(); iter != matches.end(); iter++) {
            //should be only 1
            cerr << "   " << iter->second.tostring(classdecoder) << endl;
            cerr << "   testing match equivalence: "; test(iter->second == flexgram);
            cerr << "   testing reference: "; test(iter->first == IndexReference(1,7));
            i++;
        }
        cerr << "Size check: "; test(1, i);
    }
    {
        cerr << endl << "************************** PatternAlignModel Tests ***************************************" << endl << endl;

        const string classfile = "/tmp/hamlet.colibri.cls";
        ClassDecoder classdecoder = ClassDecoder(classfile);
        ClassEncoder classencoder = ClassEncoder(classfile);

        string s = "To be or not to be";
        Pattern p = classencoder.buildpattern(s);
        string s2 = "that is the question";
        Pattern p2 = classencoder.buildpattern(s2);

        {
            cerr << "Creating a PatternAlignmentModel" << endl;
            PatternAlignmentModel<double> alignmodel = PatternAlignmentModel<double>();
            vector<double> v;
            v.push_back(3.14);
            alignmodel.add(p,p,v);
            vector<double> v2;
            v2.push_back(2.17);
            v2.push_back(0.24);
            alignmodel.add(p2,p2,v2);

            cerr << "Saving alignment model" << endl;
            alignmodel.write("/tmp/hamlet.colibri.alignmodel");
        }
        {
            cerr << "Loading PatternAlignmentModel" << endl;
            PatternAlignmentModel<double> alignmodel = PatternAlignmentModel<double>();
            alignmodel.load("/tmp/hamlet.colibri.alignmodel", PatternModelOptions());
            cerr << "Testing size: ";
            test(alignmodel.size(),2);
            cerr << "Testing [p][p][0]: ";
            test(alignmodel.getfeaturevector(p,p)->get(0), 3.14);
            cerr << "Testing [p2][p2][0]: ";
            test(alignmodel.getfeaturevector(p2,p2)->get(0), 2.17);
            cerr << "Testing [p2][p2][1]: ";
            test(alignmodel.getfeaturevector(p2,p2)->get(1), 0.24);

            cerr << "Training indexed model constrained by PatternAlignmentModel" << endl;
            IndexedCorpus c("/tmp/hamlet.colibri.dat");

            PatternModelOptions o;
            o.MINTOKENS = 1;
            IndexedPatternModel<> m = IndexedPatternModel<>(&c);
            m.train(NULL, o, alignmodel.getinterface());

        }

    }

    {

        cerr << endl << "************************** Stream tests  ***************************************" << endl << endl;

        ClassEncoder classencoder = ClassEncoder();
        stringstream sscls;
        sscls << poem;
        unordered_map<string,unsigned int> freqlist;
        classencoder.processcorpus((istream*) &sscls, freqlist);
        classencoder.buildclasses(freqlist);

        stringstream sstxt;
        sstxt << poem;

        stringstream ssdat;
        classencoder.encodefile((istream*) &sstxt, (ostream*) &ssdat,false, false);


        PatternModelOptions options;
        stringstream ss2;
        PatternModel<uint32_t> model = PatternModel<uint32_t>();
        model.train((istream*) &ssdat, options);
    }
    {
        cerr << endl << "************************** Unindexed PatternPointerModel Tests ***************************************" << endl << endl;
        cerr << "Loading class decoders/encoders" << endl;
        const string classfile = "/tmp/hamlet.colibri.cls";
        ClassDecoder classdecoder = ClassDecoder(classfile);
        ClassEncoder classencoder = ClassEncoder(classfile);

        Pattern ngram = classencoder.buildpattern("not");
        PatternPointer pngram = PatternPointer(ngram);

        cerr << "Loading corpus as IndexedCorpus" << endl;
        IndexedCorpus corpus("/tmp/hamlet.colibri.dat");

        PatternModelOptions options;
        options.DOSKIPGRAMS_EXHAUSTIVE = true;
        options.DOSKIPGRAMS = false;

        cerr << "Building unindexed POINTER model" << endl;
        PatternPointerModel<uint32_t> ppmodel(&corpus);
        PatternModel<uint32_t> refmodel(&corpus);

        cerr << endl;
        std::string infilename = "/tmp/hamlet.colibri.dat";
        std::string outputfilename = "/tmp/data.colibri.patternmodel";
        ppmodel.train(infilename, options);
        cerr << "Found " << ppmodel.size() << " patterns, " << ppmodel.types() << " types, " << ppmodel.tokens() << " tokens" << endl;
        ppmodel.print(&std::cerr, classdecoder);

        cerr << "Sanity check: ";
        unsigned int i = 0;
        for (PatternPointerModel<uint32_t>::iterator iter = ppmodel.begin(); iter != ppmodel.end(); iter++) {
            const PatternPointer p = iter->first;
            cerr << "Pattern #" << (i+1) << ", hash=" << p.hash() << ", mask=" << p.mask << "...";
            test(ppmodel.occurrencecount(p),iter->second);
            i++;
        }
		cerr << "Count check "; test(i, ppmodel.size());


        cerr << "Checking presence of Pattern" ; test(ppmodel[ngram], 7);
        cerr << "Checking presence of PatternPointer" ; test(ppmodel[pngram], 7);
        cerr << "Querying occurrencecount with Pattern (ngram)" ; test(ppmodel.occurrencecount(ngram), 7);
        cerr << "Querying occurrencecount with PatternPointer (ngram)" ; test(ppmodel.occurrencecount(pngram), 7);
        string querystring2 = "see or not to see";
        Pattern ngram2 = classencoder.buildpattern(querystring2);
        cerr << "Querying occurrencecount with Pattern (ngram) (2)" ; test(ppmodel.occurrencecount(ngram2), 2);
        string querystring = "or not to {*} .";
        Pattern skipgram = classencoder.buildpattern(querystring, false);
        PatternPointer pskipgram = PatternPointer(skipgram);
        cerr << "Querying occurrencecount with PatternPointer (skipgram)" ; test(ppmodel.occurrencecount(pskipgram), 4);
        cerr << "Querying occurrencecount with Pattern (skipgram)" ; test(ppmodel.occurrencecount(skipgram), 4);
        ppmodel.report(&std::cerr);
        cerr << endl;
        cerr << endl;
        ppmodel.histogram(&std::cerr);

        cerr << "Training reference PatternModel" << endl;
        refmodel.train(infilename, options);
        cerr << "Verifying equal size" ; test(ppmodel.size(), refmodel.size());
        cerr << "Verifying equal token count" ; test(ppmodel.tokens(), refmodel.tokens());
        cerr << "Verifying equal type count" ; test(ppmodel.types(), refmodel.types());

        string ppmodelfile = "/tmp/patternpointermodel.tmp";

        cerr << "Writing PatternPointerModel to file" << endl;
        ppmodel.write(ppmodelfile);

        cerr << "Reading PatternPointerModel from file" << endl;
        PatternPointerModel<uint32_t> ppmodel2 = PatternPointerModel<uint32_t>(ppmodelfile, options);
        cerr << "Verifying equal size" ; test(ppmodel2.size(), ppmodel.size());
        cerr << "Verifying equal token count" ; test(ppmodel2.tokens(), ppmodel.tokens());
        cerr << "Verifying equal type count" ; test(ppmodel2.types(), ppmodel.types());

        cerr << "Loading PatternPointerModel as PatternModel" << endl;
        PatternModel<uint32_t> ppmodel3 = PatternModel<uint32_t>(ppmodelfile,options);
        cerr << "Verifying equal size" ; test(ppmodel3.size(), ppmodel.size());
        cerr << "Verifying equal token count" ; test(ppmodel3.tokens(), ppmodel.tokens());
        cerr << "Verifying equal type count" ; test(ppmodel3.types(), ppmodel.types());
    }
    {
        cerr << endl << "************************** Indexed PatternPointerModel Tests ***************************************" << endl << endl;
        cerr << "Loading class decoders/encoders" << endl;
        const string classfile = "/tmp/hamlet.colibri.cls";
        ClassDecoder classdecoder = ClassDecoder(classfile);
        ClassEncoder classencoder = ClassEncoder(classfile);

        Pattern ngram = classencoder.buildpattern("not");
        PatternPointer pngram = PatternPointer(ngram);

        cerr << "Loading corpus as IndexedCorpus" << endl;
        IndexedCorpus corpus("/tmp/hamlet.colibri.dat");

        PatternModelOptions options;
        options.DOSKIPGRAMS_EXHAUSTIVE = false;
        options.DOSKIPGRAMS = true;

        cerr << "Building indexed POINTER model" << endl;
        IndexedPatternPointerModel<> ppmodel(&corpus);
        IndexedPatternModel<> refmodel(&corpus);

        cerr << endl;
        std::string infilename = "/tmp/hamlet.colibri.dat";
        std::string outputfilename = "/tmp/data.colibri.patternmodel";
        ppmodel.train(infilename, options);
        cerr << "Found " << ppmodel.size() << " patterns, " << ppmodel.types() << " types, " << ppmodel.tokens() << " tokens" << endl;
        ppmodel.print(&std::cerr, classdecoder);

        cerr << "Sanity check: ";
        unsigned int i = 0;
        for (IndexedPatternPointerModel<>::iterator iter = ppmodel.begin(); iter != ppmodel.end(); iter++) {
            const PatternPointer p = iter->first;
            cerr << "Pattern #" << (i+1) << ", hash=" << p.hash() << ", mask=" << p.mask << "...";
            test(ppmodel.occurrencecount(p),iter->second.count());
            i++;
        }
		cerr << "Count check "; test(i, ppmodel.size());


        cerr << "Checking presence of Pattern" ; test(ppmodel[ngram].count(), 7);
        cerr << "Checking presence of PatternPointer" ; test(ppmodel[pngram].count(), 7);
        cerr << "Querying occurrencecount with Pattern (ngram)" ; test(ppmodel.occurrencecount(ngram), 7);
        cerr << "Querying occurrencecount with PatternPointer (ngram)" ; test(ppmodel.occurrencecount(pngram), 7);
        string querystring2 = "see or not to see";
        Pattern ngram2 = classencoder.buildpattern(querystring2);
        cerr << "Querying occurrencecount with Pattern (ngram) (2)" ; test(ppmodel.occurrencecount(ngram2), 2);
        string querystring = "or not to {*} .";
        Pattern skipgram = classencoder.buildpattern(querystring, false);
        PatternPointer pskipgram = PatternPointer(skipgram);
        cerr << "Querying occurrencecount with PatternPointer (skipgram)" ; test(ppmodel.occurrencecount(pskipgram), 4);
        cerr << "Querying occurrencecount with Pattern (skipgram)" ; test(ppmodel.occurrencecount(skipgram), 4);
        ppmodel.report(&std::cerr);
        cerr << endl;
        cerr << endl;
        ppmodel.histogram(&std::cerr);

        cerr << "Training reference PatternModel" << endl;
        refmodel.train(infilename, options);
        cerr << "Verifying equal size" ; test(ppmodel.size(), refmodel.size());
        cerr << "Verifying equal token count" ; test(ppmodel.tokens(), refmodel.tokens());
        cerr << "Verifying equal type count" ; test(ppmodel.types(), refmodel.types());

        string ppmodelfile = "/tmp/indexedpatternpointermodel.tmp";

        cerr << "Writing PatternPointerModel to file" << endl;
        ppmodel.write(ppmodelfile);

        cerr << "Reading PatternPointerModel from file" << endl;
        IndexedPatternPointerModel<> ppmodel2 = IndexedPatternPointerModel<>(ppmodelfile, options);
        cerr << "Verifying equal size" ; test(ppmodel2.size(), ppmodel.size());
        cerr << "Verifying equal token count" ; test(ppmodel2.tokens(), ppmodel.tokens());
        cerr << "Verifying equal type count" ; test(ppmodel2.types(), ppmodel.types());
    }
    {
        cerr << endl << "************************** Ordered set test (std::set<Pattern>) ***************************************" << endl << endl;
        std::set<Pattern> patternset;
        string classfile = "/tmp/colibritest";
        ClassEncoder encoder(classfile);
        ClassDecoder decoder(classfile);

        cerr << "Encoding and adding ngram" << endl;
        string querystring = "To be or not to be";
        Pattern ngram = encoder.buildpattern(querystring, true);
        patternset.insert(ngram);

        cerr << "Encoding and adding ngram 2" << endl;
        string querystring2 = "To be";
        Pattern ngram2 = encoder.buildpattern(querystring2, true);
        patternset.insert(ngram2);

        cerr << "Encoding and adding skipgram" << endl;
        string querystring3 = "To {*} or {*} to be";
        Pattern skipgram = encoder.buildpattern(querystring3, true);
        patternset.insert(skipgram);

        cerr << "Checking set size: "; test(patternset.size(),3);

        std::set<Pattern>::iterator iter = patternset.begin();
        cerr << "Set integrity #1: "; test(iter->tostring(decoder),querystring3);
        iter++;
        cerr << "Set integrity #2: "; test(iter->tostring(decoder),querystring2);
        iter++;
        cerr << "Set integrity #3: "; test(iter->tostring(decoder),querystring);

    }


}
