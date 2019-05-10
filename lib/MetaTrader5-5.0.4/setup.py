#!/usr/bin/env python
from setuptools import setup, Extension

with open("README.rst", "r") as fh:
  long_description = fh.read()

setup(
  name = 'MetaTrader5',
  version = '5.0.4',
  description = 'API Connector to MetaTrader 5 Terminal',
  long_description = long_description,
  long_description_content_type = "text/x-rst",
  author = 'MetaQuotes Software Corp.',
  author_email = 'plugins@metaquotes.net',
  maintainer = 'MetaQuotes Software Corp.',
  maintainer_email = 'plugins@metaquotes.net',
  url = 'https://www.metatrader5.com',
  license = 'MIT',
  platforms = ["Windows"],
  packages = ['MetaTrader5'],
  ext_modules = [Extension(
    name = 'MetaTrader5.C',
    sources = [ 'src/MetaTrader.cpp', 'src/Interprocess/InterClient.cpp', 'src/Connector/MT5Connector.cpp' ],
    libraries = ['Advapi32', 'Shell32', 'User32', 'Ole32'],
    define_macros=[('UNICODE', None)],
    include_dirs=['src' ]
  )],
  classifiers=[
    'Development Status :: 5 - Production/Stable',
    'Topic :: Office/Business :: Financial',
    'License :: OSI Approved :: MIT License',
    'Programming Language :: Python :: 2',
    'Programming Language :: Python :: 2.7',
    'Programming Language :: Python :: 3',
    'Programming Language :: Python :: 3.4',
    'Programming Language :: Python :: 3.5',
    'Programming Language :: Python :: 3.6',
    'Programming Language :: Python :: 3.7'
    ],
  keywords='metatrader mt5 metaquotes mql5 forex currency exchange',
  project_urls={
    'Documentation': 'https://www.mql5.com/en/docs/integration/python_metatrader5',
    'Forum': 'https://www.mql5.com/en/forum'
  },
  python_requires='>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*, <4',
)
