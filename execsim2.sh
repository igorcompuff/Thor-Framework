#!/bin/bash

echo "Retrans = $2"
echo "Failure = $3"
echo "Ddsa = $4"
echo "Redundancy = $5"
echo "File = $6"
echo "Dumb = $7"
echo "Failure Mode = $8"
NS_GLOBAL_VALUE="RngRun=$1" ./waf --run "amigrid --retrans=$2 --failure=$3 --gridColumns=5 --gridRows=5 --ddsaenabled=$4 --redundancy=$5 --daps=$5 --senders=20 --tfile=$6 --dumb=$7 --gridYShift=160 --gridXShift=160 --fmode=$8" &> log.txt
