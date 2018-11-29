
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

void remove_gain(_arg *arg){
  // Undo multiplicative gain reference to mrc file
  int32_t i;
  double cur, dev;
  // Refine gain reference
  for (i = arg->thrd; i < arg->size; i += arg->step){
    if (!isfinite(arg->gain[i]) || !isfinite(arg->mrc->input[i])){
      arg->mrc->input[i] = 0.0;
    }
    if (arg->gain[i] <= 0.0){
      cur = round(arg->mrc->input[i] / (arg->gain[i] / -15.0));
      if (cur > 15){
	arg->ovfl++;
	cur = 15;
      }
      dev = cur - (arg->mrc->input[i] / (arg->gain[i] / -15.0));
    } else {
      cur = round(arg->mrc->input[i] / arg->gain[i]);
      dev = cur - (arg->mrc->input[i] / arg->gain[i]);
      if (cur > 15){
	arg->ovfl++;
	cur = 15;
      }
      arg->rmsd += fabs(dev);
    }
    if (fabs(dev) > EPS){
      arg->badp++;
    }
    if (arg->gain[i] <= 0.0){
      arg->maxr++;
    }
    arg->mrc->input[i] = cur;
  }
  return;
}

void pack_to_4bits(_arg *arg){
  // Pack integer images to 4-bit hex
  int32_t i, j, in_p, int_p, out_p;
  int32_t dim_4b0 = (arg->mrc->n_crs[0] / 2) + (arg->mrc->n_crs[0] % 2);
  for(j = 0; j < arg->mrc->n_crs[1]; j++){
    out_p = arg->fram * dim_4b0 * arg->mrc->n_crs[1] + j * dim_4b0;
    int_p = j * arg->mrc->n_crs[0];
    for(i = arg->thrd; i < dim_4b0; i += arg->step){
      in_p = int_p + 2 * i;
      arg->mrc->output[out_p + i] = PACK_BYTE(arg->mrc->input[(in_p)], arg->mrc->input[(in_p + 1)]);
    }
  }
  return;
}
