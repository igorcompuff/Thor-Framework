#!/bin/bash

#PYTHON_BIND must be set to point to the correct pybindgen directory

./waf configure --enable-tests --enable-examples --with-pybindgen=$PYTHON_BIND
./waf

