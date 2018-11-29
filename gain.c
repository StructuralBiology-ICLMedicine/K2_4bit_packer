
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

void estimate_gain(_arg *arg){
  // Extract gain reference from images
  int32_t i;
  double cur, dev;
  // Extract gain reference
  for (i = arg->thrd; i < arg->size; i += arg->step){
    if (fabs(arg->gain[i]) < EPS){
      arg->gain[i] = 1E6;
    }
    if (fabs(arg->mrc->input[i]) <= 0.0){
      continue;
    }
    arg->gain[i] = (fabs(arg->gain[i]) < fabs(arg->mrc->input[i]) ? arg->gain[i] : arg->mrc->input[i]);
    cur = round(arg->mrc->input[i] / arg->gain[i]);
    if (cur > 15){
      arg->ovfl++;
      cur = 15;
    }
    dev = fabs(cur - arg->mrc->input[i] / arg->gain[i]);
    arg->rmsd += dev;
    if (dev > EPS){
      arg->badp++;
    }
  }
  return;
}

void refine_gain(_arg *arg){
  // Refine gain reference against images
  int32_t i;
  double cur, dev;
  // Refine gain reference
  for (i = arg->thrd; i < arg->size; i += arg->step){
    if (!isfinite(arg->gain[i])){
      arg->gain[i] = 0.0;
    }
    if (arg->mrc->input[i] <= 0.0){
      if (arg->gain[i] <= 0.0){
	arg->maxr++;
      }
      if (arg->mrc->input[i] < 0.0){
	printf("ERROR - NEGATIVE VALUES\n");
      }
      continue;
    }
    if (arg->gain[i] <= 0.0){
      arg->gain[i] = -1.0 * (arg->mrc->input[i] > (-1.0 * arg->gain[i]) ? arg->mrc->input[i] : (-1.0 * arg->gain[i])) - EPS;
      cur = round(arg->mrc->input[i] / (arg->gain[i] / -15.0));
      dev = cur - (arg->mrc->input[i] / (arg->gain[i] / -15.0));
    } else {
      cur = round(arg->mrc->input[i] / arg->gain[i]);
      dev = cur - (arg->mrc->input[i] / arg->gain[i]);
      if (cur > 15){
	arg->ovfl++;
	arg->rmsd += fabs(15 - (arg->mrc->input[i] / arg->gain[i]));
      } else {
	arg->gain[i] -= dev / (cur * (double) arg->cont);
	arg->rmsd += fabs(dev);
      }
    }
    if (fabs(dev) > EPS){
      arg->badp++;
      if (arg->gain[i] > 0.0){
	arg->gain[i] = -1.0 * fabs(arg->mrc->input[i]);
      }
    }
    if (arg->gain[i] <= 0.0){
      arg->maxr++;
    }
  }
  return;
}
