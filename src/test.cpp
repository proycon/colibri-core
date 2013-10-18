#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <unistd.h>
#include <classencoder.cpp>
#include <classdecoder.cpp>
#include <pattern.h>
#include <patternmodel.h>


using namespace std;

int main( int argc, char *argv[] ) {
	//string model = argv[1];
	//string classfile = argv[1];
	
    {
        string classfile = "/tmp/colibritest";    
        ofstream f;
        f.open(classfile.c_str(), ios::out);
        f << "2\tbe\n3\tTo\n4\tto\n5\tor\n6\tnot\n73477272\tblah\n";            
        f.close();

        
            
        cerr << "Loading class decoder" << endl;
        ClassDecoder classdecoder = ClassDecoder(classfile);
        cerr << "  Number of classes: " << classdecoder.size() << endl;

        cerr << "Loading class encoder" << endl;
        ClassEncoder encoder = ClassEncoder(classfile);
        
        
        cerr << "Encoding n-gram from string input" << endl;
        string querystring = "To be or not to be";
        Pattern ngram = encoder.input2pattern(querystring, true); 	

        cerr << "Ngram #1: " << ngram.decode(classdecoder) << endl;
        cerr << "Size (n): " << (int) ngram.n() << endl; //== size()
        cerr << "Bytesize: " << (int) ngram.bytesize() << endl;
        cerr << "Category==ngram: " << (int) (ngram.category() == NGRAM) << endl;
        cerr << "Hash: " << ngram.hash() << endl;
        cerr << "Raw: " << endl;
        ngram.out();

        
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
        cerr << "Subgrams of ngram #1: " << endl;

        
        vector<Pattern> subngrams;
        ngram.subngrams(subngrams);
        for (vector<Pattern>::iterator iter2 = subngrams.begin(); iter2 != subngrams.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cout << "'" << subngram.decode(classdecoder) << "'" << endl;
        }
        
        cerr << "Below tests should all return 1" << endl;
        
        string substring = "or not";     	
        Pattern subngram = encoder.input2pattern(substring, true);
        cerr << "Testing occurrence of substring 'or not'? " << (ngram.contains(subngram) == 1) << endl; 	
        
        string substring2 = "to not";     	
        Pattern subngram2 = encoder.input2pattern(substring2, true);
        cerr << "Testing non-occurrence of substring 'to not'? " << (ngram.contains(subngram2) == 0) << endl;
        
        string substring3 = "to be";     	
        Pattern subngram3 = encoder.input2pattern(substring3, true);
        cerr << "Testing occurrence of substring 'to be'? " << (ngram.contains(subngram3) == 1) << endl;    
        
        string substring4 = "or";     	
        Pattern subngram4 = encoder.input2pattern(substring4, true);
        cerr << "Testing occurrence of substring 'or'? " << (ngram.contains(subngram4) == 1) << endl;  
        
        
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
        cerr << "Adding to empty? " << (int) (emptypattern == ngramconc) << endl;
        cerr << "----------------------------------------------------" << endl;
        cerr << "Ngram comparison: " << endl;
        cerr << "Equality? " << (int) (ngramconc == ngram) << endl;
        cerr << "Non-equality? " << (int) (ngramconc != ngrambegin) << endl;
        cerr << "greater than? " << (int) (subngram2 > subngram3) << endl;
        cerr << "greater than? " << (int) (subngram3 > subngram4) << endl;
        cerr << "less than? " << (int) (subngram3 < subngram2) << endl;
        cerr << "Begin is less? " << (int) (ngrambegin < ngramconc) << endl;

        

        cerr << "----------------------------------------------------" << endl;
        cerr << "Encoding skip-gram from string input" << endl;
        string querystring2 = "To {*1*} or {*1*} to be";

        Pattern skipgram = encoder.input2pattern(querystring2, true);
        
        cerr << "Skipgram: " << skipgram.decode(classdecoder) << endl;
        cerr << "N: " << (int) skipgram.n() << endl;
        cerr << "Bytesize: " << (int) skipgram.bytesize() << endl;
        cerr << "Category==skipgram? " << (int) (skipgram.category() == FIXEDSKIPGRAM) << endl;
        cerr << "Skipcount==2? " << (int) (skipgram.skipcount() == 2) << endl;
    
        cerr << "Parts: " << endl;
        vector<Pattern> parts;
        skipgram.parts(parts);
        for (vector<Pattern>::iterator iter2 = parts.begin(); iter2 != parts.end(); iter2++) {                
            const Pattern part = *iter2;
            cout << "'" << part.decode(classdecoder) << "'" << endl;
        }


        cerr << "Gaps: " << endl;
        std::vector<std::pair<int,int> > gapcontainer;
        skipgram.gaps(gapcontainer);
        skipgram.parts(parts);
        for (vector<std::pair<int,int>>::iterator iter2 = gapcontainer.begin(); iter2 != gapcontainer.end(); iter2++) {                
            cout << iter2->first << "-" << iter2->second << endl;
        }
        
        
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
            cout << "'" << subngram->decode(classdecoder) << "'" << endl;
        }
    */  
        cerr << "----------------------------------------------------" << endl;

        cerr << "Encoding skip-gram from string input" << endl;
        string querystring4 = "be {*1*} not";
        
        Pattern skipgraminv2 = encoder.input2pattern(querystring4, true);
        
        cerr << "Skipgram: " << skipgraminv2.decode(classdecoder) << endl;
        cerr << "N: " << (int) skipgraminv2.n() << endl;
        cerr << "Bytesize: " << (int) skipgraminv2.bytesize() << endl;	
        
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgraminv2.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cout << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    
        cerr << "----------------------------------------------------" << endl;
        /*cerr << "Re-instantiating skipgram with skipcontent" << endl;
        Pattern rengram = skipgram.instantiate(&skipgraminv2, parts2);
        
        cerr << "Ngram: " << rengram.decode(classdecoder) << endl;
        cerr << "N: " << (int) rengram.n() << endl;
        cerr << "Size: " << (int) rengram.size() << endl;*/
        
        cerr << "----------------------------------------------------" << endl;
        string querystring5 = "be {*1*} not {*2*} be";
        Pattern skipgram5 = encoder.input2pattern(querystring5, true);
        cout << skipgram5.decode(classdecoder) << endl;
        
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram5.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cout << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    	 
        cerr << "getgaps: " << endl;
        vector<pair<int,int> > gaps;
        skipgram5.gaps(gaps);
        for (vector<pair<int,int >>::iterator iter2 = gaps.begin(); iter2 != gaps.end(); iter2++) {
            cout << iter2->first << ':' << iter2->second << endl; 
        }
        cerr << "getparts: " << endl;
        vector<pair<int,int> > p;
        skipgram5.parts(p);
        for (vector<pair<int,int >>::iterator iter2 = p.begin(); iter2 != p.end(); iter2++) {
            cout << iter2->first << ':' << iter2->second << endl; 
        }	
        /*cerr << "mask: " << endl;
        vector<bool> m;
        skipgram5.mask(m);
        for (vector<bool>::iterator iter2 = m.begin(); iter2 != m.end(); iter2++) {
            if (*iter2)  {
                cout << "1";
            } else {
                cout << "0";
            }
        }*/		
        cout << endl;
        
        cerr << "slice(5,1): " << endl;
        Pattern token = Pattern(skipgram5,5,1);
        cout << token.decode(classdecoder) << endl;
        

        cerr << "slice(2,4): " << endl;	    
        Pattern s5slice2 = Pattern(skipgram5,2,4);
        cout << s5slice2.decode(classdecoder) << endl;
        cerr << endl; 
        
        
        cerr << "slice(1,3): " << endl;	    
        Pattern s5slice3 = Pattern(skipgram5,1,3);
        cout << s5slice3.decode(classdecoder) << endl;
        cerr << endl;     

        
        cerr << "----------------------------------------------------" << endl;
        string querystring6 = "be {*1*} not";
        Pattern skipgram6 = encoder.input2pattern(querystring6, true);
        cout << skipgram6.decode(classdecoder) << endl;
        skipgram6.out();
        
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram6.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cout << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    	 
        cerr << "Gaps: " << endl;
        vector<pair<int,int> > gaps6;
        skipgram6.gaps(gaps6);
        for (vector<pair<int,int >>::iterator iter2 = gaps6.begin(); iter2 != gaps6.end(); iter2++) {
            cout << iter2->first << ':' << iter2->second << endl; 
        }
        cerr << "parts: " << endl;
        vector<pair<int,int> > p6;
        skipgram6.parts(p6);
        for (vector<pair<int,int >>::iterator iter2 = p6.begin(); iter2 != p6.end(); iter2++) {
            cout << iter2->first << ':' << iter2->second << endl; 
        }	



        cerr << "----------------------------------------------------" << endl;
        string querystring7 = "blah {*1*} or {*2*} blah";
        Pattern skipgram7 = encoder.input2pattern(querystring7, true);
        cout << skipgram7.decode(classdecoder) << endl;
        
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram7.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cout << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    	 
        cerr << "gaps: " << endl;
        gaps.clear();
        skipgram7.gaps(gaps);
        for (vector<pair<int,int >>::iterator iter2 = gaps.begin(); iter2 != gaps.end(); iter2++) {
            cout << iter2->first << ':' << iter2->second << endl; 
        }
        cerr << "parts: " << endl;
        p.clear();
        skipgram7.parts(p);
        for (vector<pair<int,int >>::iterator iter2 = p.begin(); iter2 != p.end(); iter2++) {
            cout << iter2->first << ':' << iter2->second << endl; 
        }	
        /*cerr << "mask: " << endl;
        m.clear();
        skipgram7.mask(m);
        for (vector<bool>::iterator iter2 = m.begin(); iter2 != m.end(); iter2++) {
            if (*iter2)  {
                cout << "1";
            } else {
                cout << "0";
            }
        }		*/
        cout << endl;
        
        cerr << "slice(5,1): " << endl;
        token = Pattern(skipgram7,5,1);
        cout << token.decode(classdecoder) << endl;
        
        cerr << "----------------------------------------------------" << endl;
        string querystring8 = "{*1*} or blah {*2*}";
        Pattern skipgram8 = encoder.input2pattern(querystring8, true);
        cout << skipgram8.decode(classdecoder) << endl;
        
            
        cerr << "Parts: " << endl;
        parts2.clear();
        skipgram8.parts(parts2);
        for (vector<Pattern>::iterator iter2 = parts2.begin(); iter2 != parts2.end(); iter2++) {                
            const Pattern subngram = *iter2;
            cout << "'" << subngram.decode(classdecoder) << "'" << endl;
        }    	 
        cerr << "gaps: " << endl;
        gaps.clear();
        skipgram8.gaps(gaps); //TODO: FIX!!
        for (vector<pair<int,int >>::iterator iter2 = gaps.begin(); iter2 != gaps.end(); iter2++) {
            cout << iter2->first << ':' << iter2->second << endl; 
        }
        cerr << "parts: " << endl;
        p.clear();
        skipgram8.parts(p);
        for (vector<pair<int,int >>::iterator iter2 = p.begin(); iter2 != p.end(); iter2++) {
            cout << iter2->first << ':' << iter2->second << endl; 
        }	
        /*cerr << "mask: " << endl;
        m.clear();
        skipgram8.mask(m);
        for (vector<bool>::iterator iter2 = m.begin(); iter2 != m.end(); iter2++) {
            if (*iter2)  {
                cout << "1";
            } else {
                cout << "0";
            }
        }*/		
        cout << endl;
        
        cerr << "slice(2,1): " << endl;
        token = Pattern(skipgram8,2,1);
        cout << token.decode(classdecoder) << endl;	   
        cerr << endl;
        
        cerr << "slice(0,4): " << endl;	    
        Pattern s8slice1 = Pattern(skipgram8,0,4);
        cout << s8slice1.decode(classdecoder) << endl;
        cerr << endl; 

        cerr << "slice(1,2): " << endl;	    
        Pattern s8slice2 = Pattern(skipgram8,1,2);
        cout << s8slice2.decode(classdecoder) << endl;
        cerr << endl; 

        cerr << "slice(1,4): " << endl;	    
        Pattern s8slice3 = Pattern(skipgram8,1,4);
        cout << s8slice3.decode(classdecoder) << endl;
        cerr << endl; 
        
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
        cerr << "Integrity check for ngram?" << (ngram == ngram_read) <<  endl;
        cerr << "Integrity check for skipgram?" << (skipgram == skipgram_read) <<  endl;




        }
        { 

        string rawcorpusfile = "/tmp/hamlet.txt";
        ofstream * out = new ofstream(rawcorpusfile);
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
        "To see or not to see .\n"
        "To pee or not to pee .\n"; //See that Shakespeare? I could be a poet too! 
        *out << string(poem);
        out->close();

        cerr << "Class encoding corpus..." << endl;
        system("colibri-classencode /tmp/hamlet.txt");



        cerr << "Class decoding corpus..." << endl;
        system("colibri-classdecode -c /tmp/hamlet.colibri.cls -f /tmp/hamlet.colibri.dat");


        PatternModelOptions options;
        options.DOREVERSEINDEX = true;
        options.DOFIXEDSKIPGRAMS = true;

        cerr << "Building unindexed model" << endl;
        PatternModel<uint32_t> unindexedmodel;
        const string classfile = "/tmp/hamlet.colibri.cls";
        ClassDecoder classdecoder = ClassDecoder(classfile);
        ClassEncoder classencoder = ClassEncoder(classfile);

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

        cerr << endl;
        cerr << "Writing to file" << endl;
        unindexedmodel.write(outputfilename);


        cerr << "Reading from file" << endl;
        PatternModel<uint32_t> unindexedmodel2 = PatternModel<uint32_t>(outputfilename, options);
        cerr << "Outputting report again" << endl;
        unindexedmodel2.report(&std::cerr);



        cerr << endl;
        cerr << "Building indexed model" << endl;
        IndexedPatternModel<> indexedmodel;
        indexedmodel.train(infilename, options);
        cerr << "Found " << indexedmodel.size() << " patterns, " << indexedmodel.types() << " types, " << indexedmodel.tokens() << " tokens" << endl;
        indexedmodel.print(&std::cerr, classdecoder);
        cerr << endl;
        indexedmodel.report(&std::cerr);
        cerr << endl;
        indexedmodel.histogram(&std::cerr);



        string querystring  = "To die , to sleep";
        Pattern ngram = classencoder.input2pattern(querystring, true); 	
        cerr << "Extracting subsumption relations for " << querystring << endl;

        std::map<Pattern,int> relations = indexedmodel.getsubsumed(ngram);
        indexedmodel.outputrelations(ngram, relations, classdecoder, &cerr,"SUBSUMES");


        string querystring2  = "not";
        cerr << "Extracting subsumption relations for " << querystring2 << endl;
        Pattern patternnot = classencoder.input2pattern(querystring2, true); 	
        std::map<Pattern,int> relations2 = indexedmodel.getsubsumes(patternnot);
        indexedmodel.outputrelations(patternnot, relations2, classdecoder, &cerr,"SUBSUMED-BY");


        std::map<Pattern,int> relations3 = indexedmodel.getleftneighbours(patternnot);
        indexedmodel.outputrelations(patternnot, relations3, classdecoder, &cerr,"RIGHT-OF");

        std::map<Pattern,int> relations4 = indexedmodel.getrightneighbours(patternnot);
        indexedmodel.outputrelations(patternnot, relations4, classdecoder, &cerr,"LEFT-OF");

    }
}

