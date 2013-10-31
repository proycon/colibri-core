#!/usr/bin/env python3

from __future__ import print_function, unicode_literals, division, absolute_import

import os
import sys
from copy import copy

try:
    import colibricore
except ImportError:
    print("Run setup.py install first!",file=sys.stderr)
    raise

with open("/tmp/colibritest",'w') as f:
    f.write("2\tbe\n3\tTo\n4\tto\n5\tor\n6\tnot\n73477272\tblah\n")

decoder = colibricore.ClassDecoder("/tmp/colibritest")
encoder = colibricore.ClassEncoder("/tmp/colibritest")

ngram = encoder.buildpattern("To be or not to be")

print("Ngram: ", ngram.tostring(decoder))
print("Size: ", len(ngram))
print("Bytesize: ", ngram.bytesize())
print("Category==NGRAM", (ngram.category() == colibricore.Category.NGRAM) )
print("Hash: ", hash(ngram))


print("Slicing ngram")
ngram2 = ngram[2:2]

print("Sliced ngram: ", ngram2.tostring(decoder))

print("Copying n-gram:")
ngram3 = copy(ngram)
print(ngram3.tostring(decoder))

print("Equality check")
assert( ngram == ngram3 )



print("Subgrams of ngram:")
for subngram in ngram.subngrams():
    print(subngram.tostring(decoder))

subngram = encoder.buildpattern("or not")
print("Testing occurrence of substring 'or not'...")
assert (subngram in ngram)

subngram2 = encoder.buildpattern("to be")
print("Testing occurrence of substring 'to be'...")
assert (subngram2 in ngram)

subngram3 = encoder.buildpattern("or")
print("Testing occurrence of substring 'or'...")
assert (subngram3 in ngram)

print("Testing gram addition:")
ngramconc = subngram + subngram2
print(ngramconc.tostring(decoder))
assert (ngramconc.tostring(decoder) == "or not to be" )

print("Testing sorting")
for subngram in sorted(ngram.subngrams()):
    print(subngram.tostring(decoder))

print("Skipgram test")
skipgram = encoder.buildpattern("To {*1*} or {*1*} to be")
print("Skipgram: ", skipgram.tostring(decoder))
print("Size: ", len(skipgram))
print("Bytesize: ", skipgram.bytesize())
print("Category==SKIPGRAM", (skipgram.category() == colibricore.Category.SKIPGRAM) )
print("Hash: ", hash(skipgram))
print("Skipcount check...")
assert(skipgram.skipcount() == 2)

print("Parts:")
for part in skipgram.parts():
    print(part.tostring(decoder))

print("Gaps:")
for begin,length in skipgram.gaps():
    print(begin,length)

print("Converting to flexgram")
flexgram = skipgram.toflexgram()
print("Flexgram: ", flexgram.tostring(decoder))
print("Size: ", len(flexgram))
print("Bytesize: ", flexgram.bytesize())
print("Category==SKIPGRAM", (flexgram.category() == colibricore.Category.FLEXGRAM) )
print("Hash: ", hash(flexgram))
print("Skipcount check...")
assert(flexgram.skipcount() == 2)

print("Parts:")
for part in flexgram.parts():
    print(part.tostring(decoder))

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

with open('/tmp/hamlet.txt','w',encoding='utf-8') as f:
    f.write(hamlet)


print("Class encoding corpus...")
os.system("colibri-classencode /tmp/hamlet.txt");

options = colibricore.PatternModelOptions()
options.DOREVERSEINDEX = True
options.DOSKIPGRAMS_EXHAUSTIVE = True
options.DOSKIPGRAMS = False
print(options.DOSKIPGRAMS)



print("Building unindexed model")
unindexedmodel = colibricore.UnindexedPatternModel()
unindexedmodel.train("/tmp/hamlet.colibri.dat",options)

print("Found ", len(unindexedmodel), " patterns, " , unindexedmodel.types()," types, " , unindexedmodel.tokens(), " tokens")
unindexedmodel.printmodel(decoder);
print("REPORT:")
unindexedmodel.report();
print("HISTOGRAM:")
unindexedmodel.histogram();

outputfilename = "/tmp/data.colibri.patternmodel"
print("Writing to file")
unindexedmodel.write(outputfilename);

print("Loading unindexed corpus")
unindexedmodel = colibricore.UnindexedPatternModel("/tmp/data.colibri.patternmodel")
