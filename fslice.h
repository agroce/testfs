/* Copyright 2015 Peter Goodman (peter@trailofbits.com), all rights reserved. */

#ifndef FSLICE_H_
#define FSLICE_H_

__attribute__((weak))
extern void __fslice_read_block(void *addr, size_t size, int nr);

#define fslice_read_block(a,s,nr) \
  if (__fslice_read_block) __fslice_read_block(a,s,nr)

__attribute__((weak))
extern void __fslice_write_block(void *addr, size_t size, int nr);

#define fslice_write_block(a,s,nr) \
  if (__fslice_write_block) __fslice_write_block(a,s,nr)

__attribute__((weak))
extern void __fslice_name(void *name, int len);

#define fslice_name(a,n) \
  if (__fslice_name) __fslice_name(a,n)

__attribute__((weak))
extern void __fslice_data(void *content, int len);

#define fslice_data(a,n) \
  if (__fslice_data) __fslice_data(a,n)

#endif  // FSLICE_H_
