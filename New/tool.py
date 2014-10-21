#!/usr/bin/env python
#-*- coding: UTF-8 -*-

import math
import json
import urllib
import sys
import getopt
import os
import numpy as np
from datetime import datetime


IMAGE_SIZE		=	(640,640)
DEFAULT_ZOOM	=	16
SCALE			=	2

tileSize = 256

initialResolution = 2 * math.pi * 6378137 / tileSize

originShift = 2 * math.pi * 6378137 / 2.0


HYPER_STEP		=	0.0005					#	Step usado no banco de dados - 0.0005 dá uma precisão de ~100 metros
HYPER_BRUSH		=	2						#	Tamanho do Brush de interpolação local 
HYPER_GAP		=	5 						#	Gap para interpolação entre tiles
HYPER_BLUR		=	5						#	Blur para suavização de bordas

HYPER_BRUSH_INT	=	[	
						[0	,0	,0	,0	,0],		#	|
						[0	,0.5,0.5,0.5,0],		#	|
						[0	,0.5,1	,0.5,0],		#	|---- Brush de Interpolação
						[0	,0.5,0.5,0.5,0],		#	|
						[0	,0	,0	,0	,0]			#	|
					]						


'''
	Funções do HyperSignal
'''

def LatLonToHyper(lat,lon):
	'''
		Entra com Latitude e Longitude, retorna x e y nas coordenadas do HyperSignal
	'''
	return (int(math.ceil((lon+180)/HYPER_STEP)), int(math.ceil((lat+90)/HYPER_STEP)))

def HyperToLatLon(x,y):
	'''
		Entra com a coordenada do HyperSignal e retorna latitude,longitude
	'''
	return ( y * HYPER_STEP - 90,  x * HYPER_STEP - 180 )

def GetGoogleTileHSRange(z,x,y):
	'''
		Retorna range do Tile da google nas coordenadas do HyperSignal
		(xmin, xmax, ymin, ymax)
	'''
	bounds = GoogleTileLatLonBounds(z,x,y)
	b1	=	LatLonToHyper(bounds[0],bounds[1])
	b2	=	LatLonToHyper(bounds[2],bounds[3])	
	return ( b1[0], b2[0], b1[1], b2[1] )

def GetGoogleTileFromHS(x,y,zoom):
	'''
		Retorna o z,y,x do tile da google onde o HS(x,y) está.
	'''
	lat, lon	=	HyperToLatLon(x,y)
	return GoogleTile(lat, lon, zoom)

def OperatorCorrect(operator):
	operator = operator.replace("'","").replace('"',"").replace("`","").replace("´","")
	for key, value in config.OPSREPLACES.iteritems():
		operator	=	operator.replace(key,value)
	return operator


'''
	Google Map Tile Functions
'''

def LatLonToMeters(lat, lon ):
        "Converts given lat/lon in WGS84 Datum to XY in Spherical Mercator EPSG:900913"

        mx = lon * originShift / 180.0
        my = math.log( math.tan((90 + lat) * math.pi / 360.0 )) / (math.pi / 180.0)

        my = my * originShift / 180.0
        return mx, my

def MetersToTile(mx, my, zoom):
        "Returns tile for given mercator coordinates"

        px, py = MetersToPixels( mx, my, zoom)
        return PixelsToTile( px, py)

def MetersToPixels(mx, my, zoom):
        "Converts EPSG:900913 to pyramid pixel coordinates in given zoom level"

        res = Resolution( zoom )
        px = (mx + originShift) / res
        py = (my + originShift) / res
        return px, py

def Resolution( zoom ):
        "Resolution (meters/pixel) for given zoom level (measured at Equator)"

        # return (2 * math.pi * 6378137) / (self.tileSize * 2**zoom)
        return initialResolution / (2**zoom)

def PixelsToTile(px, py):
        "Returns a tile covering region in given pixel coordinates"

        tx = int( math.ceil( px / float(tileSize) ) - 1 )
        ty = int( math.ceil( py / float(tileSize) ) - 1 )
        return tx, ty

def TileLatLonBounds(tx, ty, zoom ):
        "Returns bounds of the given tile in latutude/longitude using WGS84 datum"

        bounds = TileBounds( tx, ty, zoom)
        minLat, minLon = MetersToLatLon(bounds[0], bounds[1])
        maxLat, maxLon = MetersToLatLon(bounds[2], bounds[3])

        return ( minLat, minLon, maxLat, maxLon )

def TileBounds(tx, ty, zoom):
        "Returns bounds of the given tile in EPSG:900913 coordinates"

        minx, miny = PixelsToMeters( tx*tileSize, ty*tileSize, zoom )
        maxx, maxy = PixelsToMeters( (tx+1)*tileSize, (ty+1)*tileSize, zoom )
        return ( minx, miny, maxx, maxy )

def PixelsToMeters(px, py, zoom):
        "Converts pixel coordinates in given zoom level of pyramid to EPSG:900913"

        res = Resolution( zoom )
        mx = px * res - originShift
        my = py * res - originShift
        return mx, my

def MetersToLatLon(mx, my ):
        "Converts XY point from Spherical Mercator EPSG:900913 to lat/lon in WGS84 Datum"

        lon = (mx / originShift) * 180.0
        lat = (my / originShift) * 180.0
        lat = 180 / math.pi * (2 * math.atan( math.exp( lat * math.pi / 180.0)) - math.pi / 2.0)
        return lat, lon


def GetTileBounds(lat, lon, zoom):
        mx, my = LatLonToMeters( lat, lon )
        tminx, tminy = MetersToTile( mx, my, zoom )
        return TileLatLonBounds( tminx, tminy, zoom )

def TruncSix(val):
	return math.ceil(val*1e6)/1e6

def GetMetersPerPixel(tileBounds,zoom):
	M1 = LatLonToMeters(tileBounds[0], tileBounds[1])
	M2 = LatLonToMeters(tileBounds[2], tileBounds[3])
	PX = MetersToPixels(M1[0], M1[1], zoom)
	PY = MetersToPixels(M2[0], M2[1], zoom)
	X = [ TruncSix(M2[0] - M1[0]), TruncSix(M2[1] - M1[1]) ]
	P = [ int(math.ceil(PY[0] - PX[0])), int(math.ceil(PY[1] - PX[1])) ]
	return [ X[0]/P[0] , X[1] / P[1] ]

def GetTilePixelSize(tileBounds,zoom):
	M1 = LatLonToMeters(tileBounds[0], tileBounds[1])
	M2 = LatLonToMeters(tileBounds[2], tileBounds[3])
	PX = MetersToPixels(M1[0], M1[1], zoom)
	PY = MetersToPixels(M2[0], M2[1], zoom)
	P = [ int(math.ceil(PY[0] - PX[0])), int(math.ceil(PY[1] - PX[1])) ]
	return [ P[0] , P[1] ]

def GetTilePixelBounds(tileBounds,zoom):
	M1 = LatLonToMeters(tileBounds[0], tileBounds[1])
	M2 = LatLonToMeters(tileBounds[2], tileBounds[3])
	PX = MetersToPixels(M1[0], M1[1], zoom)
	PY = MetersToPixels(M2[0], M2[1], zoom)
	return [ int(math.ceil(PX[0])), int(math.ceil(PX[1])), int(math.ceil(PY[0])), int(math.ceil(PY[1])) ]

def LatLonToPixels(lat, lon, zoom):
	meters = LatLonToMeters(lat, lon)
	pp = MetersToPixels(meters[0], meters[1], zoom)
	return [ int(pp[0]), int(pp[1]) ]

def GoogleTile(lat, lon, zoom):	
	mx, my = LatLonToMeters( lat, lon )
	tx, ty = MetersToTile( mx, my, zoom )
	return zoom, tx, (2**zoom - 1) - ty

def GetTile(lat, lon, zoom):
	mx, my = LatLonToMeters( lat, lon )
	tx, ty = MetersToTile( mx, my, zoom )
	return tx, ty

def GoogleTile2Tile(z,x,y):
	return ( x, -y + 2 ** z -1 )

def GoogleTileLatLonBounds(z,x,y):
	tx,ty = GoogleTile2Tile(z,x,y)
	return TileLatLonBounds(tx, ty, z)

def TileLatLonPerPixel(tx,ty,zoom):
	bounds	=	TileLatLonBounds(tx, ty, zoom );
	#boundsm	=	(	LatLonToMeters(bounds[0], bounds[1] ), LatLonToMeters(bounds[2], bounds[3] ) )
	deltax	=	bounds[2] - bounds[0]	#	Latitude
	deltay	=	bounds[3] - bounds[1]	#	Longitude
	return	(	deltax / tileSize, deltay / tileSize )