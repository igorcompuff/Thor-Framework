import os.path
import shutil
from optparse import OptionParser
from subprocess import call

parser = OptionParser()
parser.add_option("-d", "--ddsa", action="store_true", dest="ddsa", default=False, help="Use ddsa in the simulations")
parser.add_option("-f", "--failure", action="store_true", dest="failure", default=False, help="Simulate with a DAP failure")
parser.add_option("-r", "--retrans", action="store", dest="retrans", help="Number of retransmissions", type=int, default=0)
parser.add_option("-n", "--number", action="store", dest="rounds", help="Number of simulation rounds", type=int, default=1)

(options, args) = parser.parse_args()

baseDir = "./simulations/"

if options.ddsa:
	baseDir = baseDir + "ddsa/retrans/" + str(options.retrans) + "/"
	ddsaOption = "1"
else:
	baseDir += "nonddsa/"
	ddsaOption = "0"

if options.failure:
	baseDir += "failure/"
	failureOption = "1"
else:
	baseDir += "nofailure/"
	failureOption = "0"
for i in range(1, options.rounds + 1):
	call(["./execsim.sh", str(i), str(options.retrans), failureOption, ddsaOption])
	execDir = baseDir + str(i)
	if 	os.path.exists(execDir):
		shutil.rmtree(execDir)
	os.makedirs(execDir)
	pcapDir = execDir + "/pcap"
	os.makedirs(pcapDir)
	for f in os.listdir("./"):
		if f.endswith('.txt'):
			shutil.move(os.path.join("./", f), os.path.join(execDir,f))
		elif f.endswith('.pcap'):
			shutil.move(os.path.join("./", f), os.path.join(pcapDir,f))






