#!/usr/bin/env python

import e_mod

print "Init Epiphany"
e_mod.InitEpiphany();
print "Doing work"
k = [       157,83 ,153,147,223,114,248,200,120,185,30 ,86 ,50 ,28 ,29 ,180,153,169,225,146,20 ,115,229,108,50 ,206,89 ,51 ,89 ,99 ,19 ,112,74 ,9  ,232,185,36 ,54 ,210,66 ,151,206,52 ,100,232,163,191,135,125,81 ,122,246,20 ,197,194,210,87 ,122,11 ,16 ,163,90 ,1  ,152 ]
d = ""
for i in k:
	d += chr(i)

t, o = e_mod.DoWork(d)

print "Time %s" %t
f = open("pymat","w")
f.write(o)
f.close()