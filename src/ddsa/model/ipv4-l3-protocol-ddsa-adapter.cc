/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ipv4-l3-protocol-ddsa-adapter.h"
#include "ddsa-routing-protocol-adapter.h"

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("Ipv4L3ProtocolDdsaAdapter");

  namespace ddsa {

    NS_OBJECT_ENSURE_REGISTERED (Ipv4L3ProtocolDdsaAdapter);

    TypeId
    Ipv4L3ProtocolDdsaAdapter::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::ddsa::Ipv4L3ProtocolDdsaAdapter")
        .SetParent<Ipv4L3Protocol> ()
        .SetGroupName ("Ddsa")
        .AddConstructor<Ipv4L3ProtocolDdsaAdapter> ();
      return tid;
    }

    Ipv4L3ProtocolDdsaAdapter::Ipv4L3ProtocolDdsaAdapter()
    {

    }

    Ipv4L3ProtocolDdsaAdapter::~Ipv4L3ProtocolDdsaAdapter(){}


    void
    Ipv4L3ProtocolDdsaAdapter::Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol,
				     Ptr<Ipv4Route> route)
    {
      Ipv4L3Protocol::Send(packet, source, destination, protocol, route);

      Ptr<Ipv4RoutingProtocol> ipRoutingProt = GetRoutingProtocol();
      Ptr<DdsaRoutingProtocolAdapter> ddsaRoutingProt;

      Ptr<Ipv4Route> routeToDap;
      Dap selectedDap;

      Socket::SocketErrno sockerr;

      if (ipRoutingProt == NULL)
	{
	  return;
	}

      ddsaRoutingProt = DynamicCast<DdsaRoutingProtocolAdapter> (ipRoutingProt);

      if (!ddsaRoutingProt)
	{
	  return;
	}

      int retrans = ddsaRoutingProt->GetNRetransmissions();

      while (retrans-- > 0)
	{
	  selectedDap = ddsaRoutingProt->SelectDap();

	  routeToDap = ddsaRoutingProt->RouteOutput(packet, selectedDap.address, 0, sockerr);

	  Ipv4L3Protocol::Send(packet, source, selectedDap.address, protocol, routeToDap);

	}
    }

  }
}

