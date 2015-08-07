#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
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
        cerr << string( ".... ok") << endl;
    } else {
        cerr << ".... FAILED!" << endl;
        exit(2);
    }
}

void test(int value , int ref) {
    if (value != ref) {
        cerr << value << " .... FAILED! expected " << ref << endl;
        exit(2);
    } else {
        cerr << value << " .... ok" << endl;
    }
}


int main( int argc, char *argv[] ) {
	//string model = argv[1];
	//string classfile = argv[1];
    

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
        f << "5\tbe\n6\tTo\n7\tto\n8\tor\n9\tnot\n73477272\tblah\n";            
        f.close();

        
            
        cerr << "Loading class decoder" << endl;
        ClassDecoder classdecoder = ClassDecoder(classfile);
        cerr << "  Number of classes: " << classdecoder.size() << endl;

        cerr << "Loading class encoder" << endl;
        ClassEncoder encoder = ClassEncoder(classfile);
        
        
        cerr << "Encoding n-gram from string input" << endl;
        string querystring = "To be or not to be";
        Pattern ngram = encoder.buildpattern(querystring, true); 	

        cerr << "Ngram #1: " << ngram.decode(classdecoder) << endl;
        cerr << "Size (n): "; test(ngram.n(), 6);
        cerr << "Bytesize: " << (int) ngram.bytesize() << endl;
        cerr << "Category==ngram: " << (int) (ngram.category() == NGRAM) << endl;
        cerr << "Hash: " << ngram.hash() << endl;
        cerr << "Raw: " << endl;
        ngram.out();

        
        {
            ClassDecoder classdecoder2 = ClassDecoder();
            cerr << "Decoding beginpattern: " << BEGINPATTERN.tostring(classdecoder2) << endl;
            cerr << "Decoding endpattern: " << ENDPATTERN.tostring(classdecoder2) << endl;

            cerr << "Decoding non-existing classes (should return UNKNOWNS):" << ngram.tostring(classdecoder2) << endl;

        }
        cerr << "----------------------------------------------------" << endl;
        cerr << "Copy constructor" << endl;
        Pattern ngramcopy = Pattern(ngram);
        cerr << endl;
        

        cerr << "Slice constructor, specific subngram" << endl;
        Pattern ngram2 = Pattern(ngram, 2, 2);
        
        cerr << "Ngram: " << ngram2.decode(classdecoder) << endl;
        cerr << "N: " << (int) ngram2.n() << endl;
        cerr << "Bytesize: " << (int) ngram2.bytesize() << endl;

        cerr << "Empty/null pattern" << endl;
        Pattern emptypattern;
        cerr << "N: " << (int) emptypattern.n() << endl;
        cerr << "Bytesize: " << (int) emptypattern.bytesize() << endl;


        cerr << "----------------------------------------------------" << endl;
        cerr << "Tokens of ngram #1: " << endl;

        
        vector<Pattern> tokens;
        ngram.ngrams(tokens,1);
        cerr << "Testing correct size "; test(tokens.size() == 6);
        for (vector<Pattern>::iterator iter2 = tokens.begin(); iter2 != tokens.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cerr << "'" << subngram.decode(classdecoder) << "'" << endl;
        }

        cerr << "----------------------------------------------------" << endl;
        cerr << "Subgrams of ngram #1: " << endl;

        
        vector<Pattern> subngrams;
        ngram.subngrams(subngrams);
        for (vector<Pattern>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cerr << "'" << subngram.decode(classdecoder) << "'" << endl;
        }
        
        cerr << "Below tests should all return 1" << endl;
        
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
        cerr << "Reverse of ngram #1: " << endl;

        Pattern revngram = ngram.reverse();
        cerr << "Reverse ngram: " << revngram.decode(classdecoder) << endl;
        cerr << "N: " << (int) revngram.n() << endl;


        cerr << "----------------------------------------------------" << endl;
        cerr << "Pattern Pointer tests" << endl;
        PatternPointer pngram = PatternPointer(&ngram);
        Pattern derefngram = Pattern(pngram);
        cerr << "Testing equivalence after pointer construction and derefence"; test(ngram == derefngram);

        cerr << "Tokens of ngram #1 (as patternpointers): " << endl;
        vector<PatternPointer> ptokens;
        ngram.ngrams(ptokens,1);
        cerr << "Testing correct size "; test(ptokens.size() == 6);
        for (vector<PatternPointer>::iterator iter2 = ptokens.begin(); iter2 != ptokens.end(); iter2++) {                
            const PatternPointer subngram = *iter2;
            cerr << "'" << subngram.decode(classdecoder) << "'" << endl;
        }

        cerr << "Subgrams of ngram #1 (as patternpointers): " << endl;
        vector<PatternPointer> psubngrams;
        pngram.subngrams(psubngrams);
        for (vector<PatternPointer>::iterator iter2 = psubngrams.begin(); iter2 != psubngrams.end(); iter2++) {                
            const PatternPointer psubngram = *iter2;
            cerr << "'" << psubngram.tostring(classdecoder) << "'" << endl;
        }


        cerr << "Subgrams of ngram #1 (as patternpointers, from pattern): " << endl;
        vector<PatternPointer> psubngrams2;
        ngram.subngrams(psubngrams2);
        for (vector<PatternPointer>::iterator iter2 = psubngrams2.begin(); iter2 != psubngrams2.end(); iter2++) {                
            const PatternPointer psubngram2 = *iter2;
            cerr << "'" << psubngram2.tostring(classdecoder) << "'" << endl;
        }


        cerr << "----------------------------------------------------" << endl;
        cerr << "Ngram addition: " << endl;
        const Pattern ngrambegin = Pattern(ngram,0,2);
        Pattern ngramconc = ngrambegin; 
        ngramconc = ngramconc + subngram;
        ngramconc = ngramconc + subngram3;
        cerr << "Ngram: " << ngramconc.decode(classdecoder) << endl;
        cerr << "N: " << (int) ngramconc.n() << endl;
        cerr << "Bytesize: " << (int) ngramconc.bytesize() << endl;
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
        cerr << "Encoding skip-gram from string input" << endl;
        string querystring2 = "To {*1*} or {*1*} to be";

        Pattern skipgram = encoder.buildpattern(querystring2, true);
        
        cerr << "Skipgram: " << skipgram.decode(classdecoder) << endl;
        cerr << "N: " << (int) skipgram.n() << endl;
        cerr << "Bytesize: " << (int) skipgram.bytesize() << endl;
        cerr << "Category==skipgram? " ; test(skipgram.category() == SKIPGRAM) ;
        cerr << "Skipcount==2? " ; test(skipgram.skipcount() == 2) ;
    
        cerr << "Parts: " << endl;
        vector<Pattern> parts;
        skipgram.parts(parts);
        for (vector<Pattern>::iterator iter2 = parts.begin(); iter2 != parts.end(); iter2++) {                
            const Pattern part = *iter2;
            cerr << "'" << part.decode(classdecoder) << "'" << endl;
        }


        cerr << "Gaps: " << endl;
        std::vector<std::pair<int,int> > gapcontainer;
        skipgram.gaps(gapcontainer);
        skipgram.parts(parts);
        for (vector<std::pair<int,int>>::iterator iter2 = gapcontainer.begin(); iter2 != gapcontainer.end(); iter2++) {                
            cerr << iter2->first << "-" << iter2->second << endl;
        }
        
        Pattern revskipgram = skipgram.reverse();
        cerr << "Reverse skipgram: " << revskipgram.decode(classdecoder) << endl;
        cerr << "N: " << (int) revskipgram.n() << endl;
        
        cerr << "----------------------------------------------------" << endl;

        cerr << "Extracting skip content based on skip gram and full instance" << endl;
        Pattern skipcontent = skipgram.extractskipcontent(ngram);
        
        cerr << "Skipcontent: " << skipcontent.decode(classdecoder) << endl;
        cerr << "N: " << (int) skipcontent.n() << endl;
        cerr << "Bytesize: " << (int) skipcontent.bytesize() << endl;
        
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
        string querystring4 = "be {*1*} not";
        
        Pattern skipgraminv2 = encoder.buildpattern(querystring4, true);
        
        cerr << "Skipgram: " << skipgraminv2.decode(classdecoder) << endl;
        cerr << "N: " << (int) skipgraminv2.n() << endl;
        cerr << "Bytesize: " << (int) skipgraminv2.bytesize() << endl;	
        
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgraminv2.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cerr << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    
        cerr << "----------------------------------------------------" << endl;
        /*cerr << "Re-instantiating skipgram with skipcontent" << endl;
        Pattern rengram = skipgram.instantiate(&skipgraminv2, parts2);
        
        cerr << "Ngram: " << rengram.decode(classdecoder) << endl;
        cerr << "N: " << (int) rengram.n() << endl;
        cerr << "Size: " << (int) rengram.size() << endl;*/
        
        cerr << "----------------------------------------------------" << endl;
        string querystring5 = "be {*1*} not {*2*} be";
        Pattern skipgram5 = encoder.buildpattern(querystring5, true);
        cerr << skipgram5.decode(classdecoder) << endl;
        
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram5.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cerr << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    	 
        cerr << "getgaps: " << endl;
        vector<pair<int,int> > gaps;
        skipgram5.gaps(gaps);
        for (vector<pair<int,int >>::iterator iter2 = gaps.begin(); iter2 != gaps.end(); iter2++) {
            cerr << iter2->first << ':' << iter2->second << endl; 
        }
        cerr << "getparts: " << endl;
        vector<pair<int,int> > p;
        skipgram5.parts(p);
        for (vector<pair<int,int >>::iterator iter2 = p.begin(); iter2 != p.end(); iter2++) {
            cerr << iter2->first << ':' << iter2->second << endl; 
        }	
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
        
        cerr << "slice(5,1): " << endl;
        Pattern token = Pattern(skipgram5,5,1);
        cerr << token.decode(classdecoder) << endl;
        

        cerr << "slice(2,4): " << endl;	    
        Pattern s5slice2 = Pattern(skipgram5,2,4);
        cerr << s5slice2.decode(classdecoder) << endl;
        cerr << endl; 
        
        
        cerr << "slice(1,3): " << endl;	    
        Pattern s5slice3 = Pattern(skipgram5,1,3);
        cerr << s5slice3.decode(classdecoder) << endl;
        cerr << endl;     

        
        cerr << "----------------------------------------------------" << endl;
        string querystring6 = "be {*1*} not";
        Pattern skipgram6 = encoder.buildpattern(querystring6, true);
        cerr << skipgram6.decode(classdecoder) << endl;
        skipgram6.out();
        
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram6.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cerr << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    	 
        cerr << "Gaps: " << endl;
        vector<pair<int,int> > gaps6;
        skipgram6.gaps(gaps6);
        for (vector<pair<int,int >>::iterator iter2 = gaps6.begin(); iter2 != gaps6.end(); iter2++) {
            cerr << iter2->first << ':' << iter2->second << endl; 
        }
        cerr << "parts: " << endl;
        vector<pair<int,int> > p6;
        skipgram6.parts(p6);
        for (vector<pair<int,int >>::iterator iter2 = p6.begin(); iter2 != p6.end(); iter2++) {
            cerr << iter2->first << ':' << iter2->second << endl; 
        }	



        cerr << "----------------------------------------------------" << endl;
        string querystring7 = "blah {*1*} or {*2*} blah";
        Pattern skipgram7 = encoder.buildpattern(querystring7, true);
        cerr << skipgram7.decode(classdecoder) << endl;
        
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram7.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cerr << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    	 
        cerr << "gaps: " << endl;
        gaps.clear();
        skipgram7.gaps(gaps);
        for (vector<pair<int,int >>::iterator iter2 = gaps.begin(); iter2 != gaps.end(); iter2++) {
            cerr << iter2->first << ':' << iter2->second << endl; 
        }
        cerr << "parts: " << endl;
        p.clear();
        skipgram7.parts(p);
        for (vector<pair<int,int >>::iterator iter2 = p.begin(); iter2 != p.end(); iter2++) {
            cerr << iter2->first << ':' << iter2->second << endl; 
        }	
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
        
        cerr << "slice(5,1): " << endl;
        token = Pattern(skipgram7,5,1);
        cerr << token.decode(classdecoder) << endl;
        
        cerr << "----------------------------------------------------" << endl;
        string querystring8 = "{*1*} or blah {*2*}";
        Pattern skipgram8 = encoder.buildpattern(querystring8, true);
        cerr << skipgram8.decode(classdecoder) << endl;
        
            
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram8.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cerr << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    	 
        cerr << "gaps: " << endl;
        gaps.clear();
        skipgram8.gaps(gaps); //TODO: FIX!!
        for (vector<pair<int,int >>::iterator iter2 = gaps.begin(); iter2 != gaps.end(); iter2++) {
            cerr << iter2->first << ':' << iter2->second << endl; 
        }
        cerr << "parts: " << endl;
        p.clear();
        skipgram8.parts(p);
        for (vector<pair<int,int >>::iterator iter2 = p.begin(); iter2 != p.end(); iter2++) {
            cerr << iter2->first << ':' << iter2->second << endl; 
        }	
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
        
        cerr << "slice(2,1): " << endl;
        token = Pattern(skipgram8,2,1);
        cerr << token.decode(classdecoder) << endl;	   
        cerr << endl;
        
        cerr << "slice(0,4): " << endl;	    
        Pattern s8slice1 = Pattern(skipgram8,0,4);
        cerr << s8slice1.decode(classdecoder) << endl;
        cerr << endl; 

        cerr << "slice(1,2): " << endl;	    
        Pattern s8slice2 = Pattern(skipgram8,1,2);
        cerr << s8slice2.decode(classdecoder) << endl;
        cerr << endl; 

        cerr << "slice(1,4): " << endl;	    
        Pattern s8slice3 = Pattern(skipgram8,1,4);
        cerr << s8slice3.decode(classdecoder) << endl;
        cerr << endl;


        cerr << "----------------------------------------------------" << endl;
        Pattern dynskipgram5 = skipgram5.toflexgram();
        cerr << "Converting skipgram '" << querystring5 << "' to flexgram:" << endl;	    
        cerr << dynskipgram5.decode(classdecoder) << endl;
        cerr << "Size (n): " << (int) dynskipgram5.n() << endl; //== size()
        cerr << "Bytesize: " << (int) dynskipgram5.bytesize() << endl;
        cerr << "Category==flexgram: "; test(dynskipgram5.category() == FLEXGRAM);
        cerr << "Raw" << endl;
        dynskipgram5.out();
        cerr << "Parts: " << endl;
        vector<Pattern> dynparts;
        dynskipgram5.parts(dynparts);
        for (vector<Pattern>::iterator iter2 = dynparts.begin(); iter2 != dynparts.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cerr << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    	 

        cerr << "----------------------------------------------------" << endl;
        
        cerr << "Writing patterns to file: " << endl;
        ofstream * out = new ofstream("/tmp/patterns.tmp");
        ngram.write(out);
        skipgram.write(out);
        out->close();

        cerr << "Reading from file" << endl;
        ifstream * in = new ifstream("/tmp/patterns.tmp");
        Pattern ngram_read = Pattern(in);
        Pattern skipgram_read = Pattern(in);
        in->close();
        cerr << "Integrity check for ngram? " ; test(ngram == ngram_read) ;
        cerr << "Integrity check for skipgram? " ; test(skipgram == skipgram_read) ;


        
        PatternMap<uint32_t> map1;
        map1[ngram] = 1;
        map1[dynskipgram5] = 2;
        cerr << "Integrity for PatternMap" ; test(map1.size() , 2);
        cerr << "Integrity for PatternMap" ; test(map1[ngram], 1);
        cerr << "Integrity for PatternMap" ; test(map1[dynskipgram5] , 2);
        cerr << "Saving patternmap" << endl; 
        map1.write("/tmp/patternmap.tmp");


        PatternMap<uint32_t> map2;
        cerr << "Loading patternmap" << endl; 
        map2.read("/tmp/patternmap.tmp");
        cerr << "Integrity for PatternMap" ; test(map2.size() , 2);
        cerr << "Integrity for PatternMap" ; test(map2[ngram] , 1);
        cerr << "Integrity for PatternMap" ; test(map2[dynskipgram5] , 2);

        AlignedPatternMap<uint32_t> amap1;
        amap1[ngram][ngram] = 1;
        amap1[dynskipgram5][dynskipgram5] = 2;
        cerr << "Integrity for AlignedPatternMap" ; test(amap1.size(), 2);
        cerr << "Integrity for AlignedPatternMap" ; test(amap1[ngram][ngram], 1);
        cerr << "Integrity for AlignedPatternMap" ; test(amap1[dynskipgram5][dynskipgram5], 2);
        cerr << "Saving AlignedPatternMap" << endl; 
        amap1.write("/tmp/alignedpatternmap.tmp");


        AlignedPatternMap<uint32_t> amap2;
        cerr << "Loading alignedpatternmap" << endl; 
        amap2.read("/tmp/alignedpatternmap.tmp");
        cerr << "Integrity for AlignedPatternMap" ; test(amap2.size() , 2);
        cerr << "Integrity for AlignedPatternMap" ; test(amap2[ngram][ngram] , 1);
        cerr << "Integrity for AlignedPatternMap" ; test(amap2[dynskipgram5][dynskipgram5] , 2);
        }
        
        { 

        string rawcorpusfile = "/tmp/hamlet.txt";
        ofstream * out = new ofstream(rawcorpusfile);
        *out << string(poem);
        out->close();

        cerr << "Class encoding corpus..." << endl;
        system("colibri-classencode /tmp/hamlet.txt");



        cerr << "Class decoding corpus..." << endl;
        system("colibri-classdecode -c /tmp/hamlet.colibri.cls -f /tmp/hamlet.colibri.dat");



        cerr << "Loading class decoders/encoders" << endl;
        const string classfile = "/tmp/hamlet.colibri.cls";
        ClassDecoder classdecoder = ClassDecoder(classfile);
        ClassEncoder classencoder = ClassEncoder(classfile);

        cerr << "Loading corpus as IndexedCorpus" << endl;
        IndexedCorpus corpus = IndexedCorpus("/tmp/hamlet.colibri.dat");
        cerr << "Total number of tokens: " << corpus.size() << endl;
        Pattern firstword = corpus.getpattern(IndexReference(1,0),1);
        cerr << "First word:  " << firstword.tostring(classdecoder) << endl;
        Pattern needle = classencoder.buildpattern(string("fair Ophelia"));
        vector<IndexReference> matches = corpus.findpattern(needle);
        if (matches.empty()) {
            cerr << "*************** ERROR: No matches found ************" << endl;
            exit(2);
        } else {
            cerr << "'fair Ophelia' found at " << matches[0].tostring() << endl;
        }
        cerr << endl;


        PatternModelOptions options;
        options.DOREVERSEINDEX = true;
        options.DOSKIPGRAMS_EXHAUSTIVE = true;
        options.DOSKIPGRAMS = false ;

        cerr << "Building unindexed model" << endl;
        PatternModel<uint32_t> unindexedmodel;

        cerr << endl;
        std::string infilename = "/tmp/hamlet.colibri.dat";
        std::string outputfilename = "/tmp/data.colibri.patternmodel";
        unindexedmodel.train(infilename, options);
        cerr << "Found " << unindexedmodel.size() << " patterns, " << unindexedmodel.types() << " types, " << unindexedmodel.tokens() << " tokens" << endl;
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
        cerr << "Writing to file" << endl;
        unindexedmodel.write(outputfilename);


        options.DEBUG = true;
        cerr << "Reading from file" << endl;
        PatternModel<uint32_t> unindexedmodel2 = PatternModel<uint32_t>(outputfilename, options);
        options.DEBUG = false;
        cerr << "Outputting report again" << endl;
        unindexedmodel2.report(&std::cerr);
        cerr << "Equal tokens? " ; test(unindexedmodel.tokens() == unindexedmodel2.tokens() );
        cerr << "Equal types? " ; test(unindexedmodel.types() == unindexedmodel2.types() );
        cerr << "Equal size? " ; test(unindexedmodel.size() == unindexedmodel2.size() );
        cerr << "Testing unindexedmodel2.has()"; test( unindexedmodel2.has(ngram) );
        cerr << "Testing unindexedmodel2.occurrencecount()"; test( unindexedmodel2.occurrencecount(ngram),6 );



        cerr << endl;
        options.DOSKIPGRAMS_EXHAUSTIVE = false;
        options.DOSKIPGRAMS = true ;
        cerr << "Building indexed model" << endl;
        IndexedPatternModel<> indexedmodel;
        indexedmodel.train(infilename, options);
        cerr << "Found " << indexedmodel.size() << " patterns, " << indexedmodel.types() << " types, " << indexedmodel.tokens() << " tokens" << endl;
        indexedmodel.print(&std::cerr, classdecoder);
        cerr << endl;
        indexedmodel.report(&std::cerr);
        cerr << endl;
        indexedmodel.histogram(&std::cerr);

        cerr << "Iterating over all patterns" << endl;
        for (IndexedPatternModel<>::iterator iter = indexedmodel.begin(); iter != indexedmodel.end(); iter++) {
            const Pattern pattern = iter->first;
            const IndexedData data = iter->second;
            cerr << pattern.tostring(classdecoder) << endl;
        }


        cerr << "Writing to file" << endl;
        indexedmodel.write(outputfilename);
        cerr << "Reading from file" << endl;
        IndexedPatternModel<> indexedmodel2 = IndexedPatternModel<>(outputfilename, options);
        cerr << "Outputting report again" << endl;
        indexedmodel2.report(&std::cerr);


        cerr << "Reading indexed model as unindexed" << endl;
        PatternModel<uint32_t> indexedasunindexedmodel = PatternModel<uint32_t>(outputfilename, options);

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
        indexedmodel.outputrelations(skipgram, relations6, classdecoder, &cerr,"INSTANTIATED-BY");


        cerr << "All relations for  " << querystring3 << " in one go" << endl;
        indexedmodel.outputrelations(skipgram, classdecoder, &cerr);
        
        cerr << endl;

        cerr << "Building indexed model with mintokens=1" << endl;
        options.MINTOKENS = 1;
        IndexedPatternModel<> indexedmodel1;
        indexedmodel1.train(infilename, options);
        cerr << "Found " << indexedmodel1.size() << " patterns, " << indexedmodel1.types() << " types, " << indexedmodel1.tokens() << " tokens" << endl;


        cerr << "Computing flexgrams out of skipgrams" << endl;
        int foundflex = indexedmodel.computeflexgrams_fromskipgrams();
        cerr << "Found " << foundflex << " flexgrams" << endl;
        cerr << "outputting all" << endl;
        indexedmodel.print(&std::cerr, classdecoder);
        cerr << "Outputting report again, now with flexgrams" << endl;
        indexedmodel.report(&std::cerr);


        options.DOREVERSEINDEX = false;
        options.DOSKIPGRAMS_EXHAUSTIVE = false;
        options.DOSKIPGRAMS = false ;
        cerr << "Building unindexed model using ordered patternmap" << endl;
        PatternModel<uint32_t,BaseValueHandler<uint32_t>,HashOrderedPatternMap<uint32_t,BaseValueHandler<uint32_t>>> unindexedmodel3;

        cerr << endl;
        unindexedmodel3.train(infilename, options);
        cerr << "Found " << unindexedmodel3.size() << " patterns, " << unindexedmodel3.types() << " types, " << unindexedmodel3.tokens() << " tokens" << endl;


        cerr << "Testing low-level PatternSet" << endl;
        PatternSet<uint32_t> set;
        for (IndexedPatternModel<>::iterator iter = indexedmodel.begin(); iter != indexedmodel.end(); iter++) {
            set.insert(iter->first);
        }

        //double inserts:
        for (IndexedPatternModel<>::iterator iter = indexedmodel.begin(); iter != indexedmodel.end(); iter++) {
            set.insert(iter->first);
        }


    }
    {
        const string classfile = "/tmp/hamlet.colibri.cls";
        ClassDecoder classdecoder = ClassDecoder(classfile);

        cerr << "Loading corpus as IndexedCorpus" << endl;
        IndexedCorpus corpus = IndexedCorpus("/tmp/hamlet.colibri.dat");


        cerr << "Building pattern model, passing corpus as reverse index" << endl;
        PatternModel<uint32_t> model = PatternModel<uint32_t>(&corpus);

        PatternModelOptions options;
        options.MINLENGTH = 3;
        options.MAXLENGTH = 5;
        options.MINTOKENS = 1;

        model.train("/tmp/hamlet.colibri.dat", options);
        
        for (IndexedCorpus::iterator iter = corpus.begin(); iter != corpus.end(); iter++) {
            cerr << iter->ref.tostring() << endl;
            vector<Pattern> patterns = model.getreverseindex(iter->ref);
            for (Pattern p : patterns) {
                cerr << p.tostring(classdecoder) << endl;
            }
        }

    }
    {
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
        }

    }

    {
        ClassEncoder classencoder = ClassEncoder();
        stringstream sscls;
        sscls << poem;
        unordered_map<string,int> freqlist;
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


}
