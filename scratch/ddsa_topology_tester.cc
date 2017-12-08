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

NS_LOG_COMPONENT_DEFINE ("DdsaTopologyTest");



/*
 * DdsaTopologyTest.cpp
 *
 *  Created on: 13 de jun de 2017
 *      Author: igor
 */

class DdsaTopologyTest
{
  public:
  DdsaTopologyTest ();
    virtual
    ~DdsaTopologyTest ();
    void Configure(int argc, char *argv[]);
    Ptr<ListPositionAllocator> CreateMetersPosition();
    Ptr<ListPositionAllocator> CreateDapsPosition();
    void InstallMobilityModel(Ptr<ListPositionAllocator> metersPosition, Ptr<ListPositionAllocator> dapsPosition);
    void ConfigureWifi();
    void CreateControllerLan();
    void ConfigureIpAddressing();
    void ConfigureNodesStack();
    void CreateNodes();
    bool ComplyWithMinRedundancy();
    void SaveDapsCurrentPosition();

  private:
    void Parse(int argc, char *argv[]);
    void ConfigureStack(LqOlsrHelper & lqOlsrDapHelper, NodeContainer nodes, Ipv4L3ProtocolDdsaAdapter::NodeType nType );
    void ConfigureDapStack(LqOlsrHelper & helper);
    void ConfigureMeterStack(LqOlsrHelper & helper);
    Ipv4ListRoutingHelper ConfigureRouting(const Ipv4RoutingHelper & ipv4Routing);
    void ConfigureControllerStack();
    void ConfigureDapsHna();
    Vector GetVector(std::string xStr, std::string yStr);
    double GetCoordinate(std::stringstream * coordStream);
    std::vector<Vector> GetPreviousRedundancyDapsPositions();
    Ipv4Address GetNodeAddress(const Ptr<NetDevice> & device, const Ptr<Node> node);


    std::string phyMode;
    uint32_t totalDaps; //Total number of Daps
    bool verbose;
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
    Ptr<LogDistancePropagationLossModel> myLossModel;
    YansWifiPhyHelper myWifiPhy;
    Ipv4InterfaceContainer olsrIpv4Devices;
    Ipv4InterfaceContainer csmaIpv4Devices;
    EventGarbageCollector m_events;
    NetDeviceContainer olsrDevices;
    NetDeviceContainer csmaDevices;
    int minRedundancy;
    Ptr<UniformRandomVariable> m_rnd;
};

DdsaTopologyTest::DdsaTopologyTest (): phyMode ("DsssRate1Mbps")
{
    totalDaps = 3;
    verbose = false;
    gridXShift = 10.0;
    gridYShift = 20.0;
    gridColumns = 3;
    gridRows = 1;
    myLossModel = CreateObject<LogDistancePropagationLossModel> ();
    myWifiPhy = YansWifiPhyHelper::Default();
    withFailure = false;
    ddsaEnabled = true;
    minRedundancy = 1;

    m_rnd = CreateObject<UniformRandomVariable>();
}

DdsaTopologyTest::~DdsaTopologyTest ()
{
  // TODO Auto-generated destructor stub
}


void
DdsaTopologyTest::Parse(int argc, char *argv[])
{
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("totalDaps", "Total number of Daps", totalDaps);
  cmd.AddValue ("gridXShift", "Horizontal distance between two meters in the grid", gridXShift);
  cmd.AddValue ("gridYShift", "Vertical distance between two meters in the grid ", gridYShift);
  cmd.AddValue ("gridColumns", "Number of columns in the grid ", gridColumns);
  cmd.AddValue ("gridRows", "Number of rows in the grid ", gridRows);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("failure", "Cause the failure of one DAP", withFailure);
  cmd.AddValue ("ddsaenabled", "Enable ddsa", ddsaEnabled);
  cmd.AddValue ("redundancy", "Minimal redundancy", minRedundancy);

  cmd.Parse (argc, argv);
}

//Simulation Configuration

void
DdsaTopologyTest::Configure(int argc, char *argv[])
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
DdsaTopologyTest::CreateNodes()
{
  daps.Create(totalDaps); // Creates the specified number of daps
  meters.Create(gridRows * gridColumns); //Creates the Specified NUmber of meters
  controllers.Create(1); // Creates one controller.
}

Ptr<ListPositionAllocator>
DdsaTopologyTest::CreateMetersPosition()
{
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (int row = 0; row < gridRows; row++)
    {
      int y = row * gridYShift + 50;

      for (int col = 0; col < gridColumns; col++)
	{
	  positionAlloc->Add(Vector (col * gridXShift, y, 0.0));
	}
    }

  return positionAlloc;
}

double
DdsaTopologyTest::GetCoordinate(std::stringstream * coordStream)
{
  std::string token;
  std::getline(*coordStream, token, '=');
  std::getline(*coordStream, token, '=');

  return std::atof(token.substr(1).c_str());
}

Vector
DdsaTopologyTest::GetVector(std::string xStr, std::string yStr)
{
  std::stringstream xStream(xStr);
  std::stringstream yStream(yStr);

  double x = GetCoordinate(&xStream);
  double y = GetCoordinate(&yStream);

  return Vector(x, y, 0.0);
}

std::vector<Vector>
DdsaTopologyTest::GetPreviousRedundancyDapsPositions()
{
  std::vector<Vector> positions;
  std::string dap, x, y, space;
  int previousRedundancy = minRedundancy - 1;
  std::stringstream stream;
  stream << "/home/igor/github/ns-3.26/positions/dap_pos_" << previousRedundancy << ".txt";
  std::string path = stream.str();
  std::ifstream file(path);

  if (file)
  {
    getline(file, dap);

    while(dap.find("#") == std::string::npos)
    {
      getline(file, x);
      getline(file, y);
      getline(file, space);
      positions.push_back(GetVector(x, y));
      getline(file, dap);
    }
  }

  return positions;
}

Ptr<ListPositionAllocator>
DdsaTopologyTest::CreateDapsPosition()
{
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  double maxX = (gridColumns - 1) * gridXShift;
  double maxY = (gridRows - 1) * gridYShift;

  std::vector<Vector> previousPositions = GetPreviousRedundancyDapsPositions();

  for(std::vector<Vector>::iterator it = previousPositions.begin(); it != previousPositions.end(); it++)
    {
      positionAlloc->Add (*it);
    }

  for (uint32_t dap = 1; dap <= totalDaps - previousPositions.size(); dap++)
    {
      double x = m_rnd->GetValue(0.0, maxX);
      double y = m_rnd->GetValue(0.0, maxY);

      positionAlloc->Add (Vector (x, y, 0.0));
    }


  return positionAlloc;
}

void
DdsaTopologyTest::InstallMobilityModel(Ptr<ListPositionAllocator> metersPosition, Ptr<ListPositionAllocator> dapsPosition)
{
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (metersPosition);
  mobility.Install (meters);

  mobility.SetPositionAllocator (dapsPosition);
  mobility.Install (NodeContainer(daps, controllers));
}

void
DdsaTopologyTest::ConfigureWifi()
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
DdsaTopologyTest::CreateControllerLan()
{
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  csmaDevices = csma.Install (NodeContainer (controllers, daps));
  csma.EnablePcap ("csma", csmaDevices, false);
}

void
DdsaTopologyTest::ConfigureStack(LqOlsrHelper & lqOlsrDapHelper, NodeContainer nodes, Ipv4L3ProtocolDdsaAdapter::NodeType nType )
{
  Ipv4ListRoutingHelper ipv4Routing = ConfigureRouting(lqOlsrDapHelper);
  DdsaInternetStackHelper ddsa;
  ddsa.SetRoutingHelper (ipv4Routing); // has effect on the next Install ()
  ddsa.SetNodeType(nType);
  ddsa.Install (nodes);
}

void
DdsaTopologyTest::ConfigureDapStack(LqOlsrHelper & helper)
{
  for (uint32_t i = 0; i < totalDaps; i++)
    {
      helper.ExcludeInterface (daps.Get (i), 2);
    }

  ConfigureStack(helper, daps, Ipv4L3ProtocolDdsaAdapter::NodeType::DAP);
}

void
DdsaTopologyTest::ConfigureMeterStack(LqOlsrHelper & helper)
{
  Ipv4L3ProtocolDdsaAdapter::NodeType mType = ddsaEnabled ? Ipv4L3ProtocolDdsaAdapter::NodeType::METER :
							    Ipv4L3ProtocolDdsaAdapter::NodeType::NON_DDSA;
  ConfigureStack(helper, meters, mType);
}

void
DdsaTopologyTest::ConfigureControllerStack()
{
  InternetStackHelper internet;

  internet.Install (controllers);
}

Ipv4ListRoutingHelper
DdsaTopologyTest::ConfigureRouting(const Ipv4RoutingHelper & ipv4Routing)
{
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (ipv4Routing, 10);

  return list;
}

void
DdsaTopologyTest::ConfigureDapsHna()
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
DdsaTopologyTest::ConfigureNodesStack()
{
  TypeId metricTid = Etx::GetTypeId();

  LqOlsrHelper helper(metricTid);
  ConfigureDapStack(helper);

  DDsaHelper meterHelper(metricTid);
  meterHelper.Set("Retrans", IntegerValue(0));
  ConfigureMeterStack(meterHelper);

  ConfigureControllerStack();
  ConfigureDapsHna();
}

void
DdsaTopologyTest::ConfigureIpAddressing()
{
  NS_LOG_INFO ("Assign IP Addresses.");

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  olsrIpv4Devices = ipv4.Assign (olsrDevices);

  ipv4.SetBase (Ipv4Address("172.16.1.0"), "255.255.255.0");
  csmaIpv4Devices = ipv4.Assign (csmaDevices);
}

bool
DdsaTopologyTest::ComplyWithMinRedundancy()
{
  for(NodeContainer::Iterator it = meters.Begin(); it != meters.End(); it++)
    {
      Ptr<DdsaRoutingProtocolAdapter> ddsa = (*it)->GetObject<DdsaRoutingProtocolAdapter>();

      if (ddsa && ddsa->GetTotalCurrentEligibleDaps() < minRedundancy)
	{
	  return false;
	}
    }

  return true;
}

void
DdsaTopologyTest::SaveDapsCurrentPosition()
{
  std::ofstream statFile;

  std::string fileName = "dap_pos.txt";

  statFile.open(fileName, std::ofstream::out | std::ofstream::app);

  for(NodeContainer::Iterator it = daps.Begin(); it != daps.End(); it++)
    {
      Ptr<MobilityModel> model = (*it)->GetObject<MobilityModel>();

      if (model != NULL)
	{
	  statFile << "Dap: " << (*it)->GetId() << "\n";
	  statFile << "X = " << model->GetPosition().x << "\n";
	  statFile << "Y = " << model->GetPosition().y << "\n";
	  statFile << "\n";
	}
    }

  statFile << "######################################################\n";

  statFile.close();
}

int main (int argc, char *argv[])
{
  DdsaTopologyTest simulation;

  simulation.Configure(argc, argv);
  simulation.CreateNodes();

  Ptr<ListPositionAllocator> metersPositionAlloc = simulation.CreateMetersPosition();
  Ptr<ListPositionAllocator> dapsPositionAlloc = simulation.CreateDapsPosition();

  simulation.InstallMobilityModel(metersPositionAlloc, dapsPositionAlloc);
  simulation.ConfigureWifi();
  simulation.CreateControllerLan();
  simulation.ConfigureNodesStack();
  simulation.ConfigureIpAddressing();

  //Log
  //LogComponentEnable("LqOlsrRoutingProtocol", LOG_LEVEL_ALL);
  //LogComponentEnable("DdsaRoutingProtocolAdapter", LOG_LEVEL_DEBUG);
  //LogComponentEnable("Etx", LOG_LEVEL_ALL);
  //LogComponentEnable("YansWifiPhy", LOG_LEVEL_DEBUG);
  //LogComponentEnable("DdsaRoutingProtocolDapAdapter", LOG_LEVEL_DEBUG);

  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();
  Simulator::Destroy ();

  if (simulation.ComplyWithMinRedundancy())
    {
      simulation.SaveDapsCurrentPosition();
    }

  return 0;
}



