#!/usr/bin/env python3

from __future__ import print_function
import glob
import sys
import os
import platform
from os.path import expanduser
from distutils.core import setup, Extension
from Cython.Distutils import build_ext
from Cython.Build import cythonize


HOMEDIR = expanduser("~")

ROOTDIR = os.path.abspath(os.path.dirname(__file__))


# cython's include is sucky unfortunately :(
# We'll have our own include mechanism:
for filename in glob.glob(os.path.join(ROOTDIR ,"*.in.p??")):
    extension = filename[-3:]
    print("(Writing " + filename[:-6]+extension + ")" ,file=sys.stderr)
    with open(filename,'r') as f_in:
        with open(filename[:-6]+extension,'w') as f_out:
            for line in f_in:
                found = line.find('@include') #generic include
                foundlen = 8

                if found != -1:
                    includefilename = line[found+foundlen+1:].strip()
                    with open(os.path.join(ROOTDIR,includefilename)) as f_inc:
                        for incline in f_inc:
                            f_out.write(" " * found + incline)
                else:
                    f_out.write(line)



def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

#defaults:
includedirs = []
libdirs = []
if platform.system() == "Darwin":
    #we are running on Mac OS X (with homebrew hopefully), stuff is in specific locations:
    if platform.machine().lower() == "arm64":
        libdirs.append("/opt/homebrew/lib")
        includedirs.append("/opt/homebrew/include")
        includedirs.append("/opt/homebrew/include/colibri-core")

#add some common default paths
includedirs += ['/usr/include/colibri-core', '/usr/local/include/colibri-core' ]
libdirs += ['/usr/lib','/usr/local/lib']
if 'VIRTUAL_ENV' in os.environ:
    includedirs.insert(0,os.environ['VIRTUAL_ENV'] + '/include/colibri-core')
    libdirs.insert(0,os.environ['VIRTUAL_ENV'] + '/lib')
if 'INCLUDE_DIRS' in os.environ:
    includedirs = list(os.environ['INCLUDE_DIRS'].split(':')) + includedirs
if 'LIBRARY_DIRS' in os.environ:
    libdirs = list(os.environ['LIBRARY_DIRS'].split(':')) + libdirs

if platform.system() == "Darwin":
    extra_options = ["--stdlib=libc++"]
else:
    extra_options = []

extensions = cythonize([ Extension("colibricore",
                ["unordered_set.pxd","unordered_map.pxd", "colibricore_classes.pxd", "colibricore_wrapper.pyx"],
                language='c++',
                include_dirs=includedirs,
                library_dirs=libdirs,
                libraries=['colibricore','bz2'],
                extra_compile_args=['--std=c++0x'] + extra_options,
                ) ],
                compiler_directives={"language_level": "3"}
            )

setup(
    name = 'colibricore',
    author = "Maarten van Gompel",
    author_email = "proycon@anaproy.nl",
    description = ("Colibri Core is an NLP tool as well as a C++ and Python library (all included in this package) for working with basic linguistic constructions such as n-grams and skipgrams (i.e patterns with one or more gaps, either of fixed or dynamic size) in a quick and memory-efficient way. At the core is the tool ``colibri-patternmodeller`` which allows you to build, view, manipulate and query pattern models."),
    license = "GPLv3",
    keywords = "nlp computational_linguistics frequency ngram skipgram pmi cooccurrence linguistics",
    long_description=read('README.md'),
    long_description_content_type = "text/markdown",
    version = '2.5.9',
    ext_modules = extensions,
    cmdclass = {'build_ext': build_ext},
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Topic :: Text Processing :: Linguistic",
        "Programming Language :: Cython",
        "Programming Language :: Python :: 3",
        "Operating System :: POSIX",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
    ],
    install_requires=['Cython >= 0.29'],
)
