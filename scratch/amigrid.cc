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
#include <iostream>
#include <fstream>
#include <vector>
#include <cstddef>
#include <numeric>

using namespace ns3;
using namespace ns3::lqmetric;
using namespace ns3::lqolsr;

NS_LOG_COMPONENT_DEFINE ("amigrid");



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
    NetDeviceContainer ConfigureWifi();
    NetDeviceContainer CreateControllerLan();
    void ConfigureRouting();
    void ConfigureIpAddressing(const NetDeviceContainer & olsrDevices, const NetDeviceContainer & csmaDevices);
    void ConfigureMeterApplication(uint16_t port, Time start, Time stop);
    void ConfigureControllerSocket(uint16_t port);
    void ConfigureDapSocket(uint16_t port);
    //static void ReceivePacket (Ptr<Socket> socket);
    void CreateNodes();
    void PrintStatistics();
    void SheduleFailure();
    void ExecuteFailure(Ptr<Node> node);

  private:
    void Parse(int argc, char *argv[]);
    void PacketSent(Ptr<const Packet>);
    void ControllerReceivePacket (Ptr<Socket> socket);
    void SendPacketToController(Ptr<Node> node);
    void DapReceivePacket (Ptr<Socket> socket);
    double CalculateDapSelection(int dapIndex);

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
    int packetsSent;
    int packetsReceived;
    Ptr<LogDistancePropagationLossModel> myLossModel;
    YansWifiPhyHelper myWifiPhy;
    Ipv4InterfaceContainer olsrIpv4Devices;
    Ipv4InterfaceContainer csmaIpv4Devices;
    EventGarbageCollector m_events;
    std::vector<int> dapSelectionHistory;
};

AmiGridSim::AmiGridSim (): phyMode ("DsssRate1Mbps")
{
    packetSize = 400;
    totalDaps = 3;
    verbose = false;
    assocMethod1 = false;
    assocMethod2 = true;
    gridXShift = 10.0;
    gridYShift = 10.0;
    gridColumns = 3;//12;
    gridRows = 1;//3;
    myLossModel = CreateObject<LogDistancePropagationLossModel> ();
    myWifiPhy = YansWifiPhyHelper::Default();
    packetsSent = 0;
    packetsReceived = 0;
    withFailure = false;
    ddsaEnabled = true;
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

  cmd.Parse (argc, argv);
}

void
AmiGridSim::Configure(int argc, char *argv[])
{
  Parse(argc, argv);

  for (uint32_t i = 0; i < totalDaps; i++)
    {
      dapSelectionHistory.push_back(0);
    }

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

NetDeviceContainer
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
  NetDeviceContainer devices = wifi.Install (myWifiPhy, wifiMac, allOlsrNodes);
  myWifiPhy.EnablePcap ("olsr-hna", devices);

  return devices;
}

NetDeviceContainer
AmiGridSim::CreateControllerLan()
{
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  NetDeviceContainer csmaDevices = csma.Install (NodeContainer (controllers, daps));
  csma.EnablePcap ("csma", csmaDevices, false);

  return csmaDevices;
}

//void
//AmiGridSim::ConfigureDapStack(TypeId metricTid)
//{
//  LqOlsrHelper lqOlsrDapHelper = ddsaEnabled ? DDsaHelper(metricTid) : LqOlsrHelper(metricTid) ;
//
//  for (uint32_t i = 0; i < totalDaps; i++)
//    {
//      lqOlsrDapHelper.ExcludeInterface (daps.Get (i), 2);
//    }
//
//  Ipv4ListRoutingHelper list = ConfigureRouting(lqOlsrDapHelper);
//  ConfigureInternetStack(list, daps);
//}
//
//Ipv4ListRoutingHelper
//AmiGridSim::ConfigureRouting(const Ipv4RoutingHelper & ipv4Routing)
//{
//  Ipv4StaticRoutingHelper staticRouting;
//  Ipv4ListRoutingHelper list;
//  list.Add (staticRouting, 0);
//  list.Add (ipv4Routing, 10);
//}
//
//void
//AmiGridSim::ConfigureInternetStack(const Ipv4RoutingHelper & ipv4Routing, const NodeContainer & nodes)
//{
//  InternetStackHelper internet;
//  internet.SetRoutingHelper (ipv4Routing); // has effect on the next Install ()
//  internet.Install (nodes);
//}
//
//void
//AmiGridSim::ConfigureDdsaInternetStack(const Ipv4RoutingHelper & ipv4Routing, NodeContainer nodes, ns3::ddsa::Ipv4L3ProtocolDdsaAdapter::NodeType nodeType)
//{
//  DdsaInternetStackHelper ddsa_internet;
//  ddsa_internet.SetRoutingHelper (ipv4Routing); // has effect on the next Install ()
//  ddsa_internet.SetNodeType(nodeType);
//  ddsa_internet.Install (nodes);
//}

void
AmiGridSim::ConfigureRouting()
{
  TypeId metricTid = Etx::GetTypeId();
  DDsaHelper ddsaMeterHelper(metricTid);

  LqOlsrHelper ddsaDapHelper(metricTid);

  for (uint32_t i = 0; i < totalDaps; i++)
    {
      ddsaDapHelper.ExcludeInterface (daps.Get (i), 2);
    }

  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper listMeters;
  listMeters.Add (staticRouting, 0);
  listMeters.Add (ddsaMeterHelper, 10);

  Ipv4ListRoutingHelper listDaps;
  listDaps.Add (staticRouting, 0);
  listDaps.Add (ddsaDapHelper, 10);

  DdsaInternetStackHelper internet_meters;
  internet_meters.SetRoutingHelper (listMeters); // has effect on the next Install ()
  internet_meters.SetNodeType(ns3::ddsa::Ipv4L3ProtocolDdsaAdapter::NodeType::METER);
  internet_meters.Install (meters);

  DdsaInternetStackHelper internet_daps;
  internet_daps.SetRoutingHelper (listDaps); // has effect on the next Install ()
  internet_daps.SetNodeType(ns3::ddsa::Ipv4L3ProtocolDdsaAdapter::NodeType::DAP);
  internet_daps.Install (daps);

  InternetStackHelper internet_controller;
  internet_controller.Install (controllers);

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
AmiGridSim::ConfigureIpAddressing(const NetDeviceContainer & olsrDevices, const NetDeviceContainer & csmaDevices)
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
  OnOffHelper onOff ("ns3::UdpSocketFactory", Address (InetSocketAddress (csmaIpv4Devices.GetAddress(0), port)));
  onOff.SetConstantRate (DataRate ("960bps"), packetSize);
  ApplicationContainer apps = onOff.Install(NodeContainer(meters.Get(3)));

  Ptr<Application> app = apps.Get(0);

  OnOffApplication * appOnOff = dynamic_cast<OnOffApplication*>(&(*app));

  if (appOnOff)
    {
      appOnOff->TraceConnectWithoutContext("Tx", MakeCallback(&AmiGridSim::PacketSent, this));
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
AmiGridSim::PacketSent(Ptr<const Packet> packetSent)
{
  NS_LOG_UNCOND ("Packet Sent!");
  packetsSent++;
}

void
AmiGridSim::ControllerReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      NS_LOG_UNCOND ("Controller: Received one packet at " << Simulator::Now().GetSeconds() << " seconds.");
      packetsReceived++;
    }
}

void
AmiGridSim::SendPacketToController(Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source = Socket::CreateSocket (node, tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("172.16.1.1"), 80);
  source->Connect (remote);
  source->Send (Create<Packet> (packetSize));
}

void
AmiGridSim::DapReceivePacket (Ptr<Socket> socket)
{
  NS_LOG_UNCOND ("Dap " << socket->GetNode()->GetId() << ": Received one packet at " << Simulator::Now().GetSeconds() << " seconds.");

  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      ptrdiff_t index = std::find(daps.Begin(), daps.End(), socket->GetNode()) - daps.Begin();
      dapSelectionHistory[index]++;

      SendPacketToController(socket->GetNode());
    }
}

double
AmiGridSim::CalculateDapSelection(int dapIndex)
{
  int sum = std::accumulate(dapSelectionHistory.begin(), dapSelectionHistory.end(), 0);

  return dapSelectionHistory[dapIndex] / (double)sum;
}

void
AmiGridSim::PrintStatistics()
{
  std::ofstream statFile;
  statFile.open ("statistics.txt");
  statFile << "Statistics generated from the AmiGrid simulation.\n";
  statFile << "Number of packets sent: " << packetsSent << std::endl;
  statFile << "Number of packets received: " << packetsReceived << std::endl;
  statFile << "Delivery rate: " << 100 * packetsReceived / (double)packetsSent << "%" << std::endl;

  int loss = packetsSent - packetsReceived;

  statFile << "Loss rate: " << 100 * loss / (double)packetsSent << "%" << std::endl;

  for(uint32_t i = 0; i < daps.GetN(); i++)
    {
      statFile << "Dap " << daps.Get(i)->GetId() << " selection rate: " << CalculateDapSelection(i) << "%" << std::endl;
    }

  statFile.close();
}

void
AmiGridSim::SheduleFailure()
{
  if (withFailure)
    {
      m_events.Track (Simulator::Schedule (Seconds (80), &AmiGridSim::ExecuteFailure, this, daps.Get(0)));
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


int main (int argc, char *argv[])
{
  AmiGridSim simulation;

  simulation.Configure(argc, argv);
  simulation.CreateNodes();

  Ptr<ListPositionAllocator> metersPositionAlloc = simulation.CreateMetersPosition();
  Ptr<ListPositionAllocator> dapsPositionAlloc = simulation.CreateDapsPosition();

  simulation.InstallMobilityModel(metersPositionAlloc, dapsPositionAlloc);
  NetDeviceContainer olsrDevices = simulation.ConfigureWifi();
  NetDeviceContainer csmaDevices = simulation.CreateControllerLan();
  simulation.ConfigureRouting();
  simulation.ConfigureIpAddressing(olsrDevices, csmaDevices);

  uint16_t port = 80;
  Time metersStart = Seconds(10.0);
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
  simulation.PrintStatistics();
  return 0;
}



