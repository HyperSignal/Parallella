#ifndef TOOLS_H
#define TOOLS_H

#include "common.h"

//unsigned char bilinear(unsigned char *data, float x, float y, int mw);


#define MAX_CIRCLE_ANGLE      512
#define HALF_MAX_CIRCLE_ANGLE (MAX_CIRCLE_ANGLE/2)
#define QUARTER_MAX_CIRCLE_ANGLE (MAX_CIRCLE_ANGLE/4)
#define MASK_MAX_CIRCLE_ANGLE (MAX_CIRCLE_ANGLE - 1)
#define PI 3.14159265358979323846f


//static const float HCPI = HALF_MAX_CIRCLE_ANGLE / PI;


#ifdef _HOST_BUILD
#include <math.h>
float fast_cossin_table[MAX_CIRCLE_ANGLE];
static void BuildCosTable()	{
	for (int i = 0 ; i < MAX_CIRCLE_ANGLE ; i++)
      fast_cossin_table[i] = (float)sin((double)i * PI / HALF_MAX_CIRCLE_ANGLE);
}
#endif

#ifndef _HOST_BUILD

static inline float fastcos(int n, float *table)	{
   if (n < 0)
      return table[((-n) + QUARTER_MAX_CIRCLE_ANGLE)&MASK_MAX_CIRCLE_ANGLE];
   else
      return table[(n + QUARTER_MAX_CIRCLE_ANGLE)&MASK_MAX_CIRCLE_ANGLE];
}

static inline float min(float v1, float v2)	{
	return v1 > v2 ? v2 : v1;
}

static inline unsigned char val(const unsigned char *data, int x, int y, int mw, int size)	{
	if(x > mw)	x = mw;
	return (y * mw + x) > size ? 0 : data[y * mw + x];
}


static inline void setval(unsigned char *data, int x, int y, int mw, unsigned char value, int size)	{
	if((y * mw + x ) < size)
		data[y * mw + x] = value;
}

static inline unsigned char bilinear(const unsigned char *data, float x, float y, int mw, int size)	{	
	int rx = (int)(x);
	int ry = (int)(y);
	float fracX = x - rx;
	float fracY = y - ry;
	float invfracX = 1.f - fracX;
	float invfracY = 1.f - fracY;
	
	unsigned char a = val(data,rx,ry,mw,size);
	unsigned char b = val(data,rx+1,ry,mw,size);
	unsigned char c = val(data,rx,ry+1,mw,size);
	unsigned char d = val(data,rx+1,ry+1,mw,size);
	
	return ( a * invfracX + b * fracX) * invfracY + ( c * invfracX + d * fracX) * fracY;
}

static inline float cosineInterpolate(float y0, float y1, float mu, float *table)	{
	float dcos = fastcos(mu*HALF_MAX_CIRCLE_ANGLE, table);
	float mu2 = 0.5f * (1.f - dcos);// (1.f - dcos )/2.f;
	return (y0*(1.f-mu2)+y1*mu2);
}

static unsigned char bicosine(const unsigned char *data, float x, float y, int mw, float *table, int size)	{

	int rx	=	(int)(x);		//	Parte Inteira do X	
	int ry	=	(int)(y);		//	Parte Inteira do Y
	float fracX	=	x - rx;		//	Parte Fracionaria do X, X=1.63, fracX = 0.63
	float fracY	=	y - ry;		//	Parte Fracionaria do Y

	//	Achar valor c0 = f(x0,y0)  e c1 = f(x1,y0)
	unsigned char c0 = val(data, rx  , ry, mw,size);
	unsigned char c1 = val(data, rx+1, ry, mw,size);

	unsigned char c2 = val(data, rx,   ry+1, mw,size);
	unsigned char c3 = val(data, rx+1, ry+1, mw,size);

	//	Interpolar de c0 a c1 pra achar top = f(fracX,y0)
	float top		=	cosineInterpolate((float)c0,(float)c1,fracX, table);
	
	//	Interpolar de c2 a c3 pra achar bottom = f(fracX,y1)
	float bottom	=	cosineInterpolate((float)c2,(float)c3,fracX, table);

	//	Interpolar de top pra bottom pra achar c = f(fracX,fracY)
	float result = cosineInterpolate(top, bottom, fracY, table);
	return result;
}
/* BROKEN
static inline unsigned char bicubic(const unsigned char *data, float x, float y, int mw)	{
	// widthStep = mw
	// nChanels = 1
	int rx = (int)(x);
	int ry = (int)(y);
    unsigned char C[5];
    unsigned char d0,d2,d3,a0,a1,a2,a3;
    unsigned char jj = 0;
    //for(jj=0;jj<4;jj++)	{
    	const int z = ry - 1 + jj;

		a0 = val(data, rx, z, mw);

		d0 = val(data, rx-1, z, mw) - a0;
		d2 = val(data, rx+1, z, mw) - a0;
		d3 = val(data, rx+2, z, mw) - a0;
		
		a1 = -1.f / 3 * d0 + d2 - 1.f / 6 * d3;
		a2 =  1.f / 2 * d0 + 1.f / 2 * d2;
		a3 = -1.f / 6 * d0 - 1.f / 2 * d2 + 1.f / 6 * d3;
		C[jj] =  a0 + a1*x + a2*x*x + a3*x*x*x; 
	//}
	
	d0 = C[0]-C[1];
	d2 = C[2]-C[1];
	d3 = C[3]-C[1];
	a0=C[1];

	a1 = -1.f / 3 * d0 + d2 -1.f / 6 * d3;
	a2 = 1.f / 2 * d0 + 1.f / 2 * d2;
	a3 = -1.f / 6 * d0 - 1.f / 2 * d2 + 1.f / 6 * d3;

	return a0 + a1*y + a2*y*y + a3*y*y*y;
}
*/
#endif
#endif