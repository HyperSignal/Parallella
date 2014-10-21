#!/usr/bin/env python

import struct
import numpy as np

output = ""
size = 0
id = 0xAE

#	unsigned int id, width, height, A,B,C,D;
#	unsigned char swidth, sheight, *sample;

works = [
	(0, 256, 256, 0, 0, 256, 256, 22, 22, 0),
	(1, 256, 256, 0, 0, 256, 256, 22, 22, 0),
	(2, 256, 256, 0, 0, 256, 256, 22, 22, 0),
	(3, 256, 256, 0, 0, 256, 256, 22, 22, 0)
]

for i in range(len(works)):
	sample = np.random.randint(64, size=22*22)
	#print sample
	work = works[i]
	data = struct.pack("IIIIIIIBBI",work[0],work[1],work[2],work[3],work[4],work[5],work[6],work[7],work[8],work[9])
	output += data
	size += len(data)
	for k in sample:
		output += chr(k)
		size += 1

output = struct.pack("III",id,size,len(works)) + output

f = open("testpacket.bin","wb")
f.write(output)
f.close()
