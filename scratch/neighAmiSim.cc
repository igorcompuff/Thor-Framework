
#include "ns3/core-module.h"
#include "ns3/etx-ff.h"
#include "ns3/lq-olsr-header.h"
#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/ddsa-helper.h"
#include "ns3/lq-olsr-helper.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/csma-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/ddsa-internet-stack-helper.h"
#include "ns3/integer.h"
#include "ns3/simple-header.h"
#include "ns3/malicious-dap-routing-protocol.h"
#include "ns3/original-ddsa-routing-protocol.h"
#include "ns3/dumb-ddsa-routing-protocol.h"
#include "ns3/dynamic-retrans-ddsa-routing-protocol.h"
#include "ns3/failing-ipv4-l3-protocol.h"
#include "ns3/global-retrans-ddsa-ipv4-l3-protocol.h"
#include "ns3/individual-retrans-ddsa-ipv4-l3-protocol.h"
#include "ns3/flooding-ddsa-routing-protocol.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstddef>
#include <numeric>
#include <string>
#include <math.h>

using namespace ns3;
using namespace ns3::lqmetric;
using namespace ns3::lqolsr;
using namespace ns3::ddsa;
using namespace ns3::ami;

NS_LOG_COMPONENT_DEFINE ("NeighborhoodAmiSim");


struct LatencyInformation
{
  public:
    Time time_sent;
    Time time_received;

    LatencyInformation()
    {
      time_sent = Time::Max();
      time_received = Time::Max();;
    }

    Time GetLatency()
    {
    	return time_received - time_sent;
    }
};

struct AmiPacketInformation
{
  private:
    uint16_t seqNumber;
    int nReceived;
    int nSent;
    LatencyInformation latency;
    bool received;
  public:

    AmiPacketInformation()
    {
      seqNumber = 0;
      nReceived = 0;
      nSent = 0;
      received = false;
      //latency = LatencyInfo();
    }

    uint16_t GetSeqNumber()
    {
      return seqNumber;
    }

    void SetSeqNumber(uint16_t sNumber)
    {
      seqNumber = sNumber;
    }

    void IncrementReceived()
    {
      nReceived++;
    }

    int GetNReceived()
    {
      return nReceived;
    }

    void SetLatency(LatencyInformation latency)
    {
      this->latency = latency;
    }

    LatencyInformation GetLatencyInfo()
    {
      return latency;
    }

    void Receive()
    {
      this->received = true;
      latency.time_received = Simulator::Now();
      nReceived = 1;
    }

    void Send()
    {
      latency.time_sent = Simulator::Now();
      nSent++;
    }

    bool Received()
    {
      return received;
    }

    int GetNSent()
    {
      return nSent;
    }
};


 /*NeighborhoodAmiSim.cpp
 *
 *  Created on: 13 de jun de 2017
 * Author: igor
 */


class NeighborhoodAmiSim
{
	public:

		enum ProtocolMode
		{
			ETX = 0, ORIGINAL_DDSA = 1, DUMB_DDSA = 2, DYNAMIC_RETRANS_DDSA = 3, FLOODING = 4
		};

		NeighborhoodAmiSim ();
		virtual ~NeighborhoodAmiSim ();

		void ConfigureSimulation(int argc, char *argv[]);


		//Simulation Results
		void GenerateSimulationPacketInfo();
		void GenerateSimulationStatistcs();
		double GetMeterRecoveryTime(uint16_t meterId);
		double GetMeterMeanLatency(uint16_t meterId);
		void GetMeterDeliveryRate(uint16_t meterId, double * beforeFailure, double * aftarFailure, double * total);
		double GetMeterTotalReceivedPackets(uint16_t meterId);
		uint64_t GetMeterTotalLatency(uint16_t meterId);
		uint32_t GetTotalUnnecessaryCopies(uint16_t meterId);
		uint32_t GetTotalSentMessages(uint16_t meterId);

		//Tests

		void TestStack();
		bool TestNonDdsaMeterStack();
		bool TestOriginalDdsaMeterStack();
		bool TestDumbDdsaMeterStack();
		bool TestDynamicDdsaMeterStack();
		bool TestFloodingDdsaMeterStack();
		bool TestDapsStack();

		const double BEST_LINK = 112;//50.0;
		const double GOOD_LINK = 112;//90.0;
		const double BAD_LINK = 114.02;
		const double NO_LINK = 200.0;

	private:

		//Utility methods
		void Parse(int argc, char *argv[]);
		void UpdateRoutingProtocolWithControllerAddress();
		void UpdateRoutingProtocolWithControllerAddress(Ptr<Node> node, Ipv4Address controllerAddress);
		void SendPacketToController(Ptr<Packet> packet, Ptr<Node> node);
		void ParseSenders();
		Ptr<Node> GetNodeById(uint32_t id, NodeContainer nodes);
		void SheduleFailure(double time);
		Ptr<Node> GetRandomFailingNode();
		void ConnectDdsaMetersSentPacketTrace();
		void ConnectEtxMetersSentPacketTrace();
		Ptr<Ipv4RoutingProtocol> GetNodeEffectiveRoutingProtocol(Ptr<Node> node);
		int GetSimulationRound();
		std::string GetSimulationDir();
		Ptr<Node> GetNodeFromIpAddress(NetDeviceContainer devices, const Ipv4Address & address);

		//Simulation Configuration

		void ConfigureLogComponents();
		void ConfigureWifi();
		void CreateControllerLan();
		void ConfigureIpAddressing();
		void ConfigureMeterApplication(uint16_t port, Time start, Time stop);
		void ConfigureControllerSocket(uint16_t port);
		void ConfigureNodesStack();
		void ConfigureDapSocket(uint16_t port);
		void CreateNodes();
		void InstallInternetStack(DDsaHelper & lqOlsrDapHelper, NodeContainer nodes, std::string ipv4l3ProtocolTid);
		void ConfigureDapStack();
		void ConfigureMeterStack();
		void InstallFloodingMeterStack();
		void InstallOriginalMeterStack();
		void InstallDumbMeterStack();
		void InstallNonDdsaMeterStack();
		void InstallDynamicRetransDdsa();
		Ipv4ListRoutingHelper ConfigureRouting(const Ipv4RoutingHelper & ipv4Routing);
		void ConfigureControllerStack();
		void ConfigureDapsHna();
		void PopulateArpCache();
		Ptr<ArpCache> CreatePopulatedArpCache(NodeContainer nodes);

		//Topology Configuration

		void CreateTopology();
		void AddNeighborhood(Ptr<ListPositionAllocator> meterPositionAlloc, int startX, int startY);
		void ConfigureIntraNeighborhoodLinks(NodeContainer neighborhood);
		void ConfigureNeighborhoodToDapLinks(NodeContainer neighborhood, int neighborhoodIndex);
		void ConfigureHorizontalInterNeighborhoodLinks(NodeContainer neighborhoodSrc, NodeContainer neighborhoodDest);
		void ConfigureVerticalInterNeighborhoodLinks(NodeContainer neighborhoodSrc, NodeContainer neighborhoodDest);

		//Callback methods

		void DdsaApplicationPacketSent(Ptr<const Packet>, const Ipv4Address & dapAddress);
		void EtxAplicationPacketSent(const Ipv4Header & header, Ptr<const Packet> packet, uint32_t intId);
		void ControllerReceivePacket (Ptr<Socket> socket);
		void DapReceivedPacket (Ptr<Socket> socket);
		void ExecuteFailure();
		void Collision();

		//Instance variables
		std::string phyMode;
		bool withFailure;
		int failureMode;
		int failingDapId;
		CommandLine cmd;
		NodeContainer daps;
		NodeContainer meters;
		NodeContainer controllers;
		std::string senderIndexes;
		std::vector<uint32_t> senderIndexList;
		Ptr<MatrixPropagationLossModel> myLossModel;
		YansWifiPhyHelper myWifiPhy;
		Ipv4InterfaceContainer olsrIpv4Devices;
		Ipv4InterfaceContainer csmaIpv4Devices;
		EventGarbageCollector m_events;
		NetDeviceContainer olsrDevices;
		NetDeviceContainer csmaDevices;

		int ddsaMode;
		int totalDaps;
		int totalMeters;
		int totalControllers;
		double alpha;
		std::map<uint16_t, std::map<uint16_t, AmiPacketInformation> > packetsSent;
		int ddsa_retrans;
		uint64_t collisionCount;
		Time failureTime;
		Ptr<UniformRandomVariable> m_rnd;
};

NeighborhoodAmiSim::NeighborhoodAmiSim (): phyMode ("DsssRate1Mbps")
{
	totalDaps = 6;
	totalMeters = 36;
	totalControllers = 1;
    myWifiPhy = YansWifiPhyHelper::Default();
    withFailure = false;
    ddsa_retrans = 0;
    m_rnd = CreateObject<UniformRandomVariable>();
    failureMode = 0; //Full failure
    myLossModel = CreateObject<MatrixPropagationLossModel> ();
    failingDapId = -1;
    ddsaMode = 0; //Disabled
    alpha = 0.0;
    collisionCount = 0;
}

NeighborhoodAmiSim::~NeighborhoodAmiSim ()
{
  // TODO Auto-generated destructor stub
}

void
NeighborhoodAmiSim::Parse(int argc, char *argv[])
{
  cmd.AddValue ("failure", "Cause the failure of one DAP", withFailure);
  cmd.AddValue ("senders", "Comma separated list of sender indexes.", senderIndexes);
  cmd.AddValue ("retrans", "Number of retransmissions", ddsa_retrans);
  cmd.AddValue ("fmode", "Failure mode", failureMode);
  cmd.AddValue ("dap", "Failing DAP id", failingDapId);
  cmd.AddValue ("ddsamode", "DDSA mode of operation", ddsaMode);
  cmd.AddValue ("alpha", "DDSA alpha", alpha);

  cmd.Parse (argc, argv);
}

//Simulation Configuration

void
NeighborhoodAmiSim::ParseSenders()
{
	if (senderIndexes == "*")
	{
		for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
		{
			senderIndexList.push_back((*it)->GetId());
		}
	}
	else
	{
		if (senderIndexes == "none")
		{
			return;
		}

		std::string indexStr = "";
		for(std::string::size_type i = 0; i < senderIndexes.size(); ++i)
		{
			if (senderIndexes[i] == ',')
			{
				senderIndexList.push_back(std::stoi(indexStr));
				indexStr = "";
			}
			else
			{
				indexStr+= senderIndexes[i];
			}
		}

		if (!indexStr.empty())
		{
			senderIndexList.push_back(std::stoi(indexStr));
		}
	}
}

void
NeighborhoodAmiSim::ConnectDdsaMetersSentPacketTrace()
{
	for(NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
	{
		Ptr<Node> meter = *it;
		Ptr<DdsaIpv4L3ProtocolBase> leprot = meter->GetObject<DdsaIpv4L3ProtocolBase>();
		NS_ASSERT(leprot);
		leprot->TraceConnectWithoutContext("L3tx", MakeCallback(&NeighborhoodAmiSim::DdsaApplicationPacketSent, this));
	}
}

void
NeighborhoodAmiSim::ConnectEtxMetersSentPacketTrace()
{
	for(NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
	{
		Ptr<Node> meter = *it;
		Ptr<Ipv4L3Protocol> leprot = meter->GetObject<Ipv4L3Protocol>();
		NS_ASSERT(leprot);
		leprot->TraceConnectWithoutContext("SendOutgoing", MakeCallback(&NeighborhoodAmiSim::EtxAplicationPacketSent, this));
	}
}

Ptr<Ipv4RoutingProtocol>
NeighborhoodAmiSim::GetNodeEffectiveRoutingProtocol(Ptr<Node> node)
{
	Ptr<Ipv4> stack = node->GetObject<Ipv4> ();

	Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (stack->GetRoutingProtocol ());
	Ptr<Ipv4RoutingProtocol> rp;

	for (uint32_t i = 0; i < lrp->GetNRoutingProtocols(); i++)
	{
		int16_t priority;

		rp = lrp->GetRoutingProtocol(i, priority);

		if (priority > 0)
		{
			return rp;
		}
	}

	NS_ASSERT_MSG(true, "The specified node has no routing protocol other than StaticRoutingProtocol");

	return NULL;
}

char *
CopyString(char * cstr)
{
	int totalChars = 0;
	char * index = cstr;

	while(*index != '\0')
	{
		index++;
		totalChars++;
	}

	char * resultChar = (char *) malloc((totalChars + 1) * sizeof(char));
	char * resultIndex = resultChar;
	index = cstr;

	for (int i = 1; i <= (totalChars + 1); i++)
	{
		*resultIndex = *index;

		resultIndex++;
		index++;
	}

	return resultChar;
}

Ptr<Node>
NeighborhoodAmiSim::GetNodeFromIpAddress(NetDeviceContainer devices, const Ipv4Address & address)
{
	for (uint32_t i = 0; i < devices.GetN (); ++i)
	{
		Ptr<NetDevice> device = devices.Get (i);
		Ptr<Ipv4> ipv4 = device->GetNode()->GetObject<Ipv4> ();
		int32_t interface = ipv4->GetInterfaceForDevice(device);
		Ipv4Address deviceAddress = ipv4->GetAddress(interface, 0).GetLocal();

		if (deviceAddress == address)
		{
			return device->GetNode();
		}
	}

	return NULL;
}

int
NeighborhoodAmiSim::GetSimulationRound()
{
	char * strVar = CopyString(std::getenv("NS_GLOBAL_VALUE"));
	char * strtk = strtok (strVar,"=");
	strtk = strtok (NULL,"=");
	int simRound = std::stoi(strtk, NULL);
	free(strVar);

	return simRound;
}

void
NeighborhoodAmiSim::ConfigureSimulation(int argc, char *argv[])
{
	totalDaps = 6;
	totalMeters = 36;
	totalControllers = 1;

	Parse(argc, argv);
	CreateNodes();
	ParseSenders();
	CreateTopology();
	ConfigureWifi();
	CreateControllerLan();
	ConfigureNodesStack();
	ConfigureIpAddressing();
	PopulateArpCache();

	uint16_t applicationPort = 80;
	Time applicationStart = Seconds(100.0);
	Time applicationStop = Seconds(490.0);

	ConfigureMeterApplication(applicationPort, applicationStart, applicationStop);
	ConfigureControllerSocket(applicationPort);
	ConfigureDapSocket(applicationPort);
	SheduleFailure(150);
	ConfigureLogComponents();

}

void
NeighborhoodAmiSim::ConfigureLogComponents()
{
	//LogComponentEnable("FailingIpv4L3Protocol", LOG_LEVEL_DEBUG);
	//LogComponentEnable("GlobalRetransDdsaIpv4L3Protocol", LOG_LEVEL_DEBUG);
	//LogComponentEnable("Etx", LOG_LEVEL_DEBUG);
	//LogComponentEnable("MaliciousEtx", LOG_LEVEL_DEBUG);
	//LogComponentEnable("YansWifiPhy", LOG_LEVEL_DEBUG);
	//LogComponentEnable("Ipv4L3ProtocolDdsaAdapter", LOG_LEVEL_DEBUG);
	//LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_ALL);
	//LogComponentEnable("Ipv4L3ProtocolSmartDdsaAdapter", LOG_LEVEL_ALL);
}

void
NeighborhoodAmiSim::CreateNodes()
{
  daps.Create(totalDaps); // Creates the specified number of daps
  meters.Create(totalMeters); //Creates the Specified NUmber of meters
  controllers.Create(totalControllers); // Creates one controller.
}

void
NeighborhoodAmiSim::AddNeighborhood(Ptr<ListPositionAllocator> meterPositionAlloc, int startX, int startY)
{
	for (int i = 0; i < 3; i++)
	{
		int y = startY + i * 10;

		for (int j = 0; j < 3; j++)
		{
			int x = startX + j * 10;

			meterPositionAlloc->Add (Vector (x, y, 0.0));
		}
	}
}

void
NeighborhoodAmiSim::ConfigureIntraNeighborhoodLinks(NodeContainer neighborhood)
{
	myLossModel->SetLoss (neighborhood.Get (0)->GetObject<MobilityModel>(), neighborhood.Get (1)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (0)->GetObject<MobilityModel>(), neighborhood.Get (3)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (0)->GetObject<MobilityModel>(), neighborhood.Get (4)->GetObject<MobilityModel>(), BEST_LINK);

	myLossModel->SetLoss (neighborhood.Get (1)->GetObject<MobilityModel>(), neighborhood.Get (2)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (1)->GetObject<MobilityModel>(), neighborhood.Get (3)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (1)->GetObject<MobilityModel>(), neighborhood.Get (4)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (1)->GetObject<MobilityModel>(), neighborhood.Get (5)->GetObject<MobilityModel>(), BEST_LINK);

	myLossModel->SetLoss (neighborhood.Get (2)->GetObject<MobilityModel>(), neighborhood.Get (4)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (2)->GetObject<MobilityModel>(), neighborhood.Get (5)->GetObject<MobilityModel>(), BEST_LINK);


	myLossModel->SetLoss (neighborhood.Get (3)->GetObject<MobilityModel>(), neighborhood.Get (4)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (3)->GetObject<MobilityModel>(), neighborhood.Get (6)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (3)->GetObject<MobilityModel>(), neighborhood.Get (7)->GetObject<MobilityModel>(), BEST_LINK);

	myLossModel->SetLoss (neighborhood.Get (4)->GetObject<MobilityModel>(), neighborhood.Get (5)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (4)->GetObject<MobilityModel>(), neighborhood.Get (6)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (4)->GetObject<MobilityModel>(), neighborhood.Get (7)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (4)->GetObject<MobilityModel>(), neighborhood.Get (8)->GetObject<MobilityModel>(), BEST_LINK);

	myLossModel->SetLoss (neighborhood.Get (5)->GetObject<MobilityModel>(), neighborhood.Get (6)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (5)->GetObject<MobilityModel>(), neighborhood.Get (7)->GetObject<MobilityModel>(), BEST_LINK);
	myLossModel->SetLoss (neighborhood.Get (5)->GetObject<MobilityModel>(), neighborhood.Get (8)->GetObject<MobilityModel>(), BEST_LINK);

	myLossModel->SetLoss (neighborhood.Get (6)->GetObject<MobilityModel>(), neighborhood.Get (7)->GetObject<MobilityModel>(), BEST_LINK);

	myLossModel->SetLoss (neighborhood.Get (7)->GetObject<MobilityModel>(), neighborhood.Get (8)->GetObject<MobilityModel>(), BEST_LINK);
}

void
NeighborhoodAmiSim::ConfigureHorizontalInterNeighborhoodLinks(NodeContainer neighborhoodSrc, NodeContainer neighborhoodDest)
{
	myLossModel->SetLoss (neighborhoodSrc.Get (2)->GetObject<MobilityModel>(), neighborhoodDest.Get (0)->GetObject<MobilityModel>(), BAD_LINK);
	myLossModel->SetLoss (neighborhoodSrc.Get (2)->GetObject<MobilityModel>(), neighborhoodDest.Get (3)->GetObject<MobilityModel>(), BAD_LINK);

	myLossModel->SetLoss (neighborhoodSrc.Get (5)->GetObject<MobilityModel>(), neighborhoodDest.Get (0)->GetObject<MobilityModel>(), BAD_LINK);
	myLossModel->SetLoss (neighborhoodSrc.Get (5)->GetObject<MobilityModel>(), neighborhoodDest.Get (3)->GetObject<MobilityModel>(), BAD_LINK);
	myLossModel->SetLoss (neighborhoodSrc.Get (5)->GetObject<MobilityModel>(), neighborhoodDest.Get (6)->GetObject<MobilityModel>(), BAD_LINK);

	myLossModel->SetLoss (neighborhoodSrc.Get (8)->GetObject<MobilityModel>(), neighborhoodDest.Get (3)->GetObject<MobilityModel>(), BAD_LINK);
	myLossModel->SetLoss (neighborhoodSrc.Get (8)->GetObject<MobilityModel>(), neighborhoodDest.Get (6)->GetObject<MobilityModel>(), BAD_LINK);
}

void
NeighborhoodAmiSim::ConfigureVerticalInterNeighborhoodLinks(NodeContainer neighborhoodSrc, NodeContainer neighborhoodDest)
{
	myLossModel->SetLoss (neighborhoodSrc.Get (6)->GetObject<MobilityModel>(), neighborhoodDest.Get (0)->GetObject<MobilityModel>(), BAD_LINK);
	myLossModel->SetLoss (neighborhoodSrc.Get (6)->GetObject<MobilityModel>(), neighborhoodDest.Get (1)->GetObject<MobilityModel>(), BAD_LINK);

	myLossModel->SetLoss (neighborhoodSrc.Get (7)->GetObject<MobilityModel>(), neighborhoodDest.Get (0)->GetObject<MobilityModel>(), BAD_LINK);
	myLossModel->SetLoss (neighborhoodSrc.Get (7)->GetObject<MobilityModel>(), neighborhoodDest.Get (1)->GetObject<MobilityModel>(), BAD_LINK);
	myLossModel->SetLoss (neighborhoodSrc.Get (7)->GetObject<MobilityModel>(), neighborhoodDest.Get (2)->GetObject<MobilityModel>(), BAD_LINK);

	myLossModel->SetLoss (neighborhoodSrc.Get (8)->GetObject<MobilityModel>(), neighborhoodDest.Get (1)->GetObject<MobilityModel>(), BAD_LINK);
	myLossModel->SetLoss (neighborhoodSrc.Get (8)->GetObject<MobilityModel>(), neighborhoodDest.Get (2)->GetObject<MobilityModel>(), BAD_LINK);
}

void
NeighborhoodAmiSim::ConfigureNeighborhoodToDapLinks(NodeContainer neighborhood, int neighborhoodIndex )
{
	myLossModel->SetLoss (neighborhood.Get (0)->GetObject<MobilityModel>(), daps.Get (neighborhoodIndex)->GetObject<MobilityModel>(), GOOD_LINK);
	myLossModel->SetLoss (neighborhood.Get (1)->GetObject<MobilityModel>(), daps.Get (neighborhoodIndex)->GetObject<MobilityModel>(), GOOD_LINK);
	myLossModel->SetLoss (neighborhood.Get (2)->GetObject<MobilityModel>(), daps.Get (neighborhoodIndex)->GetObject<MobilityModel>(), GOOD_LINK);
	myLossModel->SetLoss (neighborhood.Get (6)->GetObject<MobilityModel>(), daps.Get (neighborhoodIndex + 2)->GetObject<MobilityModel>(), GOOD_LINK);
	myLossModel->SetLoss (neighborhood.Get (7)->GetObject<MobilityModel>(), daps.Get (neighborhoodIndex + 2)->GetObject<MobilityModel>(), GOOD_LINK);
	myLossModel->SetLoss (neighborhood.Get (8)->GetObject<MobilityModel>(), daps.Get (neighborhoodIndex + 2)->GetObject<MobilityModel>(), GOOD_LINK);
}

void
NeighborhoodAmiSim::CreateTopology()
{
	//Creating neighborhoods

	NodeContainer neighborhood1;
	NodeContainer neighborhood2;
	NodeContainer neighborhood3;
	NodeContainer neighborhood4;

	for (uint32_t i = 0; i < meters.GetN(); i++)
	{

		if (i < 9)
		{
			neighborhood1.Add(meters.Get(i));
		}
		else if (i < 18)
		{
			neighborhood2.Add(meters.Get(i));
		}
		else if (i < 27)
		{
			neighborhood3.Add(meters.Get(i));
		}
		else
		{
			neighborhood4.Add(meters.Get(i));
		}
	}

	//Defining the positions

	Ptr<ListPositionAllocator> dapPositionAlloc = CreateObject<ListPositionAllocator> ();
	Ptr<ListPositionAllocator> meterNeigh1PositionAlloc = CreateObject<ListPositionAllocator> ();
	Ptr<ListPositionAllocator> meterNeigh2PositionAlloc = CreateObject<ListPositionAllocator> ();
	Ptr<ListPositionAllocator> meterNeigh3PositionAlloc = CreateObject<ListPositionAllocator> ();
	Ptr<ListPositionAllocator> meterNeigh4PositionAlloc = CreateObject<ListPositionAllocator> ();

	Ptr<ListPositionAllocator> controllerPositionAlloc = CreateObject<ListPositionAllocator> ();

	controllerPositionAlloc->Add (Vector (-30, -30, 0.0));

	dapPositionAlloc->Add (Vector (10, 0, 0.0));
	dapPositionAlloc->Add (Vector (50, 0, 0.0));
	dapPositionAlloc->Add (Vector (10, 40, 0.0));
	dapPositionAlloc->Add (Vector (50, 40, 0.0));
	dapPositionAlloc->Add (Vector (10, 80, 0.0));
	dapPositionAlloc->Add (Vector (50, 80, 0.0));

	AddNeighborhood(meterNeigh1PositionAlloc, 0, 10);
	AddNeighborhood(meterNeigh2PositionAlloc, 40, 10);
	AddNeighborhood(meterNeigh3PositionAlloc, 0, 50);
	AddNeighborhood(meterNeigh4PositionAlloc, 40, 50);

	//Installing MobilityModel

	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

	mobility.SetPositionAllocator (meterNeigh1PositionAlloc);
	mobility.Install (neighborhood1);
	mobility.SetPositionAllocator (meterNeigh2PositionAlloc);
	mobility.Install (neighborhood2);
	mobility.SetPositionAllocator (meterNeigh3PositionAlloc);
	mobility.Install (neighborhood3);
	mobility.SetPositionAllocator (meterNeigh4PositionAlloc);
	mobility.Install (neighborhood4);

	mobility.SetPositionAllocator (dapPositionAlloc);
	mobility.Install (daps);

	mobility.SetPositionAllocator (controllerPositionAlloc);
	mobility.Install (controllers);

	//Installing propagation loss

	myLossModel->SetDefaultLoss (NO_LINK);

	//Configure intra neighborhood links
	ConfigureIntraNeighborhoodLinks(neighborhood1);
	ConfigureIntraNeighborhoodLinks(neighborhood2);
	ConfigureIntraNeighborhoodLinks(neighborhood3);
	ConfigureIntraNeighborhoodLinks(neighborhood4);

	//Configure inter-neighborhood links

	//Configure Horizontal Neighborhoods
	ConfigureHorizontalInterNeighborhoodLinks(neighborhood1, neighborhood2);
	ConfigureHorizontalInterNeighborhoodLinks(neighborhood3, neighborhood4);

	//Configure Vertical Neighborhoods

	ConfigureVerticalInterNeighborhoodLinks(neighborhood1, neighborhood3);
	ConfigureVerticalInterNeighborhoodLinks(neighborhood2, neighborhood4);

	//Configure Diagonal Neighborhoods

	myLossModel->SetLoss (neighborhood1.Get (8)->GetObject<MobilityModel>(), neighborhood4.Get (0)->GetObject<MobilityModel>(), BAD_LINK);
	myLossModel->SetLoss (neighborhood2.Get (6)->GetObject<MobilityModel>(), neighborhood3.Get (2)->GetObject<MobilityModel>(), BAD_LINK);


	//Configuring links to DAPs

	ConfigureNeighborhoodToDapLinks(neighborhood1, 0);
	ConfigureNeighborhoodToDapLinks(neighborhood2, 1);
	ConfigureNeighborhoodToDapLinks(neighborhood3, 2);
	ConfigureNeighborhoodToDapLinks(neighborhood4, 3);
}

void
NeighborhoodAmiSim::ConfigureWifi()
{
	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
	// Fix non-unicast data rate to be the same as that of unicast
	Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

	Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
	wifiChannel->SetPropagationLossModel (myLossModel);
	wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());

	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue (phyMode), "ControlMode",StringValue (phyMode));

	//wifi.EnableLogComponents ();  // Turn on all Wifi logging

	// This is one parameter that matters when using FixedRssLossModel
	// set it to zero; otherwise, gain will be added
	//wifiPhy.Set ("RxGain", DoubleValue (0) );
	myWifiPhy.Set ("RxNoiseFigure", DoubleValue (10) );
	myWifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
	myWifiPhy.SetChannel (wifiChannel);

	WifiMacHelper wifiMac; // Add a mac and disable rate control
	wifiMac.SetType ("ns3::AdhocWifiMac"); // Set it to adhoc mode

	NodeContainer allOlsrNodes;
	allOlsrNodes.Add(meters);
	allOlsrNodes.Add(daps);
	olsrDevices = wifi.Install (myWifiPhy, wifiMac, allOlsrNodes);
	myWifiPhy.EnablePcap ("olsr-hna", olsrDevices, false);

	for(NetDeviceContainer::Iterator it = olsrDevices.Begin(); it != olsrDevices.End(); it++)
	{
		Ptr<NetDevice> device = *it;
		Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice>(device);
		if (wifiDevice)
		{
			Ptr<WifiMac> wifiMac = wifiDevice->GetMac();
			Ptr<RegularWifiMac> regWifiMac = DynamicCast<RegularWifiMac>(wifiMac);

			if (regWifiMac)
			{
				regWifiMac->GetDcaTxop()->TraceConnectWithoutContext("Collision", MakeCallback(&NeighborhoodAmiSim::Collision, this));
				NS_ASSERT(regWifiMac->GetDcaTxop());
			}
		}

	}

}

void
NeighborhoodAmiSim::CreateControllerLan()
{
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  csmaDevices = csma.Install (NodeContainer (controllers, daps));
  csma.EnablePcap ("csma", csmaDevices, false);
}

Ipv4ListRoutingHelper
NeighborhoodAmiSim::ConfigureRouting(const Ipv4RoutingHelper & ipv4Routing)
{
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (ipv4Routing, 10);

  return list;
}

void
NeighborhoodAmiSim::PopulateArpCache()
{
	NodeContainer nodes;
	nodes.Add(meters);
	nodes.Add(daps);
	nodes.Add(controllers);

	Ptr<ArpCache> arpcache = CreatePopulatedArpCache(nodes);

	for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it)
	{
		Ptr<Ipv4L3Protocol> ip = (*it)->GetObject<Ipv4L3Protocol> ();

		NS_ASSERT(ip);

		for (uint32_t interface = 0; interface < ip->GetNInterfaces(); interface++)
		{
				Ptr<Ipv4Interface> ipIface = ip->GetInterface(interface);
				ipIface->SetArpCache(arpcache);
		}
	}
}

Ptr<ArpCache>
NeighborhoodAmiSim::CreatePopulatedArpCache(NodeContainer nodes)
{
	Ptr<ArpCache> arpCache = CreateObject<ArpCache> ();

	for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it)
	{
		Ptr<Ipv4L3Protocol> ip = (*it)->GetObject<Ipv4L3Protocol> ();

		NS_ASSERT(ip);

		for (uint32_t interface = 0; interface < ip->GetNInterfaces(); interface++)
		{
			Ptr<Ipv4Interface> ipIface = ip->GetInterface(interface);
			NS_ASSERT(ipIface != 0);
			Ptr<NetDevice> device = ipIface->GetDevice();
			NS_ASSERT(device != 0);
			Mac48Address macaddr = Mac48Address::ConvertFrom(device->GetAddress ());

			for(uint32_t i = 0; i < ipIface->GetNAddresses (); i ++)
			{
				Ipv4Address ipAddr = ipIface->GetAddress (i).GetLocal();

				if(ipAddr != Ipv4Address::GetLoopback())
				{
						ArpCache::Entry * entry = arpCache->Add(ipAddr);
						entry->SetMacAddresss(macaddr);
						entry->MarkPermanent();
				}
			}
		}
	}

	return arpCache;
}


void
NeighborhoodAmiSim::InstallInternetStack(DDsaHelper & lqOlsrDapHelper, NodeContainer nodes, std::string ipv4l3ProtocolTid)
{
  Ipv4ListRoutingHelper ipv4Routing = ConfigureRouting(lqOlsrDapHelper);

  DdsaInternetStackHelper ddsa;
  ddsa.SetRoutingHelper (ipv4Routing); // has effect on the next Install ()
  ddsa.SetIpv4L3ProtocolTypeId(ipv4l3ProtocolTid);
  ddsa.Install (nodes);
}

void
NeighborhoodAmiSim::ConfigureControllerStack()
{
  InternetStackHelper internet;

  internet.Install (controllers);
}

void
NeighborhoodAmiSim::ConfigureDapStack()
{
	TypeId metricTid = Etx::GetTypeId();
	TypeId routingProtocolTid = MaliciousDapRoutingProtocol::GetTypeId();

	DDsaHelper helper(metricTid, routingProtocolTid);

	for (int i = 0; i < totalDaps; i++)
	{
		helper.ExcludeInterface (daps.Get (i), 2);
	}

	InstallInternetStack(helper, daps, "ns3::ddsa::FailingIpv4L3Protocol");
}

void
NeighborhoodAmiSim::InstallFloodingMeterStack()
{
	DDsaHelper helper(Etx::GetTypeId(), FloodingDdsaRoutingProtocol::GetTypeId());
	InstallInternetStack(helper, meters, "ns3::ddsa::IndividualRetransDdsaIpv4L3Protocol");

	ConnectDdsaMetersSentPacketTrace();
}

void
NeighborhoodAmiSim::InstallOriginalMeterStack()
{
	DDsaHelper helper(Etx::GetTypeId(), OriginalDdsaRoutingProtocol::GetTypeId());
	helper.Set("Retrans", IntegerValue(ddsa_retrans));
	helper.Set("Alpha", DoubleValue(alpha));

	InstallInternetStack(helper, meters, "ns3::ddsa::GlobalRetransDdsaIpv4L3Protocol");

	ConnectDdsaMetersSentPacketTrace();
}

void
NeighborhoodAmiSim::InstallDumbMeterStack()
{
	DDsaHelper helper(Etx::GetTypeId(), DumbDdsaRoutingProtocol::GetTypeId());
	helper.Set("Retrans", IntegerValue(ddsa_retrans));
	helper.Set("Alpha", DoubleValue(alpha));

	InstallInternetStack(helper, meters, "ns3::ddsa::GlobalRetransDdsaIpv4L3Protocol");

	ConnectDdsaMetersSentPacketTrace();
}

void
NeighborhoodAmiSim::InstallNonDdsaMeterStack()
{
	DDsaHelper helper(Etx::GetTypeId(), lqolsr::RoutingProtocol::GetTypeId());

	InstallInternetStack(helper, meters, "ns3::Ipv4L3Protocol");

	ConnectEtxMetersSentPacketTrace();
}

void
NeighborhoodAmiSim::InstallDynamicRetransDdsa()
{
	DDsaHelper helper(Etx::GetTypeId(), DynamicRetransDdsaRoutingProtocol::GetTypeId());
	helper.Set("Prob", DoubleValue(0.99)); //The intended delyvery probability must be 99%
	helper.Set("Threshold", DoubleValue(0.1)); //Daps with delivery probability below 10% will be excluded
	helper.Set("LinkTrans", IntegerValue(4)); //Link layer transmissions: 1 + 3 retransmissions
	helper.Set("Alpha", DoubleValue(alpha));

	InstallInternetStack(helper, meters, "ns3::ddsa::GlobalRetransDdsaIpv4L3Protocol");

	ConnectDdsaMetersSentPacketTrace();
}

void
NeighborhoodAmiSim::ConfigureMeterStack()
{
	TypeId metricTid = Etx::GetTypeId();
	TypeId routingProtocolTid;
	std::string l3ProtTid = "";

	switch(ddsaMode)
	{
		case ProtocolMode::ETX:
		{
			InstallNonDdsaMeterStack();
			break;
		}
		case ProtocolMode::ORIGINAL_DDSA:
		{
			InstallOriginalMeterStack();
			break;
		}
		case ProtocolMode::DUMB_DDSA:
		{
			InstallDumbMeterStack();
			break;
		}
		case ProtocolMode::DYNAMIC_RETRANS_DDSA:
		{
			InstallDynamicRetransDdsa();
			break;
		}
		case ProtocolMode::FLOODING:
		{
			InstallFloodingMeterStack();
			break;
		}
	}
}

void
NeighborhoodAmiSim::ConfigureNodesStack()
{
	ConfigureDapStack();
	ConfigureMeterStack();
	ConfigureControllerStack();
	ConfigureDapsHna();
}

void
NeighborhoodAmiSim::ConfigureDapsHna()
{
	for (uint32_t k = 0; k < daps.GetN() ; k++)
	{
		Ptr<Ipv4> stack = daps.Get (k)->GetObject<Ipv4> ();
		//Ptr<Ipv4RoutingProtocol> rp_Gw = (stack->GetRoutingProtocol ());
		Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (stack->GetRoutingProtocol ());

		Ptr<MaliciousDapRoutingProtocol> rp;

		for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++)
		{
			int16_t priority;
			Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
			if (DynamicCast<MaliciousDapRoutingProtocol> (temp))
			{
				rp = DynamicCast<MaliciousDapRoutingProtocol> (temp);
			}
		}

		// Specify the required associations directly.
		rp->AddHostNetworkAssociation (Ipv4Address ("172.16.1.0"), Ipv4Mask ("255.255.255.0"));
	}
}

void
NeighborhoodAmiSim::UpdateRoutingProtocolWithControllerAddress(Ptr<Node> node, Ipv4Address controllerAddress)
{
	Ptr<DdsaIpv4L3ProtocolBase> ddsarp = node->GetObject<DdsaIpv4L3ProtocolBase>();

	NS_ASSERT (ddsarp);
	ddsarp->SetControllerAddress(controllerAddress);
}

void
NeighborhoodAmiSim::UpdateRoutingProtocolWithControllerAddress()
{
	for (uint32_t i = 0; i < csmaDevices.GetN (); ++i)
	{
		Ptr<NetDevice> device = csmaDevices.Get(i);
		Ptr<Node> node = device->GetNode();

		if (node->GetId() == controllers.Get(0)->GetId())
		{
			for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
			{
				Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
				int32_t interface = ipv4->GetInterfaceForDevice (device);

				UpdateRoutingProtocolWithControllerAddress(*it, ipv4->GetAddress(interface, 0).GetLocal());
			}
		}
	}
}

void
NeighborhoodAmiSim::ConfigureIpAddressing()
{
	NS_LOG_INFO ("Assign IP Addresses.");

	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.1.0.0", "255.255.0.0");
	olsrIpv4Devices = ipv4.Assign (olsrDevices);

	ipv4.SetBase (Ipv4Address("172.16.1.0"), "255.255.255.0");
	csmaIpv4Devices = ipv4.Assign (csmaDevices);

	if (ddsaMode != ProtocolMode::ETX)
	{
		UpdateRoutingProtocolWithControllerAddress();
	}
}

Ptr<Node>
NeighborhoodAmiSim::GetNodeById(uint32_t id, NodeContainer nodes)
{
	for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); it++)
	{
		Ptr<Node> meter = *it;
		if (meter->GetId() == id)
		{
			return meter;
		}
	}

	return 0;
}

void
NeighborhoodAmiSim::ConfigureMeterApplication(uint16_t port, Time start, Time stop)
{
	NodeContainer nodes;
	AmiAppHelper amiHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (csmaIpv4Devices.GetAddress(0), port)));
	//amiHelper.SetAttribute("Retrans", IntegerValue(ddsa_retrans));

	ApplicationContainer apps;

	for(int i : senderIndexList)
	{
		Ptr<Node> sender = GetNodeById(i, meters);//meters.Get(i);

		apps.Add(amiHelper.Install(sender));
		//apps.Get(apps.GetN() - 1)->TraceConnectWithoutContext("Tx", MakeCallback(&NeighborhoodAmiSim::ApplicationPacketSent, this));
	}

	if (apps.GetN() > 0)
	{
		apps.Start (start);
		apps.Stop (stop);
	}
}

void
NeighborhoodAmiSim::ConfigureControllerSocket(uint16_t port)
{
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);

	Ptr<Socket> recvController = Socket::CreateSocket (controllers.Get (0), tid);
	recvController->Bind (local);
	recvController->SetRecvCallback (MakeCallback (&NeighborhoodAmiSim::ControllerReceivePacket, this));
}

void
NeighborhoodAmiSim::ConfigureDapSocket(uint16_t port)
{
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);

	for (NodeContainer::Iterator it = daps.Begin(); it != daps.End(); it++)
	{
		Ptr<Socket> recvDap = Socket::CreateSocket (*it, tid);
		recvDap->Bind (local);
		recvDap->SetRecvCallback (MakeCallback (&NeighborhoodAmiSim::DapReceivedPacket, this));
	}
}

void
NeighborhoodAmiSim::SendPacketToController(Ptr<Packet> packet, Ptr<Node> node)
{
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	Ptr<Socket> source = Socket::CreateSocket (node, tid);
	InetSocketAddress remote = InetSocketAddress (Ipv4Address ("172.16.1.1"), 80);
	source->Connect (remote);
	source->Send (packet);
}

//Callbacks

void
NeighborhoodAmiSim::DapReceivedPacket (Ptr<Socket> socket)
{
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom (from)))
	{
		AmiHeader amiHeader;
		packet->PeekHeader(amiHeader);

		amiHeader.SetDapId(socket->GetNode()->GetId());

		//uint16_t seqNumber = amiHeader.GetPacketSequenceNumber();
		//NS_LOG_UNCOND ("Dap " << socket->GetNode()->GetId() << ": Received packet " << seqNumber << " at " << Simulator::Now().GetSeconds() << " seconds.");

		Ptr<Packet> pkt = Create<Packet>();
		pkt->AddHeader(amiHeader);

		SendPacketToController(pkt, socket->GetNode());
	}
}

void
NeighborhoodAmiSim::DdsaApplicationPacketSent(Ptr<const Packet> packetSent, const Ipv4Address & dapAddress)
{
	Ptr<Packet> pCopy = packetSent->Copy();

	UdpHeader l4Header;
	AmiHeader amiHeader;
	pCopy->RemoveHeader(l4Header);
	pCopy->RemoveHeader(amiHeader);

	if (amiHeader.GetReadingInfo() == 1)
	{
		uint32_t seqNumber = amiHeader.GetPacketSequenceNumber();
		uint32_t meterId = amiHeader.GetMeterId();

		packetsSent[meterId][seqNumber].SetSeqNumber(seqNumber);
		packetsSent[meterId][seqNumber].Send();

		if (dapAddress != Ipv4Address::GetBroadcast())
		{
			uint32_t dapId = GetNodeFromIpAddress(olsrDevices, dapAddress)->GetId();
			NS_LOG_UNCOND ("Application packet " << seqNumber << " sent by meter " << meterId << " at " << Simulator::Now().GetSeconds() << " seconds to DAP " << dapId);
		}
		else
		{
			NS_LOG_UNCOND ("Application packet " << seqNumber << " sent by meter " << meterId << " at " << Simulator::Now().GetSeconds() << " seconds");
		}
	}
}

void
NeighborhoodAmiSim::EtxAplicationPacketSent(const Ipv4Header & header, Ptr<const Packet> packet, uint32_t intId)
{
	DdsaApplicationPacketSent(packet, Ipv4Address::GetAny());
}

void
NeighborhoodAmiSim::ControllerReceivePacket (Ptr<Socket> socket)
{
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom (from)))
	{
		AmiHeader amiHeader;
		packet->PeekHeader(amiHeader);

		if (amiHeader.GetReadingInfo() == 1)
		{
			uint32_t seqNumber = amiHeader.GetPacketSequenceNumber();
			uint32_t meterId = amiHeader.GetMeterId();
			uint32_t dapId = amiHeader.GetDapId();

			if (!packetsSent[meterId][seqNumber].Received())
			{
				NS_LOG_UNCOND ("Controller: Received packet " << seqNumber << " sent by meter " << meterId
															  << " through DAP " << dapId
															  << " at " << Simulator::Now().GetSeconds());
				packetsSent[meterId][seqNumber].Receive();
			}
			else
			{
				NS_LOG_UNCOND ("Controller: Received duplicated copy of packet " << seqNumber
																				 << " sent by meter "
																				 << meterId
																				 << " through DAP "
																				 << dapId
																				 << " at " << Simulator::Now().GetSeconds());
				packetsSent[meterId][seqNumber].IncrementReceived();
			}
		}
		else
		{
			NS_LOG_UNCOND ("Controller: Received non-ami packet at " << Simulator::Now().GetSeconds());
		}
	}
}

void
NeighborhoodAmiSim::Collision()
{
	collisionCount++;
}

//Failure generation

void
NeighborhoodAmiSim::SheduleFailure(double time)
{
	failureTime = Seconds (time);

	if (withFailure)
	{
		m_events.Track (Simulator::Schedule (failureTime, &NeighborhoodAmiSim::ExecuteFailure, this));
	}
}

Ptr<Node>
NeighborhoodAmiSim::GetRandomFailingNode()
{
  return daps.Get(m_rnd->GetInteger(0, daps.GetN() - 1));
}

void
NeighborhoodAmiSim::ExecuteFailure()
{
	Ptr<Node> failingDap = failingDapId < 0 ? GetRandomFailingNode() : GetNodeById(failingDapId, daps);;

	Ptr<ddsa::FailingIpv4L3Protocol> l3Prot = failingDap->GetObject<ddsa::FailingIpv4L3Protocol>();

	if (l3Prot)
	{
		l3Prot->MakeFail();
		l3Prot->SetFailureType(static_cast<FailingIpv4L3Protocol::FailureType>(failureMode));
		NS_LOG_UNCOND ("Node " << failingDap->GetId() << " is not working(t = " << Simulator::Now().GetSeconds() << "s)");
	}


	if (failureMode == FailingIpv4L3Protocol::FailureType::MALICIOUS)
	{
		Ptr<MaliciousDapRoutingProtocol> rp = failingDap->GetObject<MaliciousDapRoutingProtocol>();

		if (rp)
		{
			rp->StartMalicious();
		}
	}
}

//Statistics

std::string
NeighborhoodAmiSim::GetSimulationDir()
{
	std::string dir = "sim/";
	int simulationRound = GetSimulationRound();

	if (withFailure)
	{
		switch (failureMode)
		{
			case FailingIpv4L3Protocol::FailureType::FULL:
			{
				dir += "full_failure/";
				break;
			}
			case FailingIpv4L3Protocol::FailureType::MALICIOUS:
			{
				dir += "malicious_failure/";
				break;
			}
		}
	}
	else
	{
		dir += "no_failure/";
	}

	switch(ddsaMode)
	{
		case ProtocolMode::ETX:
		{
			dir += "etx/";
			break;
		}
		case ProtocolMode::ORIGINAL_DDSA:
		{
			dir += "original/";
			break;
		}
		case ProtocolMode::DUMB_DDSA:
		{
			dir += "dumb/";
			break;
		}
		case ProtocolMode::DYNAMIC_RETRANS_DDSA:
		{
			dir += "dynamic_global/";
			break;
		}
		case ProtocolMode::FLOODING:
		{
			dir += "flooding/";
			break;
		}
	}



	return dir + std::to_string(ddsa_retrans) + "/Round" + std::to_string(simulationRound) + "/";
}

void
NeighborhoodAmiSim::GetMeterDeliveryRate(uint16_t meterId, double * beforeFailure, double * aftarFailure, double * total)
{
	int totalSentBeforeFailure = 0;
	int totalRecvBeforeFailure = 0;
	int totalSentAfterFailure = 0;
	int totalRecvAfterFailure = 0;

	std::map<uint16_t, AmiPacketInformation> meterInfo = packetsSent[meterId];

	for (std::map<uint16_t, AmiPacketInformation>::iterator it = meterInfo.begin(); it != meterInfo.end(); it++)
	{
		if (withFailure && it->second.GetLatencyInfo().time_sent < failureTime)
		{
			totalSentBeforeFailure++;

			if (it->second.Received())
			{
				totalRecvBeforeFailure++;
			}
		}
		else if (withFailure)
		{
			totalSentAfterFailure++;

			if (it->second.Received())
			{
				totalRecvAfterFailure++;
			}
		}
	}

	*beforeFailure = (totalRecvBeforeFailure * 100) / (double)totalSentBeforeFailure;
	*aftarFailure = (totalRecvAfterFailure * 100) / (double)totalSentAfterFailure;
	*total = ((totalRecvAfterFailure + totalRecvBeforeFailure) * 100) / (double)(totalSentBeforeFailure + totalSentAfterFailure);
}

double
NeighborhoodAmiSim::GetMeterMeanLatency(uint16_t meterId)
{
	int64_t totalLatency = 0;
	int totalReceived = 0;
	std::map<uint16_t, AmiPacketInformation> meterInfo = packetsSent[meterId];

	for (std::map<uint16_t, AmiPacketInformation>::iterator it = meterInfo.begin(); it != meterInfo.end(); it++)
	{
		if (it->second.Received())
		{
			totalReceived++;
			totalLatency += it->second.GetLatencyInfo().GetLatency().GetMilliSeconds();
		}
	}

	return totalReceived != 0 ? (double)totalLatency / totalReceived : Time::Max().GetSeconds();
}

double
NeighborhoodAmiSim::GetMeterRecoveryTime(uint16_t meterId)
{
	Time lastReceivedTime;

	std::map<uint16_t, AmiPacketInformation> meterInfo = packetsSent[meterId];

	for (std::map<uint16_t, AmiPacketInformation>::iterator it = meterInfo.begin(); it != meterInfo.end(); it++)
	{
		if (it->second.Received())
		{
			if (it->second.GetLatencyInfo().time_received.GetSeconds() <= failureTime.GetSeconds())
			{
				lastReceivedTime = it->second.GetLatencyInfo().time_received;
			}
			else
			{
				double unavailabilityTime = it->second.GetLatencyInfo().time_received.GetSeconds() - lastReceivedTime.GetSeconds();

				//Estou supondo que os pacotes são enviados a cada 3 segundos
				if (unavailabilityTime < 6.0)
				{
					unavailabilityTime = 0.0;
				}

				return unavailabilityTime;
			}
		}
	}

	return Time::Max().GetSeconds();
}

double
NeighborhoodAmiSim::GetMeterTotalReceivedPackets(uint16_t meterId)
{
	int totalReceived = 0;
	std::map<uint16_t, AmiPacketInformation> meterInfo = packetsSent[meterId];

	for (std::map<uint16_t, AmiPacketInformation>::iterator it = meterInfo.begin(); it != meterInfo.end(); it++)
	{
		if (it->second.Received())
		{
			totalReceived++;
		}
	}

	return totalReceived;
}

uint64_t
NeighborhoodAmiSim::GetMeterTotalLatency(uint16_t meterId)
{
	uint64_t totalLatency = 0;
	std::map<uint16_t, AmiPacketInformation> meterInfo = packetsSent[meterId];

	for (std::map<uint16_t, AmiPacketInformation>::iterator it = meterInfo.begin(); it != meterInfo.end(); it++)
	{
		if (it->second.Received())
		{
			totalLatency += it->second.GetLatencyInfo().GetLatency().GetMilliSeconds();
		}
	}

	return totalLatency;
}

uint32_t
NeighborhoodAmiSim::GetTotalUnnecessaryCopies(uint16_t meterId)
{
	uint32_t totalUnnecessaryCopies = 0;
	std::map<uint16_t, AmiPacketInformation> meterInfo = packetsSent[meterId];

	for (std::map<uint16_t, AmiPacketInformation>::iterator it = meterInfo.begin(); it != meterInfo.end(); it++)
	{
		if (it->second.Received())
		{
			totalUnnecessaryCopies += it->second.GetNReceived() - 1;
		}
	}

	return totalUnnecessaryCopies;
}

uint32_t
NeighborhoodAmiSim::GetTotalSentMessages(uint16_t meterId)
{
	uint32_t totalSentMessages = 0;
	std::map<uint16_t, AmiPacketInformation> meterInfo = packetsSent[meterId];

	for (std::map<uint16_t, AmiPacketInformation>::iterator it = meterInfo.begin(); it != meterInfo.end(); it++)
	{
		totalSentMessages += it->second.GetNSent();
	}

	return totalSentMessages;
}

void
NeighborhoodAmiSim::GenerateSimulationStatistcs()
{
	std::ofstream statFile;
	double beforeFailure;
	double afterFailure;
	double total;
	double meanLatency;
	double unavailabilityTime;
	int totalReceived = 0;
	int totalSent = 0;
	uint64_t totalLatency = 0;
	//uint32_t totalUnecessaryCopies = 0;
	uint32_t totalSentMessages = 0;

	std::string simulationDir = GetSimulationDir();
	std::string filename = simulationDir + "stat.txt";
	statFile.open(filename);

	for (std::map<uint16_t, std::map<uint16_t, AmiPacketInformation> >::iterator it = packetsSent.begin(); it != packetsSent.end(); it++)
	{
		uint16_t meterId = it->first;
		GetMeterDeliveryRate(meterId, &beforeFailure, &afterFailure, &total);
		meanLatency = GetMeterMeanLatency(it->first);
		unavailabilityTime = GetMeterRecoveryTime(it->first);
		totalReceived += GetMeterTotalReceivedPackets(it->first);
		totalSent += it->second.size();
		totalLatency += GetMeterTotalLatency(it->first);
		//totalUnecessaryCopies += GetTotalUnnecessaryCopies(meterId);
		totalSentMessages += GetTotalSentMessages(meterId);

		statFile << "Meter " << it->first << "\n";
		statFile << "Delivey Rate: " << total << "%\n";
		statFile << "Delivey Rate (Before Failure): " << beforeFailure << "%\n";
		statFile << "Delivey Rate (After Failure): " << afterFailure << "%\n";

		if ( meanLatency != Time::Max().GetSeconds())
		{
			statFile << "Mean Latency: " << meanLatency << " ms\n";
		}

		if (unavailabilityTime != Time::Max().GetSeconds())
		{
			statFile << "Recovery time: " << unavailabilityTime << " s\n\n";
		}
		else
		{
			statFile << "Recovery time: Infinity" << "\n\n";
		}
	}

	statFile << "Total Delivey Rate: " << (totalReceived * 100) / (double)totalSent << "%\n";
	statFile << "Total Mean Latency: " << (double)totalLatency / totalReceived << " ms\n";
	statFile << "Total Collisions: " << collisionCount << "\n";
	statFile << "Total sent messages: " << totalSentMessages << "\n";

	statFile.close();
}

void
NeighborhoodAmiSim::GenerateSimulationPacketInfo()
{
	std::ofstream packetsFile;
	std::string simulationDir = GetSimulationDir();
	std::string filename =  simulationDir + "packets.txt";

	packetsFile.open(filename);


	for (std::map<uint16_t, std::map<uint16_t, AmiPacketInformation> >::iterator it = packetsSent.begin(); it != packetsSent.end(); it++)
	{

		std::map<uint16_t, AmiPacketInformation> sent = it->second;

		packetsFile << "\nPackets sent by meter " << it->first << "\n\n";

		for (std::map<uint16_t, AmiPacketInformation>::iterator info = sent.begin(); info != sent.end(); info++)
		{
			packetsFile << "\nPacket " << info->second.GetSeqNumber() << " -----------------------";
			packetsFile << (info->second.Received() ? "\n" : "(Lost)\n");

			packetsFile << "Sent at: " << info->second.GetLatencyInfo().time_sent.GetSeconds() << " seconds\n";

			if (info->second.Received())
			{
				packetsFile << "Received at: " << info->second.GetLatencyInfo().time_received.GetSeconds() << " seconds";

				if (withFailure && info->second.GetLatencyInfo().time_received >= failureTime)
				{
					packetsFile << "(After failure)\n";
				}
				else
				{
					packetsFile << "\n";
				}

				packetsFile << "Latency: " << info->second.GetLatencyInfo().GetLatency().GetMilliSeconds() << "ms \n";
				packetsFile << "Received copies: " << info->second.GetNReceived() << "\n";
				packetsFile << "Sent copies: " << info->second.GetNSent() << "\n";
				packetsFile << "Unneessary copies: " << info->second.GetNReceived() - 1 << "\n";
			}
		}
	}

	packetsFile.close();
}

//Tests

bool
NeighborhoodAmiSim::TestNonDdsaMeterStack()
{
	bool metersComply = true;

	for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
	{
		Ptr<Node> meter = *it;

		Ptr<Ipv4L3Protocol> nonDdsaL3Prot = meter->GetObject<Ipv4L3Protocol>();
		Ptr<DdsaIpv4L3ProtocolBase> ddsaL3Prot = meter->GetObject<DdsaIpv4L3ProtocolBase>();

		Ptr<Ipv4RoutingProtocol> rp = GetNodeEffectiveRoutingProtocol(meter);

		metersComply = nonDdsaL3Prot && !ddsaL3Prot && DynamicCast<lqolsr::RoutingProtocol> (rp) && !DynamicCast<DdsaRoutingProtocolBase> (rp);
	}

	return metersComply;
}

bool
NeighborhoodAmiSim::TestOriginalDdsaMeterStack()
{
	bool metersComply = true;

	for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
	{
		Ptr<Node> meter = *it;

		Ptr<GlobalRetransDdsaIpv4L3Protocol> ddsaL3Prot = meter->GetObject<GlobalRetransDdsaIpv4L3Protocol>();

		Ptr<Ipv4RoutingProtocol> rp = GetNodeEffectiveRoutingProtocol(meter);

		metersComply = ddsaL3Prot && DynamicCast<OriginalDdsaRoutingProtocol> (rp);
	}

	return metersComply;
}

bool
NeighborhoodAmiSim::TestDumbDdsaMeterStack()
{
	bool metersComply = true;

	for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
	{
		Ptr<Node> meter = *it;

		Ptr<GlobalRetransDdsaIpv4L3Protocol> ddsaL3Prot = meter->GetObject<GlobalRetransDdsaIpv4L3Protocol>();

		Ptr<Ipv4RoutingProtocol> rp = GetNodeEffectiveRoutingProtocol(meter);

		metersComply = ddsaL3Prot && DynamicCast<DumbDdsaRoutingProtocol> (rp);
	}

	return metersComply;
}

bool
NeighborhoodAmiSim::TestDynamicDdsaMeterStack()
{
	bool metersComply = true;

	for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
	{
		Ptr<Node> meter = *it;

		Ptr<GlobalRetransDdsaIpv4L3Protocol> ddsaL3Prot = meter->GetObject<GlobalRetransDdsaIpv4L3Protocol>();

		Ptr<Ipv4RoutingProtocol> rp = GetNodeEffectiveRoutingProtocol(meter);

		metersComply = ddsaL3Prot && DynamicCast<DynamicRetransDdsaRoutingProtocol> (rp);
	}

	return metersComply;
}

bool
NeighborhoodAmiSim::TestFloodingDdsaMeterStack()
{
	bool metersComply = true;

	for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
	{
		Ptr<Node> meter = *it;

		Ptr<IndividualRetransDdsaIpv4L3Protocol> ddsaL3Prot = meter->GetObject<IndividualRetransDdsaIpv4L3Protocol>();

		Ptr<Ipv4RoutingProtocol> rp = GetNodeEffectiveRoutingProtocol(meter);

		metersComply = ddsaL3Prot && DynamicCast<FloodingDdsaRoutingProtocol> (rp);
	}

	return metersComply;
}

bool
NeighborhoodAmiSim::TestDapsStack()
{
	bool dapsComply = true;

	for (NodeContainer::Iterator it = daps.Begin(); it != daps.End(); it++)
	{
		Ptr<Node> dap = *it;

		Ptr<FailingIpv4L3Protocol> ddsaL3Prot = dap->GetObject<FailingIpv4L3Protocol>();

		Ptr<Ipv4RoutingProtocol> rp = GetNodeEffectiveRoutingProtocol(dap);

		dapsComply = ddsaL3Prot && DynamicCast<MaliciousDapRoutingProtocol> (rp);
	}

	return dapsComply;
}

void
NeighborhoodAmiSim::TestStack()
{
	switch(ddsaMode)
	{
		case ProtocolMode::ETX:
		{
			NS_ASSERT_MSG(TestNonDdsaMeterStack(), "There is a problem with ETX meter stack");
			break;
		}
		case ProtocolMode::ORIGINAL_DDSA:
		{
			NS_ASSERT_MSG(TestOriginalDdsaMeterStack(), "There is a problem with Original DDSA meter stack");
			break;
		}
		case ProtocolMode::DUMB_DDSA:
		{
			NS_ASSERT_MSG(TestDumbDdsaMeterStack(), "There is a problem with Dumb DDSA meter stack");
			break;
		}
		case ProtocolMode::DYNAMIC_RETRANS_DDSA:
		{
			NS_ASSERT_MSG(TestDynamicDdsaMeterStack(), "There is a problem with Dynamic DDSA meter stack");
			break;
		}
		case ProtocolMode::FLOODING:
		{
			NS_ASSERT_MSG(TestFloodingDdsaMeterStack(), "There is a problem with Dynamic DDSA meter stack");
			break;
		}
	}

	NS_ASSERT_MSG(TestDapsStack(), "There is a problem daps stack");

}

int main (int argc, char *argv[])
{
	try
	{
		NeighborhoodAmiSim simulation;

		simulation.ConfigureSimulation(argc, argv);
		simulation.TestStack();

		Simulator::Stop (Seconds (500.0));
		Simulator::Run ();

		NS_LOG_UNCOND("Simulation finished. Creating statistics.");

		simulation.GenerateSimulationPacketInfo();
		simulation.GenerateSimulationStatistcs();

		Simulator::Destroy ();
	}
	catch (const std::exception & ex)
	{
		std::cerr << ex.what() << "\n";
		return 1;
	}

	return 0;
}

