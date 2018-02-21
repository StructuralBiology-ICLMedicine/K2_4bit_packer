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

/*----------------------------------------------------------------------------------------------------------------*/

// MRC image structure
typedef struct {
  // All standard MRC header values
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
  // Convenience values and pointers to be assigned to the maps / file
  size_t      wordsize;
  float         *input;
  float        *output;
  FILE           *file;
} _mrc;

/*----------------------------------------------------------------------------------------------------------------*/

size_t get_wordsize(_mrc *mrc){
  // Obtain word size from mrc mode
  switch(mrc->mode){
  case 0:
    return sizeof(int8_t);
    break;
  case 1:
    return sizeof(int16_t);
    break;
  case 2:
    return sizeof(float);
    break;
  default:
    break;
    return 0;
  }
  return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

void expand_frame(_mrc *mrc){
  // Expand frame to float from mrc file - THIS FUNCTION ASSUMES 16x INFLATION FOR 16 BIT DATA AS FOR SerialEM
  int32_t i = mrc->n_crs[0] * mrc->n_crs[1];
  int8_t  *input8  = (int8_t *)  mrc->input;
  int16_t *input16 = (int16_t *) mrc->input;
  switch(mrc->mode){
  case 0:
    while(i--){
      mrc->input[i] = (float) input8[i];
    }
    return;
    break;
  case 1:
    while(i--){
      mrc->input[i] = ((float) input16[i]) / 16.0;
    }
    return;
    break;
  case 2:
    return;
    break;
  default:
    printf(" Error expanding frame \n");
    return;
    break;
  }  
  return;
}

/*----------------------------------------------------------------------------------------------------------------*/

int read_mrc_file(_mrc *mrc, char* filename){
  // Read map header and data and fill corresponding data structure
  if (!mrc){
    printf(" Error reading %s - no mrc structure allocated\n", filename);
    return -1;
  }
  mrc->file = fopen(filename, "rb");
  if (!mrc->file){
    printf(" Error reading %s - bad file handle\n", filename);
    fclose(mrc->file);
    return -1;
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
  /* Assign 32bit float array for data, initialize it to zero and read frame 0. Note that
     the endianness is not corrected: architectures may therefore be cross-incompatible */
  if(mrc->n_crs[0] <= 0 || mrc->n_crs[1] <= 0 || mrc->n_crs[2] <= 0){
    printf(" Error reading %s - check endianness of image matches that of machine\n", filename);
    return -1;
  }
  mrc->wordsize = get_wordsize(mrc);
  if(!mrc->wordsize){
    printf(" Error reading %s - unsupported mrc file mode\n", filename);
    return -1;
  }
  mrc->input = malloc(mrc->n_crs[1] * mrc->n_crs[0] * sizeof(float));
  if (!mrc->input){
    printf(" Error reading %s - no memory allocated\n", filename);
    return -1;
  }
  fread(mrc->input, mrc->wordsize, mrc->n_crs[1] * mrc->n_crs[0], mrc->file);
  expand_frame(mrc);
  return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

void next_mrc_frame(_mrc *mrc){
  // Read next frame from mrc file
  fread(mrc->input, mrc->wordsize, mrc->n_crs[1] * mrc->n_crs[0], mrc->file);
  expand_frame(mrc);
  return;
}

/*----------------------------------------------------------------------------------------------------------------*/

void close_mrc(_mrc *mrc){
  // Free header and data structures
  fclose(mrc->file);
  free(mrc->input);
  return;
}

/*----------------------------------------------------------------------------------------------------------------*/

int write_mrc_file(_mrc* mrc, char *filename){
  // Writes MRC file given an mrc structure and data
  int32_t i, j, k, brokenpix = 0;
  float mean;
  // Update to single frame
  mrc->n_crs[2] = 1;
  mrc->n_xyz[2] = 1;
  // Output header to file
  mrc->file = fopen(filename, "wb");
  if (!mrc->file){
    fprintf(stderr, "\n Error writing output - bad file handle\n");
    return -1;
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
  // Flush low values to local mean
  int32_t neighbours[20] = {2 * mrc->n_crs[1] + 1,
			    2 * mrc->n_crs[1] + 0,
			    2 * mrc->n_crs[1] - 1,
			    1 * mrc->n_crs[1] + 2,
                            1 * mrc->n_crs[1] + 1,
			    1 * mrc->n_crs[1] + 0,
			    1 * mrc->n_crs[1] - 1,
			    1 * mrc->n_crs[1] - 2,
			    0 * mrc->n_crs[1] + 2,
                            0 * mrc->n_crs[1] + 1,
			    0 * mrc->n_crs[1] - 1,
			    0 * mrc->n_crs[1] - 2,
			   -1 * mrc->n_crs[1] + 2,
                           -1 * mrc->n_crs[1] + 1,
			   -1 * mrc->n_crs[1] + 0,
			   -1 * mrc->n_crs[1] - 1,
			   -1 * mrc->n_crs[1] - 2,
			   -2 * mrc->n_crs[1] + 1,
			   -2 * mrc->n_crs[1] + 0,
			   -2 * mrc->n_crs[1] - 1};
  for(i = 0; i < mrc->n_crs[0] * mrc->n_crs[1]; i++){
    if(mrc->output[i] < 0.0625){
      brokenpix++;
      mean = 0;
      k = 0;
      for(j = 0; j < 20; j++){
	if(mrc->output[((mrc->n_crs[0] * mrc->n_crs[1]) + i + neighbours[j]) % (mrc->n_crs[0] * mrc->n_crs[1])] > 0.0625){
	  mean += mrc->output[((mrc->n_crs[0] * mrc->n_crs[1]) + i + neighbours[j]) % (mrc->n_crs[0] * mrc->n_crs[1])];
	  k++;
	}
      }
      mrc->output[i] = mean / (k ? k : 1.0);
    }
  }
  printf("\n %i broken pixels identified and masked...\n", brokenpix);
  // Output data to file
  fwrite(mrc->output, sizeof(float), mrc->n_crs[1] * mrc->n_crs[0], mrc->file);
  return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

int extract_gain(_mrc *mrc){
  // Extract gain reference from images
  int32_t i, j, ptr;
  double input, output, remainder, eps;
  // Set epsilon based on mode
  switch(mrc->mode){
  case 0:
    eps = 1e-4;
  case 1:
    eps = 1e-1;
  case 2:
    eps = 1e-4;
  }
  // Extract gain reference
  for(j = 0; j < mrc->n_crs[1]; j++){
    for(i = 0; i < mrc->n_crs[0]; i++){
      ptr = j * mrc->n_crs[0] + i;
      if(mrc->output[ptr] <= 0.0){
	mrc->output[ptr] = mrc->input[ptr];
      } else {
	input  = (double) mrc->input[ptr];
	output = (double) mrc->output[ptr];
	remainder = fmodf(input, output) / output;
	if(remainder > eps && remainder < 1.0 - eps){
	  mrc->output[ptr] = (float) (output * remainder);
	} else if(remainder > 1.0 + eps){
	  mrc->output[ptr] = (float) (input / remainder);
	} else if(remainder >= 1.0 - eps && remainder <= 1.0 + eps){
	  mrc->output[ptr] = (float) ((fmodf(input, output) + output) / 2.0);
	}
      }
    }
  }
  return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

int main(int argc, char *argv[]){
  // Read stdin and estimate gain reference from image stacks
  int32_t i;
  _mrc mrc;
  char filename[256];
  mrc.output = NULL;
  // Argument description for information
  if (argc < 2){
    printf("\n Usage - (list_of_mrc_stacks) | %s output_filename \n\n", argv[0]);
    return 1;
  }
  // Scan stdin and feed files to function
  printf("\n");
  while (scanf("%255s", filename) == 1 && !feof(stdin)){
    // Read file
    if(read_mrc_file(&mrc, filename)){
      printf("\n MRC file %s not found!\n", filename);
      close_mrc(&mrc);
      return -1;
    }
    printf(" %s - #", filename);
    fflush(stdout);
    if(!mrc.output){
      mrc.output = calloc(mrc.n_crs[1] * mrc.n_crs[0], sizeof(float));
    }
    // Loop over file
    for(i = 0; i < mrc.n_crs[2]; i++){
      if(extract_gain(&mrc)){
	printf("\n MRC file %s doesn't match!\n", filename);
	close_mrc(&mrc);
	return -1;
      }
      next_mrc_frame(&mrc);
      printf("#");
      fflush(stdout);
    }
    close_mrc(&mrc);
    printf(" \n");
    fflush(stdout);
  }
  printf("\n -> %s\n", argv[1]);
  fflush(stdout);
  // Write gain estimate
  if (write_mrc_file(&mrc, argv[1])){
    printf("\n Error writing %s!\n", argv[1]);
    close_mrc(&mrc);
    return -1;
  }
  // Over and out
  free(mrc.output);
  printf("\n ++++ That's all folks! ++++ \n\n");
  return 0;
}
