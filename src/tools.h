#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef TOOLS_H
#define M_PI 3.14159265358979323846
#define TOOLS_H

unsigned char bicosine(const unsigned char *data, double x, double y, int mw);
unsigned char bilinear(const unsigned char *data, double x, double y, int mw);

inline unsigned char val(const unsigned char *data, int x, int y, int mw)	{
	int p = y * mw + x;
	return data[p];
}

inline double cosineInterpolate(double y0, double y1, double mu)	{
	double mu2 = (1-cos(mu*M_PI))/2;
	return (y0*(1-mu2)+y1*mu2);
}

inline void setval(unsigned char *data, int x, int y, int mw, unsigned char value)	{
	int p = y * mw + x;
	data[p] = value;
}


#endif