#!/usr/bin/env python

import Image, numpy as np, math


def hsv2rgb(h, s, v):
    h = float(h)
    s = float(s)
    v = float(v)
    h60 = h / 60.0
    h60f = math.floor(h60)
    hi = int(h60f) % 6
    f = h60 - h60f
    p = v * (1 - s)
    q = v * (1 - f * s)
    t = v * (1 - (1 - f) * s)
    r, g, b = 0, 0, 0
    if hi == 0: r, g, b = v, t, p
    elif hi == 1: r, g, b = q, v, p
    elif hi == 2: r, g, b = p, v, t
    elif hi == 3: r, g, b = p, q, v
    elif hi == 4: r, g, b = t, p, v
    elif hi == 5: r, g, b = v, p, q
    r, g, b, a= int(r * 255), int(g * 255), int(b * 255), int(192)
    
    return r, g, b, a 

def BuildTable(s,v):
    table = []
    for i in range(0,256):
        if i > 31:
            val = (0,0,0,0)
        else:
            val = hsv2rgb((i/31.0)*120, s, v)
        table.append(val)
    return table

def BuildImage(data,w,h, table):
	arr = np.zeros((h,w,4), dtype=np.uint8)
	for y in range(h):
		for x in range(w):
			p = y * w + x
			r,g,b,a = table[ord(data[p])]
			arr[y][x][0] = r
			arr[y][x][1] = g
			arr[y][x][2] = b
			arr[y][x][3] = a
	return Image.fromarray(arr)

table = BuildTable(1.0,0.902)

f = open("colortable.mat","wb")
for i in range(0,256):
    f.write(chr(table[i][0]))
    f.write(chr(table[i][1]))
    f.write(chr(table[i][2]))
    f.write(chr(table[i][3]))
f.close()

f = open("output.mat")
data = f.read()
f.close()
image = BuildImage(data,384,384, table)
image.save("output.png")

