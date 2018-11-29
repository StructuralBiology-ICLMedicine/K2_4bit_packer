
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

// Main algorithm function
int main(int argc, char *argv[]){

  // Read stdin and estimate or refine gain, or convert image stacks to 4bit

  // Parameters
  int32_t i, j, flag = 0, mode = 0;
  int64_t ovfl, badp, maxr, size = 0;
  long double rmsd;
  double *gain = NULL;
  float *tmp;
  _mrc mrc;
  char file_r[1024];
  char file_w[1024];

  // Argument capture
  gain = parse_args(&mode, argc, argv);

  // Thread setup
  int n = thread_number();
  _arg arg[n];
  for(i = 0; i < n; i++){
    arg[i].mrc  = &mrc;
    arg[i].gain = gain;
    arg[i].thrd = i;
    arg[i].step = n;
    arg[i].cont = 0;
  }
  pthread_t threads[n];
  pthread_t frame_thread;
  mrc.input  = NULL;
  mrc.buffer = NULL;
  mrc.output = NULL;
  
  // Scan stdin and feed files to function
  printf("\n");
  while (scanf("%1019s", file_r) == 1 && !feof(stdin)){

    // Read and check file
    if(read_mrc(&mrc, file_r)){
      printf("\n\t MRC file %s not found!\n", file_r);
      close_mrc(&mrc);
      continue;
    }
    if (size == 0){
      size = mrc.n_crs[0] * mrc.n_crs[1];
      for(i = 0; i < n; i++){
	arg[i].size = size;
      }
    } else if (mrc.n_crs[0] * mrc.n_crs[1] != size){
      printf("\n\t MRC file %s incorrect size!\n", file_r);
      close_mrc(&mrc);
      continue;
    }
    if(gain == NULL){
      gain = malloc(size * sizeof(double));
      for(i = 0; i < n; i++){
	arg[i].gain = gain;
	arg[i].size = size;
	if (arg[i].gain == NULL){
	  printf("\n\t Memory allocation failed!\n");
	  fflush(stdout);
	  exit(1);
	}
      }
    }
    printf("\t %s -> #", file_r);
    fflush(stdout);

    // Reporting variables
    rmsd = 0.0;
    badp =   0;
    ovfl =   0;
    maxr =   0;

    // Calc gain reference or pack to 4-bit
    for(j = 0; j < mrc.n_crs[2]; j++){
      if (pthread_create(&frame_thread, NULL, (void*) read_frame, &mrc)){
	printf("\n\t Thread initialisation failed!\n");
	fflush(stdout);
	exit(1);
      }

      // Start threads
      for (i = 0; i < n; i++){
	arg[i].rmsd = 0.0;
	arg[i].maxr =   0;
	arg[i].badp =   0;
	arg[i].ovfl =   0;
	arg[i].fram =   j;
	arg[i].cont++;
	if (mode == 1){
	  if (pthread_create(&threads[i], NULL, (void*) estimate_gain, &arg[i])){
	    printf("\n\t Thread initialisation failed!\n");
	    fflush(stdout);
	    exit(1);
	  }
	} else if (mode > 1){
	  if (pthread_create(&threads[i], NULL, (void*) refine_gain, &arg[i])){
	    printf("\n\t Thread initialisation failed!\n");
	    fflush(stdout);
	    exit(1);
	  }
	} else {
	  if (pthread_create(&threads[i], NULL, (void*) remove_gain, &arg[i])){
	    printf("\n\t Thread initialisation failed!\n");
	    fflush(stdout);
	    exit(1);
	  }
	}
      }

      // Join threads
      for (i = 0; i < n; i++){
	if (pthread_join(threads[i], NULL)){
	  printf("\n\t Thread failed during run!\n");
	  fflush(stdout);
	  exit(1);
	}
	rmsd += arg[i].rmsd;
	maxr += arg[i].maxr;
	badp += arg[i].badp;
	ovfl += arg[i].ovfl;
      }
      if (pthread_join(frame_thread, NULL)){
	printf("\n\t Thread failed during run!\n");
	fflush(stdout);
	exit(1);
      }

      // Pack if necessary
      if (!mode){
	for (i = 0; i < n; i++){
	  if (pthread_create(&threads[i], NULL, (void*) pack_to_4bits, &arg[i])){
	    printf("\n\t Thread initialisation failed!\n");
	    fflush(stdout);
	    exit(1);
	  }
	}
	for (i = 0; i < n; i++){
	  if (pthread_join(threads[i], NULL)){
	    printf("\n\t Thread failed during run!\n");
	    fflush(stdout);
	    exit(1);
	  }
	}
      }

      tmp = mrc.input;
      mrc.input = mrc.buffer;
      mrc.buffer = tmp;
      printf("#");
      fflush(stdout);
    }

    // Write out 4bit packed stacks if required
    if(!mode){
      snprintf(file_w, 1023, "%s%s", file_r, "4bit");
      if (write_mrc(&mrc, file_w)){
	printf(" - Error writing %s!\n", file_w);
	close_mrc(&mrc);
	continue;
      }
      printf(" -> 4bit ");
    }

    // Report results to user
    rmsd /= (long double) (mrc.n_crs[2] * size);
    printf("\n\t MeanDev %12.3Lg   |   ErrPix %12lli   |   BadPix %12lli   |   Overflows %12lli   |   TotalPix %10lli\n", rmsd, badp, maxr, ovfl, size * mrc.n_crs[2]);
    fflush(stdout);

    // Convenience gain mrc
    if(!mode && !flag){
      gain_mrc(gain, "gain.mrc", size, &mrc);
      flag++;
    } else if (mode == 1 && arg[0].cont >= 256){
      mode++;
      flag++;
    }
    close_mrc(&mrc);

    // Write raw if required
    if (mode == 2 && arg[0].cont >= 512){
      write_raw(arg[0].gain, "gain.raw", size);
    }
  }

  // Over and out
  printf("\n\t ++++ That's all folks! ++++ \n\n");
  return 0;
}
