#include "process.h"

void *DoInterpolate(void *_work)	{
	HSWork *work = _work;				//	Cast to HSWork
	double x2, y2;
	unsigned char val;

	work->start = clock();
	//pthread_mutex_lock(&printlock);
	//printf("Scale(%f,%f)\n",work->sx,work->sy);
	//pthread_mutex_unlock(&printlock);

	// Sanity Check
	if(work->x0 < 0 || work->y0 < 0 )	{
		pthread_mutex_lock(&printlock);
		printf("(Work %d) Error: x0,x1 overlaps the sample size!\n", work->workid);
		printf("(Work %d) P0(%f,%f) Sample(%d,%d)\n", work->workid,work->x0,work->y0,SAMPLE_WIDTH,SAMPLE_HEIGHT);
		printf("(Work %d) ABORTING", work->workid);
		pthread_mutex_unlock(&printlock);
		pthread_mutex_lock(&work->workmutex);
		work->done = 1;
		work->error = 1;
		pthread_mutex_unlock(&work->workmutex);
		pthread_exit(NULL);
	}

	//printf("(Work %d) Starting work\n", work->workid);

	pthread_mutex_lock(&work->workmutex);
	/*
	for(int y=0;y<OUTPUT_HEIGHT;y++)	{
		for(int x=0;x<OUTPUT_WIDTH;x++)	{
			x2 = ( x / ((OUTPUT_WIDTH)  * (work->sx) ))		* (SAMPLE_WIDTH-1);
			y2 = ( y / ((OUTPUT_HEIGHT) * (work->sy) ))		* (SAMPLE_HEIGHT-1);
			x2 += work->x0;
			y2 += work->y0;
			#if defined USE_BILINEAR
				val = bilinear(work->sample,x2,y2,SAMPLE_WIDTH);
			#elif defined USE_BICOSINE
				val = bicosine(work->sample,x2,y2,SAMPLE_WIDTH);
			#else	//	Defaults to BILINEAR, in future use NEAREST or MEDIUM
				val = bilinear(work->sample,x2,y2,SAMPLE_WIDTH);
			#endif
			setval(work->output,x,y,OUTPUT_WIDTH,val);
		}
	}*/

	const float sw_ow = ((SAMPLE_WIDTH-1) /  (float)OUTPUT_WIDTH ) / work->sx;
	const float sh_oh = ((SAMPLE_HEIGHT-1) / (float)OUTPUT_HEIGHT) /  work->sy;

	for(int y=0;y<OUTPUT_HEIGHT;y++)  {
		for(int x=0;x<OUTPUT_WIDTH;x++) {
			x2 = sw_ow * x;
			y2 = sh_oh * y;
			x2 += work->x0;
			y2 += work->y0;
			val = bilinear(work->sample,x2,y2,SAMPLE_WIDTH);
			setval(work->output,x,y,OUTPUT_WIDTH,val);
		}
	}
	work->done = 1;
	work->error = 0;
	work->end = clock();
	pthread_mutex_unlock(&work->workmutex);
	//printf("(Work %d) Finished\n", work->workid);
	pthread_exit(NULL);
}