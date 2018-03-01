#!/usr/bin/python
import os.path
import shutil
import collections
from optparse import OptionParser
from subprocess import call

class Simulation:

	def __init__(self, rounds, retransmissions, daps, topologyFile):
		self.rounds = rounds
		self.retransmissions = retransmissions
		self.daps = daps
		self.topologyFile = topologyFile

	def configure(self, ddsaOption, dumbOption, failureOption, failureMode, resultDir):
                self.ddsaOption = ddsaOption
                self.dumbOption = dumbOption
                self.failureOption = failureOption
		self.failureMode = failureMode
                
		errorModeDir = "results"
		
		if failureOption == "1":
			if failureMode == "0":
				errorModeDir = "results_full_failure"
			else:
				errorModeDir = "results_malicious_failure"

		self.resultsDir = os.path.join(resultDir, errorModeDir)
                self.resultsDir = os.path.join(self.resultsDir, self.getMode())
                self.resultsDir = os.path.join(self.resultsDir, str(self.daps))

	def getMode(self):
		if self.ddsaOption == "0":
                	mode = "Etx"
                elif self.dumbOption == "1":
                	mode = "Ddsa_Dumb"
                else:
                	mode = "Ddsa"
		
		return mode
	
	def executeRounds(self):

		for j in range(1, self.rounds + 1):
                	print("Round " + str(j))        	
			roundDir = os.path.join(self.resultsDir, str(j))
			print("Round directory: " + roundDir)
			if not os.path.exists(roundDir):
				os.makedirs(roundDir)
				call(["./execsim2.sh", str(j), str(self.retransmissions), self.failureOption, self.ddsaOption, str(self.daps), self.topologyFile, self.dumbOption, self.failureMode])

                        for f in os.listdir("./"):
                        	if f.endswith('.txt') or f.endswith('.pcap'):
					shutil.move(os.path.join("./", f), os.path.join(roundDir,f))
                                

	def simulateWithoutFailure(self, resultsDir):
		print("Executing simulations without dap failure for all modes")
                #self.configure("1", "0", "0", "0", resultsDir)
                #self.executeRounds()

                #self.configure("1", "1", "0", "0", resultsDir)
                #self.executeRounds()

                self.configure("0", "0", "0", "0", resultsDir)
                self.executeRounds()
	
	def simulateWithFullFailure(self, resultsDir):
		print("Executing simulations with full dap failure for all modes")
                self.configure("1", "0", "1", "0", resultsDir)
                self.executeRounds()

                self.configure("1", "1", "1", "0", resultsDir)
                self.executeRounds()

                self.configure("0", "0", "1", "0", resultsDir)
                self.executeRounds()

	def simulateWithMaliciousFailure(self, resultsDir):
		print("Executing simulations with malicious dap failure for all modes")
                self.configure("1", "0", "1", "1", resultsDir)
                self.executeRounds()

                self.configure("1", "1", "1", "1", resultsDir)
                self.executeRounds()

                self.configure("0", "0", "1", "1", resultsDir)
                self.executeRounds()


def parse():
	parser = OptionParser()
	parser.add_option("-c", "--copies", action="store", dest="retrans", help="Number of retransmissions", type=int, default=0)
	parser.add_option("-n", "--rounds", action="store", dest="rounds", help="Number of simulation rounds", type=int, default=1)
	parser.add_option("-d", "--daps", action="store", dest="daps", help="Number of daps", type=int, default=1)
	parser.add_option("-t", "--topology", action="store", dest="topologyFile", help="Topology File", type=str, default="")
	parser.add_option("-r", "--results", action="store", dest="resultsDir", help="Directory where results are stored", type=str, default="")
	(options, args) = parser.parse_args()

	return options


options = parse()

print("Executing Simulation for mode comparison. \n")
print("Number of rounds = " + str(options.rounds))
print("Redundancy = " + str(options.daps) + "\n")
print("Topology File = " + options.topologyFile)

simulation = Simulation(options.rounds, options.retrans, options.daps, options.topologyFile)


simulation.simulateWithoutFailure(options.resultsDir)
simulation.simulateWithFullFailure(options.resultsDir)
simulation.simulateWithMaliciousFailure(options.resultsDir)
