#!/bin/bash

echo "This is a test build of interpolator"

if [ ! -d Build/host/ ]
then
	mkdir -p Build/host/
fi 

echo "Cleaning"
rm -fr Build/host/*

echo "Compiling tools"
gcc -c src/tools.c -std=c99 -o Build/host/tools.o

echo "Compiling process"
gcc -c src/process.c -std=c99 -o Build/host/process.o

echo "Compiling test"
gcc -c src/interpolate.c -std=c99 -o Build/host/interpolate.o

echo "Linking all together"
gcc -o interpolate Build/host/*.o -lm -lpthread