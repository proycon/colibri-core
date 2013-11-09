#!/usr/bin/env python3

from __future__ import print_function, unicode_literals, division, absolute_import
from datetime import datetime
import sys
import os.path
from collections import defaultdict

from pynlpl.textprocessors import MultiWindower
import colibricore
import psutil


def getmem():
    # return the memory usage in MB
    process = psutil.Process(os.getpid())
    mem = process.get_memory_info()[0] / float(2 ** 20)
    return mem

def begin():
    return datetime.now(), getmem()


def end(data):
    begintime, beginmem = data
    d = datetime.now() - begintime
    memd = getmem() - beginmem
    print("--> Duration: " + str(d.total_seconds()) + "s -- Memory: " + str(round(memd,2)) + "MB\n")

def main():
    try:
        textfile = sys.argv[1]
    except:
        print("Specify a text file (plain text, UTF-8, one sentence per line, preferably tokenised) to use as a basis",file=sys.stderr)
        sys.exit(2)

    try:
        tmpdir = sys.argv[2]
    except:
        tmpdir = "/tmp/"

    classfile = tmpdir + "/" + os.path.basename(textfile) + '.colibri.cls'
    datafile = tmpdir + "/" + os.path.basename(textfile) + '.colibri.dat'
    modelfile = tmpdir + "/" + os.path.basename(textfile) + '.colibri.patternmodel'

    if not os.path.exists(textfile):
        print("File does not exist",file=sys.stderr)
        sys.exit(2)


    linecount = 0
    print("Reading text file (Python)")
    b = begin()
    with open(textfile,'r',encoding='utf-8') as f:
        for line in f:
            linecount += 1
    end(b)
    print("\t(Read " + str(linecount) + " lines)")


    linecount = 0
    print("Extracting and counting n-grams (up to 8-grams) naively (Python defaultdict + Pynlpl MultiWindower)")
    ngrams=defaultdict(int)
    b = begin()
    with open(textfile,'r',encoding='utf-8') as f:
        for line in f:
            for ngram in MultiWindower(line, 1,8):
                ngrams[ngram] += 1
    end(b)
    print("\t(Found " + str(len(ngrams)) + " ngrams)")



    print("Building class encoder")
    encoder = colibricore.ClassEncoder()
    b = begin()
    encoder.build(textfile)
    end(b)




    print("Class encoding corpus")
    b = begin()
    encoder.encodefile(textfile, datafile)
    end(b)

    print("Saving corpus")
    b = begin()
    encoder.save(classfile)
    end(b)

    print("Loading decoder")
    b = begin()
    decoder = colibricore.ClassDecoder(classfile)
    end(b)

    print("Extracting and counting ALL n-grams (up to 8-grams, threshold=1) with UnindexedPatternModel (without reverse index)")
    model = colibricore.UnindexedPatternModel()
    options = colibricore.PatternModelOptions(mintokens=1,maxlength=8,doreverseindex=False)
    b = begin()
    model.train(datafile, options)
    end(b)

    print("Saving model")
    b = begin()
    model.write(modelfile)
    end(b)



    print("Extracting and counting aLL n-grams (up to 8-grams,threshold=1) with UnindexedPatternModel (with reverse index)")
    model = colibricore.UnindexedPatternModel()
    options = colibricore.PatternModelOptions(mintokens=1,maxlength=8,doreverseindex=True)
    b = begin()
    model.train(datafile, options)
    end(b)


    print("Extracting and counting ALL n-grams (up to 8-grams,threshold=1) with IndexedPatternModel (with reverse index)")
    model = colibricore.IndexedPatternModel()
    options = colibricore.PatternModelOptions(mintokens=1,maxlength=8)
    b = begin()
    model.train(datafile, options)
    end(b)


    print("Extracting and counting n-grams with treshold 2 (up to 8-grams) with IndexedPatternModel (with reverse index)")
    model = colibricore.IndexedPatternModel()
    options = colibricore.PatternModelOptions(mintokens=2,maxlength=8)
    b = begin()
    model.train(datafile, options)
    end(b)


if __name__ == '__main__':
    main()
