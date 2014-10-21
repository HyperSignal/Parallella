#!/usr/bin/env python

import Image, numpy as np, math
from struct import unpack, calcsize

SS = calcsize("IIIIIIIBBI")

f = open("output.mat")
data = f.read(8)
id, size = unpack("II",data)

packs = []

if id == 0xEA:
	print "Match ID! 0xEA"
	data = f.read(4)
	count = unpack("I", data)[0]
	print "%s works available" %count
	for i in range(count):
		data = f.read(SS)
		#	unsigned int id, width, height, A,B,C,D;
		#	unsigned char swidth, sheight, *sample;
		id, width, height, A,B,C,D,swidth,sheight,sample = unpack("IIIIIIIBBI", data)
		#print id, width, height, A,B,C,D,swidth,sheight,sample 
		data = f.read(width*height*4)
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
	print "Done!"
	
	for i in packs:
		print "Generating image for %s (%s,%s)"%(i["id"],i["width"], i["height"])
		arr = np.fromstring(i["sample"], dtype=np.uint8)
		print len(arr)
		arr = np.reshape(arr, (i["width"], i["height"], 4))
		image = Image.fromarray(arr)
		image.save("output-%s.png" %i["id"])
	
else:
	print "Invalid ID :( %s" %(hex(id))


f.close()
'''
arr = np.fromfile("output.mat", dtype=np.uint8)
print len(arr)
arr = np.reshape(arr, (256, 256, 4))
image = Image.fromarray(arr)
image.save("output.png")
'''