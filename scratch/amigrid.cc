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
#include "ns3/integer.h"
#include "ns3/simple-header.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstddef>
#include <numeric>

using namespace ns3;
using namespace ns3::lqmetric;
using namespace ns3::lqolsr;
using namespace ns3::ddsa;
using namespace ns3::ami;

NS_LOG_COMPONENT_DEFINE ("amigrid");

struct AmiPacketInfo
{
  private:
    uint16_t seqNumber;
    int nReceived;
  public:

    AmiPacketInfo()
    {
      seqNumber = 0;
      nReceived = 0;
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
};



/*
 * AmiGridSim.cpp
 *
 *  Created on: 13 de jun de 2017
 *      Author: igor
 */

class AmiGridSim
{
  public:
    AmiGridSim ();
    virtual
    ~AmiGridSim ();
    void Configure(int argc, char *argv[]);
    Ptr<ListPositionAllocator> CreateMetersPosition();
    Ptr<ListPositionAllocator> CreateDapsPosition();
    void InstallMobilityModel(Ptr<ListPositionAllocator> metersPosition, Ptr<ListPositionAllocator> dapsPosition);
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
    void SheduleFailure();
    void ExecuteFailure(Ptr<Node> node);
    void GenerateDapStatistics();

  private:
    void Parse(int argc, char *argv[]);
    void ConfigureStack(LqOlsrHelper & lqOlsrDapHelper, NodeContainer nodes, Ipv4L3ProtocolDdsaAdapter::NodeType nType );
    void ConfigureDapStack(LqOlsrHelper & helper);
    void ConfigureMeterStack(LqOlsrHelper & helper);
    Ipv4ListRoutingHelper ConfigureRouting(const Ipv4RoutingHelper & ipv4Routing);
    void ConfigureControllerStack();
    void ConfigureDapsHna();

    void ApplicationPacketSent(Ptr<const Packet>);
    void ControllerReceivePacket (Ptr<Socket> socket);
    void SendPacketToController(Ptr<Packet> packet, Ptr<Node> node);

    void DapReceivePacket (Ptr<Socket> socket);
    double CalculateDapSelection(const Ipv4Address & dapAddress);
    void DapReceivedPacket(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interfaceId);
    double CalculateDeliveryRate();
    double CalculateLossRate();
    Ipv4Address GetNodeAddress(const Ptr<NetDevice> & device, const Ptr<Node> node);
    void DapSelectedToNewCopy(const Ipv4Address & dapAddress, Ptr<const Packet> packet);
    void UpdateDapSelectionHistory(const Ipv4Address & address, uint16_t seqNumber);

    std::string phyMode;
    uint32_t packetSize; // bytes
    uint32_t totalDaps; //Total number of Daps
    bool verbose;
    bool assocMethod1;
    bool assocMethod2;
    bool withFailure;
    bool ddsaEnabled;
    CommandLine cmd;
    NodeContainer daps;
    NodeContainer meters;
    NodeContainer controllers;
    double gridXShift; // Horizontal distance between two meters in the grid
    double gridYShift; // Vertical distance between two meters in the grid
    int gridColumns;
    int gridRows;
    int senderNodeIndex;
    Ptr<LogDistancePropagationLossModel> myLossModel;
    YansWifiPhyHelper myWifiPhy;
    Ipv4InterfaceContainer olsrIpv4Devices;
    Ipv4InterfaceContainer csmaIpv4Devices;
    EventGarbageCollector m_events;
    NetDeviceContainer olsrDevices;
    NetDeviceContainer csmaDevices;

    //Statistics
    int totalSentApplicationPackets;
    int totalSentCopies;
    std::vector<uint16_t> receivedPackets;

    std::map<Ipv4Address, std::vector<AmiPacketInfo> > dapSelectionHistory;
};

AmiGridSim::AmiGridSim (): phyMode ("DsssRate1Mbps")
{
    packetSize = 400;
    totalDaps = 3;
    verbose = false;
    assocMethod1 = false;
    assocMethod2 = true;
    gridXShift = 10.0;
    gridYShift = 20.0;
    gridColumns = 3;//12;
    gridRows = 1;//3;
    myLossModel = CreateObject<LogDistancePropagationLossModel> ();
    myWifiPhy = YansWifiPhyHelper::Default();
    senderNodeIndex = -1;
    withFailure = false;
    ddsaEnabled = true;
    totalSentApplicationPackets = 0;
    totalSentCopies = 0;
}

AmiGridSim::~AmiGridSim ()
{
  // TODO Auto-generated destructor stub
}


void
AmiGridSim::Parse(int argc, char *argv[])
{
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("totalDaps", "Total number of Daps", totalDaps);
  cmd.AddValue ("gridXShift", "Horizontal distance between two meters in the grid", gridXShift);
  cmd.AddValue ("gridYShift", "Vertical distance between two meters in the grid ", gridYShift);
  cmd.AddValue ("gridColumns", "Number of columns in the grid ", gridColumns);
  cmd.AddValue ("gridRows", "Number of rows in the grid ", gridRows);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("assocMethod1", "Use SetRoutingTableAssociation () method", assocMethod1);
  cmd.AddValue ("assocMethod2", "Use AddHostNetworkAssociation () method", assocMethod2);
  cmd.AddValue ("failure", "Cause the failure of one DAP", withFailure);
  cmd.AddValue ("ddsaenabled", "Enable ddsa", ddsaEnabled);
  cmd.AddValue ("sender", "Sender index. If negative, all meters will be configured as senders (Default).", senderNodeIndex);

  cmd.Parse (argc, argv);
}

//Simulation Configuration

void
AmiGridSim::Configure(int argc, char *argv[])
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
AmiGridSim::CreateNodes()
{
  daps.Create(totalDaps); // Creates the specified number of daps
  meters.Create(gridRows * gridColumns); //Creates the Specified NUmber of meters
  controllers.Create(1); // Creates one controller.
}
Ptr<ListPositionAllocator>
AmiGridSim::CreateMetersPosition()
{
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (int row = 0; row < gridRows; row++)
    {
      int y = row * gridYShift;

      for (int col = 0; col < gridColumns; col++)
	{
	  positionAlloc->Add (Vector (col * gridXShift, y, 0.0));
	}
    }

  return positionAlloc;
}

Ptr<ListPositionAllocator>
AmiGridSim::CreateDapsPosition()
{
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  double y = -15.0;
  double columnsPerDap = gridColumns / (double)totalDaps;
  double x = ((columnsPerDap - 1) * gridXShift) / 2;

  for (uint32_t dap = 0; dap < totalDaps; dap++)
    {
      positionAlloc->Add (Vector (x, y, 0.0));

      x += (columnsPerDap * gridXShift);
    }

  positionAlloc->Add (Vector ((x + 10) * -1, y, 0.0));

  return positionAlloc;
}

void
AmiGridSim::InstallMobilityModel(Ptr<ListPositionAllocator> metersPosition, Ptr<ListPositionAllocator> dapsPosition)
{
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (metersPosition);
  mobility.Install (meters);

  mobility.SetPositionAllocator (dapsPosition);
  mobility.Install (NodeContainer(daps, controllers));
}

void
AmiGridSim::ConfigureWifi()
{
  Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
  wifiChannel->SetPropagationLossModel (myLossModel);
  wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue (phyMode), "ControlMode",StringValue (phyMode));

  if (verbose)
  {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
  }

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
AmiGridSim::CreateControllerLan()
{
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  csmaDevices = csma.Install (NodeContainer (controllers, daps));
  csma.EnablePcap ("csma", csmaDevices, false);
}

void
AmiGridSim::ConfigureStack(LqOlsrHelper & lqOlsrDapHelper, NodeContainer nodes, Ipv4L3ProtocolDdsaAdapter::NodeType nType )
{
  Ipv4ListRoutingHelper ipv4Routing = ConfigureRouting(lqOlsrDapHelper);
  DdsaInternetStackHelper ddsa;
  ddsa.SetRoutingHelper (ipv4Routing); // has effect on the next Install ()
  ddsa.SetNodeType(nType);
  ddsa.Install (nodes);
}

void
AmiGridSim::ConfigureDapStack(LqOlsrHelper & helper)
{
  for (uint32_t i = 0; i < totalDaps; i++)
    {
      helper.ExcludeInterface (daps.Get (i), 2);
    }

  ConfigureStack(helper, daps, Ipv4L3ProtocolDdsaAdapter::NodeType::DAP);

  if (!ddsaEnabled)
    {
      for (NodeContainer::Iterator it = daps.Begin(); it != daps.End(); it++)
	{
	  Ptr<ns3::ddsa::Ipv4L3ProtocolDdsaAdapter> ipv4L3 = (*it)->GetObject<ddsa::Ipv4L3ProtocolDdsaAdapter>();
	  NS_ASSERT (ipv4L3);

	  ipv4L3->TraceConnectWithoutContext("Rx", MakeCallback(&AmiGridSim::DapReceivedPacket, this));
	}
    }
}

void
AmiGridSim::ConfigureMeterStack(LqOlsrHelper & helper)
{
  Ipv4L3ProtocolDdsaAdapter::NodeType mType = ddsaEnabled ? Ipv4L3ProtocolDdsaAdapter::NodeType::METER :
							    Ipv4L3ProtocolDdsaAdapter::NodeType::NON_DDSA;
  ConfigureStack(helper, meters, mType);

  if (ddsaEnabled)
     {
       for (NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
 	{
 	  Ptr<ns3::ddsa::Ipv4L3ProtocolDdsaAdapter> ipv4L3 = (*it)->GetObject<ddsa::Ipv4L3ProtocolDdsaAdapter>();
 	  NS_ASSERT (ipv4L3);

 	  ipv4L3->TraceConnectWithoutContext("DapSelection", MakeCallback(&AmiGridSim::DapSelectedToNewCopy, this));
 	}
     }
}

void
AmiGridSim::ConfigureControllerStack()
{
  InternetStackHelper internet;

  internet.Install (controllers);
}

Ipv4ListRoutingHelper
AmiGridSim::ConfigureRouting(const Ipv4RoutingHelper & ipv4Routing)
{
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (ipv4Routing, 10);

  return list;
}

void
AmiGridSim::ConfigureDapsHna()
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

	if (assocMethod1)
	{
		// Create a special Ipv4StaticRouting instance for RoutingTableAssociation
		// Even the Ipv4StaticRouting instance added to list may be used
		Ptr<Ipv4StaticRouting> hnaEntries = Create<Ipv4StaticRouting> ();

		// Add the required routes into the Ipv4StaticRouting Protocol instance
		// and have the node generate HNA messages for all these routes
		// which are associated with non-OLSR interfaces specified above.
		hnaEntries->AddNetworkRouteTo (Ipv4Address ("172.16.1.0"), Ipv4Mask ("255.255.255.0"), uint32_t (2), uint32_t (1));
		lqolsr_rp->SetRoutingTableAssociation (hnaEntries);
	}

	if (assocMethod2)
	{
		// Specify the required associations directly.
	    lqolsr_rp->AddHostNetworkAssociation (Ipv4Address ("172.16.1.0"), Ipv4Mask ("255.255.255.0"));
	}
    }
}

void
AmiGridSim::ConfigureNodesStack()
{
  TypeId metricTid = Etx::GetTypeId();

  LqOlsrHelper helper(metricTid);
  ConfigureDapStack(helper);

  DDsaHelper meterHelper(metricTid);
  meterHelper.Set("Retrans", IntegerValue(10));
  ConfigureMeterStack(meterHelper);

  ConfigureControllerStack();
  ConfigureDapsHna();
}

void
AmiGridSim::ConfigureIpAddressing()
{
  NS_LOG_INFO ("Assign IP Addresses.");

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  olsrIpv4Devices = ipv4.Assign (olsrDevices);

  ipv4.SetBase (Ipv4Address("172.16.1.0"), "255.255.255.0");
  csmaIpv4Devices = ipv4.Assign (csmaDevices);
}

void
AmiGridSim::ConfigureMeterApplication(uint16_t port, Time start, Time stop)
{

  AmiAppHelper amiHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (csmaIpv4Devices.GetAddress(0), port)));

  //OnOffHelper onOff ("ns3::UdpSocketFactory", Address (InetSocketAddress (csmaIpv4Devices.GetAddress(0), port)));
  //onOff.SetConstantRate (DataRate ("960bps"), packetSize);
  NodeContainer nodes;

  if (senderNodeIndex > 0 && senderNodeIndex < (int)meters.GetN())
    {
      nodes.Add(meters.Get(senderNodeIndex));
    }
  else
    {
      nodes.Add(meters);
    }

  ApplicationContainer apps = amiHelper.Install(nodes);; //onOff.Install(nodes);

  Ptr<Application> app = apps.Get(0);

  AmiApplication * amiApp = dynamic_cast<AmiApplication*>(&(*app));

  //OnOffApplication * appOnOff = dynamic_cast<OnOffApplication*>(&(*app));


  if (amiApp)
    {
      amiApp->TraceConnectWithoutContext("Tx", MakeCallback(&AmiGridSim::ApplicationPacketSent, this));
    }

  apps.Start (start);
  apps.Stop (stop);
}

void
AmiGridSim::ConfigureControllerSocket(uint16_t port)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);

  Ptr<Socket> recvController = Socket::CreateSocket (controllers.Get (0), tid);
  recvController->Bind (local);
  recvController->SetRecvCallback (MakeCallback (&AmiGridSim::ControllerReceivePacket, this));
}

void
AmiGridSim::ConfigureDapSocket(uint16_t port)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);

  for (NodeContainer::Iterator it = daps.Begin(); it != daps.End(); it++)
    {
      Ptr<Socket> recvDap = Socket::CreateSocket (*it, tid);
      recvDap->Bind (local);
      recvDap->SetRecvCallback (MakeCallback (&AmiGridSim::DapReceivePacket, this));
    }
}

void
AmiGridSim::DapReceivePacket (Ptr<Socket> socket)
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
AmiGridSim::DapReceivedPacket(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interfaceId)
{
  Ipv4Address address = ipv4->GetAddress(interfaceId, 0).GetLocal();
  Ipv4Mask mask = ipv4->GetAddress(interfaceId, 0).GetMask();

  Ipv4Header ipHeader;
  packet->PeekHeader (ipHeader);

  if (ipHeader.GetDestination().IsSubnetDirectedBroadcast(mask))
    {
      return;
    }

  AmiHeader amiHeader;

  if (packet->PeekHeader(amiHeader) != 0)
    {
      uint16_t seqNumber = amiHeader.GetPacketSequenceNumber();
      NS_LOG_UNCOND ("Dap " << address << ": Received packet " << seqNumber << " at " << Simulator::Now().GetSeconds() << " seconds.");
    }
}

//Statistics Generation

void
AmiGridSim::UpdateDapSelectionHistory(const Ipv4Address & address, uint16_t seqNumber)
{
  std::vector<AmiPacketInfo> infos;

  if (dapSelectionHistory.find(address) == dapSelectionHistory.end())
    {
      dapSelectionHistory[address] = infos;
    }
  else
    {
      infos = dapSelectionHistory[address];
    }

  bool found = false;
  std::vector<AmiPacketInfo>::iterator it = infos.begin();
  while (it++ != infos.end() && !found)
    {
      if (it->GetSeqNumber() == seqNumber)
	{
	  it->IncrementReceived();
	  found = true;
	}
    }

  if (!found)
    {
      AmiPacketInfo info;
      info.SetSeqNumber(seqNumber);

      infos.push_back(info);
    }
}

void
AmiGridSim::ApplicationPacketSent(Ptr<const Packet> packetSent)
{
  AmiHeader amiHeader;
  if (packetSent->PeekHeader(amiHeader) != 0)
    {
      totalSentApplicationPackets++;
    }
  else
    {
      NS_LOG_UNCOND ("ERROR!");
    }
}

void
AmiGridSim::ControllerReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      AmiHeader amiHeader;

      if (packet->PeekHeader(amiHeader) != 0)
	{
	  uint16_t seqNumber = amiHeader.GetPacketSequenceNumber();
	  NS_LOG_UNCOND ("Controller: Received packet " << seqNumber << " at " << Simulator::Now().GetSeconds());

	  if (std::find(receivedPackets.begin(), receivedPackets.end(), seqNumber) == receivedPackets.end())
	    {
	      receivedPackets.push_back(seqNumber);
	    }
	}
      else
	{
	  NS_LOG_UNCOND ("Controller: Received non-ami packet at " << Simulator::Now().GetSeconds());
	}
    }
}


void
AmiGridSim::DapSelectedToNewCopy(const Ipv4Address & dapAddress, Ptr<const Packet> packet)
{
  AmiHeader amiHeader;

  if (packet->PeekHeader(amiHeader) != 0)
  {
      uint16_t seqNumber = amiHeader.GetPacketSequenceNumber();
      NS_LOG_UNCOND ("Dap " << dapAddress << " selected for packet " << seqNumber);
      UpdateDapSelectionHistory(dapAddress, seqNumber);
      totalSentCopies++;
  }
}

double
AmiGridSim::CalculateDeliveryRate()
{
  return 100 * receivedPackets.size() / (double)totalSentApplicationPackets;
}

double
AmiGridSim::CalculateLossRate()
{
  return 100 - CalculateDeliveryRate();
}

double
AmiGridSim::CalculateDapSelection(const Ipv4Address & dapAddress)
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
AmiGridSim::GenerateDapStatistics()
{
  std::ofstream statFile;

  for(std::map<Ipv4Address, std::vector<AmiPacketInfo> >::iterator it = dapSelectionHistory.begin();
      it != dapSelectionHistory.end(); it++)
    {
      std::ostringstream stream;

      stream << "dap(";
      it->first.Print(stream);
      stream << ").txt";

      std::string fileName = stream.str();

      statFile.open(fileName);

      statFile << "Dap " << it->first << " ( selection rate = " << CalculateDapSelection(it->first) << "\n\n";

      for (std::vector<AmiPacketInfo>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
	{
	  statFile << "seq " << it2->GetSeqNumber() << " ( " << it2->GetNReceived() << " )\n";
	}

      statFile.close();
    }
}

void
AmiGridSim::GenerateGeneralStatistics()
{
  std::ofstream statFile;

  std::string fileName = "statistics";

  if (ddsaEnabled)
    {
      fileName += "_ddsa";
    }

  if (withFailure)
    {
      fileName += "_failure";
    }

  fileName += ".txt";

  statFile.open (fileName);
  statFile << "Statistics generated from the AmiGrid simulation.\n";
  statFile << "Number of application packets sent: " << totalSentApplicationPackets << std::endl;
  statFile << "Number of total copies sent: " << totalSentCopies << std::endl;
  statFile << "Number of packets received: " << receivedPackets.size() << std::endl;
  statFile << "Delivery rate: " << CalculateDeliveryRate() << "%" << std::endl;
  statFile << "Loss rate: " << CalculateLossRate() << "%" << std::endl;

  statFile.close();
}

//Failure generation

void
AmiGridSim::SheduleFailure()
{
  if (withFailure)
    {
      m_events.Track (Simulator::Schedule (Seconds (80), &AmiGridSim::ExecuteFailure, this, daps.Get(1)));
    }
}

void
AmiGridSim::ExecuteFailure(Ptr<Node> node)
{
  NS_LOG_UNCOND ("Node " << node->GetId() << " is not working(t = " << Simulator::Now().GetSeconds() << "s)");

  Ptr<ddsa::Ipv4L3ProtocolDdsaAdapter> l3Prot = node->GetObject<ddsa::Ipv4L3ProtocolDdsaAdapter>();

  if (l3Prot)
    {
      l3Prot->MakeFail();
    }
}

void
AmiGridSim::SendPacketToController(Ptr<Packet> packet, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source = Socket::CreateSocket (node, tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("172.16.1.1"), 80);
  source->Connect (remote);
  source->Send (packet);
}

int main (int argc, char *argv[])
{
  AmiGridSim simulation;

  simulation.Configure(argc, argv);
  simulation.CreateNodes();

  Ptr<ListPositionAllocator> metersPositionAlloc = simulation.CreateMetersPosition();
  Ptr<ListPositionAllocator> dapsPositionAlloc = simulation.CreateDapsPosition();

  simulation.InstallMobilityModel(metersPositionAlloc, dapsPositionAlloc);
  simulation.ConfigureWifi();
  simulation.CreateControllerLan();
  simulation.ConfigureNodesStack();
  simulation.ConfigureIpAddressing();

  uint16_t port = 80;
  Time metersStart = Seconds(20.0);
  Time metersStop = Seconds(190.0);

  simulation.ConfigureMeterApplication(port, metersStart, metersStop);
  simulation.ConfigureControllerSocket(port);
  simulation.ConfigureDapSocket(port);
  simulation.SheduleFailure();

  //Log
  //LogComponentEnable("LqOlsrRoutingProtocol", LOG_LEVEL_DEBUG);
  //LogComponentEnable("DdsaRoutingProtocolAdapter", LOG_LEVEL_DEBUG);
  //LogComponentEnable("Etx", LOG_LEVEL_ALL);
  //LogComponentEnable("OnOffApplication", LOG_LEVEL_FUNCTION);
  //LogComponentEnable("DdsaRoutingProtocolDapAdapter", LOG_LEVEL_DEBUG);

  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();
  Simulator::Destroy ();
  simulation.GenerateDapStatistics();
  simulation.GenerateGeneralStatistics();
  return 0;
}



