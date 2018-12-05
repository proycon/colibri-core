#!/usr/bin/env python3

# This is a bit of a non-traditional setup, it does some fancy things such as
# invoking compilation of the C++ part of Colibri Core

from __future__ import print_function
import glob
import sys
import os
import platform
from os.path import expanduser
from distutils.core import setup, Extension
try:
    from Cython.Distutils import build_ext
except ImportError:
    print("Cython was not found, install Cython first through your package manager or using: pip install cython",file=sys.stderr)
    sys.exit(3)


HOMEDIR = expanduser("~")

ROOTDIR = os.path.abspath(os.path.dirname(__file__))



#attempt to pre-detect compiler (gcc vs clang)
cxx = 'c++'
if 'CXX' in os.environ:
    cxx = os.environ['CXX']


compilerfound = False
for compiler in (cxx,'gcc','clang'):
    compilerversionfile = os.path.join(ROOTDIR,"compilerversion")
    r = os.system(compiler + " --version > " + compilerversionfile)
    if r == 0:
        compilerfound = True
        break

if not compilerfound:
    print("No C++ Compiler found! Set the CXX environment variable to point to your compiler",file=sys.stderr)
    sys.exit(2)

compilerversion = open(compilerversionfile,'r').read()

# cython's include is sucky unfortunately :(
# And we need some conditional includes based on gcc vs clang
# We'll have our own:
for filename in glob.glob(os.path.join(ROOTDIR ,"*.in.p??")):
    extension = filename[-3:]
    print("(Writing " + filename[:-6]+extension + ")" ,file=sys.stderr)
    with open(filename,'r') as f_in:
        with open(filename[:-6]+extension,'w') as f_out:
            for line in f_in:
                found = line.find('@include') #generic include'
                foundlen = 8

                foundgcc = line.find('@includegcc') #gcc-only include
                if foundgcc != -1:
                    if compilerversion.find('clang') == -1: #anything that is not clang is gcc for our purposes
                        found = foundgcc
                        foundlen = 11
                    else:
                        continue

                foundclang = line.find('@includeclang') #clang-only include
                if foundclang != -1:
                    if compilerversion.find('clang') != -1:
                        found = foundclang
                        foundlen = 13
                    else:
                        continue

                if found != -1:
                    includefilename = line[found+foundlen+1:].strip()
                    with open(os.path.join(ROOTDIR,includefilename)) as f_inc:
                        for incline in f_inc:
                            f_out.write(" " * found + incline)
                else:
                    f_out.write(line)



def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

#Not the most elegant hack, but we're going to try compile colibri-core here before we continue with the rest:
#  create an empty file 'manual' to skip this:

#defaults:
includedirs = ["/usr/local/include/colibri-core","/usr/include/colibri-core", "/usr/include/libxml2"]
libdirs = ["/usr/local/lib","/usr/lib"]
if ('install' in sys.argv[1:] or 'build_ext' in sys.argv[1:]) and '--help' not in sys.argv[1:]:

    if '-n' in sys.argv[1:]:
        print("Dry run not supported for colibri-core compilation",file=sys.stderr)
        sys.exit(2)

    os.chdir(ROOTDIR)

    print("Detected compiler: ", compilerversion,file=sys.stderr)

    #see if we got a prefix:
    prefix = None
    for i, arg in enumerate(sys.argv):
        if i == 0: continue
        if arg == "--prefix" or arg == "--install-base":
            prefix = sys.argv[i+1]
        elif arg == "--root":
            prefix = sys.argv[i+1] + '/usr/'
        elif arg[:15] == "--install-base=":
            prefix = arg[15:]
        elif arg[:9] == "--prefix=":
            prefix = arg[9:]
        elif arg[:7] == "--root=":
            prefix = sys.argv[i+1] + arg[7:]
        elif arg == "--user":
            prefix = HOMEDIR + "/.local/"

    if 'VIRTUAL_ENV' in os.environ and not prefix:
        prefix = os.environ['VIRTUAL_ENV']

    if prefix is None:
        prefix = "/usr" #default location is /usr rather than /usr/local

    if not os.path.exists('manual'):
        if not os.path.exists(ROOTDIR + "/configure") or '-f' in sys.argv[1:] or '--force' in sys.argv[1:]:
            print("Bootstrapping colibri-core",file=sys.stderr)
            r = os.system("bash bootstrap")
            if r != 0:
                print("Bootstrapping colibri-core failed: make sure you have a basic build environment with gcc/clang, autoconf, automake and autoconf-archive installed",file=sys.stderr)
                sys.exit(2)
        if not os.path.exists(ROOTDIR + "/Makefile") or '-f' in sys.argv[1:] or '--force' in sys.argv[1:]:
            if prefix:
                r = os.system("./configure --prefix=" + prefix)
            else:
                r = os.system("./configure")
            if r != 0:
                print("Configure of colibri-core failed",file=sys.stderr)
                sys.exit(2)
        r = os.system("make")
        if r != 0:
            print("Make of colibri-core failed",file=sys.stderr)
            sys.exit(2)
        r = os.system("make install")
        if r != 0:
            print("Make install of colibri-core failed",file=sys.stderr)
            sys.exit(2)
        else:
            print("\nInstallation of Colibri Core C++ library and tools completed succesfully.",file=sys.stderr)
    else:
        print("\nCompilation of Colibri Core C++ library skipped as requested... (remove 'manual' file to re-enable)",file=sys.stderr)

    if prefix != "/usr":
        includedirs = [prefix + "/include/colibri-core", prefix + "/include", prefix + "/include/libxml2"] + includedirs
        libdirs = [prefix + "/lib"] + libdirs

        print("--------------------------------------------------------------------------------------------------------------------",file=sys.stderr)
        print("**NOTE:** You may need to set LD_LIBRARY_PATH=\"" + prefix + "/lib\", prior to running Python,",file=sys.stderr)
        print("          for it to be able to find the Colibri Core shared library!",file=sys.stderr)
        print("--------------------------------------------------------------------------------------------------------------------\n",file=sys.stderr)

if platform.system() == "Darwin":
    extra_options = ["--stdlib=libc++"]
else:
    extra_options = []

extensions = [ Extension("colibricore",
                ["unordered_set.pxd","unordered_map.pxd", "colibricore_classes.pxd", "colibricore_wrapper.pyx"],
                language='c++',
                include_dirs=includedirs,
                library_dirs=libdirs,
                libraries=['colibricore'],
                extra_compile_args=['--std=c++0x'] + extra_options,
                pyrex_gdb=True
                ) ]

setup(
    name = 'colibricore',
    author = "Maarten van Gompel",
    author_email = "proycon@anaproy.nl",
    description = ("Colibri Core is an NLP tool as well as a C++ and Python library (all included in this package) for working with basic linguistic constructions such as n-grams and skipgrams (i.e patterns with one or more gaps, either of fixed or dynamic size) in a quick and memory-efficient way. At the core is the tool ``colibri-patternmodeller`` which allows you to build, view, manipulate and query pattern models."),
    license = "GPL",
    keywords = "nlp computational_linguistics frequency ngram skipgram pmi cooccurrence linguistics",
    long_description=read('README.rst'),
    version = '2.4.10',
    ext_modules = extensions,
    cmdclass = {'build_ext': build_ext},
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Topic :: Text Processing :: Linguistic",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Operating System :: POSIX",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
    ],
    install_requires=['Cython >= 0.23'],
)
