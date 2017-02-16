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
#include <sys/time.h>

using namespace std;


///==================== BEGIN MEMORY PROFILING CODE ============

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif


/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
size_t getPeakRSS( )
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
    return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
    /* AIX and Solaris ------------------------------------------ */
    struct psinfo psinfo;
    int fd = -1;
    if ( (fd = open( "/proc/self/psinfo", O_RDONLY )) == -1 )
        return (size_t)0L;      /* Can't open? */
    if ( read( fd, &psinfo, sizeof(psinfo) ) != sizeof(psinfo) )
    {
        close( fd );
        return (size_t)0L;      /* Can't read? */
    }
    close( fd );
    return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
    /* BSD, Linux, and OSX -------------------------------------- */
    struct rusage rusage;
    getrusage( RUSAGE_SELF, &rusage );
#if defined(__APPLE__) && defined(__MACH__)
    return (size_t)rusage.ru_maxrss;
#else
    return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else
    /* Unknown OS ----------------------------------------------- */
    return (size_t)0L;          /* Unsupported. */
#endif
}


/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS( )
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
    return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
    /* OSX ------------------------------------------------------ */
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
        (task_info_t)&info, &infoCount ) != KERN_SUCCESS )
        return (size_t)0L;      /* Can't access? */
    return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
    /* Linux ---------------------------------------------------- */
    long rss = 0L;
    FILE* fp = NULL;
    if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
        return (size_t)0L;      /* Can't open? */
    if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
    {
        fclose( fp );
        return (size_t)0L;      /* Can't read? */
    }
    fclose( fp );
    return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);

#else
    /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
    return (size_t)0L;          /* Unsupported. */
#endif
}

//===== END MEMORY PROFILING CODE ========




struct Measurement {
    clock_t begintime;
    double duration;
    size_t beginmem;
    size_t endmem;
};

Measurement begin(const string &  title) {
    cout << "====== " << title << "======" << endl;
    Measurement m;
    m.begintime = clock();
	m.beginmem = getCurrentRSS();
	return m;
}

void end(Measurement& m) {
    m.duration = (clock() - m.begintime) / (double) CLOCKS_PER_SEC;
	m.endmem = getCurrentRSS();
	const size_t peakmem = getPeakRSS();
	const double peak = peakmem / 1024.0 / 1024.0;
	const size_t memdiff = m.endmem-m.beginmem;
    const double mem = memdiff / 1024.0 / 1024.0;
    cout << "---> time: " <<  m.duration << " s res: " << mem << " MB peak: " << peak << " MB" << endl << endl;
}

void naivetrain(string filename, std::unordered_map<string,uint32_t> & model, int maxlength) {
	   ifstream IN;
	   IN.open( filename.c_str() );
       if (!(IN)) {
           cerr << "ERROR: File does not exist: " << filename << endl;
           exit(3);
       }
       while (IN.good()) {
          string line;
          getline(IN, line);
          vector<string> tokens;
          split(line, ' ', tokens);
          for ( size_t i = 0; i < tokens.size(); i++) {
            string token = tokens[i];
            for (int n = 1; n <= maxlength && i+n <= tokens.size(); n++) {
	      if (n > 1) token += " " + tokens[i+n-1];
	      model[token] += 1;
            }
          }
       }
}


int main( int argc, char *argv[] ) {
    if (argc != 3) {
        cerr<<"Syntax: colibri-benchmarks textfile testnr"<<endl;
        exit(2);
    }
    const string textfile = argv[1];
	const string datafile = textfile + ".colibri.dat";
	const string classfile = textfile + ".colibri.cls";

	const int testnr = atoi(argv[2]);



    ClassEncoder classencoder;

	if (testnr == 0) {
		{
			Measurement m = begin(string("0 - Building class encoding"));
			classencoder.build(textfile);
			end(m);
		}

		{
			Measurement m = begin(string("0 - Encoding corpus"));
			classencoder.encodefile(textfile,datafile,false);
			end(m);
			classencoder.save(classfile);
		}
	}

	IndexedCorpus corpus;

	if (testnr == 3) {
        Measurement m = begin(string("3 - Loading corpus data (IndexedCorpus)"));
		corpus.load(datafile);
		end(m);
	} else {
		corpus.load(datafile);
	}

    if (testnr == 4) {
        Measurement m = begin(string("4 - Training unindexed PatternModel from file: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = false;
        PatternModel<uint32_t> model;
        model.train(datafile, options);
        end(m);
    }

    if (testnr == 5) {
        Measurement m = begin(string("5 - Training unindexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = false;
        PatternModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }


    if (testnr == 6) {
        Measurement m = begin(string("6 - Training unindexed PatternModel from file: threshold 2, up to 8-grams, with skipgrams (exhaustive)"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS_EXHAUSTIVE = true;
        PatternModel<uint32_t> model;
        model.train(datafile, options);
        end(m);
    }

    if (testnr == 7) {
        Measurement m = begin(string("7 - Training unindexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, with skipgrams (exhaustive)"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS_EXHAUSTIVE = true;
        PatternModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

	if (testnr == 8) {
        Measurement m = begin(string("8 - Training unindexed PatternModel from file: threshold 1, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 1; options.MAXLENGTH = 8;
        PatternModel<uint32_t> model;
        model.train(datafile, options);
        end(m);
    }

	if (testnr == 9) {
        Measurement m = begin(string("9 - Training unindexed PatternPointerModel from preloaded corpus: threshold 1, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 1; options.MAXLENGTH = 8;
        PatternPointerModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

	if (testnr == 10) {
        Measurement m = begin(string("10 - Naive C++ implementation (std::unordered_map<string,uint32_t>): threshold 1, up to 8-grams, no skipgrams"));
        std::unordered_map<string,uint32_t> model;
        naivetrain(textfile, model,8);
        end(m);
    }

	if (testnr == 11) {
        Measurement m = begin(string("11 - Training unindexed PatternModel from file: threshold 1, up to 3-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 1; options.MAXLENGTH = 3;
        PatternModel<uint32_t> model;
        model.train(datafile, options);
        end(m);
    }

    if (testnr == 12) {
        Measurement m = begin(string("12 - Training indexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = false;
        IndexedPatternModel<> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

    if (testnr == 13) {
        Measurement m = begin(string("13 - Training indexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, with skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = true;
        IndexedPatternModel<> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

    if (testnr == 14) {
        Measurement m = begin(string("14 - Training unindexed PatternPointerModel from preloaded corpus: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8;
        PatternPointerModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

    if (testnr == 15) {
        Measurement m = begin(string("15 - Training unindexed PatternPointerModel from preloaded corpus: threshold 2, up to 8-grams, with skipgrams (exhaustive)"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS_EXHAUSTIVE = true;
        PatternPointerModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

    if (testnr == 16) {
        Measurement m = begin(string("16 - Training indexed PatternPointerModel from preloaded corpus: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8;
        IndexedPatternPointerModel<> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

    if (testnr == 17) {
        Measurement m = begin(string("17 - Training unindexed PatternModel<HashOrderedPatternMap> from preloaded corpus: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = false;
        PatternModel<uint32_t,BaseValueHandler<uint32_t>,HashOrderedPatternMap<uint32_t>> model(&corpus);
        model.train(datafile, options);
        end(m);
    }



    if (testnr == 99) {
        Measurement m = begin(string("99 - Training indexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, with skipgrams, skip-type threshold 12"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = true; options.MINSKIPTYPES = 12;
        IndexedPatternModel<> model(&corpus);
        model.train(datafile, options);
        end(m);
    }
    if (testnr == 100) {
        Measurement m = begin(string("100 - Training indexed Pattern Pointer Model from preloaded corpus: threshold 2, up to 8-grams, with skipgrams, skip-type threshold 12"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = true; options.MINSKIPTYPES = 12;
        IndexedPatternPointerModel<> model(&corpus);
        model.train(datafile, options);
        end(m);
    }
    if (testnr == 101) {
        Measurement m = begin(string("101 - Training indexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, with skipgrams, skip-type threshold 12, occurence 25"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MINTOKENS_SKIPGRAMS=25, options.MAXLENGTH = 8; options.DOSKIPGRAMS = true; options.MINSKIPTYPES = 12;
        IndexedPatternModel<> model(&corpus);
        model.train(datafile, options);
        end(m);
    }
    if (testnr == 102) {
        Measurement m = begin(string("102 - Training indexed PatternModel Pointer from preloaded corpus: threshold 2, up to 8-grams, with skipgrams, skip-type threshold 12, occurence 25"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MINTOKENS_SKIPGRAMS=25, options.MAXLENGTH = 8; options.DOSKIPGRAMS = true; options.MINSKIPTYPES = 12;
        IndexedPatternPointerModel<> model(&corpus);
        model.train(datafile, options);
        end(m);
    }
    if (testnr == 103) {
        Measurement m = begin(string("103 - Training highly-thresholded unindexed pattern pointer model with skipgrams (t=10,l=4,y=20)"));
        PatternModelOptions options;
        options.MINTOKENS = 10; options.MAXLENGTH = 4; options.MINTOKENS_SKIPGRAMS = 20; options.DOSKIPGRAMS_EXHAUSTIVE = true;
        PatternPointerModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }


/*    if (testnr == 15) {
        Measurement m = begin(string("15 - Training unindexed PatternPointerModel<OrderedPatternPointerMap> from preloaded corpus: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8;
        PatternPointerModel<uint32_t,BaseValueHandler<uint32_t>,OrderedPatternPointerMap<uint32_t>> model(&corpus);
        model.train(datafile, options);
        end(m);
    }*/
}
