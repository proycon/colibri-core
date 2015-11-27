#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import print_function, division, absolute_import

import os
import sys
from copy import copy
import pickle


def red(s):
   CSI="\x1B["
   return CSI+"1;31m" + s + CSI + "0m"

def green(s):
   CSI="\x1B["
   return CSI+"1;32m" + s + CSI + "0m"

def test(a, b = None):
    if b is None:
        if a:
            return "... " + green("ok")
        else:
            print(red("FAILED!"), file=sys.stderr)
            sys.exit(2)
    else:
        if a == b:
            return "... " + str(a) + " " + green("ok")
        else:
            print(red("FAILED!") + " Got "+ str(a) + ", expected " + str(b),file=sys.stderr)
            sys.exit(2)

try:
    import colibricore
except ImportError:
    print("Run setup.py install first!",file=sys.stderr)
    raise

with open("/tmp/colibritest",'w') as f:
    f.write("5\tbe\n6\tTo\n7\tto\n8\tor\n9\tnot\n73477272\tblah\n")

print("Loading class decoder...")
decoder = colibricore.ClassDecoder("/tmp/colibritest")
print("Loading class encoder...")
encoder = colibricore.ClassEncoder("/tmp/colibritest")

print("Building pattern...")
ngram = encoder.buildpattern("To be or not to be")

print("Ngram: ", test(ngram.tostring(decoder),"To be or not to be"))
print("Size: ", test(len(ngram),6))
print("Bytesize: ", test(ngram.bytesize(),6))
print("Category==NGRAM", test(ngram.category() == colibricore.Category.NGRAM) )
print("Hash: ", test(hash(ngram)))
print("Raw bytes: ",repr(bytes(ngram)))

print("Third token ", test(ngram[2].tostring(decoder), "or"))
print("Last token ", test(ngram[-1].tostring(decoder), "be"))

print("Slicing ngram[2:4]", test(ngram[2:4].tostring(decoder), "or not"))


print("Copying n-gram:", test(copy(ngram) == ngram))


if sys.version[0] != '2':
    #Python 3 only for now:

    print("Picking n-gram:")
    pickled = pickle.dumps(ngram)

    print("Unpicking n-gram:")
    unpickledngram = pickle.loads(pickled)

    print("Equality check", test(ngram == unpickledngram))




print("Tokens of ngram:")
tokens =["To","be","or","not","to","be"]
for token,tokenref in zip(ngram,tokens):
    test(token.tostring(decoder),tokenref)
print("Count check", test(len(list(iter(ngram))), len(tokens)))

subngrams = [
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
"be or not to be"]
print("Subgrams of ngram:")
for subngram,subngramref in zip(ngram.subngrams(),subngrams):
    print(test(subngram.tostring(decoder),subngramref) )
print("Count check", test(len(list(ngram.subngrams())), len(subngrams)))

subngram = encoder.buildpattern("or not")
print("Testing occurrence of substring 'or not'...", test(subngram in ngram))

subngram2 = encoder.buildpattern("to be")
print("Testing occurrence of substring 'to be'...", test(subngram2 in ngram))

subngram3 = encoder.buildpattern("or")
print("Testing occurrence of substring 'or'...", test(subngram3 in ngram))

print("Testing gram addition:")
ngramconc = subngram + subngram2
print(ngramconc.tostring(decoder),test(ngramconc.tostring(decoder) == "or not to be"))

print("Testing sorting")
for subngram in sorted(ngram.subngrams()):
    print(subngram.tostring(decoder))

print("Skipgram test")
skipgram = encoder.buildpattern("To {*1*} or {*1*} to be")
print("Skipgram: ", test(skipgram.tostring(decoder),"To {*} or {*} to be") )
print("Size: ", test(len(skipgram),6))
print("Bytesize: ", test(skipgram.bytesize(),6))
print("Category==SKIPGRAM", test(skipgram.category() == colibricore.Category.SKIPGRAM) )
print("Hash: ", test(hash(skipgram)))
print("Skipcount check...", test(skipgram.skipcount() == 2))

print("Parts:")
for part in skipgram.parts():
    print(part.tostring(decoder))

print("Gaps:")
for begin,length in skipgram.gaps():
    print(begin,length)

print("Converting to flexgram")
flexgram = skipgram.toflexgram()
print("Flexgram: ", test(flexgram.tostring(decoder),"To {**} or {**} to be" ))
print("Size: ", test(len(flexgram),6))
print("Bytesize: ", test(flexgram.bytesize(),6))
print("Category==SKIPGRAM", test(flexgram.category() == colibricore.Category.FLEXGRAM) )
print("Hash: ", test(hash(flexgram)))
print("Skipcount check...", test(flexgram.skipcount() == 2))

print("Parts:")
partsref = ["To","or","to be"]
for part,partref in zip(flexgram.parts(),partsref) :
    print(test(part.tostring(decoder),partref))

hamlet = """
To be or not to be , that is the question ;
Whether 'tis nobler in the mind to suffer
The slings and arrows of outrageous fortune ,
Or to take arms against a sea of troubles ,
And by opposing , end them . To die , to sleep ;
No more ; and by a sleep to say we end
The heart-ache and the thousand natural shocks
That flesh is heir to — 'tis a consummation
Devoutly to be wish'd . To die , to sleep ;
To sleep , perchance to dream . Ay , there's the rub ,
For in that sleep of death what dreams may come ,
When we have shuffled off this mortal coil ,
Must give us pause . There's the respect
That makes calamity of so long life ,
For who would bear the whips and scorns of time,
Th'oppressor's wrong , the proud man 's contumely ,
The pangs of despised love , the law 's delay ,
The insolence of office , and the spurns
That patient merit of th' unworthy takes ,
When he himself might his quietus make
With a bare bodkin ? who would fardels bear ,
To grunt and sweat under a weary life ,
But that the dread of something after death ,
The undiscovered country from whose bourn
No traveller returns , puzzles the will ,
And makes us rather bear those ills we have
Than fly to others that we know not of ?
Thus conscience does make cowards of us all ,
And thus the native hue of resolution
Is sicklied o'er with the pale cast of thought ,
And enterprises of great pitch and moment
With this regard their currents turn awry ,
And lose the name of action .
Soft you now ! The fair Ophelia ! Nymph ,
in thy orisons be all my sins remember'd .
To flee or not to flee .
To flee or not to flee .
To see or not to see .
To see or not to see .
To pee or not to pee .
"""

with open('/tmp/hamlet.txt','w') as f:
    f.write(hamlet)


print("Class encoding corpus...")
os.system("colibri-classencode /tmp/hamlet.txt")

print("Loading new decoder")
decoder = colibricore.ClassDecoder("/tmp/hamlet.colibri.cls")
encoder = colibricore.ClassEncoder("/tmp/hamlet.colibri.cls")

print("Loading corpus as IndexedCorpus")
corpus = colibricore.IndexedCorpus("/tmp/hamlet.colibri.dat")
print("Total number of tokens: ", len(corpus))
firstword = corpus[(1,0)]
print("First word: ", test(firstword.tostring(decoder),"To"))
needle = encoder.buildpattern("fair Ophelia")
for match in corpus.findpattern(needle):
    print( "'fair Ophelia' found at ", match)
print("Token iteration:")
i = 0
for ref in corpus:
    i += 1
print("Total number of tokens: ", test(len(corpus),i))


print()

options = colibricore.PatternModelOptions(doskipgrams_exhaustive=True)

print("\n===== Building unindexed model ======\n")
unindexedmodel = colibricore.UnindexedPatternModel()
unindexedmodel.train("/tmp/hamlet.colibri.dat",options)
print("Pattern count", test(len(unindexedmodel), 385))
print("Type count", test(unindexedmodel.types(), 186))
print("Token count", test(unindexedmodel.tokens(), 354))

unindexedmodel.printmodel(decoder)
print("REPORT:")
unindexedmodel.report()
print("HISTOGRAM:")
unindexedmodel.histogram()

outputfilename = "/tmp/data.colibri.patternmodel"
print("Writing to file")
unindexedmodel.write(outputfilename)

print("Loading unindexed corpus")
unindexedmodel = colibricore.UnindexedPatternModel("/tmp/data.colibri.patternmodel")
print("REPORT:")
unindexedmodel.report()

print("iterating over all patterns")
i = 0
for pattern in unindexedmodel:
    print(pattern.tostring(decoder))
    i += 1
print("Pattern count", test(i, 385))


print("iterating over all patterns and values")
i = 0
for pattern, value in unindexedmodel.items():
    print(pattern.tostring(decoder), value)
    i += 1
print("Pattern count", test(i, 385))

print("Extracting count for specific pattern")
print(test(unindexedmodel[encoder.buildpattern("to be")],2))


print("\n======= Loading corpus data =========\n")

corpus = colibricore.IndexedCorpus("/tmp/hamlet.colibri.dat")
print("Sentence count", test(corpus.sentencecount(),40))
i = 0
for sentence in corpus.sentences():
    print(sentence.tostring(decoder))
    i += 1
print("Count check",test(i,40))

print("\n======= Building indexed model =========\n")
options = colibricore.PatternModelOptions(doskipgrams=True)
indexedmodel = colibricore.IndexedPatternModel(reverseindex=corpus)
indexedmodel.train("/tmp/hamlet.colibri.dat",options)

print("Pattern count", test(len(indexedmodel), 133))
print("Type count", test(indexedmodel.types(), 186))
print("Token count", test(indexedmodel.tokens(), 354))

indexedmodel.printmodel(decoder)
print("REPORT:")
indexedmodel.report()
print("HISTOGRAM:")
indexedmodel.histogram()

outputfilename = "/tmp/data.colibri.indexedpatternmodel"
print("Writing to file")
indexedmodel.write(outputfilename)

print("Loading indexed corpus")
indexedmodel = colibricore.IndexedPatternModel("/tmp/data.colibri.indexedpatternmodel")

print("iterating over all patterns and values")
for pattern, value in indexedmodel.items():
    print(pattern.tostring(decoder), len(value))

print("Extracting count for specific pattern")
print(test(len(indexedmodel[encoder.buildpattern("to be")]),2))


print("Test done")
