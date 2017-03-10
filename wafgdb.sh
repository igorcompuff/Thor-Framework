#!/bin/bash

./waf --command-template="gdb %s" --run=$1
