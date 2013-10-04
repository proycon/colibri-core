AM_CPPFLAGS = -I@top_srcdir@/include

bin_PROGRAMS = colibri-classdecode colibri_classencode colibri_test

lib_LTLIBRARIES = libcolibricore.la

libcolibricore_la_CXXFLAGS = -Wall -std=c++11
libcolibricore_la_SOURCES = SpookyV2.cpp common.cpp algorithms.cpp classdecoder.cpp classencoder.cpp pattern.cpp

colibri_classdecode_CXXFLAGS = -Wall -std=c++11
colibri_classdecode_LDADD = libcolibricore.la
colibri_classdecode_SOURCES = classdecode.cpp
colibri_classdecode_LDFLAGS = -static

colibri_classencode_CXXFLAGS = -Wall -std=c++11
colibri_classencode_LDADD = libcolibricore.la
colibri_classencode_SOURCES = classencode.cpp
colibri_classencode_LDFLAGS = -static

colibri_test_CXXFLAGS = -Wall -std=c++0x
colibri_test_LDADD = libcolibricore.la
colibri_test_SOURCES = test.cpp
colibri_test_LDFLAGS = -static
