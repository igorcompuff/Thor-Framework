#!/bin/sh

#PYTHON_BIND must be set to point to the correct pybindgen directory

echo $PYTHON_BIND
./waf configure --enable-tests --enable-examples --with-pybindgen=$PYTHON_BIND
./waf

