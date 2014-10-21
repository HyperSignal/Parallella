
#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include <ctype.h>
#include <stdlib.h>

#include <e-hal.h>
#include "tools.h"
#include "common.h"

#define _BufOffset (0x01000000)

typedef int bool;
#define true 1
#define false 0

/*
//8x8 test sample
unsigned char Data[] = {
	157,83 ,153,147,223,114,248,200,
	120,185,30 ,86 ,50 ,28 ,29 ,180,
	153,169,225,146,20 ,115,229,108,
	50 ,206,89 ,51 ,89 ,99 ,19 ,112,
	74 ,9  ,232,185,36 ,54 ,210,66 ,
	151,206,52 ,100,232,163,191,135,
	125,81 ,122,246,20 ,197,194,210,
	87 ,122,11 ,16 ,163,90 ,1  ,152 
};*/

unsigned char *colortable;

unsigned int SAMPLE_WIDTH = 32;
unsigned int SAMPLE_HEIGHT = 32;

bool cosine_mode	= 	false;
bool pipestdout 	= 	false;
bool pipestdin 		= 	true;
bool verbose		=	false;
bool colorize		=	false;

HSInfo *schedules;
bool *schedules_status;
HSInfo *outputs;
unsigned int numsched = 0;

void USleep(int microseconds)	{
	long nanoseconds = microseconds * 1000L;	//	To nano
    struct timespec s;
    s.tv_sec = 0;
    s.tv_nsec = nanoseconds;
    nanosleep(&s, NULL);
}

void ShowHelp()	{
	fprintf(stderr, "HyperSignal Tile Generator for Epiphany V" B_PROG_VERSION " - Build: " B_BUILD_VERSION "\n");
	fprintf(stderr, "Build on " B_HOSTNAME " at " B_DATE "\n");
	fprintf(stderr, "Options: \n");
	fprintf(stderr, "\tConfiguration Parameters\n");
	fprintf(stderr, "\t\t-t      \t\t Colorize Output using colortable.mat\n");
	fprintf(stderr, "\t\t-c      \t\t Cosine Mode Interpolation (DEFAULT: Bilinear)\n");
	fprintf(stderr, "\t\t-z      \t\t Redirect output to STDOUT (Default save to output.mat)\n");
	fprintf(stderr, "\n\n");
	exit(1);
}


void ProcessDataInput() {
	unsigned int size, id, c;
	unsigned char *data;
	unsigned char *data_aux;
	c = (int)freopen (NULL,"rb",stdin);

	// First two ints are ID and Size of packet
	c = fread(&id, sizeof(int), 1, stdin);
	c = fread(&size, sizeof(int), 1, stdin);

	if(verbose)
       	fprintf(stderr, "Message ID %X - Size %u\n", id, size);	
	
	// So lets allocate a buffer to fit all the packet and read it
	data = malloc(sizeof(char) * size);
	c = fread(data, sizeof(char), size, stdin);

	if(verbose)
    	fprintf(stderr, "Read %d bytes from stdin. \n", c);	

    //	The ID should give us what we should do with the packet
    switch(id)	{
    	case 0xAE:	//	Default Input
    		//	First uint32 is the number of schedules we sent
    		numsched = *((unsigned int *)(&data[0]));
    		//	Lets point the actual data to the first byte after numsched int
    		data_aux = &data[4];
			if(verbose)
		    	fprintf(stderr, "%d data schedules. \n", numsched);	

		    //	Lets alocate output vector, schedules vector and status vector
    		schedules 			= 	malloc(sizeof(HSInfo) * numsched);
    		outputs   			= 	malloc(sizeof(HSInfo) * numsched);
    		schedules_status	=	malloc(sizeof(bool) * numsched);

    		//	Fetch the data for the schedules
    		for(int i=0;i<numsched;i++)	{
    			//	Basically output and schedules has the same type. It just have the swidth/width and sheight/height swapped and also sample size
    			memcpy(&schedules[i], data_aux, sizeof(HSInfo));
    			memcpy(&outputs[i], data_aux, sizeof(HSInfo));

    			//	Sets the output size as crop delta
    			outputs[i].width 	= 	schedules[i].C - schedules[i].A;
    			outputs[i].height 	= 	schedules[i].D - schedules[i].B;	

    			//	Lets shift what we read.
    			data_aux += sizeof(HSInfo);

    			//	Lets alocate spaces for copying and storing. For Schedule is the sample size, for the output is the outputsize.

    			schedules[i].sample 	= 	malloc(sizeof(char) * schedules[i].swidth * schedules[i].sheight   );
    			if(colorize)
	    			outputs[i].sample 	=	malloc(sizeof(char) * outputs[i].width  * outputs[i].height * 4);
	    		else
	    			outputs[i].sample 	= 	malloc(sizeof(char) * outputs[i].width  * outputs[i].height    );

    			//	Lets copy the sample
    			memcpy(schedules[i].sample, data_aux, sizeof(char) * schedules[i].swidth * schedules[i].sheight);

    			//	Shift for the end of the work
    			data_aux += sizeof(char) * schedules[i].swidth * schedules[i].sheight;
    		}
    		//	All read
    		if(verbose)
    			fprintf(stderr, "%d schedules loaded. \n",numsched);
    		break;
		case 0xEA:	//	Default output
			fprintf(stderr, "Wrong type of input! 0xEA should be the output!\n");
    	default:
    		fprintf(stderr, "Invalid ID: %d\n",id);
    		exit(1);
    }
	free(data);
}

int PreviousPowerTwo(int x)	{
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x - (x >> 1);
}

int main(int argc, char *argv[])
{
	e_platform_t platform;
	e_epiphany_t dev;
	e_mem_t emem;
	char emsg[BUF_SIZE];
	int curx = 0, cury = 0;
	char flag = 0;
	struct timeval start;
	struct timeval end;
	long elapsedTime;
	unsigned int WIDTH;

	int c;
	extern char *optarg;
	extern int optind, optopt, opterr;
	char *filename;

	if(verbose)	{
		fprintf(stderr, "HyperSignal Tile Generator for Epiphany V" B_PROG_VERSION " - Build: " B_BUILD_VERSION "\n");
		fprintf(stderr, "Build on " B_HOSTNAME " at " B_DATE "\n");		
	}

	while ((c = getopt(argc, argv, "cztv")) != -1) {
        switch(c) {
        case 'c':
        	if(verbose)
           		fprintf(stderr,"Enabling Cosine Mode\n");
            cosine_mode = true;
            break;
        case 'z':
            if(verbose)
           		fprintf(stderr,"Output for stdout\n");
            pipestdout = true;
            break;
		case 'v':
			verbose = true;
			break;
		case 't':
			colorize = true;
			break;	
        case ':': 
        	fprintf(stderr,"Option -%c requires an operand\n", optopt);
            break;
        case '?':
        	ShowHelp();
        	break;
        }
	}

	unsigned int SX,SY;
	float bx,by;

	//	Load color lookup table in RGBA Format
   	if(colorize)	{
   		if(verbose)
			fprintf(stderr, "Reading color table\n");
   		FILE *f = fopen("colortable.mat","rb");
   		colortable = malloc(sizeof(char)*256*4);
   		c = fread(colortable, sizeof(char), 256*4, f);
   		fclose(f);
   	}

   	//	Build Cosine Table
	if(cosine_mode)	{
		if(verbose)
   			fprintf(stderr, "Building cosine table\n");
		BuildCosTable();
	}

	gettimeofday(&start, NULL);

	//	Process the Input data
	if(pipestdin)	
		ProcessDataInput();


	//	Generate the works for cores
	const unsigned int numworks = numsched * 4;
	unsigned int 	CoresX = 0,
					CoresY = 0;

	if(numworks > MAX_CORES)	{
		fprintf(stderr, "Not Enough Cores!\n");
		exit(1);
	}

	CoresX = CoresY = PreviousPowerTwo(numworks-1) / 2;


	HSWork works[numworks];		//	TODO: Check for really needed works 

	for(int i=0;i< numsched;i++)	{
		HSInfo *sched = &schedules[i];

		//	Lets calculate the scale and offset
		SX = sched->width 				/ 	OUTPUT_WIDTH;
		SY = sched->height 				/ 	OUTPUT_HEIGHT;
		bx = (float)(sched->swidth ) 	/ 	(float)SX;	
		by = (float)(sched->sheight) 	/	(float)SY;

		//	Lets set the status to false
		schedules_status[i] = false;

		if(verbose)
	   		fprintf(stderr,"Preparing works for schedule %02d (%02d,%02d) \n",i,SX,SY);

		for(int y=0;y<SY;y++)	{
			for(int x=0;x<SX;x++)	{
				int p = i*4 + y * SX + x;	//	TODO: Check for really needed works
				works[p].scheduleid = i;
				works[p].x0 = x * (bx-(1.0f/SX));
				works[p].y0 = y * (by-(1.0f/SY));
				works[p].sx = SX;
				works[p].sy = SY;
				works[p].sample_width = sched->swidth;
				works[p].sample_height = sched->sheight;
				works[p].done = 0;
				works[p].error = 0;
				memcpy(works[p].sample, sched->sample, sizeof(unsigned char) * sched->swidth * sched->sheight);
			}
		}

		//	We can now free the sample to save memory
		free(sched->sample);
	}


	//	Initializes Epiphany System
	e_init(NULL);
	e_reset_system();
	e_get_platform_info(&platform);

	//	Alocate Works and cosine table Memory at Shared DRAM Space
	e_alloc(&emem, _BufOffset, sizeof(HSWork)*numworks + sizeof(float) * MAX_CIRCLE_ANGLE);

	//	Writes the works to that memory
	e_write(&emem, 0, 0,  0, (void *)&works, sizeof(HSWork)*numworks);

	//	Writes cosines to memory
	if(cosine_mode)
		e_write(&emem, 0, 0, sizeof(HSWork)*numworks, (void *)fast_cossin_table, sizeof(float) * MAX_CIRCLE_ANGLE);

	//	Open the Workgroup witn SX x SY cores
	if(verbose)
   		fprintf(stderr, "Initializing (%02d,%02d) Core Matrix\n",CoresX,CoresY);

	e_open(&dev, 0, 0, CoresY, CoresX);

	//	Resets the group.
	e_reset_group(&dev);

	// Load the device program onto the selected core
	if(cosine_mode)
		e_load_group("e_test_bicosine.srec", &dev, 0, 0, CoresY, CoresX, E_FALSE);
	else
		e_load_group("e_test_bilinear.srec", &dev, 0, 0, CoresY, CoresX, E_FALSE);

	//	Write WorkID at cores
	unsigned workid = 0;
	for(int y=0;y<CoresY;y++)	{
		for(int x=0;x<CoresX;x++)	{
			e_write(&dev, y, x, CURRENT_POS + sizeof(int) * 2, &workid,   sizeof(unsigned));
			e_write(&dev, y, x, CURRENT_POS + sizeof(int) * 3, &numworks, sizeof(unsigned));
			workid++;
			if(workid >= numworks)
				workid = 255;
		}
	}

	//	Starting group
	if(verbose)
   		fprintf(stderr, "Starting program\n");

	e_start_group(&dev);
	
	if(verbose)
   		fprintf(stderr, "Waiting...\n");

	//	Every LOOP_SLEEP ms, read the works from Shared DRAM and see if there is any change.
	while(1)	{
		for(int i=0;i<numsched;i++)	
			schedules_status[i] = true;

		if(verbose)
			fprintf(stderr, "\e[1;1H");
       		//for(int i=0;i<numworks;i++)
			//	fprintf(stderr, "\033[F");

		e_read(&emem, 0, 0, 0, &works, sizeof(works));
		workid = 0;
		for(int y=0;y<CoresY;y++)	{
			for(int x=0;x<CoresX;x++)	{
				if(workid < numworks)	{
					schedules_status[works[workid].scheduleid] &= works[workid].done;
					if(verbose)	{
						e_read(&dev, y, x, CURRENT_POS, &curx, 4);
						e_read(&dev, y, x, CURRENT_POS+sizeof(int), &cury, 4);
						fprintf(stderr, "\e[KCore %02d: (%03d,%03d)(%02d)(%02d) Schedule(%02d)\n",workid,curx,cury, works[workid].done, works[workid].error,works[workid].scheduleid);				
	           		}
	           		workid++;
	           	}
			}
		}
		flag = true;
		for(int i=0;i<numsched;i++)	
			flag &= schedules_status[i];

		if(flag)	
			break;

		USleep(LOOP_SLEEP);
	}
	gettimeofday(&end, NULL);
	elapsedTime = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
	if(verbose)
   		fprintf(stderr, "Finished %02d works in %ld ms!\n", numworks, elapsedTime);

	//	Work is finished, lets get an updated version of the works
	e_read(&emem, 0, 0, 0, &works, sizeof(works));


	if(verbose)
   		fprintf(stderr, "Consolidating data\n");

   	//	For the output stream
	unsigned int startb[3];
	startb[0] = 0xEA;
	startb[1] = 0;
	startb[2] = numsched;

   	//	Lets consolidate each schedule
   	for(int i=0;i<numsched;i++)	{
   		HSInfo *out 	= 	&outputs[i];
   		HSInfo *sched 	=	&schedules[i];
   		SX 				= 	works[i*4].sx;
   		SY 				= 	works[i*4].sy;
   		WIDTH 			=	sched->C - sched->A;
   		startb[1]		+=	sizeof(HSInfo) + out->width*out->height*4;

   		//	Iterate over the "local" division SX,SY
	 	for(int y=0;y<SY;y++)	{
			for(int x=0;x<SX;x++)	{
				int p = i*4 + y*SX+x;
				//	Iterate over the work output block to fill the output info
				for(int j=0;j<OUTPUT_HEIGHT;j++)	{
					for(int i=0;i<OUTPUT_WIDTH;i++)	{
						//	Calculate the position inside output block
						int tx 	=	i + x * (sched->width/SX);
						int ty 	= 	j + y * (sched->height/SY);
						//	Set the Crop Factor
						if(tx >= sched->A && tx < sched->C && ty >= sched->B && ty < sched->D)	{
							int pt 				=	(sched->D-ty-1)*(WIDTH)+(tx-sched->A);
							unsigned char val 	= 	works[p].output[j*OUTPUT_WIDTH+i];
							//	Write to output
							if(colorize)	{
								out->sample[pt*4] 		= 	colortable[val*4];
								out->sample[pt*4+1] 	= 	colortable[val*4+1];
								out->sample[pt*4+2] 	= 	colortable[val*4+2];
								out->sample[pt*4+3] 	= 	colortable[val*4+3];
							}else
								out->sample[pt] = val;
						}	
					}
				}
			}
		}  		
   	}
 	

	gettimeofday(&end, NULL);
	elapsedTime = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
	if(verbose)	{
   		fprintf(stderr, "Total time %ld ms!\n", elapsedTime);
   		fprintf(stderr, "Writting to output.\n");
	}
	FILE *outf = (pipestdout)?stdout:fopen("/media/LINUX_DATA/MathStudies/HyperSignal/Parallella/output.mat","wb");
	c = (int) freopen (NULL,"wb",stdout);

	fwrite(&startb,sizeof(int),3,outf);
	for(int i=0;i<numsched;i++)	{
		fwrite(&outputs[i],sizeof(HSInfo),1,outf);
		if(colorize)
			fwrite(outputs[i].sample,outputs[i].width*outputs[i].height*4,1,outf);
		else
			fwrite(outputs[i].sample,outputs[i].width*outputs[i].height,1,outf);
	}
	fclose(outf);

	if(verbose)
		fprintf(stderr, "Done\n");
	e_close(&dev);

	e_free(&emem);
	e_finalize();

	free(colortable);

	// Free Schedules
	free(schedules);

	//	Free Outputs
	for(int i=0;i<numsched;i++)
		free(outputs[i].sample);

	free(outputs);
	
	return 0;
}
