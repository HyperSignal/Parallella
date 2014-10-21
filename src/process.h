#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//#include <semaphore.h>
#include <time.h>
#include "tools.h"

#define SAMPLE_WIDTH 8
#define SAMPLE_HEIGHT 8

#define OUTPUT_WIDTH 128
#define OUTPUT_HEIGHT 128

#define SAMPLE_SIZE (SAMPLE_WIDTH * SAMPLE_HEIGHT)
#define OUTPUT_SIZE (OUTPUT_WIDTH * OUTPUT_HEIGHT)

//#define USE_BILINEAR
#define USE_BICOSINE

pthread_mutex_t printlock;

typedef struct _HSWork {
   int workid;							//	Work ID
   double x0, y0;						//	Coordinates
   double sx, sy;						//	ScaleX and ScaleY
   unsigned char sample[SAMPLE_SIZE];	//	Origin Matrix
   unsigned char output[OUTPUT_SIZE];	//	Output Matrix
   unsigned char done;					//	Done Flag
   unsigned char error;					//	Error Flag
   pthread_mutex_t workmutex;			//	Work Mutex
   clock_t start, end;					//	Work Start and End Time
} HSWork;

void *DoInterpolate(void *_work);