#!/bin/bash

colibri-benchmarks $1 0 

for i in `seq 3 14`; do
    colibri-benchmarks $1 $i 
done
