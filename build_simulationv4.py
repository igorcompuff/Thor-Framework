#!/usr/bin/python
import os.path
import shutil
import collections
from optparse import OptionParser
from subprocess import call

class Simulation:

	def __init__(self, rounds, totalTopologies, retransmissions):
		self.rounds = rounds
		self.totalTopologies = totalTopologies
		self.retransmissions = retransmissions

	def getMode(self):
		if self.ddsaOption == "0":
                	mode = "Etx"
                elif self.dumbOption == "1":
                	mode = "Ddsa_Dumb"
                else:
                	mode = "Ddsa"
		
		return mode
	
	def executeRounds(self, inputRedDir, outputRedDir, topology):

        	topologies = os.listdir(inputRedDir)
		topologyFile = os.path.join(inputRedDir, topologies[topology - 1])
		print("Topology File = " + topologyFile)
		print("Redundancy = " + self.redundancy + "\n")
		shutil.copy2(topologyFile, os.path.join(outputRedDir, topologies[topology - 1]))

		for j in range(1, self.rounds + 1):
                	print("Round " + str(j))        	
			roundDir = os.path.join(os.path.join(outputRedDir, self.getMode()), str(j))
			print("Round directory: " + roundDir)
			if not os.path.exists(roundDir):
				os.makedirs(roundDir)
				call(["./execsim.sh", str(j), str(self.retransmissions), self.failureOption, self.ddsaOption, self.redundancy, topologyFile, self.dumbOption])

                        for f in os.listdir("./"):
                        	if f.endswith('.txt') or f.endswith('.pcap'):
					shutil.move(os.path.join("./", f), os.path.join(roundDir,f))
                                

	def execute(self, topology, topologyOutputDir):
		for f in os.listdir(self.topologyDir):
			if not "results" in f:
				outputRedDir = os.path.join(topologyOutputDir,f)
                        	
				if not os.path.exists(outputRedDir):
					os.makedirs(outputRedDir)
				inputRedDir = os.path.join(self.topologyDir, f)
				self.redundancy = f

                        	self.executeRounds(inputRedDir, outputRedDir, topology)

	def executeTopologies(self):
		for i in range (1, self.totalTopologies + 1):
			topologyOutputDir = os.path.join(self.resultsDir, "top_" + str(i))
			if not os.path.exists(topologyOutputDir):
				os.makedirs(topologyOutputDir)

			self.execute(i, topologyOutputDir)

	def configure(self, topologyDir, ddsaOption, dumbOption, failureOption):
		self.topologyDir = topologyDir
                self.resultsDir = os.path.join(self.topologyDir, "results" if failureOption == "0" else "results_failure")
                self.ddsaOption = ddsaOption
                self.dumbOption = dumbOption
                self.failureOption = failureOption


	def executeRedundancyComparisonSim(self):
	        topdir = "/home/igor/github/ns-3.26/redundancytest"
		self.configure(topdir, "1", "0", "0")

		if os.path.exists(self.resultsDir):
                        shutil.rmtree(self.resultsDir)
                os.makedirs(self.resultsDir)

		print("Executing Simulation for redundancy comparison. \n")
		print("Normal DDSA is enabled with " + str(self.retransmissions) + " retransmissions")
		print("Dap failure is not enabled")
        	print("Number of rounds = " + str(self.rounds))
		print("Total number of topologies used = " + str(self.totalTopologies) + "\n")

		self.executeTopologies()

	def executeMode(self):
		
		print(self.getMode() + " is enabled with " + (str(self.retransmissions) + " retransmissions") if self.ddsaOption != "0"  else "")
		self.executeTopologies()

	def executeModesComparisonSim(self):
		topdir = "/home/igor/github/ns-3.26/topologies"

		print("Executing Simulation for mode comparison. \n")
		print("Number of rounds = " + str(self.rounds))
                print("Total number of topologies used = " + str(self.totalTopologies) + "\n")

		print("Executing simulations with dap failure for all modes")
		self.configure(topdir, "1", "0", "1")
		self.executeMode()

		self.configure(topdir, "1", "1", "1")
                self.executeMode()

		self.configure(topdir, "0", "0", "1")
                self.executeMode()

		print("Executing simulations without dap failure for all modes")
                self.configure(topdir, "1", "0", "0")
                self.executeMode()

                self.configure(topdir, "1", "1", "0")
                self.executeMode()

                self.configure(topdir, "0", "0", "0")
                self.executeMode()

def parse():
	parser = OptionParser()
	parser.add_option("-r", "--retrans", action="store", dest="retrans", help="Number of retransmissions", type=int, default=0)
	parser.add_option("-n", "--rounds", action="store", dest="rounds", help="Number of simulation rounds", type=int, default=1)
	parser.add_option("-t", "--topologies", action="store", dest="topologies", help="Number of topologies", type=int, default=1)
	parser.add_option("-m", "--simmode", action="store", dest="simmode", help="Simulation mode: 1: Redundancy Test, 2: Mode Comparison", type=int, default=1)
	(options, args) = parser.parse_args()

	return options



options = parse()

simulation = Simulation(options.rounds, options.topologies, options.retrans)

if options.simmode == 1:
	simulation.executeRedundancyComparisonSim()
else:
	simulation.executeModesComparisonSim()
