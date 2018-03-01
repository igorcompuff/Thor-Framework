#!/usr/bin/python
import os.path
import shutil
import collections
from optparse import OptionParser
from subprocess import call

class Simulation:

	def __init__(self, rounds, retransmissions, daps):
		self.rounds = rounds
		self.retransmissions = retransmissions
		self.daps = daps

	def configure(self, topologyDir, ddsaOption, dumbOption, failureOption):
                self.topologyDir = topologyDir
                self.ddsaOption = ddsaOption
                self.dumbOption = dumbOption
                self.failureOption = failureOption
                self.resultsDir = os.path.join(self.topologyDir, "results" if failureOption == "0" else "results_failure")
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
	
	def executeRounds(self, outputDir, topologyFile):

		print("Topology File = " + topologyFile)

		for j in range(1, self.rounds + 1):
                	print("Round " + str(j))        	
			roundDir = os.path.join(outputDir, str(j))
			print("Round directory: " + roundDir)
			if not os.path.exists(roundDir):
				os.makedirs(roundDir)
				call(["./execsim2.sh", str(j), str(self.retransmissions), self.failureOption, self.ddsaOption, str(self.daps), topologyFile, self.dumbOption, "0"])

                        for f in os.listdir("./"):
                        	if f.endswith('.txt') or f.endswith('.pcap'):
					shutil.move(os.path.join("./", f), os.path.join(roundDir,f))
                                

	def execute(self):
		print(self.getMode() + " is enabled with " + (str(self.retransmissions) + " retransmissions") if self.ddsaOption != "0"  else "")
		topDir = os.path.join(self.topologyDir, str(self.daps))
		for top in os.listdir(topDir):
			topFile = os.path.join(topDir, top)
			outputDir = os.path.join(self.resultsDir, top.replace(".txt", ""))
			self.executeRounds(outputDir, topFile)	

	def simulateWithoutFailure(self, topdir):
		print("Executing simulations with full dap failure for all modes")
                self.configure(topdir, "1", "0", "0")
                self.execute()

                self.configure(topdir, "1", "1", "0")
                self.execute()

                self.configure(topdir, "0", "0", "0")
                self.execute()
	
	def simulateWithFailure(self, topdir):
		print("Executing simulations without dap failure for all modes")
                self.configure(topdir, "1", "0", "1")
                self.execute()

                self.configure(topdir, "1", "1", "1")
                self.execute()

                self.configure(topdir, "0", "0", "1")
                self.execute()	


	def executeModesComparisonSim(self):
		topdir = "/home/igor/github/ns-3.26/topologies"

		print("Executing Simulation for mode comparison. \n")
		print("Number of rounds = " + str(self.rounds))
                print("Redundancy = " + str(self.daps) + "\n")
		
		self.simulateWithoutFailure(topdir)
		self.simulateWithFailure(topdir)

def parse():
	parser = OptionParser()
	parser.add_option("-r", "--retrans", action="store", dest="retrans", help="Number of retransmissions", type=int, default=0)
	parser.add_option("-n", "--rounds", action="store", dest="rounds", help="Number of simulation rounds", type=int, default=1)
	parser.add_option("-d", "--daps", action="store", dest="daps", help="Number of daps", type=int, default=1)
	
	(options, args) = parser.parse_args()

	return options



options = parse()

simulation = Simulation(options.rounds,  options.retrans, options.daps)


simulation.executeModesComparisonSim()
