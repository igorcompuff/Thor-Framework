#!/bin/bash

function execute
{
	FailureType=$1
	DdsaType=$2
	SimBaseDir=$3
	RoundStart=$4
	RoundStop=$5
	Retrans=$6
	Dap=$7

	if [ "$FailureType" = "full_failure" ]
	then
		FailureInt=0
		WithFailure=1
	elif [ "$FailureType" = "malicious_failure" ]
	then
		FailureInt=1
		WithFailure=1
	else
		FailureInt=0
		WithFailure=0
	fi

	if [ "$DdsaType" = "etx" ]
	then
		DdsaInt=0
		Retrans=0
	elif [ "$DdsaType" = "original" ]
	then
		DdsaInt=1
		#Retrans=10
	elif [ "$DdsaType" = "dumb" ]
	then
		DdsaInt=2
		#Retrans=10
	elif [ "$DdsaType" = "dynamic_global" ]
	then
		DdsaInt=3
		Retrans=3
	else
		DdsaInt=4
		Retrans=3
	fi

	for ((i = $RoundStart; i <= $RoundStop; i++))
	do
		echo "Exec round $i"
		SimPath=$SimBaseDir/$FailureType/$DdsaType/Round$i
		echo $SimPath
		NS_GLOBAL_VALUE="RngRun=$i" ./waf --run "neighAmiSim --retrans=$Retrans --failure=$WithFailure --senders=* --fmode=$FailureInt --ddsamode=$DdsaInt --dap=$Dap" &> $SimPath/log.txt
	done
}

#execute "no_failure" "etx" "/home/igorribeiro/github/ns-3.26/sim" 1 5 0 3 &
#execute "no_failure" "etx" "/home/igorribeiro/github/ns-3.26/sim" 6 10 0 3

#execute "no_failure" "dumb" "/home/igorribeiro/github/ns-3.26/sim" 1 5 10 3 &
#execute "no_failure" "dumb" "/home/igorribeiro/github/ns-3.26/sim" 6 10 10 3

#execute "full_failure" "etx" "/home/igorribeiro/github/ns-3.26/sim" 1 5 0 3 &
#execute "full_failure" "etx" "/home/igorribeiro/github/ns-3.26/sim" 6 10 0 3

execute "full_failure" "dumb" "/home/igorribeiro/github/ns-3.26/sim" 1 5 10 3 &
execute "full_failure" "dumb" "/home/igorribeiro/github/ns-3.26/sim" 6 10 10 3

execute "full_failure" "original" "/home/igorribeiro/github/ns-3.26/sim" 1 5 10 3 &
execute "full_failure" "original" "/home/igorribeiro/github/ns-3.26/sim" 6 10 10 3

#execute "full_failure" "dynamic_global" "/home/igorribeiro/github/ns-3.26/sim" 1 5 3 3 &
#execute "full_failure" "dynamic_global" "/home/igorribeiro/github/ns-3.26/sim" 6 10 3 3

#execute "full_failure" "flooding" "/home/igorribeiro/github/ns-3.26/sim" 1 5 3 3 &
#execute "full_failure" "flooding" "/home/igorribeiro/github/ns-3.26/sim" 6 10 3 3


#execute "malicious_failure" "etx" "/home/igorribeiro/github/ns-3.26/sim" 1 5 0 3 &
#execute "malicious_failure" "etx" "/home/igorribeiro/github/ns-3.26/sim" 6 10 0 3

execute "malicious_failure" "dumb" "/home/igorribeiro/github/ns-3.26/sim" 1 5 10 3 &
execute "malicious_failure" "dumb" "/home/igorribeiro/github/ns-3.26/sim" 6 10 10 3

execute "malicious_failure" "original" "/home/igorribeiro/github/ns-3.26/sim" 1 5 10 3 &
execute "malicious_failure" "original" "/home/igorribeiro/github/ns-3.26/sim" 6 10 10 3

#execute "malicious_failure" "dynamic_global" "/home/igorribeiro/github/ns-3.26/sim" 1 5 3 3 &
#execute "malicious_failure" "dynamic_global" "/home/igorribeiro/github/ns-3.26/sim" 6 10 3 3

#execute "malicious_failure" "flooding" "/home/igorribeiro/github/ns-3.26/sim" 1 5 3 3 &
#execute "malicious_failure" "flooding" "/home/igorribeiro/github/ns-3.26/sim" 6 10 3 3
