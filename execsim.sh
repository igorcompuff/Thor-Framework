#!/bin/bash
echo "Round $1"
echo "$2 Retransmissions"
echo "Failure = $3"
echo "Ddsa = $4"
NS_GLOBAL_VALUE="RngRun=$1" ./waf --run "amigrid --retrans=$2 --failure=$3 --gridRows=2 --ddsaenabled=$4 --sender=3 --gridYShift=170 --gridXShift=170" &> log.txt
