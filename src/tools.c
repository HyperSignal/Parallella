#include "tools.h"

extern inline unsigned char val(const unsigned char *data, int x, int y, int mw);
extern inline double cosineInterpolate(double y0, double y1, double mu);
extern inline void setval(unsigned char *data, int x, int y, int mw, unsigned char value);

unsigned char bicosine(const unsigned char *data, double x, double y, int mw)	{

	double baseX	=	floor(x);		//	Parte Inteira do X	
	double baseY	=	floor(y);		//	Parte Inteira do Y
	double fracX	=	x - baseX;		//	Parte Fracionaria do X, X=1.63, fracX = 0.63
	double fracY	=	y - baseY;		//	Parte Fracionaria do Y

	double y0		=	baseY;			//	Valor anterior do Y
	double y1		=	baseY+1;		//	Valor sucessivo do Y
	double x0		=	baseX;			//	Valor anterior do X
	double x1		=	baseX+1;		//	Valor sucessivo do X

	//	Achar valor c0 = f(x0,y0)  e c1 = f(x1,y0)
	unsigned char c0 = val(data, y0, x0, mw);
	unsigned char c1 = val(data, y0, x1, mw);

	unsigned char c2 = val(data, y1, x0, mw);
	unsigned char c3 = val(data, y1, x1, mw);

	//	Interpolar de c0 a c1 pra achar top = f(fracX,y0)
	double top		=	cosineInterpolate((double)c0,(double)c1,fracX);
	
	//	Interpolar de c2 a c3 pra achar bottom = f(fracX,y1)
	double bottom	=	cosineInterpolate((double)c2,(double)c3,fracX);

	//	Interpolar de top pra bottom pra achar c = f(fracX,fracY)
	return cosineInterpolate(top,bottom,fracY);

}
/**

**/

unsigned char bilinear(const unsigned char *data, double x, double y, int mw)	{	
	int rx = floor(x);
	int ry = floor(y);
	double fracX = x - rx;
	double fracY = y - ry;
	double invfracX = 1 - fracX;
	double invfracY = 1 - fracY;
	return ( val(data,rx,ry,mw) * invfracX + val(data,rx+1,ry,mw) * fracX) * invfracY + ( val(data,rx,ry+1,mw) * invfracX + val(data,rx+1,ry+1,mw) * fracX) * fracY;
}
