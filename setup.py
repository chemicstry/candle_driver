#!/usr/bin/env python

import io
import os
import sys
from setuptools import setup, Extension

# Package meta-data.
NAME = 'candle_driver'
DESCRIPTION = 'Python wrapper for the candle (gs_usb) windows driver.'
URL = 'https://github.com/chemicstry/candle_driver'
EMAIL = 'chemicstry@gmail.com'
AUTHOR = 'Jurgis Balčiūnas'
REQUIRES_PYTHON = '>=3.6.0'
VERSION = '0.1.0'

here = os.path.abspath(os.path.dirname(__file__))

# Import the README and use it as the long-description.
# Note: this will only work if 'README.md' is present in your MANIFEST.in file!
try:
  with io.open(os.path.join(here, 'README.md'), encoding='utf-8') as f:
    long_description = '\n' + f.read()
except FileNotFoundError:
  long_description = DESCRIPTION

setup(
  name=NAME,
  version=VERSION,
  description=DESCRIPTION,
  long_description=long_description,
  long_description_content_type='text/markdown',
  author=AUTHOR,
  author_email=EMAIL,
  python_requires=REQUIRES_PYTHON,
  url=URL,
  license='MIT',
  ext_modules=[Extension("candle_driver",
    sources=[
      "src/py_candle_driver.c",
      "src/py_candle_device.c",
      "src/py_candle_channel.c",
      "src/fifo.c",
      "src/candle_api/candle.c",
      "src/candle_api/candle_ctrl_req.c"
    ],
    libraries=[
      "SetupApi",
      "Ole32",
      "winusb",
    ]
  )],
)
