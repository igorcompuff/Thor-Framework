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
      m_failureType = FailureType::FULL;
    }

    Ipv4L3ProtocolDdsaAdapter::~Ipv4L3ProtocolDdsaAdapter(){}

    void
    Ipv4L3ProtocolDdsaAdapter::MakeFail()
    {
      mustFail = true;
    }

    bool
    Ipv4L3ProtocolDdsaAdapter::ShouldFailMalicious(Ptr<Packet> packet, bool receiveMethod)
    {
      Ipv4Header ipHeader;
      UdpHeader udpHeader;
      TcpHeader tcpHeader;
      ami::AmiHeader amiHeader;

      /*
       * Only received packets have Ip header aggregated. Sent packets hava only the application and the
       * transport headers
       */
      if (receiveMethod)
	{
	  packet->RemoveHeader(ipHeader);

	  if (ipHeader.GetPayloadSize () < packet->GetSize())
	  {
	    packet->RemoveAtEnd (packet->GetSize () - ipHeader.GetPayloadSize ());
	  }
	}

      /*
       * We only condier application packets using either TCP or UDP transport protocols
       */
      if (ipHeader.GetProtocol() == 17)
	{
	  packet->RemoveHeader(udpHeader);
	}
      else if (ipHeader.GetProtocol() == 6)
	{
	  packet->RemoveHeader(tcpHeader);
	}
      else
	{
	  return false;
	}

      packet->RemoveHeader(amiHeader);

      if (amiHeader.GetReadingInfo() == 1)
	{
	  return true;
	}

      return false;
    }

    void
    Ipv4L3ProtocolDdsaAdapter::Receive ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
					 const Address &to, NetDevice::PacketType packetType)
    {
      bool shouldFail = false;

      if (mustFail)
	{
	  switch(m_failureType)
	  {
	    case FailureType::FULL: shouldFail = true; break;
	    case FailureType::MALICIOUS: shouldFail = ShouldFailMalicious(p->Copy(), true); break;
	  }
	}

      if (!shouldFail)
	{
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
      bool shouldFail = false;
      if (mustFail)
	{
	  switch(m_failureType)
	  {
	    case FailureType::FULL: shouldFail = true; break;
	    case FailureType::MALICIOUS: shouldFail = ShouldFailMalicious(packet->Copy(), false); break;
	  }
	}

      if (!shouldFail)
	{
	  Ipv4L3Protocol::Send(packet, source, destination, protocol, route);
	}
      else
	{
	  NS_LOG_DEBUG("Sending: Node " << source << " is failing, so the packet will be discarded!.(t = " << Simulator::Now() << "\n");
	}
    }

    void
    Ipv4L3ProtocolDdsaAdapter::SetFailureType(FailureType fType)
    {
      m_failureType = fType;
    }

  }
}
