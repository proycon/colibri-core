#!/bin/bash

colibri-benchmarks $1 0 

for i in `seq 3 16`; do
    colibri-benchmarks $1 $i 
done
