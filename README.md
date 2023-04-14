testfs
======

This is a DeepState test harness for TestFS, which is a user level toy file system that is similar to ext3.

TestFS comes out of U. Toronto file system research:

J. Sun, D. Fryer, A. Goel, and A. D. Brown, “Using declarative invariants for protecting file-system integrity,” in Proceedings of the 6th Workshop on Programming Languages and Operating Systems. 2011.  http://www.sigops.org/sosp/sosp11/workshops/plos/06-sun.pdf

This version is based on a series of implementations, first adapted to DeepState by Peter Goodman (https://github.com/pgoodman/testfs), now maintained by Alex Groce.  The primary change is that a broken (undefined behavior causing) inode hash table has been disabled, with inode lookups always going to "disk."  Disk here is a block-device emulated with a Big Old Array.  Also, this adds a semi-POSIX layer on top of the low-level interface to TestFS.

Tests.cpp shows what a DeepState harness for a POSIX-like interface looks like.

The tests check that resets at inopportune times do not make the file system unmountable (which does not hold).

Usage
=====

Easiest way to play with this is to use the [DeepState docker](https://github.com/trailofbits/deepstate#DOCKER), and:

1.  clone this repo
2.  `cd testfs; make`

You then have a binary you can fuzz with Eclipser, a binary you can fuzz with libFuzzer, a binary you can fuzz with AFL, and a binary that collects code coverage information.
