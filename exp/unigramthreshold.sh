#!/bin/bash

colibri-classencode unigramthreshold.txt
#normal test, should output two patterns
colibri-patternmodeller -f unigramthreshold.colibri.dat -m 4 -l 4 -t 1 -c unigramthreshold.colibri.cls -P
#normal test, word constrained (3), should  output two patterns
colibri-patternmodeller -f unigramthreshold.colibri.dat -m 4 -l 4 -t 1 -W 3 -c unigramthreshold.colibri.cls -P
#normal test, word constrained (4), should  output one pattern
colibri-patternmodeller -f unigramthreshold.colibri.dat -m 4 -l 4 -t 1 -W 4 -c unigramthreshold.colibri.cls -P

