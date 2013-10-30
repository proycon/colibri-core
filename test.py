#!/usr/bin/env python3

from __future__ import print_function, unicode_literals, division, absolute_import

import os.path
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


