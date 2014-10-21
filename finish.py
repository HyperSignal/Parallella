#!/usr/bin/env python

import Image, numpy as np, math

#f = open("output.mat")
#data = f.read()
#f.close()
arr = np.fromfile("output.mat", dtype=np.uint8)
print len(arr)
arr = np.reshape(arr, (256, 256, 4))
image = Image.fromarray(arr)
image.save("output.png")

