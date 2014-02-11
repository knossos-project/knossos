from distutils.core import setup, Extension

""" This file creates a dll(windows) or .so(unix) which can be used in python.
	Simply call it with "python setup.py build"
"""

setup(name = "knossos", version="0.1", ext_modules = [Extension("knossos", ["scripting_engine.cpp"])])
