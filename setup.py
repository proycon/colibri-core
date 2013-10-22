#!/usr/bin/env python3
from distutils.core import setup, Extension
from Cython.Distutils import build_ext

extensions = [ Extension("colibricore",
                ["unordered_map.pxd", "colibricore_classes.pxd", "colibricore_wrapper.pyx"],
                language='c++',
                include_dirs=['/home/proycon/local/include/colibri-core/', '/home/proycon/local/include/', '/usr/include/libxml2' ],
                library_dirs=['/home/proycon/local/lib/','/usr/include/lib'],
                libraries=['colibri-core'],
                extra_compile_args=['--std=c++0x'],
                pyrex_gdb=True
                ) ]

setup(
    name = 'colibricore',
    ext_modules = extensions,
    cmdclass = {'build_ext': build_ext},
)
