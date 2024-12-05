AM_CXXFLAGS = -Wall -O3 -std=c++11 -Wno-vla -fPIC
AM_CPPFLAGS = -I@top_srcdir@/include
AM_LDFLAGS = -static -fPIC

bin_PROGRAMS = colibri-classdecode colibri-classencode colibri-test colibri-benchmarks colibri-patternmodeller colibri-comparemodels colibri-extractngrams

LDADD = libcolibricore.la
lib_LTLIBRARIES = libcolibricore.la

libcolibricore_la_SOURCES = SpookyV2.cpp common.cpp algorithms.cpp classdecoder.cpp classencoder.cpp pattern.cpp patternmodel.cpp

colibri_classdecode_SOURCES = classdecode.cpp
colibri_classencode_SOURCES = classencode.cpp
colibri_test_SOURCES = test.cpp
colibri_benchmarks_SOURCES = benchmarks.cpp
colibri_patternmodeller_SOURCES = patternmodeller.cpp
colibri_comparemodels_SOURCES = comparemodels.cpp
colibri_extractngrams_SOURCES = extractngrams.cpp

TESTS = tst.sh
EXTRA_DIST = tst.sh
