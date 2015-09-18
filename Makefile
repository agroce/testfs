# 
# Master Makefile for testfs
# Edited for ECE344 - Lab 6
#
# Author: Kuei Sun, Ashvin Goel
# E-mail: kuei.sun@mail.utoronto.ca
#         ashvin@eecg.toronto.edu
#
# University of Toronto
# 2014

PROGS := testfs mktestfs
COMMON_OBJECTS := bitmap.o block.o super.o inode.o dir.o file.o tx.o csum.o
COMMON_SOURCES := $(COMMON_OBJECTS:.o=.c)
DEFINES :=
INCLUDES := 
#CFLAGS := -O2 -Wall -Werror $(DEFINES) $(INCLUDES)
CFLAGS := -g -O0 -Wall -Werror $(DEFINES) $(INCLUDES)
SOURCES := testfs.c mktestfs.c $(COMMON_SOURCES)

all: depend $(PROGS)

testfs: testfs.o $(COMMON_OBJECTS)
	$(CC) -o $@ $(CFLAGS) $^ $(LOADLIBES) 
    
mktestfs: mktestfs.o $(COMMON_OBJECTS)
	$(CC) -o $@ $(CFLAGS) $^ $(LOADLIBES)     

.PHONY: zip clean $(BUILDS) $(CLEANERS)

depend:
	$(CC) -MM $(INCLUDES) $(SOURCES) > depend.mk

clean:
	rm -f *.o depend.mk $(PROGS) *.exe *.stackdump
	rm -rf *~

realclean: clean
	rm -f *.img
	
ifeq (depend.mk,$(wildcard depend.mk))
include depend.mk
endif
