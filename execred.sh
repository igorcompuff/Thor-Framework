#!/bin/bash
echo "Round $1"
#echo "Redundancy = $2"
#echo "Total Daps = $3"

NS_GLOBAL_VALUE="RngRun=$1" ./waf --run "ddsa_topology_tester --redundancy=$2 --totalDaps=$3 --gridRows=2 --gridColumns=3  --gridYShift=170 --gridXShift=170" &> log.txt
