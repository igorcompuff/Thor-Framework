#!/usr/bin/python
import os.path
import shutil
import collections
from optparse import OptionParser
from subprocess import call

class SimDirCreator:
    def __init__(self, rounds):
        self.rounds = rounds

    def createRounds(self, baseDir):
        for i in range(1, self.rounds + 1):
            roundDir = os.path.join(baseDir, "Round" + str(i))
            if not os.path.exists(roundDir):
                os.makedirs(roundDir)

    def create(self):
        basedirs = ["sim/full_failure/etx", "sim/full_failure/original",
                    "sim/full_failure/dumb", "sim/full_failure/dynamic_global",
                    "sim/full_failure/flooding",
                    "sim/malicious_failure/etx",
                    "sim/malicious_failure/original",
                    "sim/malicious_failure/dumb",
                    "sim/malicious_failure/dynamic_global",
                    "sim/malicious_failure/flooding",
                    "sim/no_failure/etx", "sim/no_failure/original",
                    "sim/no_failure/dumb", "sim/no_failure/dynamic_global",
                    "sim/no_failure/flooding"]

        filepath = "/home/igorribeiro/github/ns-3.26/"
        for d in basedirs:
            for i in range(2, 16):
                path = os.path.join(d, str(i))
                self.createRounds(filepath + path)
            #print basedirs[i] + "\n\n"

def parse():
	parser = OptionParser()
	parser.add_option("-r", "--rounds", action="store", dest="rounds", help="Number of rounds", type=int, default=1)
	(options, args) = parser.parse_args()

	return options


options = parse()

print("Creating Simulation structure for " + str(options.rounds) + " rounds at .\n")

creator = SimDirCreator(options.rounds)

creator.create()
