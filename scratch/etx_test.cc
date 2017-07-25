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
using namespace ns3::ddsa;

NS_LOG_COMPONENT_DEFINE ("etxtest");



/*
 * etx_test.cpp
 *
 *  Created on: 13 de jun de 2017
 *      Author: igor
 */

class EtxTest
{
  public:
  EtxTest ();
    virtual
    ~EtxTest ();
    void Configure(int argc, char *argv[]);
    Ptr<ListPositionAllocator> CreateNodesPosition();
    void InstallMobilityModel(Ptr<ListPositionAllocator> nodesPosition);
    NetDeviceContainer ConfigureWifi();
    void ConfigureRouting();
    void ConfigureIpAddressing(const NetDeviceContainer & olsrDevices);
    void ConfigureSenderApplication(uint16_t port, Time start, Time stop);
    void ConfigureReceiverSocket(uint16_t port);
    void CreateNodes();
    void PrintStatistics();
    void SheduleFailure();

  private:
    void Parse(int argc, char *argv[]);
    void PacketSentEvt(Ptr<const Packet>);
    void PacketReceivedEvt (Ptr<Socket> socket);
    double CalculateDeliveryRate();
    double CalculateLossRate();
    void ExecuteFailure(Ptr<Node> node);

    std::string phyMode;
    uint32_t packetSize; // bytes
    uint32_t totalNodes; //Total number of Daps
    bool verbose;
    bool withFailure;
    CommandLine cmd;
    NodeContainer nodes;
    int packetsSent;
    int packetsReceived;
    int yShift;

    Ptr<LogDistancePropagationLossModel> myLossModel;
    YansWifiPhyHelper myWifiPhy;
    Ipv4InterfaceContainer olsrIpv4Devices;
    EventGarbageCollector m_events;
};


EtxTest::EtxTest (): phyMode ("DsssRate1Mbps")
{
    packetSize = 400;
    totalNodes = 4;
    verbose = false;
    myLossModel = CreateObject<LogDistancePropagationLossModel> ();
    myWifiPhy = YansWifiPhyHelper::Default();
    packetsSent = 0;
    packetsReceived = 0;
    yShift = 50;
    withFailure = false;
}

EtxTest::~EtxTest ()
{
  // TODO Auto-generated destructor stub
}

void
EtxTest::Parse(int argc, char *argv[])
{
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("totalNodes", "Total number of nodes", totalNodes);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("yShift", "Vertical distance between two nodes.", yShift);

  cmd.Parse (argc, argv);
}

void
EtxTest::Configure(int argc, char *argv[])
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
EtxTest::CreateNodes()
{
  nodes.Create(totalNodes); // Creates the specified number of daps
}

Ptr<ListPositionAllocator>
EtxTest::CreateNodesPosition()
{
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (-50.0, 160.0, 0.0));
  positionAlloc->Add (Vector (20.0, 160.0, 0.0));
  positionAlloc->Add (Vector (-50.0, 310.0, 0.0));

//  for (uint32_t i = 0; i < totalNodes; i++)
//    {
//      double y = i * yShift;
//
//      positionAlloc->Add (Vector (10.0, y, 0.0));
//    }

  return positionAlloc;
}

void
EtxTest::InstallMobilityModel(Ptr<ListPositionAllocator> nodesPosition)
{
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (nodesPosition);
  mobility.Install (nodes);
}

NetDeviceContainer
EtxTest::ConfigureWifi()
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

  NetDeviceContainer devices = wifi.Install (myWifiPhy, wifiMac, nodes);
  myWifiPhy.EnablePcap ("olsr", devices, false);

  return devices;
}

void
EtxTest::ConfigureRouting()
{
  TypeId metricTid = Etx::GetTypeId();
  Ipv4StaticRoutingHelper staticRouting;
  LqOlsrHelper olsrHelper(metricTid);
  Ipv4ListRoutingHelper list;

  list.Add (staticRouting, 0);
  list.Add (olsrHelper, 10);

  DdsaInternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  internet.SetNodeType(ns3::ddsa::Ipv4L3ProtocolDdsaAdapter::NodeType::NON_DDSA);
  internet.Install (nodes);
}

void
EtxTest::ConfigureIpAddressing(const NetDeviceContainer & olsrDevices)
{
  NS_LOG_INFO ("Assign IP Addresses.");

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  olsrIpv4Devices = ipv4.Assign (olsrDevices);
}

void
EtxTest::ConfigureSenderApplication(uint16_t port, Time start, Time stop)
{
  OnOffHelper onOff ("ns3::UdpSocketFactory", Address (InetSocketAddress (olsrIpv4Devices.GetAddress(totalNodes - 1), port)));
  onOff.SetConstantRate (DataRate ("960bps"), packetSize);

  ApplicationContainer apps = onOff.Install(nodes.Get(0));

  Ptr<Application> app = apps.Get(0);
  OnOffApplication * appOnOff = dynamic_cast<OnOffApplication*>(&(*app));

  if (appOnOff)
    {
      appOnOff->TraceConnectWithoutContext("Tx", MakeCallback(&EtxTest::PacketSentEvt, this));
    }

  apps.Start (start);
  apps.Stop (stop);
}

void
EtxTest::ConfigureReceiverSocket(uint16_t port)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);

  Ptr<Socket> recvController = Socket::CreateSocket (nodes.Get (totalNodes - 1), tid);
  recvController->Bind (local);
  recvController->SetRecvCallback (MakeCallback (&EtxTest::PacketReceivedEvt, this));
}

void
EtxTest::PacketSentEvt(Ptr<const Packet> packetSent)
{
  NS_LOG_UNCOND ("Packet Sent!");
  packetsSent++;
}

void
EtxTest::PacketReceivedEvt(Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      NS_LOG_UNCOND ("Node " << socket->GetNode()->GetId() << " received one packet at " << Simulator::Now().GetSeconds() << " seconds from " << from);
      packetsReceived++;
    }
}

double
EtxTest::CalculateDeliveryRate()
{
  return 100 * packetsReceived / (double)packetsSent;
}

double
EtxTest::CalculateLossRate()
{
  int loss = packetsSent - packetsReceived;
  return 100 * loss / (double)packetsSent;
}

void
EtxTest::PrintStatistics()
{
  std::ofstream statFile;

  std::string fileName = "statistics.txt";

  statFile.open (fileName);
  statFile << "Statistics generated from the EtxTest simulation.\n";
  statFile << "Number of packets sent: " << packetsSent << std::endl;
  statFile << "Number of packets received: " << packetsReceived << std::endl;
  statFile << "Delivery rate: " << CalculateDeliveryRate() << "%" << std::endl;
  statFile << "Loss rate: " << CalculateLossRate() << "%" << std::endl;


  statFile.close();
}

void
EtxTest::SheduleFailure()
{
  if (withFailure)
    {
      m_events.Track (Simulator::Schedule (Seconds (100), &EtxTest::ExecuteFailure, this, nodes.Get(2)));
    }
}

void
EtxTest::ExecuteFailure(Ptr<Node> node)
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
  EtxTest simulation;

  simulation.Configure(argc, argv);
  simulation.CreateNodes();

  Ptr<ListPositionAllocator> nodesPositionAlloc = simulation.CreateNodesPosition();

  simulation.InstallMobilityModel(nodesPositionAlloc);
  NetDeviceContainer olsrDevices = simulation.ConfigureWifi();

  simulation.ConfigureRouting();
  simulation.ConfigureIpAddressing(olsrDevices);

  uint16_t port = 80;
  Time metersStart = Seconds(10.0);
  Time metersStop = Seconds(190.0);

  simulation.ConfigureSenderApplication(port, metersStart, metersStop);
  simulation.ConfigureReceiverSocket(port);
  simulation.SheduleFailure();

  //Log
  LogComponentEnable("LqOlsrRoutingProtocol", LOG_LEVEL_DEBUG);
  //LogComponentEnable("DdsaRoutingProtocolAdapter", LOG_LEVEL_DEBUG);
  //LogComponentEnable("Etx", LOG_LEVEL_DEBUG);
  //LogComponentEnable("OnOffApplication", LOG_LEVEL_FUNCTION);
  //LogComponentEnable("DdsaRoutingProtocolDapAdapter", LOG_LEVEL_DEBUG);

  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();
  Simulator::Destroy ();
  simulation.PrintStatistics();
  return 0;
}



