import collections, os, math

Statistics = collections.namedtuple("Statistics", "redundancy mean std_dev delta")

def parse(file):
	f = open(file, 'r')
    	lines = f.readlines()
    	values = {}
	dataValue = ""
    	for line in lines:
		if ':' in line:
            		dataName = line.split(':')[0]
            		dataValue = line.split(':')[1].split("//")[0]
            
			if "%" in line:
				values[dataName] = float(dataValue.replace('%', ""))
			elif " ms" in line:
				values[dataName] = float(dataValue.replace(" ms", ""))
			else:
	               		values[dataName] = int(dataValue)
	return values

def calculateDelta(dataList, std_dev):
    return std_dev / math.sqrt(len(dataList))

def calculateStandardDev(dataList, mean, field):
    soma = 0
    libertyDegree = len(dataList) - 1
    
    #Calsulate the summation of (x - mean)^2 for each sample x in dataList
    for data in dataList:
        soma += math.pow((data[field] - mean), 2)
    
    standardDev = math.sqrt(soma / libertyDegree)

    return standardDev

def calculateMean(dataList, field):
    soma = 0
    for data in dataList:
        soma+= data[field]

    return soma / len(dataList)


def saveStatisticsForRedundancyComp(filePath, stat):
    file = open(filePath, 'a')
    file.write(str(stat.redundancy) + "\t" + str(stat.mean) + "\t" + str(stat.delta) + "\n")
        
    file.close()

def statisticsForRedundancyComp(baseDir, redMin, redMax):
	for i in range(redMin, redMax + 1):
 		dataList = []
		for topology in os.listdir(baseDir):
			if topology == "plot":
				continue
			execDir = os.path.join(os.path.join(baseDir,topology),str(i))
			execDir = os.path.join(execDir,"Ddsa")
			#print(execDir)
			for r in os.listdir(execDir):
				roundDir = os.path.join(execDir,r)
				statisticsFile = os.path.join(roundDir,"statistics_ddsa.txt")
				if os.path.exists(statisticsFile):
					data = parse(statisticsFile)
            				dataList.append(data)
		
		mean = calculateMean(dataList, "d_rate")
		stdDev = calculateStandardDev(dataList, mean, "d_rate")
		delta = calculateDelta(dataList, stdDev)

		stat = Statistics(redundancy=i, mean=mean, std_dev=stdDev, delta=delta)

		plotDir = os.path.join(baseDir,"plot")
		#plotDir = os.path.join(plotDir, str(i)

		if not os.path.exists(plotDir):
			os.makedirs(plotDir)

		statFile = os.path.join(plotDir,"delivery_rate.dat")

		saveStatisticsForRedundancyComp(statFile, stat)
		
		mean = calculateMean(dataList, "l_avg")
                stdDev = calculateStandardDev(dataList, mean, "l_avg")
                delta = calculateDelta(dataList, stdDev)

		stat = Statistics(redundancy=i, mean=mean, std_dev=stdDev, delta=delta)
		
		statFile = os.path.join(plotDir,"latency.dat")

                saveStatisticsForRedundancyComp(statFile, stat)
		

	#dir = baseDir + "redundancy_" + str(i)
        #dataList = []
        #dirfilter = ["nonddsa", "nofailure", "pcap"]
        #extractData(dir, dataList, dirfilter)
        #mean = calculateMean(dataList, "d_rate")
        #std_dev = calculateStandardDev(dataList, mean, "d_rate")
        #delta = calculateDelta(dataList, std_dev)

        #stat = Statistics(mean=mean, std_dev=std_dev, delta=delta)

        #saveStatisticsForRedundancyComp("/home/igor/Documents/gnuplot.data", stat, i )   

        #print("################# Redundancy = " + str(i) + "#########################")
        #print("Mean: " + str(stat.mean))
        #print("StdDev: " + str(std_dev))
        #print("Delta: " + str(delta))
        #print("\n")

statisticsForRedundancyComp("/home/igor/github/ns-3.26/redundancytest/results", 1, 10)
