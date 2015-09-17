#!/usr/bin/env python3
from __future__ import print_function
from distutils.core import setup, Extension
from Cython.Distutils import build_ext
import glob
import os
import sys


from os.path import expanduser
HOMEDIR = expanduser("~")

ROOTDIR = os.path.abspath(os.path.dirname(__file__))

#cython's include is sucky unfortunately :( We'll have our own:
for filename in glob.glob(os.path.join(ROOTDIR ,"*.in.pyx")):
    with open(filename,'r') as f_in:
        with open(filename[:-6]+'pyx','w') as f_out:
            for line in f_in:
                found = line.find('@include')
                if found == -1:
                    f_out.write(line)
                else:
                    includefilename = line[found+9:].strip()
                    with open(os.path.join(ROOTDIR,includefilename)) as f_inc:
                        for incline in f_inc:
                            f_out.write(" " * found + incline)



def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

#not the most elegant hack, but we're going to try compile colibri-core here before we continue with the rest:

#defaults:
includedirs = ["/usr/include/colibri-core", "/usr/include/libxml2"]
libdirs = ["/usr/lib"]
if ('install' in sys.argv[1:] or 'build_ext' in sys.argv[1:]) and not '--help' in sys.argv[1:]:
    if '-n' in sys.argv[1:]:
        print("Dry run not supported for colibri-core compilation",file=sys.stderr)
        sys.exit(2)

    os.chdir(ROOTDIR)

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

    if prefix:
        includedirs += [prefix + "/include/colibri-core", prefix + "/include", prefix + "/include/libxml2"]
        libdirs += [prefix + "/lib"]







extensions = [ Extension("colibricore",
                ["unordered_map.pxd", "colibricore_classes.pxd", "colibricore_wrapper.pyx"],
                language='c++',
                include_dirs=includedirs,
                library_dirs=libdirs,
                libraries=['colibricore'],
                extra_compile_args=['--std=c++0x'],
                pyrex_gdb=True
                ) ]

setup(
    name = 'colibricore',
    author = "Maarten van Gompel",
    author_email = "proycon@anaproy.nl",
    description = ("Colibri core is an NLP tool as well as a C++ and Python library for working with basic linguistic constructions such as n-grams and skipgrams (i.e patterns with one or more gaps, either of fixed or dynamic size) in a quick and memory-efficient way. At the core is the tool ``colibri-patternmodeller`` which allows you to build, view, manipulate and query pattern models."),
    license = "GPL",
    keywords = "nlp computational_linguistics frequency ngram skipgram pmi cooccurrence linguistics",
    long_description=read('README.rst'),
    version = '1.0',
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
