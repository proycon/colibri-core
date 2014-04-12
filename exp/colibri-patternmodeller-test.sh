#!/bin/bash


if [ ! -f republic.txt ]; then
    echo "Run from colibri-core/exp directory in which republic.txt resides!">&2
fi


echo -e "\n\nTEST> Class encoding corpus">&2
colibri-classencode republic.txt
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building unindexed model">&2
colibri-patternmodeller -f republic.colibri.dat -t 2 -l 10 -u -o republic.colibri.unindexedpatternmodel
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building indexed model">&2
colibri-patternmodeller -f republic.colibri.dat -t 2 -l 10 -u -o republic.colibri.indexedpatternmodel
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building indexed model with skipgrams">&2
colibri-patternmodeller -f republic.colibri.dat -t 2 -l 10 -u -o republic.colibri.indexedpatternmodel.withskipgrams -s
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building indexed model with skipgrams and flexgrams">&2
colibri-patternmodeller -f republic.colibri.dat -t 2 -l 10 -u -o republic.colibri.indexedpatternmodel.withskipgrams -S S
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi
