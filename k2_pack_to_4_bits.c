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

// Pack two integer data values into one byte as 4bit hex values
#define PACK_BYTE(u , v) ((uint8_t)((( (uint8_t) u ) & 0x0f) | ((( (uint8_t) v ) & 0x0f) << 4)))

/*----------------------------------------------------------------------------------------------------------------*/

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
  size_t      wordsize;
  float         *input;
  int8_t       *output;
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
    return 0;
    break;
  }
  return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

void expand_frame(_mrc *mrc){
  // Expand frame to float from mrc file 
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
      mrc->input[i] = (float) input16[i];
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
  // Read map header and data and return corresponding data structure
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
  /* Assign 8 bit int array for data, initialize it to zero and read frame 0. Note that
     the endianness is not corrected: architectures may therefore be cross-incompatible */
  if(mrc->n_crs[0] <= 0 || mrc->n_crs[1] <= 0 || mrc->n_crs[2] <= 0){
    printf(" Error reading %s - check endianness of image matches that of machine\n", filename);
    return -1;
  }
  int dim_4b0 = (mrc->n_crs[0] / 2) + (mrc->n_crs[0] % 2);
  mrc->wordsize = get_wordsize(mrc);
  if(!mrc->wordsize){
    printf(" Unsupported mrc mode! \n");
    return -1;
  }
  mrc->input = malloc(mrc->n_crs[0] * mrc->n_crs[1] * sizeof(float));
  mrc->output = calloc(dim_4b0 * mrc->n_crs[1] * mrc->n_crs[2], sizeof(int8_t));
  if(!mrc->input || !mrc->output){
    printf(" Error reading %s - no memory allocated\n", filename);
    return -1;
  }
  fread(mrc->input, mrc->wordsize, mrc->n_crs[0] * mrc->n_crs[1], mrc->file);
  expand_frame(mrc);
  return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

void next_mrc_frame(_mrc *mrc){
  // Read next frame from mrc file
  fread(mrc->input, mrc->wordsize, mrc->n_crs[0] * mrc->n_crs[1], mrc->file);
  expand_frame(mrc);
  return;
}

/*----------------------------------------------------------------------------------------------------------------*/

void close_mrc(_mrc *mrc){
  // Free header and data structures
  fclose(mrc->file);
  free(mrc->input);
  free(mrc->output);
  return;
}

/*----------------------------------------------------------------------------------------------------------------*/

int write_mrc_file(_mrc* mrc, char *filename){
  // Writes 4-bit MRC file given an mrc structure and data
  // Header values are set to placeholders for speed
  int32_t dim_4b0 = (mrc->n_crs[0] / 2) + (mrc->n_crs[0] % 2);
  mrc->length_xyz[0] = 2 * dim_4b0 * (mrc->length_xyz[0] / ((float) mrc->n_xyz[0]));
  mrc->n_crs[0] = 2 * dim_4b0;
  mrc->n_xyz[0] = 2 * dim_4b0;
  mrc->mode   = 101;
  mrc->d_min  =  -1;
  mrc->d_max  =  16;
  mrc->d_mean =   4;
  mrc->rms    = 4.0;
  // Output header to file
  fclose(mrc->file);
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
  // Output data to file
  fwrite(mrc->output, sizeof(int8_t), dim_4b0 * mrc->n_crs[1] * mrc->n_crs[2], mrc->file);
  return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

int undo_gain_ref(_mrc *grf, _mrc *mrc){
  // Undo multiplicative gain reference to mrc file
  int32_t i, j, imgp, grfp;
  float val, mismatch;
  // Check parity of file sizes
  if((grf->n_crs[0] != mrc->n_crs[0]) || (grf->n_crs[1] != mrc->n_crs[1])){
    return -1;
  }
  // Divide by gain reference
  int32_t counter = 0;
  for(j = 0; j < mrc->n_crs[1]; j++){
    for(i = 0; i < mrc->n_crs[0]; i++){
      imgp = j * mrc->n_crs[0] + i;
      grfp = j * mrc->n_crs[0] + i;
      if(grf->input[grfp] < 1e-6){
	mrc->input[imgp] = 15;
      } else {
	if((val = mrc->input[imgp] / grf->input[grfp]) && fabs(val - round(val)) > 1e-6){
	  counter++;
	}
	mrc->input[imgp] = round(val);
      }
    }
  }
  mismatch = ((float) counter) / ((float) (mrc->n_crs[1] * mrc->n_crs[0]));
  if(mismatch > 0.001){
    printf(" Error - %i mismatches detected - %f of all pixels! This is too high! Check your gain reference!\n", counter, mismatch);
    fflush(stdout);
    exit(-1);
  }
  return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

int pack_to_4bits(_mrc *mrc, int frmp){
  // Pack integer images to 4-bit hex
  int32_t i, j, k, outp, imgp, overflows = 0;
  int32_t dim_4b0 = (mrc->n_crs[0] / 2) + (mrc->n_crs[0] % 2);
  // Pack bytes over loop
  for(j = 0; j < mrc->n_crs[1]; j++){
    for(i = 0; i < dim_4b0; i++){
      imgp = j * mrc->n_crs[0] + i * 2;
      outp = frmp * dim_4b0 * mrc->n_crs[1] + j * dim_4b0 + i;
      if(mrc->input[imgp] < 0 || mrc->input[imgp] > 15){
	mrc->input[imgp] = 15;
	overflows++;
      }
      if(mrc->input[imgp + 1] < 0 || mrc->input[imgp + 1] > 15){
	mrc->input[imgp + 1] = 15;
	overflows++;
      }
      mrc->output[outp] = PACK_BYTE(mrc->input[imgp], mrc->input[imgp + 1]);
    }
  }
  if(overflows > 1000){
    printf(" %i overflows encountered in frame %i\n", overflows, frmp);
  }
  return 0;
}

/*----------------------------------------------------------------------------------------------------------------*/

int main(int argc, char *argv[]){
  // Read stdin and convert image stacks to 4bit
  int32_t i;
  _mrc mrc;
  _mrc grf;
  char filename[256];
  char filepath[512];
  // Argument description for information
  if(argc < 4){
    printf("\n Usage - (list_of_mrc_stacks) | %s gain_reference.mrc input_path/ output_path/ \n\n", argv[0]);
    return 1;
  }
  // Read gain reference
  if(read_mrc_file(&grf, argv[1])){
    printf("\n Gain reference file %s not viable!\n", argv[1]);
    close_mrc(&grf);
    return -1;
  }
  // Scan stdin and feed files to function
  printf("\n");
  while (scanf("%255s", filename) == 1 && !feof(stdin)){
    // Read and check file
    snprintf(filepath, 512, "%s/%s", argv[2], filename);
    if(read_mrc_file(&mrc, filepath)){
      printf("\n MRC file %s not found!\n", filepath);
      close_mrc(&mrc);
      close_mrc(&grf);
      return -1;
    }
    printf(" %s - #", filepath);
    fflush(stdout);
    // Undo gain reference and pack to 4-bit
    for(i = 0; i < mrc.n_crs[2]; i++){
      if(undo_gain_ref(&grf, &mrc) || pack_to_4bits(&mrc, i)){
	printf("\n MRC file %s doesn't match!\n", filepath);
	close_mrc(&mrc);
	close_mrc(&grf);
	return -1;
      }
      next_mrc_frame(&mrc);
      printf("#");
      fflush(stdout);
    }
    // Write out 4bit packed stacks
    snprintf(filepath, 512, "%s/%s", argv[3], filename);
    if (write_mrc_file(&mrc, filepath)){
      printf("\n Error writing %s!\n", filepath);
      close_mrc(&mrc);
      close_mrc(&grf);
      return -1;
    }
    printf(" - %s\n", filepath);
    fflush(stdout);
    close_mrc(&mrc);
  }
  // Over and out
  close_mrc(&grf);
  printf("\n ++++ That's all folks! ++++ \n\n");
  return 0;
}
