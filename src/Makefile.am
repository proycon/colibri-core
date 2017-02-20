AM_CXXFLAGS = -Wall -O3 -std=c++11 -Wno-vla
AM_CPPFLAGS = -I@top_srcdir@/include

bin_PROGRAMS = colibri-classdecode colibri-classencode colibri-test colibri-benchmarks colibri-patternmodeller colibri-comparemodels colibri-extractngrams

lib_LTLIBRARIES = libcolibricore.la

libcolibricore_la_SOURCES = SpookyV2.cpp common.cpp algorithms.cpp classdecoder.cpp classencoder.cpp pattern.cpp patternmodel.cpp

colibri_classdecode_LDADD = libcolibricore.la
colibri_classdecode_SOURCES = classdecode.cpp
colibri_classdecode_LDFLAGS = -static

colibri_classencode_LDADD = libcolibricore.la
colibri_classencode_SOURCES = classencode.cpp
colibri_classencode_LDFLAGS = -static

colibri_test_LDADD = libcolibricore.la
colibri_test_SOURCES = test.cpp
colibri_test_LDFLAGS = -static

colibri_benchmarks_LDADD = libcolibricore.la
colibri_benchmarks_SOURCES = benchmarks.cpp
colibri_benchmarks_LDFLAGS = -static

colibri_patternmodeller_LDADD = libcolibricore.la
colibri_patternmodeller_SOURCES = patternmodeller.cpp
colibri_patternmodeller_LDFLAGS = -static

colibri_comparemodels_LDADD = libcolibricore.la
colibri_comparemodels_SOURCES = comparemodels.cpp
colibri_comparemodels_LDFLAGS = -static

colibri_extractngrams_LDADD = libcolibricore.la
colibri_extractngrams_SOURCES = extractngrams.cpp
colibri_extractngrams_LDFLAGS = -static

TESTS = tst.sh
EXTRA_DIST = tst.sh
