#include "ns3/core-module.h"
#include "ns3/etx-ff.h"
#include "ns3/lq-olsr-header.h"
#include "ns3/ddsa-routing-protocol-adapter.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"

using namespace ns3;
using namespace ns3::lqmetric;
using namespace ns3::lqolsr;
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
    std::vector<NetDeviceContainer> CreatePointToPointConnections();
    void ConfigureRouting();
    void ConfigureIpAddressing(const NetDeviceContainer & olsrDevices, const std::vector<NetDeviceContainer> & pointToPointDevices);


  private:
    void Parse(int argc, char *argv[]);


    std::string phyMode;
    double rss; // -dBm
    uint32_t packetSize; // bytes
    uint32_t numPackets;
    uint32_t totalDaps; //Total number of Daps
    double interval;// seconds
    bool verbose;
    bool assocMethod1;
    bool assocMethod2;
    CommandLine cmd;
    Time interPacketInterval;
    NodeContainer daps;
    NodeContainer meters;
    NodeContainer controllers;
    double gridXShift; // Horizontal distance between two meters in the grid
    double gridYShift; // Vertical distance between two meters in the grid
    int gridColumns;
    int gridRows;
    Ptr<LogDistancePropagationLossModel> myLossModel;
    YansWifiPhyHelper myWifiPhy;
};

AmiGridSim::AmiGridSim (): phyMode ("DsssRate1Mbps")
{
    rss = -80;
    packetSize = 1000;
    numPackets = 86;
    interval = 1.0;
    totalDaps = 3;
    verbose = false;
    assocMethod1 = false;
    assocMethod2 = false;
    gridXShift = 10.0;
    gridYShift = 10.0;
    gridColumns = 12;
    gridRows = 3;
    myLossModel = CreateObject<LogDistancePropagationLossModel> ();
    myWifiPhy = YansWifiPhyHelper::Default();
}

AmiGridSim::~AmiGridSim ()
{
  // TODO Auto-generated destructor stub
}

void
AmiGridSim::Parse(int argc, char *argv[])
{
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("totalDaps", "Total number of Daps", totalDaps);
  cmd.AddValue ("gridXShift", "Horizontal distance between two meters in the grid", gridXShift);
  cmd.AddValue ("gridYShift", "Vertical distance between two meters in the grid ", gridYShift);
  cmd.AddValue ("gridColumns", "Number of columns in the grid ", gridColumns);
  cmd.AddValue ("gridRows", "Number of rows in the grid ", gridRows);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("assocMethod1", "Use SetRoutingTableAssociation () method", assocMethod1);
  cmd.AddValue ("assocMethod2", "Use AddHostNetworkAssociation () method", assocMethod2);

  cmd.Parse (argc, argv);
}

void
AmiGridSim::Configure(int argc, char *argv[])
{
  Parse(argc, argv);
  interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

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

  for (int dap = 0; dap < totalDaps; dap++)
    {
      positionAlloc->Add (Vector (x, y, 0.0));

      x += (columnsPerDap * gridXShift);
    }

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
  mobility.Install (daps);
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

  return wifi.Install (myWifiPhy, wifiMac, allOlsrNodes);
}

std::vector<NetDeviceContainer>
AmiGridSim::CreatePointToPointConnections()
{
  std::vector<NetDeviceContainer> devices;
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  for (int i = 0; i < totalDaps; i++)
    {
      NodeContainer nodes;
      nodes.Add(daps.Get (i));
      nodes.Add(controllers.Get (0));

      devices.push_back(pointToPoint.Install(nodes));
    }

  return devices;
}

void
AmiGridSim::ConfigureRouting()
{
  TypeId metricTid = Etx::GetTypeId();
  LqOlsrHelper olsrHelper(metricTid);
  DDsaHelper ddsaHelper(metricTid);

  for (int i = 0; i < totalDaps; i++)
    {
      olsrHelper.ExcludeInterface (daps.Get (i), 2);
    }

  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper listMeters;
  listMeters.Add (staticRouting, 0);
  listMeters.Add (ddsaHelper, 10);

  Ipv4ListRoutingHelper listDaps;
  listDaps.Add (staticRouting, 0);
  listDaps.Add (olsrHelper, 10);

  InternetStackHelper internet_olsr;
  internet_olsr.SetRoutingHelper (listMeters); // has effect on the next Install ()
  internet_olsr.Install (meters);

  internet_olsr.SetRoutingHelper (listDaps); // has effect on the next Install ()
  internet_olsr.Install (daps);

  InternetStackHelper internet_controller;
  internet_controller.Install (controllers);
}

void
AmiGridSim::ConfigureIpAddressing(const NetDeviceContainer & olsrDevices, const std::vector<NetDeviceContainer> & pointToPointDevices)
{
  NS_LOG_INFO ("Assign IP Addresses.");

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4.Assign (olsrDevices);

  std::string ipBase = "172.16.";
  for (int i = 0; i < pointToPointDevices.size(); i++)
    {
      NetDeviceContainer pointToPoint = pointToPointDevices[i];
      std::string ip = ipBase + (i+1) + ".0";
      ipv4.SetBase (Ipv4Address(ip), "255.255.255.0");
      ipv4.Assign (pointToPoint);
    }
}


