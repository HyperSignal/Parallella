
#ifndef COMMON_H
#define COMMON_H

//#define SAMPLE_WIDTH 8
//#define SAMPLE_HEIGHT 8

#define _SAMPLE_WIDTH 32
#define _SAMPLE_HEIGHT 32

#define OUTPUT_WIDTH 128
#define OUTPUT_HEIGHT 128

#define SAMPLE_SIZE (_SAMPLE_WIDTH * _SAMPLE_HEIGHT)
#define OUTPUT_SIZE (OUTPUT_WIDTH * OUTPUT_HEIGHT)

#define CURRENT_POS (0x4000)

#define NUM_WORKS 4
#define BUF_SIZE 32

#define ALIGN8 8

#define MAX_CORES 16
#define MAX_CORES_X 4
#define MAX_CORES_Y 4

#define LOOP_SLEEP 20

typedef struct __attribute__((aligned(ALIGN8))) {
   int scheduleid;						       	//	Work ID
   float x0, y0;						       	//	Coordinates
   float sx, sy;						       	//	ScaleX and ScaleY
   unsigned int sample_width;					//	Sample Width
   unsigned int sample_height;					//	Sample Width
   unsigned char sample[SAMPLE_SIZE];	 		//	Origin Matrix
   unsigned char output[OUTPUT_SIZE];	 		//	Output Matrix
   unsigned char done;                  		//	Done Flag
   unsigned char error;					    	//	Error Flag
   int coreid;							      	//	Core ID
} HSWork;

typedef struct  {
	unsigned int id, width, height, A,B,C,D;
	unsigned char swidth, sheight, *sample;
} HSInfo;

#endif