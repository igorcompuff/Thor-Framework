/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ipv4-l3-protocol-ddsa-adapter.h"
#include "ns3/ipv4-routing-helper.h"
#include "ddsa-routing-protocol-adapter.h"
#include "ns3/simple-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"

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
	  Ipv4Header ipHeader;
	  UdpHeader udpHeader;
	  ami::AmiHeader amiHeader;

	  Ptr<Packet> cPacket = p->Copy();

	  cPacket->RemoveHeader(ipHeader);

	  if (ipHeader.GetPayloadSize () < p->GetSize ())
	    {
	      cPacket->RemoveAtEnd (cPacket->GetSize () - ipHeader.GetPayloadSize ());
	    }

	  cPacket->RemoveHeader(udpHeader);
	  cPacket->RemoveHeader(amiHeader);

	  NS_LOG_DEBUG("Reading info " << amiHeader.GetReadingInfo());

	  if (amiHeader.GetReadingInfo() == 1)
	    {
	      Ptr<Node> receivingNode = device->GetNode();
	      Ptr<Ipv4> ipv4 = receivingNode->GetObject<Ipv4> ();

	      int32_t interface = ipv4->GetInterfaceForDevice(device);

	      NS_LOG_DEBUG("Packet " << amiHeader.GetPacketSequenceNumber() << " from " << ipHeader.GetSource() << " destined to " << ipHeader.GetDestination() << " Received by " << ipv4->GetAddress(interface, 0).GetLocal() << " at " << Simulator::Now().GetSeconds());
	    }

	  Ipv4L3Protocol::Receive(device, p, protocol, from, to, packetType);
	}
      else
	{
	  NS_LOG_DEBUG("Receiveing: Node is failing, so the packet will be discarded!.(t = " << Simulator::Now() << "\n");
	}
    }

    void
    Ipv4L3ProtocolDdsaAdapter::Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol,
				     Ptr<Ipv4Route> route)
    {
      if (mustFail)
	{
	  NS_LOG_DEBUG("Sending: Node " << source << " is failing, so the packet will be discarded!.(t = " << Simulator::Now() << "\n");
	  return;
	}

      Ipv4L3Protocol::Send(packet, source, destination, protocol, route);
    }

    void
    Ipv4L3ProtocolDdsaAdapter::SetNodeType(NodeType nType)
    {
      m_type = nType;
    }

  }
}

