#!/usr/bin/env python3
from distutils.core import setup, Extension
from Cython.Distutils import build_ext
import glob

#cython's include is sucky unfortunately :( We'll have our own:
for filename in glob.glob("*.in.pyx"):
    with open(filename,'r') as f_in:
        with open(filename[:-6]+'pyx','w') as f_out:
            for line in f_in:
                found = line.find('@include')
                if found == -1:
                    f_out.write(line)
                else:
                    includefilename = line[found+9:].strip()
                    with open(includefilename) as f_inc:
                        for incline in f_inc:
                            f_out.write(" " * found + incline)


extensions = [ Extension("colibricore",
                ["unordered_map.pxd", "colibricore_classes.pxd", "colibricore_wrapper.pyx"],
                language='c++',
                include_dirs=['/home/proycon/local/include/colibri-core/', '/home/proycon/local/include/','/usr/include/', '/usr/include/libxml2','/usr/local/include/' ],
                library_dirs=['/home/proycon/local/lib/','/usr/lib','/usr/local/lib'],
                libraries=['colibricore'],
                extra_compile_args=['--std=c++0x'],
                pyrex_gdb=True
                ) ]

setup(
    name = 'colibricore',
    ext_modules = extensions,
    cmdclass = {'build_ext': build_ext},
)
