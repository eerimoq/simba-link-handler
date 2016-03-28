#!/usr/bin/env python

from setuptools import setup

setup(name="link",
      version="0.0.1",
      description=("Add a description"),
      author="Erik Moqvist",
      author_email='simba@simba.com',
      packages=["link"],
      package_data={"": ["doc/*.rst",
                         "doc/conf.py",
                         "doc/Makefile",
                         "doc/doxygen.cfg",
                         "doc/link/*.rst",
                         "src/link.h",
                         "src/link.mk",
                         "src/link/*.h",
                         "src/*.c",
                         "tst/*/*"]},
      license='MIT',
      classifiers=[
          'License :: OSI Approved :: MIT License',
      ],
      url='https://github.com/eerimoq/simba')
