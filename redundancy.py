#!/usr/bin/python
from subprocess import call
import os

MAX_REDUNDANCY = 15
ROUNDS = 10
nextRound = 1;

def executeRounds(totalDaps, redundancy):
	global ROUNDS
	global nextRound
	found = False
	limit = nextRound + ROUNDS
	i = 0;
	while (nextRound <= limit):
		call(["./execred.sh", str(nextRound), str(redundancy), str(totalDaps)])
		if os.path.exists("dap_pos.txt"):
			os.rename("dap_pos.txt", "positions/dap_pos_" + str(redundancy) + "_" + str(i) + ".txt")
			found = True
		nextRound += 1
		i += 1
	return found


def simulate():
	global MAX_REDUNDANCY
	for redundancy in range(1, MAX_REDUNDANCY + 1):
		totalDaps = redundancy
		found = False
		while (totalDaps <= 2 * redundancy) and not found:
			found = executeRounds(totalDaps, redundancy)
			totalDaps += 1

simulate()
