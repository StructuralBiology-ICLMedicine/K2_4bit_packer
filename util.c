
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
#include "head.h"

// Utility functions

int thread_number(void){
  // Obtain thread number from environmental variables
  char* thread_number = getenv("OMP_NUM_THREADS");
  int n = 0;
  if (thread_number){
    // If thread number specified by user - use this one
    n = atoi(thread_number);
  }
  if (n < 1){
    // If thread number still not set - try sysconf
    n = sysconf(_SC_NPROCESSORS_ONLN);
  }
  if (n < 1){
    // If variables are both empty - use a single thread
    n = 1;
  }
  return n;
}

double *parse_args(int32_t *mode, int argc, char **argv){
  // Capture user requested settings
  int i;
  double *gain = NULL;
  for (i = 1; i < argc; i++){
    if (!strcmp(argv[i], "--gain")){
     *mode = 1;
    } else if (!strcmp(argv[i], "--pack") && ((i + 1) < argc)){
     *mode = 0;
      gain = read_raw(argv[i + 1]);
    }
  }
  if (gain == NULL && !*mode){
    // Print usage and disclaimer
    printf("\n\t Usage - (list_of_mrc_stacks) | %s [ --gain ][ --pack gain.raw ] \n\n", argv[0]);
    exit(1);
  }
  return gain;
}

int read_mrc(_mrc *mrc, char* filename){
  // Read map header and data and return corresponding data structure
  if (!mrc){
    printf("\t Error reading %s - no mrc structure allocated\n", filename);
    return 1;
  }
  mrc->file = fopen(filename, "rb");
  if (!mrc->file){
    printf("\t Error reading %s - bad file handle\n", filename);
    fclose(mrc->file);
    return 1;
  }
  fread(&mrc->n_crs,      4, 3,   mrc->file);
  fread(&mrc->mode,       4, 1,   mrc->file);
  fread(&mrc->start_crs,  4, 3,   mrc->file);
  fread(&mrc->n_xyz,      4, 3,   mrc->file);
  fread(&mrc->length_xyz, 4, 3,   mrc->file);
  fread(&mrc->angle_xyz,  4, 3,   mrc->file);
  fread(&mrc->map_crs,    4, 3,   mrc->file);
  fread(&mrc->d_min,      4, 1,   mrc->file);
  fread(&mrc->d_max,      4, 1,   mrc->file);
  fread(&mrc->d_mean,     4, 1,   mrc->file);
  fread(&mrc->ispg,       4, 1,   mrc->file);
  fread(&mrc->nsymbt,     4, 1,   mrc->file);
  fread(&mrc->extra,      4, 25,  mrc->file);
  fread(&mrc->ori_xyz,    4, 3,   mrc->file);
  fread(&mrc->map,        1, 4,   mrc->file);
  fread(&mrc->machst,     1, 4,   mrc->file);
  fread(&mrc->rms,        4, 1,   mrc->file);
  fread(&mrc->nlabl,      4, 1,   mrc->file);
  fread(&mrc->label,      1, 800, mrc->file);
  /* Assign 8 bit int array for data, initialize it to zero and read frame 0. Note that
     the endianness is not corrected: architectures may therefore be cross-incompatible */
  if(mrc->n_crs[0] <= 0 || mrc->n_crs[1] <= 0 || mrc->n_crs[2] <= 0){
    printf("\t Error reading %s - check endianness of image matches that of machine\n", filename);
    return 1;
  }
  int32_t dim_4b0 = (mrc->n_crs[0] / 2) + (mrc->n_crs[0] % 2);
  if(mrc->mode != 2){
    printf("\t Unsupported mrc mode! \n");
    return 1;
  }
  mrc->input  = malloc(mrc->n_crs[0] * mrc->n_crs[1] * sizeof(float));
  mrc->buffer = malloc(mrc->n_crs[0] * mrc->n_crs[1] * sizeof(float));
  mrc->output = calloc(dim_4b0 * mrc->n_crs[1] * mrc->n_crs[2], sizeof(int8_t));
  if(!mrc->input || !mrc->buffer || !mrc->output){
    printf("\t Error reading %s - no memory allocated\n", filename);
    return 1;
  }
  fread(mrc->input, sizeof(float), mrc->n_crs[0] * mrc->n_crs[1], mrc->file);
  return 0;
}

void read_frame(_mrc *mrc){
  // Read next frame from mrc file
  fread(mrc->buffer, sizeof(float), mrc->n_crs[0] * mrc->n_crs[1], mrc->file);
  return;
}

void close_mrc(_mrc *mrc){
  // Free header and data structures
  if (mrc->file){
    fclose(mrc->file);
  }
  if (mrc->input){
    free(mrc->input);
  }
  if (mrc->buffer){
    free(mrc->buffer);
  }
  if (mrc->output){
    free(mrc->output);
  }
  return;
}

int write_mrc(_mrc* mrc, char *filename){
  // Writes 4-bit MRC file given an mrc structure and data
  // Header values are set to placeholders for speed
  int32_t dim_4b0 = (mrc->n_crs[0] / 2) + (mrc->n_crs[0] % 2);
  mrc->length_xyz[0] = 2 * dim_4b0 * (mrc->length_xyz[0] / ((float) mrc->n_xyz[0]));
  mrc->n_crs[0] = 2 * dim_4b0;
  mrc->n_xyz[0] = 2 * dim_4b0;
  mrc->mode   =  101;
  mrc->d_min  =  0.0;
  mrc->d_max  = 16.0;
  mrc->d_mean =  1.0;
  mrc->rms    =  4.0;
  // Output header to file
  fclose(mrc->file);
  mrc->file = fopen(filename, "wb");
  if (!mrc->file){
    fprintf(stderr, "\n\t Error writing output - bad file handle\n");
    return 1;
  }
  fwrite(&mrc->n_crs,      4, 3,   mrc->file);
  fwrite(&mrc->mode,       4, 1,   mrc->file);
  fwrite(&mrc->start_crs,  4, 3,   mrc->file);
  fwrite(&mrc->n_xyz,      4, 3,   mrc->file);
  fwrite(&mrc->length_xyz, 4, 3,   mrc->file);
  fwrite(&mrc->angle_xyz,  4, 3,   mrc->file);
  fwrite(&mrc->map_crs,    4, 3,   mrc->file);
  fwrite(&mrc->d_min,      4, 1,   mrc->file);
  fwrite(&mrc->d_max,      4, 1,   mrc->file);
  fwrite(&mrc->d_mean,     4, 1,   mrc->file);
  fwrite(&mrc->ispg,       4, 1,   mrc->file);
  fwrite(&mrc->nsymbt,     4, 1,   mrc->file);
  fwrite(&mrc->extra,      4, 25,  mrc->file);
  fwrite(&mrc->ori_xyz,    4, 3,   mrc->file);
  fwrite(&mrc->map,        1, 4,   mrc->file);
  fwrite(&mrc->machst,     1, 4,   mrc->file);
  fwrite(&mrc->rms,        4, 1,   mrc->file);
  fwrite(&mrc->nlabl,      4, 1,   mrc->file);
  fwrite(&mrc->label,      1, 800, mrc->file);
  // Output data to file
  if (fwrite(mrc->output, sizeof(int8_t), dim_4b0 * mrc->n_crs[1] * mrc->n_crs[2], mrc->file) == dim_4b0 * mrc->n_crs[1] * mrc->n_crs[2]){
    return 0;
  }
  return 1;
}

double *read_raw(char *filename){
  // Output data to file
  int64_t size;
  FILE *file = NULL;
  file = fopen(filename, "rb");
  if (!file){
    printf("\t Error reading %s - bad file handle\n", filename);
    fclose(file);
    exit(1);
  }
  fread(&size, sizeof(int64_t),  1, file);
  double *gain = malloc((size_t) size * sizeof(double));
  fread(gain, sizeof(double), (size_t) size, file);
  fclose(file);
  return gain;
}

void write_raw(double* gain, char *filename, int64_t size){
  // Output data to file
  FILE *file = NULL;
  file = fopen(filename, "wb");
  if (!file){
    printf("\t Error reading %s - bad file handle\n", filename);
    fclose(file);
    exit(1);
  }
  fwrite(&size, sizeof(int64_t),  1, file);
  fwrite(gain, sizeof(double), size, file);
  return;
}

void gain_mrc(double* gain, char *filename, int64_t size, _mrc *mrc){
  // Output gain data to mrc file
  int32_t i;
  // Header values are set to placeholders for speed
  mrc->n_crs[2] = 1;
  mrc->n_xyz[2] = 1;
  mrc->mode   =   2;
  mrc->d_min  = 0.0;
  mrc->d_max  = 2.0;
  mrc->d_mean = 1.0;
  mrc->rms    = 0.1;
  // Output header to file
  fclose(mrc->file);
  mrc->file = fopen(filename, "wb");
  if (!mrc->file){
    fprintf(stderr, "\n\t Error writing output - bad file handle\n");
    return;
  }
  fwrite(&mrc->n_crs,      4, 3,   mrc->file);
  fwrite(&mrc->mode,       4, 1,   mrc->file);
  fwrite(&mrc->start_crs,  4, 3,   mrc->file);
  fwrite(&mrc->n_xyz,      4, 3,   mrc->file);
  fwrite(&mrc->length_xyz, 4, 3,   mrc->file);
  fwrite(&mrc->angle_xyz,  4, 3,   mrc->file);
  fwrite(&mrc->map_crs,    4, 3,   mrc->file);
  fwrite(&mrc->d_min,      4, 1,   mrc->file);
  fwrite(&mrc->d_max,      4, 1,   mrc->file);
  fwrite(&mrc->d_mean,     4, 1,   mrc->file);
  fwrite(&mrc->ispg,       4, 1,   mrc->file);
  fwrite(&mrc->nsymbt,     4, 1,   mrc->file);
  fwrite(&mrc->extra,      4, 25,  mrc->file);
  fwrite(&mrc->ori_xyz,    4, 3,   mrc->file);
  fwrite(&mrc->map,        1, 4,   mrc->file);
  fwrite(&mrc->machst,     1, 4,   mrc->file);
  fwrite(&mrc->rms,        4, 1,   mrc->file);
  fwrite(&mrc->nlabl,      4, 1,   mrc->file);
  fwrite(&mrc->label,      1, 800, mrc->file);
  // Output data to file
  float *tmp = malloc(size * sizeof(float));
  for (i = 0; i < size; i++){
    if (!isfinite((float) gain[i]) || (float) gain[i] == 0.0){
      tmp[i] = 1.0;
    } else if (gain[i] < 0.0){
      tmp[i] = (float) gain[i] / -15.0;
    } else {
      tmp[i] = (float) gain[i];
    }
  }
  fwrite(tmp, sizeof(float), size, mrc->file);
  return;
}
