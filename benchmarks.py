#!/usr/bin/env python3

from __future__ import print_function, unicode_literals, division, absolute_import
from datetime import datetime
import sys
import os.path
from collections import defaultdict

from pynlpl.textprocessors import MultiWindower,Windower
import colibricore
#import psutil
import gc
import time
import resource

ansicolors = {"red":31,"green":32,"yellow":33,"blue":34,"magenta":35, "bold":1 }
def colorf(color, x):
    return "\x1B[" + str(ansicolors[str(color)]) + "m" + x + "\x1B[0m"


#def getmem2():
#    # return the memory usage in MB
#    process = psutil.Process(os.getpid())
#    mem = process.get_memory_info()[0] / float(2 ** 20)
#    return mem

def getmem():
    rusage_denom = 1024.
    if sys.platform == 'darwin':
        # ... it seems that in OSX the output is different units ...
        rusage_denom = rusage_denom * rusage_denom
    mem = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / rusage_denom
    return mem

def begin():
    gc.collect()
    return datetime.now(), getmem()


def savemodel(model,modelfile):
    print("Saving model")
    model.write(modelfile)
    print("--> Size of model on disk: ", round(os.path.getsize(modelfile) / 1024 / 1024,2), "MB" )

def end(data):
    begintime, beginmem = data
    d = datetime.now() - begintime
    memd = getmem() - beginmem
    gc.collect()
    print("--> Duration: " + colorf('yellow',str(d.total_seconds())+ 's') + " -- Memory: " + colorf('green',str(round(memd,2))+ ' MB') + "\n")
    return d, memd

def main():
    dopretests = True
    try:
        tests = sys.argv[1]
        if tests[0] == 'x':
            dopretests = False
            tests = tests[1:]
        if '-' in tests:
            begintest = int(tests.split('-')[0])
            endtest = int(tests.split('-')[1])
        else:
            begintest = endtest = int(tests)
    except:
        print("Specify a text file (plain text, UTF-8, one sentence per line, preferably tokenised) to use as a basis",file=sys.stderr)
        sys.exit(2)
    try:
        textfile = sys.argv[2]
    except:
        print("Specify a text file (plain text, UTF-8, one sentence per line, preferably tokenised) to use as a basis",file=sys.stderr)
        sys.exit(2)

    try:
        tmpdir = sys.argv[3]
    except:
        tmpdir = "/tmp/"


    classfile = tmpdir + "/" + os.path.basename(textfile) + '.colibri.cls'
    datafile = tmpdir + "/" + os.path.basename(textfile) + '.colibri.dat'
    modelfile = tmpdir + "/" + os.path.basename(textfile) + '.colibri.patternmodel'


    if not os.path.exists(textfile):
        print("File does not exist",file=sys.stderr)
        sys.exit(2)

    if dopretests:
        linecount = 0
        print("PRETEST #1 - Reading text file (Python)")
        b = begin()
        with open(textfile,'r',encoding='utf-8') as f:
            for line in f:
                linecount += 1
        end(b)
        print("\t(Read " + str(linecount) + " lines)")


        print("PRETEST #2 - Building class encoder")
        encoder = colibricore.ClassEncoder()
        b = begin()
        encoder.build(textfile)
        end(b)


        print("PRETEST #3 - Saving class encoder")
        b = begin()
        encoder.save(classfile)
        end(b)


        print("PRETEST #4 - Class encoding corpus")
        b = begin()
        encoder.encodefile(textfile, datafile)
        end(b)

        print("PRETEST #5 - Unloading encoder")
        b = begin()
        del encoder
        gc.collect()
        end(b)


    if begintest < endtest:
        print("Running tests " , begintest, " to ", endtest)
        for testnum in range(begintest, min(endtest+1,10)):
            os.system("python3 " + sys.argv[0] + " x" + str(testnum) + " "+ textfile + " " + tmpdir)

    else:
        testnum = begintest
        print("-------------------- " + colorf('bold','TEST') + " #" + str(testnum) +" ----------------------")
        if testnum == 1:

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

        elif testnum == 2:

            print("Extracting and counting ALL n-grams (up to 8-grams, threshold=1) with UnindexedPatternModel (without reverse index)")
            model = colibricore.UnindexedPatternModel()
            options = colibricore.PatternModelOptions(mintokens=1,maxlength=8,doreverseindex=False)
            b = begin()
            model.train(datafile, options)
            end(b)
            savemodel(model,modelfile)
            del model

        if testnum == 3:
            linecount = 0
            print("Extracting and counting n-grams (up to 8-grams, threshold=2, with look-back)  (Python defaultdict + Pynlpl Windower)")
            ngrams=defaultdict(int)
            b = begin()
            for n in range(1,9):
                with open(textfile,'r',encoding='utf-8') as f:
                    for line in f:
                        for ngram in Windower(line, n):
                            docount = True
                            if n>1:
                                for subngram in Windower(ngram,n-1):
                                    if not subngram in ngrams:
                                        docount = False
                                        break
                            if docount:
                                ngrams[ngram] += 1
            end(b)
            print("\t(Found " + str(len(ngrams)) + " ngrams)")
        if testnum == 4:
            linecount = 0
            print("Extracting and counting n-grams (up to 8-grams, threshold=2, without look-back)  (Python defaultdict + Pynlpl Windower)")
            ngrams=defaultdict(int)
            b = begin()
            for n in range(1,9):
                with open(textfile,'r',encoding='utf-8') as f:
                    for line in f:
                        for ngram in Windower(line, n):
                            ngrams[ngram] += 1
            for ngram in list(ngrams.keys()):
                if ngrams[ngram] < 2: del ngrams[ngram]
            gc.collect()
            end(b)
            print("\t(Found " + str(len(ngrams)) + " ngrams)")
        elif testnum == 5:

            print("Extracting and counting ALL n-grams (up to 8-grams, threshold=2) with UnindexedPatternModel (without reverse index)")
            model = colibricore.UnindexedPatternModel()
            options = colibricore.PatternModelOptions(mintokens=2,maxlength=8,doreverseindex=False)
            b = begin()
            model.train(datafile, options)
            end(b)
            savemodel(model,modelfile)

        elif testnum == 6:

            print("Extracting and counting ALL n-grams (up to 8-grams,threshold=1) with UnindexedPatternModel (with reverse index)")
            model = colibricore.UnindexedPatternModel()
            options = colibricore.PatternModelOptions(mintokens=1,maxlength=8,doreverseindex=True)
            b = begin()
            model.train(datafile, options)
            end(b)
            savemodel(model,modelfile)

        elif testnum == 7:

            print("Extracting and counting ALL n-grams (up to 8-grams,threshold=1) with IndexedPatternModel (with reverse index)")
            model = colibricore.IndexedPatternModel()
            options = colibricore.PatternModelOptions(mintokens=1,maxlength=8)
            b = begin()
            model.train(datafile, options)
            end(b)
            savemodel(model,modelfile)

            del model

        elif testnum == 8:
            print("Extracting and counting n-grams with treshold 2 (up to 8-grams) with IndexedPatternModel (with reverse index)")
            model = colibricore.IndexedPatternModel()
            options = colibricore.PatternModelOptions(mintokens=2,maxlength=8)
            b = begin()
            model.train(datafile, options)
            end(b)
            savemodel(model,modelfile)

        elif testnum == 9:

            print("Extracting and counting n-grams and skipgrams with treshold 2 (up to 8-grams) with IndexedPatternModel (with reverse index)")
            model = colibricore.IndexedPatternModel()
            options = colibricore.PatternModelOptions(mintokens=2,maxlength=8, doskipgrams=True)
            b = begin()
            model.train(datafile, options)
            end(b)
            savemodel(model,modelfile)

        elif testnum == 10:
            print("Extracting and counting ALL n-grams (up to 8-grams, threshold=1) with OrderedUnindexedPatternModel (without reverse index)")
            model = colibricore.OrderedUnindexedPatternModel()
            options = colibricore.PatternModelOptions(mintokens=1,maxlength=8,doreverseindex=False)
            b = begin()
            model.train(datafile, options)
            end(b)
            savemodel(model,modelfile)
            del model


        else:
            print("No such test",file=sys.stderr)
        print()

if __name__ == '__main__':
    main()
