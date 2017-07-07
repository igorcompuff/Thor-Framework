/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ipv4-l3-protocol-ddsa-adapter.h"
#include "ns3/ipv4-routing-helper.h"
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
      mustFail = false;
      m_type = NodeType::METER;
    }

    Ipv4L3ProtocolDdsaAdapter::~Ipv4L3ProtocolDdsaAdapter(){}

    void
    Ipv4L3ProtocolDdsaAdapter::MakeFail()
    {
      mustFail = true;
    }

    void
    Ipv4L3ProtocolDdsaAdapter::Receive ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
					 const Address &to, NetDevice::PacketType packetType)
    {
      if (!mustFail)
	{
	  Ipv4L3Protocol::Receive(device, p, protocol, from, to, packetType);
	}
    }

    void
    Ipv4L3ProtocolDdsaAdapter::Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol,
				     Ptr<Ipv4Route> route)
    {

      if (mustFail)
	{
	  return;
	}

      if (m_type == NodeType::METER && route && route->GetGateway () != Ipv4Address ())
	{
	  Ptr<Ipv4RoutingProtocol> ipRoutingProt = GetRoutingProtocol();

	  if (ipRoutingProt == NULL)
	    {
	      return;
	    }

	  Ptr<DdsaRoutingProtocolAdapter> ddsaRoutingProt = Ipv4RoutingHelper::GetRouting<DdsaRoutingProtocolAdapter>(ipRoutingProt);

	  if (!ddsaRoutingProt)
	    {
	      return;
	    }

	  int retransmissions = ddsaRoutingProt->GetNRetransmissions();

	  do
	    {
	      Dap selectedDap = ddsaRoutingProt->SelectDap();

	      if (selectedDap.address == Ipv4Address::GetBroadcast())
	      {
		  continue;
	      }

	      Socket::SocketErrno sockerr;
	      Ptr<Ipv4Route> routeToDap = ddsaRoutingProt->RouteOutput(packet, selectedDap.address, 0, sockerr);
	      Ipv4L3Protocol::Send(packet, source, selectedDap.address, protocol, routeToDap);
	    }
	    while(retransmissions-- > 0);
	}
      else
	{
	  Ipv4L3Protocol::Send(packet, source, destination, protocol, route);
	}
    }

    void
    Ipv4L3ProtocolDdsaAdapter::SetNodeType(NodeType nType)
    {
      m_type = nType;
    }

  }
}

