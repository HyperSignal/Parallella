#!/bin/bash

BUILD_VERSION=$(expr `cat buildnumber` + 1)
BUILD_VERSION=$(printf %06d $BUILD_VERSION)
VERSION_UPPER=`expr $BUILD_VERSION / 1000`
VERSION_LOWER=`expr $VERSION_UPPER % 10`
VERSION_UPPER=`expr $VERSION_UPPER / 10`
VERSION_STRING="$VERSION_UPPER.$VERSION_LOWER"

echo $BUILD_VERSION
echo $VERSION_STRING