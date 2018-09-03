#!/bin/bash

DdsaType=$1
RoundStart=$2
RoundStop=$3
RetransStart=$4

if [ "$DdsaType" = "original" ]
then
	DdsaInt=1
elif [ "$DdsaType" = "dumb" ]
then
	DdsaInt=2
fi

for ((ret = $RetransStart; ret <= 15; ret++))
do
	echo "Exec Retrans $ret"
	for ((i = $RoundStart; i <= $RoundStop; i++))
	do
		echo "Exec round $i"
		SimPath=sim/full_failure/$DdsaType/$ret/Round$i
		NS_GLOBAL_VALUE="RngRun=$i" ./waf --run "neighAmiSim --retrans=$ret --failure=1 --senders=* --fmode=0 --ddsamode=$DdsaInt --dap=4" &> $SimPath/log.txt
	done
done
