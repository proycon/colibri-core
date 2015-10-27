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
#include <sys/resource.h>

using namespace std;

typedef struct {
    unsigned long size,resident,share,text,lib,data,dt;
} statm_t;

void readmem(statm_t& result)
{
  const char* statm_path = "/proc/self/statm";

  FILE *f = fopen(statm_path,"r");
  if(!f){
    perror(statm_path);
    abort();
  }
  if(7 != fscanf(f,"%ld %ld %ld %ld %ld %ld %ld",
    &result.size,&result.resident,&result.share,&result.text,&result.lib,&result.data,&result.dt))
  {
    perror(statm_path);
    abort();
  }
  fclose(f);
}

struct Measurement {
    clock_t begintime;
    double duration;
    statm_t beginmem;
    statm_t endmem;
};

Measurement begin(const string &  title) {
    cout << "====== " << title << "======" << endl;
    Measurement m;
    m.begintime = clock();
	readmem(m.beginmem);
	return m;
}

void end(Measurement& m) {
    m.duration = (clock() - m.begintime) / (double) CLOCKS_PER_SEC;
	readmem(m.endmem);
    const double mem = ((unsigned long long) (m.endmem.size - m.beginmem.size)) / (double) 1024;
    cout << "---> " <<  m.duration << " s " << mem << " MB" << endl << endl;
}

int main( int argc, char *argv[] ) {
    if (argc != 2) {
        cerr<<"Syntax: colibri-benchmarks textfile"<<endl;
        exit(2);
    }
    const string textfile = argv[1];
	const string datafile = textfile + ".colibri.dat";
	const string classfile = textfile + ".colibri.cls";

    ClassEncoder classencoder;
    {
        Measurement m = begin(string("Building class encoding"));
        classencoder.build(textfile);
        end(m);
    }
    {
        Measurement m = begin(string("Encoding corpus"));
        classencoder.encodefile(textfile,datafile,false);
        end(m);
    }
    classencoder.save(classfile);
    ClassDecoder classdecoder(classfile);

	IndexedCorpus corpus;
	{
        Measurement m = begin(string("Loading corpus data (IndexedCorpus)"));
		corpus.load(datafile);
		end(m);
	}

    {
        Measurement m = begin(string("Training unindexed PatternModel from file: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = false;
        PatternModel<uint32_t> model;
        model.train(datafile, options);
        end(m);
    }

    {
        Measurement m = begin(string("Training unindexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = false;
        PatternModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

    {
        Measurement m = begin(string("Training unindexed PatternModel from file: threshold 2, up to 8-grams, with skipgrams (exhaustive)"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = true; options.DOSKIPGRAMS_EXHAUSTIVE = true;
        PatternModel<uint32_t> model;
        model.train(datafile, options);
        end(m);
    }

    {
        Measurement m = begin(string("Training unindexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, with skipgrams (exhaustive)"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = true; options.DOSKIPGRAMS_EXHAUSTIVE = true;
        PatternModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

	{
        Measurement m = begin(string("Training unindexed PatternModel from file: threshold 1, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 1; options.MAXLENGTH = 8; 
        PatternModel<uint32_t> model;
        model.train(datafile, options);
        end(m);
    }

    {
        Measurement m = begin(string("Training indexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = false;
        IndexedPatternModel<> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

    {
        Measurement m = begin(string("Training indexed PatternModel from preloaded corpus: threshold 2, up to 8-grams, with skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = true;
        IndexedPatternModel<> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

    {
        Measurement m = begin(string("Training unindexed PatternPointerModel from preloaded corpus: threshold 2, up to 8-grams, no skipgrams"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8;
        PatternPointerModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }

    {
        Measurement m = begin(string("Training unindexed PatternPointerModel from preloaded corpus: threshold 2, up to 8-grams, with skipgrams (exhaustive)"));
        PatternModelOptions options;
        options.MINTOKENS = 2; options.MAXLENGTH = 8; options.DOSKIPGRAMS = true; options.DOSKIPGRAMS_EXHAUSTIVE = true;
        PatternPointerModel<uint32_t> model(&corpus);
        model.train(datafile, options);
        end(m);
    }
}

