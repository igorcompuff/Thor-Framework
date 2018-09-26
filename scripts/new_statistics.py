import collections, os, math
from optparse import OptionParser

Statistics = collections.namedtuple("Statistics", "before after total latency")
GenStatistics = collections.namedtuple("GenStatistics", "totalDelivery totalLatency collisions sentMessages")

class MeterStatistics:
    def __init__(self, deliveryBefore, deliveryAfetr, deliveryTotal, latency):
        self.deliveryBefore = deliveryBefore
        self.deliveryAfter = deliveryAfetr
        self.deliveryTotal = deliveryTotal
        self.latency = latency


class GeneralStatistics:
    def __init__(self, totalDelivery, totalLatency, collisions, sentMessages):
        self.totalDelivery = totalDelivery
        self.totalLatency = totalLatency
        self.collisions = collisions
        self.sentMessages = sentMessages


class StatisticsGenerator:

    def __init__(self, roundBaseDir, rounds):
        self.roundBaseDir = roundBaseDir
        self.rounds = rounds
        self.meterStats = {}
        self.generalStats = []


    def parseFile(self, filePath):
        f = open(filePath)
        lines = f.readlines()

        totalMeanLatency = 0.0
        totalDelivery = 0.0
        totalCollisions = 0
        totalSentMsg = 0

        for line in lines:
            if "Total Delivey Rate" in line:
                totalDelivery = float(line.split(':')[1].replace('%', ''))
            elif "Total Mean Latency" in line:
                totalMeanLatency = float(line.split(':')[1].replace(" ms", ''))
            elif "Total Collisions" in line:
                totalCollisions = int(line.split(':')[1])
            elif "Total sent messages" in line:
                totalSentMsg = int(line.split(':')[1])
            elif "Meter" in line:
                mid = int(line.split(' ')[1].replace('\n', ''))
            elif "Delivey Rate (Before Failure)" in  line:
                beforeDelivery = float(line.split(':')[1].replace('%', ''))
            elif "Delivey Rate (After Failure)" in  line:
                afterDelivery = float(line.split(':')[1].replace('%', ''))
            elif "Delivey Rate" in  line:
                totalDelivery = float(line.split(':')[1].replace('%', ''))
            elif "Mean Latency" in line:
                latency = float(line.split(':')[1].replace(" ms", ''))
            elif line == "\n":
                stat = MeterStatistics(beforeDelivery, afterDelivery, totalDelivery, latency)

                if not mid in self.meterStats:
                    self.meterStats[mid] = []

                self.meterStats[mid].append(stat)

        self.generalStats.append(GeneralStatistics(totalDelivery,
                                                   totalMeanLatency,
                                                   totalCollisions,
                                                   totalSentMsg))


    def collectData(self):
        for i in range(1, self.rounds + 1):
            filePath = self.roundBaseDir + "Round" + str(i) + "/stat.txt"
            self.parseFile(filePath)

    def calculateGeneralMean(self):
        totalDeliverySum = 0
        totalLatencySum = 0
        totalColissions = 0
        totalSentSum = 0

        totalSamples = len(self.generalStats)

        for genStat in self.generalStats:
            totalDeliverySum += genStat.totalDelivery
            totalLatencySum += genStat.totalLatency
            totalColissions += genStat.collisions
            totalSentSum += genStat.sentMessages


        mean = GeneralStatistics(totalDelivery=totalDeliverySum/totalSamples,
                                 totalLatency=totalLatencySum/totalSamples,
                                 collisions=totalColissions/totalSamples,
                                 sentMessages=totalSentSum/totalSamples)
        return mean

    def calculateMean(self):
        meanstats = {}
        for meterid in self.meterStats:
            numberOfProbes = len(self.meterStats[meterid])
            beforeSum = 0
            afterSum = 0
            totalSum = 0
            latencySum = 0
            for stat in self.meterStats[meterid]:
                beforeSum += stat.deliveryBefore
                afterSum += stat.deliveryAfter
                totalSum += stat.deliveryTotal
                latencySum += stat.latency

            mean = Statistics(before=beforeSum/numberOfProbes, after=afterSum/numberOfProbes, total=totalSum/numberOfProbes, latency=latencySum/numberOfProbes)

            meanstats[meterid] = mean

        return meanstats

    def calculateGeneralDelta(self, mean):
        totalSamples = len(self.generalStats)
        libertyDegree = totalSamples - 1

        totalDeliverySum = 0
        totalLatencySum = 0
        totalColisionsSum = 0
        totalSentSum = 0

        for genStat in self.generalStats:
            totalDeliverySum += math.pow((genStat.totalDelivery -
                                          mean.totalDelivery), 2)
            totalLatencySum += math.pow((genStat.totalLatency - mean.totalLatency), 2)
            totalColisionsSum += math.pow((genStat.collisions - mean.collisions), 2)
            totalSentSum += math.pow((genStat.sentMessages - mean.sentMessages),2)

        totalDeliveryStdDev = math.sqrt(totalDeliverySum / libertyDegree)
        totalLatencyStdDev = math.sqrt(totalLatencySum / libertyDegree)
        totalColisionsStdDev = math.sqrt(totalColisionsSum / libertyDegree)
        totalSentStdDev = math.sqrt(totalSentSum / libertyDegree)

        totalDelivryDelta = totalDeliveryStdDev / totalSamples
        totalLatencyDelta = totalLatencyStdDev / totalSamples
        totalColisionsDelta = totalColisionsStdDev / totalSamples
        totalSentDelta = totalSentStdDev / totalSamples

        delta = GeneralStatistics(totalDelivery=totalDelivryDelta,
                                  totalLatency=totalLatencyDelta,
                                  collisions=totalColisionsDelta,
                                  sentMessages=totalSentDelta)

        return delta
    
    def calculateDelta(self, meanstats):
        deltastats = {}
        for meterid in self.meterStats:
            mean = meanstats[meterid]
            numberOfProbes = len(self.meterStats[meterid])
            beforeSum = 0
            afterSum = 0
            totalSum = 0
            latencySum = 0

            libertyDegree = numberOfProbes - 1

            #Calsulate the summation of (x - mean)^2 for each sample x in dataList
            for stat in self.meterStats[meterid]:
                beforeSum += math.pow((stat.deliveryBefore - mean.before), 2)
                afterSum += math.pow((stat.deliveryAfter - mean.after), 2)
                totalSum += math.pow((stat.deliveryTotal - mean.total), 2)
                latencySum += math.pow((stat.latency - mean.latency), 2)

            beforeStdDev = math.sqrt(beforeSum / libertyDegree)
            afterStdDev = math.sqrt(afterSum / libertyDegree)
            totalStdDev = math.sqrt(totalSum / libertyDegree)
            latencyStdDev = math.sqrt(latencySum / libertyDegree)

            beforeDelta = beforeStdDev / numberOfProbes
            afterDelta = afterStdDev / numberOfProbes
            totalDelta = totalStdDev / numberOfProbes
            latencyDelta = latencyStdDev / numberOfProbes

            delta = Statistics(before=beforeDelta, after=afterDelta, total=totalDelta, latency=latencyDelta)
            deltastats[meterid] = delta


        return deltastats

def parseOptions():
    parser = OptionParser()
    parser.add_option("-d", "--dir", action="store", dest="roundBaseDir", help="Rounds Base directory", type=str)

    (options, args) = parser.parse_args()

    return options

options = parseOptions()

stat = StatisticsGenerator(options.roundBaseDir, 10)

stat.collectData()
meanstats = stat.calculateMean()
deltastats = stat.calculateDelta(meanstats)
generalMean = stat.calculateGeneralMean()
generalDelta = stat.calculateGeneralDelta(generalMean)
print("\t\t\t\tBefore\t\t\t\tAfter\t\t\t\tTotal\t\t\t\tLatency\n")

for meterid in meanstats:
    mean = meanstats[meterid]
    delta = deltastats[meterid]


    print("Meter " + str(meterid).zfill(2) + "\t\t" + str(round(mean.before)) +
          "\t\t" + str(round(delta.before)) + "\t\t" + str(round(mean.after)) +
          "\t\t" + str(round(delta.after)) + "\t\t" + str(round(mean.total)) +
          "\t\t" + str(round(delta.total)) + "\t\t" + str(round(mean.latency)) + "\t\t"
          + str(round(delta.latency)) + "\n")

print("Total delivery\t" + str(generalMean.totalDelivery) + "\t" +
      str(generalDelta.totalDelivery) + "\nTotal Collisions\t" +
      str(generalMean.collisions) + "\t" + str(generalDelta.collisions) +
      "\nTotal Latency\t" + str(generalMean.totalLatency) + "\t" +
      str(generalDelta.totalLatency) +
      "\nTotal Messages\t" + str(generalMean.sentMessages) + "\t" +
      str(generalDelta.sentMessages) + "\n")
