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


print("Subgrams of ngram:")
for subngram in ngram.subngrams():
    print(subngram.tostring(decoder))

