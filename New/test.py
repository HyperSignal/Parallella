import tool, MySQLdb, Image, math, numpy as np, numpy, os, json, re, struct
import subprocess
from time import time

username = "hypersignal"
password = "hs1234"
database = "hypersignal"
host     = "10.0.5.180"

TWIDTH = 256
THEIGHT = 256
MAXTILES = 4
TA = 0
TB = 0
TC = TWIDTH
TD = THEIGHT

tileblock = []
dataout = ""

print "Connecting"
con = MySQLdb.connect(host,username,password)
con.select_db(database)
sumtime = 0

def FetchTilesToDo(operator,alltiles=False):
	cursor = con.cursor()
	if alltiles:
		cursor.execute("SELECT * FROM `tiles` WHERE `operator` = %s AND `z` = 15", (operator))
	else:
		cursor.execute("SELECT * FROM `tiles` WHERE `operator` = %s and `updated` = 0", (operator))

	row		=	cursor.fetchone()
	tiles	=	{}

	for zoom in range(15,16):
		tiles[zoom] = []
	numtiles	=	0

	while row is not None:
		tiles[row[2]].append( (row[2],row[0],row[1],row[3]) )
		row = cursor.fetchone()
		numtiles	=	numtiles + 1

	return tiles,numtiles

def FetchBlock(start,end,operator):
	cursor = con.cursor()
	cursor.execute("SELECT * FROM datamatrix WHERE x >= %s and x < %s and y >= %s and y < %s and operator = %s",(start[0],end[0],start[1],end[1],operator))
	blockdata = cursor.fetchall()
	block = numpy.zeros((end[0]-start[0],end[1]-start[1]),dtype=numpy.uint8)
	block.fill(-1)
	for data in blockdata:
		x			=	int(data[0]) - start[0]
		y			=	int(data[1]) - start[1]
		sig			=	int(data[2])
		block[x,y]	=	sig
	return block



def DoTile(tx,ty,tz,operator):
	global sumtime
	global dataout
	global tileblock
	if len(tileblock) < MAXTILES:
		#print "Adding tile %s,%s,%s for %s to queue" %(tx,ty,tz,operator)
		xmin,xmax,ymin,ymax		=	tool.GetGoogleTileHSRange(tz,tx,ty)
		dx	=	xmax-xmin
		dy	=	ymax-ymin

		#print "Fetching Block"
		block = FetchBlock( (xmin,ymin),(xmax,ymax), operator)

		#print "Block Shape: %s,%s" %block.shape

		data = bytearray()
		#print "Generating buffer"
		tileblock.append("%s/%s-%s-%s-%s.png" % (operator,tz,tx,ty,operator))
		#	unsigned int id, width, height, A,B,C,D;
		#	unsigned char swidth, sheight, *sample;
		dataout += struct.pack("IIIIIIIBBI",len(tileblock),TWIDTH,THEIGHT,TA,TB,TC,TD,dx,dy,0)

		for y in range(0,dy):
			for x in range(0,dx):
				dataout += chr(block[x,y])

	else:
		start = time()
		print "%s tiles at queue. Starting work" %MAXTILES
		size = len(dataout)
		id = 0xAE
		dataout = struct.pack("III",id,size,MAXTILES) + dataout
		proc = subprocess.Popen(['./run.sh', '-ctvz'],stdout=subprocess.PIPE,stdin=subprocess.PIPE)
		proc.stdin.write(dataout)
		proc.stdin.close()
		data = proc.stdout.read(12)
		id, size, numworks = struct.unpack("III",data)
		if id == 0xEA:
			#print "Got response!\n%s outputs are available at %s bytes size." %(numworks, size)
			#data = proc.stdout.read(size)
			SS = struct.calcsize("IIIIIIIBBI")
			packs = []
			for i in range(numworks):
				data = proc.stdout.read(SS)
				#	unsigned int id, width, height, A,B,C,D;
				#	unsigned char swidth, sheight, *sample;
				id, width, height, A,B,C,D,swidth,sheight,sample = struct.unpack("IIIIIIIBBI", data)
				#print id, width, height, A,B,C,D,swidth,sheight,sample 
				data = proc.stdout.read(width*height*4)
				packs.append({
					"id"		:	id,
					"width"		:	width,
					"height"	:	height,
					"A"			:	A,
					"B"			:	B,
					"C"			:	C,
					"D"			:	D,
					"swidth"	:	swidth,
					"sheight"	:	sheight,
					"sample"	:	data
				})

			for i in packs:
				filename = tileblock[i["id"]-1]
				#print "Generating image for %s (%s,%s) - %s"%(i["id"],i["width"], i["height"],filename)
				arr = np.fromstring(i["sample"], dtype=np.uint8)
				arr = np.reshape(arr, (i["width"], i["height"], 4))
				image = Image.fromarray(arr)
				image.save("%s" %filename)
			delta = time() - start
			sumtime += delta
			mstile = delta / MAXTILES
			tilesec = 1.0 / mstile
			print "Done {:d} tiles in {:3.4f} seconds ({:3.4f} tiles/sec - {:1.4f} ms per tile)!".format(MAXTILES, delta, tilesec, mstile) 
			tileblock = []
			dataout = ""
			proc.wait()


donetiles = 0
tileslist, numtiles = FetchTilesToDo("VIVO",True)
starttime = time()
print "%s tiles to do" %numtiles
for tile in tileslist[15]:
	DoTile(tile[1],tile[2],tile[0], tile[3])

delta = starttime - time()
sumtime /= numtiles
print "Done %s tiles in %s seconds" %(donetiles,delta)
print "Average of %s per tile" %(sumtime)
con.close()