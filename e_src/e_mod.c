#include <Python.h>
#include <e-hal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "common.h"
#include "tools.h"

static 	e_platform_t 		platform;
static 	e_epiphany_t 		dev;
static 	e_mem_t 			emem;

#define _BufOffset (0x01000000)
#define OUT_X 512
#define OUT_Y 512

unsigned char outdata[OUT_X*OUT_Y];

/**
 *	Initializes Epiphany Core
 **/
static PyObject* InitEpiphany(PyObject* self)	{
	e_init(NULL);
	e_reset_system();
	e_get_platform_info(&platform);
	return Py_None;
}

static PyObject* Finalize(PyObject* self)	{
	e_finalize();
	return Py_None;
}
/**
 *	Does an Work
 **/

static PyObject *DoWork(PyObject *self, PyObject *args)	{
	char *sample;
	int size;
	struct timeval start;
	struct timeval end;
	long elapsedTime;
	int curx = 0, cury = 0;
	char flag = 0;

	//InitEpiphany(self);

	if(!PyArg_ParseTuple(args, "s#", &sample, &size))	{
		printf("Invalid arguments! Should be a bytearray.");
		return NULL;
	}
	if(size != SAMPLE_WIDTH * SAMPLE_HEIGHT)	{
		printf("Sample size should be %d x %d\n",SAMPLE_WIDTH,SAMPLE_HEIGHT);
		return NULL;
	}

	const unsigned int SX = OUT_X / OUTPUT_WIDTH;
	const unsigned int SY = OUT_Y / OUTPUT_HEIGHT;
	const float bx = (SAMPLE_WIDTH ) / SX;	
	const float by = (SAMPLE_HEIGHT) / SY;


	HSWork works[SX*SY]; 
	printf("Preparing works. (%02d,%02d) \n",SX,SY);
	for(int y=0;y<SY;y++)	{
		for(int x=0;x<SX;x++)	{
			int p = y*SX+x;
			works[p].workid = p;
			works[p].x0 = x * (bx-(1.0f/SX));
			works[p].y0 = y * (by-(1.0f/SY));
			works[p].sx = SX;
			works[p].sy = SY;
			works[p].done = 0;
			works[p].error = 0;
			memcpy(works[p].sample, sample, sizeof(unsigned char) * SAMPLE_WIDTH * SAMPLE_HEIGHT);
		}
	}
	printf("Allocating memory\n");
	//	Alocate Works Memory at Shared DRAM Space
	if(e_alloc(&emem, _BufOffset, sizeof(HSWork)*SX*SY) != E_OK)	{
		printf("Error alocating %d bytes in Shared Memory\n",sizeof(HSWork)*SX*SY);
		return NULL;
	}
	printf("Writting data\n");
	//	Writes the works to that memory
	if(e_write(&emem, 0, 0,  0, (void *)&works, sizeof(HSWork)*SX*SY) != E_OK)	{
		printf("Error writting %d bytes in Shared Memory\n",sizeof(HSWork)*SX*SY);
		return NULL;
	}

	printf("Opening workgroup\n");
	//	Open the Workgroup witn SX x SY cores
	if(e_open(&dev, 0, 0, SY, SX) != E_OK)	{
		printf("Error opening Epiphany Workgroup.\n");
		return NULL;
	}

	printf("Reseting workgroup\n");
	//	Resets the group.
	if(e_reset_group(&dev) != E_OK)	{
		printf("Error reseting workgroup.\n");
		return NULL;
	}

	printf("Loading programs\n");
	// Load the device program onto the selected core
	if(e_load_group("e_test.srec", &dev, 0, 0, SY, SX, E_FALSE) != E_OK)	{
		printf("Error loading program.\n");
		return NULL;
	}

	printf("Writting SX/SY\n");

	for(int y=0;y<SY;y++)	{
		for(int x=0;x<SX;x++)	{
			if(e_write(&dev, y, x, CURRENT_POS + sizeof(int) * 2, &SX, sizeof(unsigned)) != E_OK)	{
				printf("Error writting to %d position in core (%02d,%02d)\n",CURRENT_POS + sizeof(int)*2, x,y);
				return NULL;
			}
			if(e_write(&dev, y, x, CURRENT_POS + sizeof(int) * 3, &SY, sizeof(unsigned)) != E_OK)	{
				printf("Error writting to %d position in core (%02d,%02d)\n",CURRENT_POS + sizeof(int)*2, x,y);
				return NULL;
			}		
		}
	}

	printf("Starting program\n");
	if(e_start_group(&dev) != E_OK)	{
		printf("Error starting workgroup.\n");
		return NULL;		
	};
	printf("Waiting...\n");

	gettimeofday(&start, NULL);
	//	Read the works from Shared DRAM and see if there is any change.
	while(1)	{
		flag = 1;
		e_read(&emem, 0, 0, 0, &works, sizeof(works));
		for(int y=0;y<SY;y++)	{
			for(int x=0;x<SX;x++)	{
				flag &= works[y*SX+x].done;
				e_read(&dev, y, x, CURRENT_POS, &curx, 4);
				e_read(&dev, y, x, CURRENT_POS+sizeof(int), &cury, 4);
				printf("Core %02d: (%03d,%03d)(%d)(%d)\n",y*SX+x,curx,cury, works[y*SX+x].done, works[y*SX+x].error);				
			}
		}
		if(flag)	
			break;

		for(int i=0;i<SX*SY;i++)
			printf("\033[F");
		//sleep(1);
	}
	gettimeofday(&end, NULL);
	elapsedTime = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
	printf("Finished work in %ld ms!\n", elapsedTime);

	//	Work is finished, lets get an updated version of the works
	e_read(&emem, 0, 0, 0, &works, sizeof(works));
	printf("Consolidating data\n");
	for(int y=0;y<SY;y++)	{
		for(int x=0;x<SX;x++)	{
			int p = y*SX+x;
			for(int j=0;j<OUTPUT_HEIGHT;j++)	{
				for(int i=0;i<OUTPUT_WIDTH;i++)	{
					int tx = i + x * (OUT_X/SX);
					int ty = j + y * (OUT_Y/SY);
					outdata[ty*OUT_X+tx] = works[p].output[j*OUTPUT_HEIGHT+i];
				}
			}
		}
	}
	gettimeofday(&end, NULL);
	elapsedTime = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
	
	e_close(&dev);
	e_free(&emem);

	return Py_BuildValue("ks#", elapsedTime, outdata, OUT_X * OUT_Y);
}


static char e_mod_InitEpiphany[]	=	"InitEpiphany( ): Initializes the Epiphany Plataform.\n";
static char e_mod_DoWork[]			=	"DoWork(sample): Starts a work on Epiphany\n";
static char e_mod_Finalize[]		=	"Finalize( ): Finalizes Epiphany Work\n";

static PyMethodDef e_mod_funcs[] = {
    {"InitEpiphany", 	(PyCFunction)InitEpiphany 	, METH_NOARGS,  e_mod_InitEpiphany},
    {"DoWork", 			(PyCFunction)DoWork 		, METH_VARARGS, e_mod_DoWork},
    {"Finalize", 		(PyCFunction)Finalize		, METH_NOARGS,  e_mod_Finalize},
    {NULL}
};

void inite_mod(void)
{
    Py_InitModule3("e_mod", e_mod_funcs, "Interpolator Module");
}