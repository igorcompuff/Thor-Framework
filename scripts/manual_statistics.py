import collections, os, math

Statistics = collections.namedtuple("Statistics", "redundancy mean std_dev delta")
StatisticsMode = collections.namedtuple("StatisticsMode", "mode mean std_dev delta")

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


def saveStatistics(filePath, stat):
    file = open(filePath, 'a')
    file.write(str(stat.mode) + "\t" + str(stat.mean) + "\t" + str(stat.delta) + "\n")

    file.close()

def getModeStatistics(resultsDir, redundancy, mode):
	dataList = []
	modeDir = os.path.join(resultsDir, mode)
	redundancyDir = os.path.join(modeDir, str(redundancy))
	
	if not os.path.exists(redundancyDir):
		print("Redundancy " + str(redundancy) + " for mode " + mode + " do not exist.")
		
		return dataList

	for r in os.listdir(redundancyDir):
		roundDir = os.path.join(redundancyDir,r)
			
		for name in os.listdir(roundDir):			
			if not "statistics" in name:
				continue
			statisticsFile = os.path.join(roundDir,name)
			data = parse(statisticsFile)
			dataList.append(data)
			print(statisticsFile)
	return dataList

def statisticsForModeComparison(baseDir, redundancy):
	plotDir = os.path.join(baseDir,"plot")

	if not os.path.exists(plotDir):
        	os.makedirs(plotDir)

	deliveryFile = os.path.join(plotDir,"delivery_rate.dat")
	latencyFile = os.path.join(plotDir,"latency.dat")
	
	if os.path.exists(deliveryFile):
		os.remove(deliveryFile)

	if os.path.exists(latencyFile):
                os.remove(latencyFile)
	
	#ddsaList = getModeStatistics(baseDir, redundancy, "Ddsa")
	etxList = getModeStatistics(baseDir, redundancy, "Etx")
	#dumbList = getModeStatistics(baseDir, redundancy, "Ddsa_Dumb")

	#Statistics for delivery rate

	#mean = calculateMean(ddsaList, "d_rate")
	#stdDev = calculateStandardDev(ddsaList, mean, "d_rate")
	#delta = calculateDelta(ddsaList, stdDev)
	
	#stat = StatisticsMode(mode="Ddsa", mean=mean, std_dev=stdDev, delta=delta)

	#saveStatistics(deliveryFile, stat)

	mean = calculateMean(etxList, "d_rate")
        stdDev = calculateStandardDev(etxList, mean, "d_rate")
        delta = calculateDelta(etxList, stdDev)

        stat = StatisticsMode(mode="Etx", mean=mean, std_dev=stdDev, delta=delta)

        saveStatistics(deliveryFile, stat)

	#mean = calculateMean(dumbList, "d_rate")
        #stdDev = calculateStandardDev(dumbList, mean, "d_rate")
        #delta = calculateDelta(dumbList, stdDev)

        #stat = StatisticsMode(mode="Dumb_Ddsa", mean=mean, std_dev=stdDev, delta=delta)

        #saveStatistics(deliveryFile, stat)

	#Statistics for latency

	#mean = calculateMean(ddsaList, "l_avg")
        #stdDev = calculateStandardDev(ddsaList, mean, "l_avg")
        #delta = calculateDelta(ddsaList, stdDev)

        #stat = StatisticsMode(mode="Ddsa", mean=mean, std_dev=stdDev, delta=delta)

        #saveStatistics(latencyFile, stat)

        mean = calculateMean(etxList, "l_avg")
        stdDev = calculateStandardDev(etxList, mean, "l_avg")
        delta = calculateDelta(etxList, stdDev)

        stat = StatisticsMode(mode="Etx", mean=mean, std_dev=stdDev, delta=delta)

        saveStatistics(latencyFile, stat)

        #mean = calculateMean(dumbList, "l_avg")
        #stdDev = calculateStandardDev(dumbList, mean, "l_avg")
        #delta = calculateDelta(dumbList, stdDev)

        #stat = StatisticsMode(mode="Dumb_Ddsa", mean=mean, std_dev=stdDev, delta=delta)

        #saveStatistics(latencyFile, stat)

statisticsForModeComparison("/home/igor/github/ns-3.26/newsimulations/modesComparison/results", 3)
#statisticsForModeComparison("/home/igor/github/ns-3.26/newsimulations/modesComparison/results_full_failure", 3)
#statisticsForModeComparison("/home/igor/github/ns-3.26/newsimulations/modesComparison/results_malicious_failure", 3)
