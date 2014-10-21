#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>

#include "tools.h"
#include "process.h"

#define null NULL
#define OUT_X 512
#define OUT_Y 512

//#define SINGLE_THREAD

/*
	Expanding 4x4 Matrix to 320x320
*/
/*
const unsigned char Data[]	=	{
	255	,	67	,	99	,	44,
	0	,	100	,	255	,	99,
	12	,	77	,	45	,	0,
	25 	,	8	,	66	,	33
};
*/

const unsigned char Data[] = {
	157,83 ,153,147,223,114,248,200,
	120,185,30 ,86 ,50 ,28 ,29 ,180,
	153,169,225,146,20 ,115,229,108,
	50 ,206,89 ,51 ,89 ,99 ,19 ,112,
	74 ,9  ,232,185,36 ,54 ,210,66 ,
	151,206,52 ,100,232,163,191,135,
	125,81 ,122,246,20 ,197,194,210,
	87 ,122,11 ,16 ,163,90 ,1  ,152 
};

const int 	MatrixW	=	8,
			MatrixH	=	8;

int main()	{
	#ifndef SINGLE_THREAD
		// Hardcoded Sample: 256x256 scale fitting 64x64 samples. This gives us 2x2 threads
		const int SX = OUT_X / OUTPUT_WIDTH;
		const int SY = OUT_Y / OUTPUT_HEIGHT;
		const int bx = (SAMPLE_WIDTH ) / SX;	//	2 blocks horizontal
		const int by = (SAMPLE_HEIGHT) / SY;	//	2 blocks vertical

		pthread_t threads[SX*SY];
		HSWork works[SX*SY];

		pthread_mutex_init(&printlock, NULL);

	#endif
	unsigned char *out;
	struct timeval start;
	struct timeval end;
	long elapsedTime;

	FILE * f;
	f = fopen("initial.mat", "w");
	printf("Writting Initial Matrix to initial.mat.\n");
	fwrite(Data,sizeof(unsigned char), MatrixW*MatrixH, f);
	fclose(f);
	
	printf("Interpolating\n");
	
	out = malloc(sizeof(unsigned char) * OUT_X * OUT_Y);	//	Allocate output array
	
	#ifdef SINGLE_THREAD
		printf("Single Thread mode\n");
		if(out != null)	{
    		gettimeofday(&start, NULL);
			//	Interpolate
			int mapdx = OUT_X,
				mapdy = OUT_Y;
		  	const float sw_ow = ((MatrixW-1) /  (float)mapdx );
			const float sh_oh = ((MatrixH-1) / (float)mapdy);

			for(int y=0;y<mapdy;y++)  {
				for(int x=0;x<mapdx;x++) {
					float x2 = sw_ow * x;
					float y2 = sh_oh * y;
					unsigned char val = bilinear(Data,x2,y2,MatrixW);
					setval(out,x,y,OUTPUT_WIDTH,val);
				}
			}
			/*
			for(int y=0;y<mapdy;y++)	{
				for(int x=0;x<mapdx;x++)	{
					double x2 = ( (double)x / mapdx) * (MatrixW-1);
					double y2 = ( (double)y / mapdy) * (MatrixH-1);
					//setval(out,x,y,mapdx,bilinear(Data,x2,y2,MatrixW));
					setval(out,x,y,mapdx,bicosine(Data,x2,y2,MatrixW));
				}
			}*/
			gettimeofday(&end, NULL);
		}else{
			printf("Error: Not allocated!\n");
		}
	#else
		printf("Multi Thread Mode\n");
		for(int y=0;y<SY;y++)	{
			for(int x=0;x<SX;x++)	{
				int p = y*SX+x;
				works[p].workid = p;
				works[p].x0 = x * (bx-(1.0/SX));
				works[p].y0 = y * (by-(1.0/SY));
				works[p].sx = SX;
				works[p].sy = SY;
				memcpy(works[p].sample, Data, sizeof(unsigned char) * MatrixW * MatrixH);
				pthread_mutex_init(&works[p].workmutex, NULL);
			}
		}
		gettimeofday(&start, NULL);
		printf("Starting works\n");
		for(int i=0;i<SX*SY;i++)	{
			//printf("Starting work %d\n",i);
			pthread_create(&threads[i],NULL,DoInterpolate,&works[i]);
		}
		unsigned char done;
		printf("Waiting all works to be done.\n");
		while(1)	{
			done = 1;
			for(int i=0;i<SX*SY;i++)	{
				//int status = pthread_mutex_trylock(&works[i].workmutex);
				//if(status == 0)	{
					done &= works[i].done;
					//pthread_mutex_unlock(&works[i].workmutex);
				//}else
				//	done = 0;
			}
			if(done)
				break;
		}
		gettimeofday(&end, NULL);
		elapsedTime = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
    	printf("Done start in %lu ms\n",elapsedTime);
		printf("All works done!\n Concatenating data...\n");
		for(int y=0;y<SY;y++)	{
			for(int x=0;x<SX;x++)	{
				int p = y*SX+x;
    			elapsedTime += (double)(works[p].end - works[p].start) / CLOCKS_PER_SEC;
				for(int j=0;j<OUTPUT_HEIGHT;j++)	{
					for(int i=0;i<OUTPUT_WIDTH;i++)	{
						int tx = i + x * (OUT_X/SX);
						int ty = j + y * (OUT_Y/SY);
						out[ty*OUT_X+tx] = works[p].output[j*OUTPUT_HEIGHT+i];
					}
				}
			}
		}
		//elapsedTime /= SX*SY;
    	//printf("Thread Done in %f seconds\n",elapsedTime);
	#endif

	gettimeofday(&end, NULL);	
	elapsedTime = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
	printf("Done all in %lu ms\n",elapsedTime);

    #ifdef SINGLE_THREAD
    	f = fopen("single.time","a");
    	char buff[256];
    	sprintf(buff,"%lu\n",elapsedTime);
    	fwrite(buff,sizeof(char),strlen(buff),f);
    	fclose(f);
    #else
    	f = fopen("multi.time","a");
    	char buff[256];
    	sprintf(buff,"%lu\n",elapsedTime);
    	fwrite(buff,sizeof(char),strlen(buff),f);
    	fclose(f);
   	#endif
	
	if(out != NULL)	{
		printf("Saving output matrix to output.mat\n");
		f = fopen("output.mat","w");
		fwrite(out,sizeof(unsigned char),OUT_X*OUT_Y,f);
		fclose(f);
	}
	printf("Done!");
	return 0;
}
