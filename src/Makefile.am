AM_CPPFLAGS = -I@top_srcdir@/include

bin_PROGRAMS = classdecode classencode 

lib_LTLIBRARIES = libcolibricore.la

libcolibricore_la_CXXFLAGS = -Wall -std=c++11
libcolibricore_la_SOURCES = classdecoder.cpp classencoder.cpp pattern.cpp

classdecode_CXXFLAGS = -Wall -std=c++11
classdecode_LDADD = libcolibring.la
classdecode_SOURCES = classdecode.cpp
classdecode_LDFLAGS = -static

classencode_CXXFLAGS = -Wall -std=c++11
classencode_LDADD = libcolibring.la
classencode_SOURCES = classencode.cpp
classencode_LDFLAGS = -static


