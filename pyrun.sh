#!/bin/bash

set -e

ESDK=${EPIPHANY_HOME}
ELIBS=${ESDK}/tools/host/lib:${LD_LIBRARY_PATH}
EHDF=${EPIPHANY_HDF}

echo "Compiling python module"
sudo -E python setup.py install

echo "Running"
cp test.py Build/epiphany

cd Build/epiphany/

sudo -E LD_LIBRARY_PATH=${ELIBS} EPIPHANY_HDF=${EHDF} python test.py
