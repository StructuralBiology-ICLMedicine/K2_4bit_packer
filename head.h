
/*                                                                         
 * Copyright 27/11/2018 - Dr. Christopher H. S. Aylett                     
 *                                                                         
 * This program is free software; you can redistribute it and/or modify    
 * it under the terms of version 3 of the GNU General Public License as    
 * published by the Free Software Foundation.                              
 *                                                                         
 * This program is distributed in the hope that it will be useful,         
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           
 * GNU General Public License for more details - YOU HAVE BEEN WARNED!     
 *                                                                         
 * Program: K2 bit packer V1.1                                             
 *                                                                         
 * Authors: Chris Aylett                                                   
 *                                                                         
 */

// Library header inclusion for linking                                     
#include <sched.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <float.h>

// Epsilon for bad pixels
#define EPS 1E-6

// Pack two integer data values into one byte as 4bit hex values
#define PACK_BYTE(u , v) ((uint8_t)((( (uint8_t) u ) & 0x0f) | ((( (uint8_t) v ) & 0x0f) << 4)))

// MRC image structure
typedef struct {
  // All standard MRC header values - crs refer to column, row and segment
  int32_t     n_crs[3];
  int32_t         mode;
  int32_t start_crs[3];
  int32_t     n_xyz[3];
  float  length_xyz[3];
  float   angle_xyz[3];
  int32_t   map_crs[3];
  float          d_min;
  float          d_max;
  float         d_mean;
  int32_t         ispg;
  int32_t       nsymbt;
  int32_t    extra[25];
  int32_t   ori_xyz[3];
  char          map[4];
  char       machst[4];
  float            rms;
  int32_t        nlabl;
  char      label[800];
  // Convenience values and pointers to be assigned to the map and file data
  float        *buffer;
  float         *input;
  int8_t       *output;
  FILE           *file;
} _mrc;

// Thread argument structure
typedef struct {
  _mrc    *mrc;
  double *gain;
  double  rmsd;
  int64_t maxr;
  int64_t badp;
  int64_t ovfl;
  int64_t cont;
  int64_t size;
  int32_t mode;
  int32_t fram;
  int32_t thrd;
  int32_t step;
} _arg;

int thread_number(void);
// Get thread number

double *parse_args(int32_t *mode, int argc, char **argv);
// Read arguments and load gain

int read_mrc(_mrc *mrc, char* filename);
// Read map header and fill mrc struct

int write_mrc(_mrc* mrc, char *filename);
// Writes 4-bit MRC file given an mrc file structure
// Several header values are ignored to better speed

void gain_mrc(double* gain, char *filename, int64_t size, _mrc* mrc);
// Float Gain MRC file for convenience

void read_frame(_mrc *mrc);
// Read next frame from mrc file

void close_mrc(_mrc *mrc);
// Free header and data structures

double *read_raw(char *filename);
// Read raw gain reference in double

void write_raw(double* gain, char *filename, int64_t size);
// Writes 64-bit raw file given the double gain data
// No header data or any other info included in file

void estimate_gain(_arg *arg);
// Extract gain reference from frame
// Thread function

void refine_gain(_arg *arg);
// Refine gain reference given frame
// Thread function

void remove_gain(_arg *arg);
// Remove gain reference from frame
// Thread function

void pack_to_4bits(_arg *arg);
// Pack integer images to 4-bit hex
// Thread function
