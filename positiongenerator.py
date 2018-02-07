#!/usr/bin/python
from subprocess import call
import os

TOTAL = 200

def execute():
	pos = 1
	sim_round = pos;
	while (pos <= TOTAL):
		call(["./execred.sh", str(sim_round), "1", "1"])
		if os.path.exists("dap_pos.txt"):
			os.rename("dap_pos.txt", "positions/pos_" + str(pos) + ".txt")
			pos+= 1
		sim_round += 1


def simulate():
	global MAX_REDUNDANCY
	for redundancy in range(1, MAX_REDUNDANCY + 1):
		totalDaps = redundancy
		found = False
		while (totalDaps <= 2 * redundancy) and not found:
			found = executeRounds(totalDaps, redundancy)
			totalDaps += 1

execute()
