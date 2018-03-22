#include "ns3/core-module.h"
#include "ns3/etx-ff.h"
#include "ns3/lq-olsr-header.h"
#include "ns3/ddsa-routing-protocol-adapter.h"
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
#include "ns3/ipv4-l3-protocol-ddsa-adapter.h"
#include "ns3/ddsa-routing-protocol-adapter.h"
#include "ns3/integer.h"
#include "ns3/simple-header.h"
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


struct LatencyInfo
{
  public:
    Time time_sent;
    Time time_received;

    LatencyInfo()
    {
      time_sent = Time::Max();
      time_received = Time::Max();;
    }
};

struct AmiPacketInfo
{
  private:
    uint16_t seqNumber;
    int nReceived;
    int nSent;
    LatencyInfo latency;
    bool received;
  public:

    AmiPacketInfo()
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

    void SetLatency(LatencyInfo latency)
    {
      this->latency = latency;
    }

    LatencyInfo GetLatency()
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



/*
 * NeighborhoodAmiSim.cpp
 *
 *  Created on: 13 de jun de 2017
 *      Author: igor
 */

class NeighborhoodAmiSim
{
  public:
	NeighborhoodAmiSim ();
    virtual ~NeighborhoodAmiSim ();
    void Configure(int argc, char *argv[]);

    //Topologia

    void CreateTopology();



    void ConfigureWifi();
    void CreateControllerLan();
    void ConfigureIpAddressing();
    void ConfigureMeterApplication(uint16_t port, Time start, Time stop);
    void ConfigureControllerSocket(uint16_t port);
    void ConfigureNodesStack();
    void ConfigureDapSocket(uint16_t port);
    void CreateNodes();

    void GenerateGeneralStatistics();
    //void PrintReceivedPackets(std::ofstream stream);
    void SheduleFailure(double time);
    Ptr<Node> GetRandomFailingNode();
    Ptr<Node> GetDdsaFailingNode();
    Ptr<Node> GetNonDdsaFailingNode();
    void ExecuteFailure();
    void GenerateDapStatistics();
    void GenerateControllerReceptionStatistics();

    bool IsDdsaEnabled()
    {
      return ddsaEnabled;
    }

    const double BEST_LINK = 112;//50.0;
    const double GOOD_LINK = 112;//90.0;
    const double BAD_LINK = 114.02;
    const double NO_LINK = 200.0;

  private:
    void Parse(int argc, char *argv[]);
    void ConfigureStack(LqOlsrHelper & lqOlsrDapHelper, NodeContainer nodes, DdsaRoutingProtocolAdapter::NodeType nType );
    void ConfigureDapStack(LqOlsrHelper & helper);
    void ConfigureMeterStack(LqOlsrHelper & helper);
    Ipv4ListRoutingHelper ConfigureRouting(const Ipv4RoutingHelper & ipv4Routing);
    void ConfigureControllerStack();
    void ConfigureDapsHna();

    void
	AddNeighborhood(Ptr<ListPositionAllocator> meterPositionAlloc, int startX, int startY);
    void ConfigureIntraNeighborhoodLinks(NodeContainer neighborhood);
    void ConfigureNeighborhoodToDapLinks(NodeContainer neighborhood, int neighborhoodIndex);
    void ConfigureHorizontalInterNeighborhoodLinks(NodeContainer neighborhoodSrc, NodeContainer neighborhoodDest);
    void ConfigureVerticalInterNeighborhoodLinks(NodeContainer neighborhoodSrc, NodeContainer neighborhoodDest);

    void ApplicationPacketSent(Ptr<const Packet>);
    void ControllerReceivePacket (Ptr<Socket> socket);
    void SendPacketToController(Ptr<Packet> packet, Ptr<Node> node);

    void NeighborExpired(Ipv4Address neighborAddress);
    void DdsaDapReceivedPacket (Ptr<Socket> socket);
    void SenderRouteUpdated(std::vector<ddsa::Dap> daps, const Ipv4Address & address);
    double CalculateDapSelection(const Ipv4Address & dapAddress);
    void NonDdsaDapReceivedPacket(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interfaceId);
    double CalculateDeliveryRate();
    double CalculateLossRate();
    Ipv4Address GetNodeAddress(const Ptr<NetDevice> & device, const Ptr<Node> node);
    void DapSelectedToNewCopy(const Ipv4Address & dapAddress, Ptr<Packet> packet);
    void UpdateDapSelectionHistory(const Ipv4Address & address, uint16_t seqNumber);
    double CalculateMeanCost(const Ipv4Address & dapAddress);
    double CalculateMeanLatency();
    double CalculateMeanProbability(const Ipv4Address & dapAddress);
    Ipv4Address GetIpAddressForOlsrNode(Ptr<Node> node);
    Ptr<NetDevice> GetDeviceForNode(Ptr<Node> node, NetDeviceContainer devices);
    Ipv4Address GetIpAddressForNode(Ptr<Node> node, Ptr<NetDevice> device);
    Ipv4Address GetIpAddressForCsmaNode(Ptr<Node> node);
    int GetTotalSentCopies();
    void ParseSenders();

    std::string phyMode;
    bool withFailure;
    int failureMode;
    bool ddsaEnabled;
    CommandLine cmd;
    NodeContainer daps;
    NodeContainer meters;
    NodeContainer controllers;
    std::string senderIndexes;
    std::vector<int> senderIndexList;
    Ptr<MatrixPropagationLossModel> myLossModel;
    YansWifiPhyHelper myWifiPhy;
    Ipv4InterfaceContainer olsrIpv4Devices;
    Ipv4InterfaceContainer csmaIpv4Devices;
    EventGarbageCollector m_events;
    NetDeviceContainer olsrDevices;
    NetDeviceContainer csmaDevices;

    //Statistics
    int totalDaps;
    int totalMeters;
    int totalControllers;
    int totalSentCopies;
    std::map<uint16_t, std::map<uint16_t, AmiPacketInfo> > packetsSent;
    std::map<Ipv4Address, std::vector<AmiPacketInfo> > dapSelectionHistory;
    std::map<Ipv4Address, std::vector<Dap> > costHistory;
    int ddsa_retrans;
    bool dumb;
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
    ddsaEnabled = true;
    totalSentCopies = 0;
    ddsa_retrans = 0;
    dumb = false;
    m_rnd = CreateObject<UniformRandomVariable>();
    failureMode = 0; //Full failure

    myLossModel = CreateObject<MatrixPropagationLossModel> ();
}

NeighborhoodAmiSim::~NeighborhoodAmiSim ()
{
  // TODO Auto-generated destructor stub
}

void
NeighborhoodAmiSim::Parse(int argc, char *argv[])
{
  cmd.AddValue ("failure", "Cause the failure of one DAP", withFailure);
  cmd.AddValue ("ddsaenabled", "Enable ddsa", ddsaEnabled);
  cmd.AddValue ("senders", "Comma separated list of sender indexes.", senderIndexes);
  cmd.AddValue ("retrans", "Number of retransmissions", ddsa_retrans);
  cmd.AddValue ("dumb", "Ddsa in dumb mode", dumb);
  cmd.AddValue ("fmode", "Failure mode", failureMode);

  cmd.Parse (argc, argv);
}

//Simulation Configuration

void
NeighborhoodAmiSim::ParseSenders()
{
  if (senderIndexes == "*")
    {
      for(uint32_t i = 0; i < meters.GetN(); i++)
		{
		  senderIndexList.push_back(i);
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
NeighborhoodAmiSim::Configure(int argc, char *argv[])
{
  Parse(argc, argv);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));
}

void
NeighborhoodAmiSim::CreateNodes()
{
  daps.Create(totalDaps); // Creates the specified number of daps
  meters.Create(totalMeters); //Creates the Specified NUmber of meters
  controllers.Create(totalControllers); // Creates one controller.

  ParseSenders();
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

void
NeighborhoodAmiSim::ConfigureStack(LqOlsrHelper & lqOlsrDapHelper, NodeContainer nodes, DdsaRoutingProtocolAdapter::NodeType nType )
{
  Ipv4ListRoutingHelper ipv4Routing = ConfigureRouting(lqOlsrDapHelper);
  DdsaInternetStackHelper ddsa;
  ddsa.SetRoutingHelper (ipv4Routing); // has effect on the next Install ()
  ddsa.SetNodeType(nType);
  ddsa.Install (nodes);
}

void
NeighborhoodAmiSim::ConfigureDapStack(LqOlsrHelper & helper)
{
  for (int i = 0; i < totalDaps; i++)
    {
      helper.ExcludeInterface (daps.Get (i), 2);
    }

  ConfigureStack(helper, daps, DdsaRoutingProtocolAdapter::NodeType::DAP);

  if (!ddsaEnabled)
    {
      for (NodeContainer::Iterator it = daps.Begin(); it != daps.End(); it++)
	{
	  Ptr<ns3::ddsa::Ipv4L3ProtocolDdsaAdapter> ipv4L3 = (*it)->GetObject<ddsa::Ipv4L3ProtocolDdsaAdapter>();
	  NS_ASSERT (ipv4L3);

	  ipv4L3->TraceConnectWithoutContext("Rx", MakeCallback(&NeighborhoodAmiSim::NonDdsaDapReceivedPacket, this));
	}
    }
}

void
NeighborhoodAmiSim::ConfigureMeterStack(LqOlsrHelper & helper)
{
  ConfigureStack(helper, meters, DdsaRoutingProtocolAdapter::NodeType::METER);

  if (ddsaEnabled)
     {

      Ptr<Node> sender = meters.Get(senderIndexList[0]);

      for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
		{
		  Ptr<ns3::ddsa::DdsaRoutingProtocolAdapter> rp = (*it)->GetObject<ddsa::DdsaRoutingProtocolAdapter>();
		  NS_ASSERT (rp);
		  rp->TraceConnectWithoutContext("DapSelection", MakeCallback(&NeighborhoodAmiSim::DapSelectedToNewCopy, this));
		  rp->TraceConnectWithoutContext("NeighborExpired", MakeCallback(&NeighborhoodAmiSim::NeighborExpired, this));
		  if ((*it)->GetId() == sender->GetId())
			{
			  rp->TraceConnectWithoutContext("RouteComputed", MakeCallback(&NeighborhoodAmiSim::SenderRouteUpdated, this));
			}
		}
     }
}

void
NeighborhoodAmiSim::ConfigureControllerStack()
{
  InternetStackHelper internet;

  internet.Install (controllers);
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
NeighborhoodAmiSim::ConfigureDapsHna()
{
  for (uint32_t k = 0; k < daps.GetN() ; k++)
    {
		Ptr<Ipv4> stack = daps.Get (k)->GetObject<Ipv4> ();
		Ptr<Ipv4RoutingProtocol> rp_Gw = (stack->GetRoutingProtocol ());
		Ptr<Ipv4ListRouting> lrp_Gw = DynamicCast<Ipv4ListRouting> (rp_Gw);

		Ptr<lqolsr::RoutingProtocol> lqolsr_rp;

		for (uint32_t i = 0; i < lrp_Gw->GetNRoutingProtocols ();  i++)
		{
			int16_t priority;
			Ptr<Ipv4RoutingProtocol> temp = lrp_Gw->GetRoutingProtocol (i, priority);
			if (DynamicCast<lqolsr::RoutingProtocol> (temp))
			{
				lqolsr_rp = DynamicCast<lqolsr::RoutingProtocol> (temp);
			}
		}

		// Specify the required associations directly.
		lqolsr_rp->AddHostNetworkAssociation (Ipv4Address ("172.16.1.0"), Ipv4Mask ("255.255.255.0"));
    }
}

void
NeighborhoodAmiSim::ConfigureNodesStack()
{
  TypeId metricTid = Etx::GetTypeId();

  DDsaHelper helper(metricTid);
  ConfigureDapStack(helper);

  DDsaHelper meterHelper(metricTid);
  meterHelper.Set("Dumb", BooleanValue(dumb));
  //meterHelper.Set("Alpha", DoubleValue(0.5));
  ConfigureMeterStack(ddsaEnabled ? meterHelper : helper);

  ConfigureControllerStack();
  ConfigureDapsHna();
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

  if (!ddsaEnabled)
    {
      return;
    }

  for (uint32_t i = 0; i < csmaDevices.GetN (); ++i)
    {
      Ptr<NetDevice> device = csmaDevices.Get(i);
      Ptr<Node> node = device->GetNode();

      if (node->GetId() == controllers.Get(0)->GetId())
	{
	  for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
	    {
	      Ptr<ns3::ddsa::DdsaRoutingProtocolAdapter> ddsaRp = (*it)->GetObject<ns3::ddsa::DdsaRoutingProtocolAdapter>();
	      NS_ASSERT (ddsaRp);
	      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
	      int32_t interface = ipv4->GetInterfaceForDevice (device);
	      ddsaRp->SetControllerAddress(ipv4->GetAddress(interface, 0).GetLocal());
	    }
	}
    }
}

void
NeighborhoodAmiSim::ConfigureMeterApplication(uint16_t port, Time start, Time stop)
{
  NodeContainer nodes;
  AmiAppHelper amiHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (csmaIpv4Devices.GetAddress(0), port)));
  ApplicationContainer apps;
  for(int i : senderIndexList)
    {
      Ptr<Node> sender = meters.Get(i);
      if (ddsaEnabled)
		{
		  amiHelper.SetAttribute("Retrans", IntegerValue(ddsa_retrans));
		}

      apps.Add(amiHelper.Install(sender));
      apps.Get(apps.GetN() - 1)->TraceConnectWithoutContext("Tx", MakeCallback(&NeighborhoodAmiSim::ApplicationPacketSent, this));
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
      recvDap->SetRecvCallback (MakeCallback (&NeighborhoodAmiSim::DdsaDapReceivedPacket, this));
    }
}

//Callbacks

void
NeighborhoodAmiSim::NeighborExpired(Ipv4Address neighborAddress)
{
	NS_LOG_UNCOND("Neighbor " << neighborAddress << " expired.");
}

void
NeighborhoodAmiSim::SenderRouteUpdated(std::vector<ddsa::Dap> daps, const Ipv4Address & address)
{
  NS_LOG_UNCOND("Route Updated");

  for(std::vector<ddsa::Dap>::iterator it = daps.begin(); it != daps.end(); it++)
    {
      NS_LOG_UNCOND("DAP " << it->address << " updated: " << it->cost << " at " << Simulator::Now().GetSeconds());

      if (daps.size() == this->daps.GetN() && senderIndexList.size() == 1)
		{
		  costHistory[it->address].push_back(*it);
		}
    }
}

void
NeighborhoodAmiSim::DdsaDapReceivedPacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      AmiHeader amiHeader;

      if (packet->PeekHeader(amiHeader) != 0)
      {
		  uint16_t seqNumber = amiHeader.GetPacketSequenceNumber();
		  NS_LOG_UNCOND ("Dap " << socket->GetNode()->GetId() << ": Received packet " << seqNumber << " at " << Simulator::Now().GetSeconds() << " seconds.");

		  Ptr<Packet> pkt = Create<Packet>();
		  pkt->AddHeader(amiHeader);
		  SendPacketToController(pkt, socket->GetNode());
      }
    }
}

void
NeighborhoodAmiSim::NonDdsaDapReceivedPacket(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interfaceId)
{
  Ipv4Address address = ipv4->GetAddress(interfaceId, 0).GetLocal();
  Ipv4Mask mask = ipv4->GetAddress(interfaceId, 0).GetMask();

  Ipv4Header ipHeader;
  UdpHeader udp;
  AmiHeader amiHeader;

  Ptr<Packet> pktCopy = packet->Copy();

  if (pktCopy->RemoveHeader(ipHeader) != 0 && ipHeader.GetDestination().IsSubnetDirectedBroadcast(mask))
    {
      return;
    }

  if (pktCopy->RemoveHeader(udp) != 0)
    {

      if (pktCopy->RemoveHeader(amiHeader) != 0)
		  {
			uint16_t seqNumber = amiHeader.GetPacketSequenceNumber();
			NS_LOG_UNCOND ("Dap " << address << ": Received packet " << seqNumber << " at " << Simulator::Now().GetSeconds() << " seconds.");
			UpdateDapSelectionHistory(address, seqNumber);
			totalSentCopies++;
		  }
    }
}

void
NeighborhoodAmiSim::UpdateDapSelectionHistory(const Ipv4Address & address, uint16_t seqNumber)
{
  bool found = false;
  std::vector<AmiPacketInfo>::iterator it = dapSelectionHistory[address].begin();
  while (it != dapSelectionHistory[address].end() && !found)
    {
      if (it->GetSeqNumber() == seqNumber)
	{
	  it->IncrementReceived();
	  found = true;
	}

      it++;
    }

  if (!found)
    {
      AmiPacketInfo info;
      info.SetSeqNumber(seqNumber);
      info.IncrementReceived();
      dapSelectionHistory[address].push_back(info);
    }
}

void
NeighborhoodAmiSim::ApplicationPacketSent(Ptr<const Packet> packetSent)
{
  AmiHeader amiHeader;
  packetSent->PeekHeader(amiHeader);

  uint16_t seqNumber = amiHeader.GetPacketSequenceNumber();
  uint16_t meterId = amiHeader.GetNodeId();

  NS_LOG_UNCOND ("Application packet" << seqNumber << " sent at" << " at " << Simulator::Now().GetSeconds() << " seconds.");

  packetsSent[meterId][seqNumber].SetSeqNumber(seqNumber);
  packetsSent[meterId][seqNumber].Send();
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
		  uint16_t seqNumber = amiHeader.GetPacketSequenceNumber();
		  uint16_t meterId = amiHeader.GetNodeId();

		  NS_LOG_UNCOND ("Controller: Received packet " << seqNumber << " sent by " << meterId << " at " << Simulator::Now().GetSeconds());


		  if (!packetsSent[meterId][seqNumber].Received())
			{
			  packetsSent[meterId][seqNumber].Receive();
			}
		  else
			{
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
NeighborhoodAmiSim::DapSelectedToNewCopy(const Ipv4Address & dapAddress, Ptr<Packet> packet)
{
  UdpHeader udp;
  AmiHeader amiHeader;

  packet->PeekHeader(amiHeader);

  if (amiHeader.GetReadingInfo() == 1)
    {
      uint16_t seqNumber = amiHeader.GetPacketSequenceNumber();

      NS_LOG_UNCOND ("Dap " << dapAddress << " selected for packet " << seqNumber << " at " << Simulator::Now().GetSeconds() << " seconds.");
      UpdateDapSelectionHistory(dapAddress, seqNumber);
      totalSentCopies++;
    }
  else
    {
      NS_LOG_UNCOND ("Dap selection failed");
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

Ptr<Node>
NeighborhoodAmiSim::GetDdsaFailingNode()
{
  Ptr<Node> failingDap = 0;

  if (senderIndexList.size() == 1)
    {
      Ptr<DdsaRoutingProtocolAdapter> ddsa = meters.Get(senderIndexList[0])->GetObject<DdsaRoutingProtocolAdapter>();
      ddsa::Dap bestDap = ddsa->GetBestCostDap();

      if (ddsa)
		{
		  if (bestDap.address != Ipv4Address::GetBroadcast())
			{
			  for (NodeContainer::Iterator it = daps.Begin(); it != daps.End(); it++)
				{
				  if (bestDap.address == GetIpAddressForOlsrNode(*it))
					{
					  failingDap = *it;
					}
				}
			}
		}
    }

  if (!failingDap)
    {
      failingDap = GetRandomFailingNode();
    }


    return failingDap;
}

Ptr<Node>
NeighborhoodAmiSim::GetNonDdsaFailingNode()
{
  Ptr<Node> failingDap = 0;

  if (senderIndexList.size() == 1)
    {
      Ptr<lqolsr::RoutingProtocol> rp = meters.Get(senderIndexList[0])->GetObject<lqolsr::RoutingProtocol>();

      if (rp)
		{
		  Ipv4Address bestDapAddress = rp->GetBestHnaGateway();

		  for (NodeContainer::Iterator it = daps.Begin(); it != daps.End(); it++)
			{
			  if (bestDapAddress == GetIpAddressForOlsrNode(*it))
				{
				  failingDap = *it;
				}
			}
		}
    }

  if (!failingDap)
    {
      failingDap = GetRandomFailingNode();
    }

  return failingDap;

}

void
NeighborhoodAmiSim::ExecuteFailure()
{
  Ptr<Node> failingDap = 0;

  failingDap = ddsaEnabled ? GetDdsaFailingNode() : GetNonDdsaFailingNode();


  if (!failingDap)
    {
      failingDap = GetRandomFailingNode();
    }

  Ptr<ddsa::Ipv4L3ProtocolDdsaAdapter> l3Prot = failingDap->GetObject<ddsa::Ipv4L3ProtocolDdsaAdapter>();

  if (l3Prot)
    {
      l3Prot->MakeFail();
      l3Prot->SetFailureType(static_cast<Ipv4L3ProtocolDdsaAdapter::FailureType>(failureMode));
      NS_LOG_UNCOND ("Node " << failingDap->GetId() << " is not working(t = " << Simulator::Now().GetSeconds() << "s)");
    }


    if (failureMode == 1)
      {
		Ptr<ddsa::DdsaRoutingProtocolAdapter> rp = failingDap->GetObject<ddsa::DdsaRoutingProtocolAdapter>();
		if (rp)
		  {
			rp->SetMalicious(true);
		  }
      }
}

//Statistics

double
NeighborhoodAmiSim::CalculateDeliveryRate()
{
  int totalReceived = 0;
  double totalSent = 0;

  for (std::map<uint16_t, std::map<uint16_t, AmiPacketInfo> >::iterator it = packetsSent.begin(); it != packetsSent.end(); it++)
    {
      std::map<uint16_t, AmiPacketInfo> sent = it->second;
      totalSent+= sent.size();

      for (std::map<uint16_t, AmiPacketInfo>::iterator info = sent.begin(); info != sent.end(); info++)
		{
		  if (info->second.Received())
			{
			  totalReceived++;
			}
		}
    }

  return 100 * totalReceived / totalSent;
}

double
NeighborhoodAmiSim::CalculateLossRate()
{
  return 100 - CalculateDeliveryRate();
}

double
NeighborhoodAmiSim::CalculateDapSelection(const Ipv4Address & dapAddress)
{
  std::vector<AmiPacketInfo> infos = dapSelectionHistory[dapAddress];

  int sum = 0;

  for (std::vector<AmiPacketInfo>::iterator it = infos.begin(); it != infos.end(); it++)
    {
      sum += it->GetNReceived();
    }

  return sum > 0 ? (sum / (double)totalSentCopies) * 100 : 0;
}

void
NeighborhoodAmiSim::GenerateControllerReceptionStatistics()
{
  std::ofstream statFile;
  statFile.open("packets.txt");


  for (std::map<uint16_t, std::map<uint16_t, AmiPacketInfo> >::iterator it = packetsSent.begin(); it != packetsSent.end(); it++)
    {

      std::map<uint16_t, AmiPacketInfo> sent = it->second;

      statFile << "Packets sent by meter " << it->first << "\n\n";

      for (std::map<uint16_t, AmiPacketInfo>::iterator info = sent.begin(); info != sent.end(); info++)
		{
		  statFile << "Packet " << info->second.GetSeqNumber() << " -----------------------";
		  statFile << (info->second.Received() ? "\n" : "(Lost)\n");

		  statFile << "Sent at: " << info->second.GetLatency().time_sent.GetSeconds() << " seconds\n";

		  if (info->second.Received())
			{
			  int64_t latency = info->second.GetLatency().time_received.GetMilliSeconds() - info->second.GetLatency().time_sent.GetMilliSeconds();

			  statFile << "Received at: " << info->second.GetLatency().time_received.GetSeconds() << " seconds";

			  if (withFailure && info->second.GetLatency().time_received >= failureTime)
				{
				  statFile << "(After failure)\n";
				}
			  else
				{
				  statFile << "\n";
				}

			  statFile << "Latency: " << latency << "ms \n";
			  statFile << "Received copies: " << info->second.GetNReceived() << "\n\n";
			}
		}
    }

  statFile.close();
}

double
NeighborhoodAmiSim::CalculateMeanCost(const Ipv4Address & dapAddress)
{
  if (costHistory.find(dapAddress) != costHistory.end())
    {
      std::vector<Dap> costs = costHistory[dapAddress];
      double sum = 0;
      for(std::vector<ddsa::Dap>::iterator it = costs.begin(); it != costs.end(); it++)
		{
		  sum+= it->cost;
		}

      return sum / costs.size();
    }

  return -1;
}

double
NeighborhoodAmiSim::CalculateMeanLatency()
{
  int64_t sum = 0;
  int totalReceived = 0;

  for (std::map<uint16_t, std::map<uint16_t, AmiPacketInfo> >::iterator it = packetsSent.begin(); it != packetsSent.end(); it++)
    {
      std::map<uint16_t, AmiPacketInfo> sent = it->second;

      for (std::map<uint16_t, AmiPacketInfo>::iterator info = sent.begin(); info != sent.end(); info++)
		{
		  if (info->second.Received())
			{
			  int64_t latency = info->second.GetLatency().time_received.GetMilliSeconds() - info->second.GetLatency().time_sent.GetMilliSeconds();
			  sum+= latency;
			  totalReceived++;
			}
		}
    }

  return (double)sum / totalReceived;
}

double
NeighborhoodAmiSim::CalculateMeanProbability(const Ipv4Address & dapAddress)
{
  if (costHistory.find(dapAddress) != costHistory.end())
    {
      std::vector<Dap> costs = costHistory[dapAddress];
      double sum = 0;
      for(std::vector<ddsa::Dap>::iterator it = costs.begin(); it != costs.end(); it++)
		{
		  sum+= it->probability;
		}

      return sum / costs.size();
    }

  return -1;
}

int
NeighborhoodAmiSim::GetTotalSentCopies()
{
  int total = 0;

  for (std::map<uint16_t, std::map<uint16_t, AmiPacketInfo> >::iterator it = packetsSent.begin(); it != packetsSent.end(); it++)
    {
      std::map<uint16_t, AmiPacketInfo> sent = it->second;

      for (std::map<uint16_t, AmiPacketInfo>::iterator info = sent.begin(); info != sent.end(); info++)
		  {
		  	  total+= info->second.GetNSent();
		  }
    }

  return total;
}

void
NeighborhoodAmiSim::GenerateDapStatistics()
{
  std::ofstream statFile;

  int total = ddsaEnabled ? totalSentCopies : GetTotalSentCopies();

  for(std::map<Ipv4Address, std::vector<AmiPacketInfo> >::iterator it = dapSelectionHistory.begin();
      it != dapSelectionHistory.end(); it++)
    {
      std::ostringstream stream;
      int totalCopies = 0;

      stream << "dap(";
      it->first.Print(stream);
      stream << ").txt";

      std::string fileName = stream.str();

      statFile.open(fileName);

      statFile << "Dap " << it->first << " ( selection rate = " << CalculateDapSelection(it->first) << " )\n\n";

      for (std::vector<AmiPacketInfo>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
		{
		  statFile << "seq " << it2->GetSeqNumber() << " ( " << it2->GetNReceived() << " )\n";
		  totalCopies += it2->GetNReceived();
		}

      statFile << "Total copies: " << totalCopies << "(" << totalCopies * 100.0 / total << "%)\n";
      statFile << "Average cost: " << CalculateMeanCost(it->first) << "\n";
      statFile << "Average probability: " << CalculateMeanProbability(it->first) << "\n";
      statFile.close();
    }
}

void
NeighborhoodAmiSim::GenerateGeneralStatistics()
{
  std::ofstream statFile;

  std::string fileName = "statistics";

  if (ddsaEnabled)
    {
      fileName += "_ddsa";
    }
  else
    {
      fileName += "_nonddsa";
    }

  if (withFailure)
    {
      fileName += "_failure";
    }

  fileName += ".txt";

  statFile.open (fileName);
  statFile << "Statistics generated from the AmiGrid simulation.\n";
  statFile << "d_rate:" << CalculateDeliveryRate() << "%" << "//Delivery rate" << std::endl;
  statFile << "l_rate: " << CalculateLossRate() << "%" << "//Loss rate" << std::endl;
  statFile << "l_avg: " << CalculateMeanLatency() << " ms" << "//Average latency" << std::endl;

  statFile.close();
}

Ptr<NetDevice>
NeighborhoodAmiSim::GetDeviceForNode(Ptr<Node> node, NetDeviceContainer devices)
{
  for (uint32_t i = 0; i < devices.GetN (); ++i)
    {
      Ptr<NetDevice> device = devices.Get (i);

      if (device->GetNode()->GetId() == node->GetId())
		{
    	  return device;
		}
    }

    return NULL;
}

Ipv4Address
NeighborhoodAmiSim::GetIpAddressForNode(Ptr<Node> node, Ptr<NetDevice> device)
{
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

  int32_t interface = ipv4->GetInterfaceForDevice(device);

  return ipv4->GetAddress(interface, 0).GetLocal();
}

Ipv4Address
NeighborhoodAmiSim::GetIpAddressForCsmaNode(Ptr<Node> node)
{
  Ptr<NetDevice> device = GetDeviceForNode(node, csmaDevices);

  return GetIpAddressForNode(node, device);
}

Ipv4Address
NeighborhoodAmiSim::GetIpAddressForOlsrNode(Ptr<Node> node)
{
  Ptr<NetDevice> device = GetDeviceForNode(node, olsrDevices);

  return GetIpAddressForNode(node, device);
}

int main (int argc, char *argv[])
{
  NeighborhoodAmiSim simulation;

  simulation.Configure(argc, argv);
  simulation.CreateNodes();
  simulation.CreateTopology();

  simulation.ConfigureWifi();
  simulation.CreateControllerLan();
  simulation.ConfigureNodesStack();
  simulation.ConfigureIpAddressing();

  uint16_t port = 80;
  Time metersStart = Seconds(100.0);
  Time metersStop = Seconds(200.0);

  simulation.ConfigureMeterApplication(port, metersStart, metersStop);
  simulation.ConfigureControllerSocket(port);
  simulation.ConfigureDapSocket(port);
  simulation.SheduleFailure(150);

  //Log
  LogComponentEnable("LqOlsrRoutingProtocol", LOG_LEVEL_DEBUG);
  //LogComponentEnable("DdsaRoutingProtocolAdapter", LOG_LEVEL_DEBUG);
  //LogComponentEnable("Etx", LOG_LEVEL_DEBUG);
  //LogComponentEnable("MaliciousEtx", LOG_LEVEL_DEBUG);
  //LogComponentEnable("YansWifiPhy", LOG_LEVEL_DEBUG);
  //LogComponentEnable("Ipv4L3ProtocolDdsaAdapter", LOG_LEVEL_DEBUG);
  //LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_ALL);

  Simulator::Stop (Seconds (250.0));
  Simulator::Run ();

  if (simulation.IsDdsaEnabled())
    {
      simulation.GenerateDapStatistics();
    }

  simulation.GenerateGeneralStatistics();
  simulation.GenerateControllerReceptionStatistics();

  Simulator::Destroy ();

  return 0;
}
