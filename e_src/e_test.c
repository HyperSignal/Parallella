#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "tools.h"
#include "e_lib.h"

#define BUFSTART (0x8f000000)

unsigned *vals = (unsigned *)CURRENT_POS; //  4 byte array: CURX, CURY, SX, SY - Last two written by HOST

int main(void) {
  e_coreid_t coreid = e_get_coreid();
  float x2, y2;
  unsigned char val;
  unsigned sx, sy;
  float costable[MAX_CIRCLE_ANGLE];

  unsigned workid   =   vals[2];
  unsigned numworks =   vals[3];

  if(workid > MAX_CORES-1) {
    vals[0] = workid;
    vals[1] = -1;
    return EXIT_SUCCESS;
  }

  HSWork *ext_works =   (void *) BUFSTART;
  HSWork work;

  e_dma_copy(&work, &ext_works[workid], sizeof(HSWork));
  e_dma_copy(&costable, &ext_works[numworks], sizeof(float) * MAX_CIRCLE_ANGLE);

  int *curx         =   (int *)CURRENT_POS, *cury = (int *)(CURRENT_POS + 1);
  vals[0] = 0;
  vals[1] = 0;

  const float sw_ow     =   ((work.sample_width-1) /  (float)OUTPUT_WIDTH ) /  work.sx;
  const float sh_oh     =   ((work.sample_height-1) / (float)OUTPUT_HEIGHT) /  work.sy;
  const int sample_size =   work.sample_width * work.sample_height;
  const int output_size =   OUTPUT_WIDTH * OUTPUT_HEIGHT;
  for(int y=0;y<OUTPUT_HEIGHT;y++)  {
    vals[1] = y;
    for(int x=0;x<OUTPUT_WIDTH;x++) {
      vals[0] = x;
      x2 = sw_ow * x;
      y2 = sh_oh * y;
      x2 += work.x0;
      y2 += work.y0;
      #ifdef BICOSINE_INTERPOLATION
      val = bicosine(work.sample, x2, y2, work.sample_width, costable,sample_size);
      #else
      val = bilinear(work.sample,x2,y2,work.sample_width,sample_size);
      #endif
      setval(work.output,x,y,OUTPUT_WIDTH,val,output_size);
    }
  }
  
  vals[0] = -1;
  vals[1] = -5;
  work.done = 1;
  work.error = 5;

  e_dma_copy(&ext_works[workid], &work, sizeof(HSWork));
  
  return EXIT_SUCCESS;
}