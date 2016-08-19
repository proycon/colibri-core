#!/bin/bash


if [ ! -f republic.txt ]; then
    echo "Run from colibri-core/exp directory in which republic.txt resides!">&2
    exit 2
fi


echo -e "\n\nTEST> Class encoding corpus">&2
colibri-classencode republic.txt
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Class decoding corpus">&2
colibri-classdecode -f republic.colibri.dat -c republic.colibri.cls
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Class decoding v1 corpus">&2
colibri-classdecode -f hamlet.v1.colibri.dat -c hamlet.colibri.cls
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building unindexed model">&2
colibri-patternmodeller -f republic.colibri.dat -t 2 -l 10 -u -o republic.colibri.unindexedpatternmodel
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building indexed model">&2
colibri-patternmodeller -f republic.colibri.dat -t 2 -l 10 -o republic.colibri.indexedpatternmodel
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

#echo -e "\n\nTEST> Extending indexed model with skipgrams">&2   #segfaults on travis for some reason, works fine elsewhere
#colibri-patternmodeller -f republic.colibri.dat -i republic.colibri.indexedpatternmodel -t 2 -l 10 -s -o republic.colibri.indexedpatternmodel.withskipgrams
#if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

#echo -e "\n\nTEST> Extending indexed model with flexgrams">&2
#colibri-patternmodeller -i republic.colibri.indexedpatternmodel.withskipgrams -S S -t 2 -l 10 -o republic.colibri.indexedpatternmodel.withflexgrams
#if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi


echo -e "\n\nTEST> Building unindexed model on v1 data">&2
colibri-patternmodeller -f hamlet.v1.colibri.dat -t 2 -l 10 -u -R
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Class encoding corpus">&2
colibri-classencode -c republic.colibri.cls -e apology.txt
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi


echo -e "\n\nTEST> Building unindexed model on test data (unconstrained)">&2
colibri-patternmodeller -f apology.colibri.dat -t 2 -l 10 -u -o apology.colibri.unindexedpatternmodel
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Loading unindexed model">&2
colibri-patternmodeller -i apology.colibri.unindexedpatternmodel -R
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building indexed model on test data (unconstrained)">&2
colibri-patternmodeller -f apology.colibri.dat -t 2 -l 10 -o apology.colibri.indexedpatternmodel
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Loading indexed model">&2
colibri-patternmodeller -i apology.colibri.indexedpatternmodel -R
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building unindexed model on test data (unconstrained, with skipgrams)">&2
colibri-patternmodeller -f apology.colibri.dat -t 2 -l 10 -s -u -o apology.s.colibri.unindexedpatternmodel
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Loading unindexed model">&2
colibri-patternmodeller -i apology.s.colibri.unindexedpatternmodel -R
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building indexed model on test data (unconstrained, with skipgrams)">&2
colibri-patternmodeller -f apology.colibri.dat -t 2 -l 10 -s -o apology.s.colibri.indexedpatternmodel
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Loading indexed model">&2
colibri-patternmodeller -i apology.s.colibri.indexedpatternmodel -f apology.colibri.dat -R
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi




if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi
echo -e "\n\nTEST> Building unindexed model on test data (using training data -j)">&2
colibri-patternmodeller -f apology.colibri.dat -t 2 -l 10 -u -o apology-republic-overlap.colibri.unindexedpatternmodel -j republic.colibri.unindexedpatternmodel 
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building indexed model on test data (using training data -j)">&2
colibri-patternmodeller -f apology.colibri.dat -t 2 -l 10 -o apology-republic-overlap.colibri.indexedpatternmodel -j republic.colibri.indexedpatternmodel 
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi



echo -e "\n\nTEST> Building unindexed model from indexed model (using -I)">&2
colibri-patternmodeller -I -f republic.colibri.dat -i republic.colibri.unindexedpatternmodel -t 2 -l 10 -o republic.colibri.indexedpatternmodel.fromunindexed
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi




echo -e "\n\nTEST> Building indexed model on test data (using -I)">&2
colibri-patternmodeller -I -f apology.colibri.dat -i republic.colibri.unindexedpatternmodel -t 2 -l 10 -o apology.colibri.indexedpatternmodel.fromunindexed.I
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building indexed model on test data (using -I)">&2
colibri-patternmodeller -I -f apology.colibri.dat -i republic.colibri.unindexedpatternmodel -t 2 -l 10 -o apology.colibri.indexedpatternmodel.fromunindexed.I
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building unindexed model on test data (using -I)">&2
colibri-patternmodeller -I -u -f apology.colibri.dat -i republic.colibri.unindexedpatternmodel -t 2 -l 10 -o apology.colibri.unindexedpatternmodel.fromunindexed.I
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi

echo -e "\n\nTEST> Building indexed model using Two-stage building">&2
colibri-patternmodeller -2 -f republic.colibri.dat -t 2 -l 10 -o republic.colibri.indexedpatternmodel.2stage
if [ ! "$?" = "0" ]; then echo "Test failed">&2; exit 2; fi
