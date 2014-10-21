#!/bin/bash

set -e

ESDK=${EPIPHANY_HOME}
ELIBS=${ESDK}/tools/host/lib
EINCS=${ESDK}/tools/host/include
ELDF=${ESDK}/bsps/current/fast.ldf

SCRIPT=$(readlink -f "$0")
EXEPATH=$(dirname "$SCRIPT")
cd $EXEPATH

CROSS_PREFIX=
case $(uname -p) in
	arm*)
		# Use native arm compiler (no cross prefix)
		CROSS_PREFIX=
		;;
	   *)
		# Use cross compiler
		CROSS_PREFIX="arm-linux-gnueabihf-"
		;;
esac

if [ ! -d Build/epiphany/ ]
then
	mkdir -p Build/epiphany/
fi 

BUILD_VERSION=$(expr `cat buildnumber` + 1)
BUILD_VERSION=$(printf %06d $BUILD_VERSION)
VERSION=`cat version`
DATE=`date`
HOSTNAME=`hostname`

rm -fr Build/epiphany/*
rm -fr ../New/epiphany/*

#echo "Compiling python module"

#sudo -E python setup.py install

echo "Compiling tools"
e-gcc -T ${ELDF} -std=c99 -O3 -c e_src/tools.c -std=c99 -o Build/epiphany/e_tools.o -le-lib

# Build HOST side application
echo "Building Host application"
${CROSS_PREFIX}gcc -std=c99 -O3 -D_HOST_BUILD -DB_PROG_VERSION="\"${VERSION}\"" -DB_BUILD_VERSION="\"${BUILD_VERSION}\"" -DB_HOSTNAME="\"${HOSTNAME}\"" -DB_DATE="\"${DATE}\"" e_src/tools.c e_src/test.c  -o Build/epiphany/test.elf -I ${EINCS} -L ${ELIBS} -le-hal -lm 

# Build DEVICE side program
echo "Building Bilinear Device Program"
e-gcc -T ${ELDF} -std=c99 -O3 e_src/e_test.c Build/epiphany/*.o -o Build/epiphany/e_test_bilinear.elf -le-lib

# Convert ebinary to SREC file
e-objcopy --srec-forceS3 --output-target srec Build/epiphany/e_test_bilinear.elf Build/epiphany/e_test_bilinear.srec


# Build DEVICE side program
echo "Building Bicosine Device Program"
e-gcc -T ${ELDF} -std=c99 -O3 -DBICOSINE_INTERPOLATION e_src/e_test.c Build/epiphany/*.o -o Build/epiphany/e_test_bicosine.elf -le-lib

# Convert ebinary to SREC file
e-objcopy --srec-forceS3 --output-target srec Build/epiphany/e_test_bicosine.elf Build/epiphany/e_test_bicosine.srec

echo "Copying colortable.mat"
cp colortable.mat Build/epiphany/
cp Build/epiphany/* ../New/epiphany

echo $BUILD_VERSION | sed 's/0*//' > buildnumber