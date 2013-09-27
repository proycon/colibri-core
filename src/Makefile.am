AM_CPPFLAGS = -I@top_srcdir@/include

bin_PROGRAMS = classdecode classencode test

lib_LTLIBRARIES = libcolibricore.la

libcolibricore_la_CXXFLAGS = -Wall -std=c++11
libcolibricore_la_SOURCES = SpookyV2.cpp common.cpp classdecoder.cpp classencoder.cpp pattern.cpp

classdecode_CXXFLAGS = -Wall -std=c++11
classdecode_LDADD = libcolibricore.la
classdecode_SOURCES = classdecode.cpp
classdecode_LDFLAGS = -static

classencode_CXXFLAGS = -Wall -std=c++11
classencode_LDADD = libcolibricore.la
classencode_SOURCES = classencode.cpp
classencode_LDFLAGS = -static

test_CXXFLAGS = -Wall -std=c++0x
test_LDADD = libcolibricore.la
test_SOURCES = test.cpp
test_LDFLAGS = -static
