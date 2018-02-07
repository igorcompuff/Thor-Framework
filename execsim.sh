#!/bin/bash
NS_GLOBAL_VALUE="RngRun=$1" ./waf --run "amigrid --retrans=$2 --failure=$3 --gridRows=2 --ddsaenabled=$4 --redundancy=$5 --daps=$5 --sender=3 --tfile=$6 --dumb=$7 --gridYShift=170 --gridXShift=170" &> log.txt
