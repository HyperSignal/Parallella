#!/bin/bash

set -e

ESDK=${EPIPHANY_HOME}
ELIBS=${ESDK}/tools/host/lib:${LD_LIBRARY_PATH}
EHDF=${EPIPHANY_HDF}

cd epiphany/

sudo -E LD_LIBRARY_PATH=${ELIBS} EPIPHANY_HDF=${EHDF}  ./test.elf "$@" < /dev/stdin
