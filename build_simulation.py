import os.path
import shutil
import collections
from optparse import OptionParser
from subprocess import call

def parse():
	parser = OptionParser()
	parser.add_option("-d", "--ddsa", action="store_true", dest="ddsa", default=False, help="Use ddsa in the simulations")
	parser.add_option("-f", "--failure", action="store_true", dest="failure", default=False, help="Simulate with a DAP failure")
	parser.add_option("-r", "--retrans", action="store", dest="retrans", help="Number of retransmissions", type=int, default=0)
	parser.add_option("-n", "--number", action="store", dest="rounds", help="Number of simulation rounds", type=int, default=1)
	parser.add_option("-i", "--init", action="store", dest="init_redundancy", help="Initial redundancy", type=int, default=2)
	parser.add_option("-e", "--end", action="store", dest="end_redundancy", help="Final redundancy", type=int, default=2)
	(options, args) = parser.parse_args()

	return options

	

def execRound(roundNumber, options, redundancy, baseDir):
	ddsaOption = "1" if options.ddsa else "0"
	failureOption = "1" if options.failure else "0"

	call(["./execsim.sh", str(roundNumber), str(options.retrans), failureOption, ddsaOption, str(redundancy)])
	
	execDir = baseDir + str(roundNumber)

	#Create the directories
	if os.path.exists(execDir):
		shutil.rmtree(execDir)
	pcapDir = execDir + "/pcap"
	os.makedirs(execDir)
	os.makedirs(pcapDir)

	#Move round output files to the proper directories
	for f in os.listdir("./"):
		if f.endswith('.txt'):
                        shutil.move(os.path.join("./", f), os.path.join(execDir,f))
                elif f.endswith('.pcap'):
                        shutil.move(os.path.join("./", f), os.path.join(pcapDir,f))
	

def execRounds(options, redundancy, baseDir):
	for i in range(1, options.rounds + 1):
		execRound(i, options, redundancy, baseDir)

def completeBaseDirPath(options, initialBaseDir):
	
	baseDir = initialBaseDir
	
	baseDir = baseDir + ("ddsa/retrans/" + str(options.retrans) + "/") if options.ddsa else "nonddsa/"
	baseDir = baseDir + "failure/" if options.failure else "nofailure/"

        return baseDir


def runSimulation(options):
	
	print("Executing Simulation with the following parameters:\n")
	print("Init Redundancy = " + str(options.init_redundancy))
	print("End Redundancy = " + str(options.end_redundancy))
	print("DDSA Enabled = " + ("yes, with " + str(options.retrans) + " retransmissions") if options.ddsa else "no")
	print("Dap failure enabled = " + "yes" if options.failure else "no")
	print("Number of rounds = " + str(options.rounds) + "\n")
	for redundancy in range(options.init_redundancy, options.end_redundancy + 1):
		baseDir = "./simulations/redundancy_" + str(redundancy) + "/"
		baseDir = completeBaseDirPath(options, baseDir)
		print("####################" + " Redundancy = " + str(redundancy) + " #############################\n")
		execRounds(options, redundancy, baseDir)

options = parse()
runSimulation(options)
