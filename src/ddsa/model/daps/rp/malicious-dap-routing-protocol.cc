/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#define NS_LOG_APPEND_CONTEXT std::clog << "[node " << GetMyMainAddress() << "] (" << Simulator::Now().GetSeconds() << " s) ";

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"
#include "ns3/lq-olsr-repositories.h"
#include <algorithm>
#include "malicious-dap-routing-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/simple-header.h"
#include "ns3/lq-olsr-util.h"

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("MaliciousDapRoutingProtocol");

  namespace ddsa {

    NS_OBJECT_ENSURE_REGISTERED (MaliciousDapRoutingProtocol);

    TypeId
	MaliciousDapRoutingProtocol::GetTypeId (void)
    {
		static TypeId tid = TypeId ("ns3::ddsa::MaliciousDapRoutingProtocol")
			.SetParent<RoutingProtocol> ()
			.SetGroupName ("Ddsa")
			.AddConstructor<MaliciousDapRoutingProtocol> ();

		return tid;
    }

    MaliciousDapRoutingProtocol::MaliciousDapRoutingProtocol()
    {
      malicious = false;
    }

    MaliciousDapRoutingProtocol::~MaliciousDapRoutingProtocol(){}


    float
	MaliciousDapRoutingProtocol::GetCostToTcSend(lqolsr::LinkTuple *link_tuple)
    {
    	return malicious ? 1.0f : RoutingProtocol::GetCostToTcSend(link_tuple);
    }

    uint32_t
	MaliciousDapRoutingProtocol::GetHelloInfoToSendHello(Ipv4Address neiAddress)
    {
		uint32_t info;
		if (malicious)
		{
			info = pack754_32(1.0f);
		}
		else
		{
			info = RoutingProtocol::GetHelloInfoToSendHello(neiAddress);
		}

		return info;
    }

    void
	MaliciousDapRoutingProtocol::StartMalicious()
    {
    	malicious = true;
    }

    void
	MaliciousDapRoutingProtocol::StopMalicious()
    {
    	malicious = false;
    }

    bool
	MaliciousDapRoutingProtocol::IsMalicious()
    {
    	return malicious;
    }

	void
	MaliciousDapRoutingProtocol::SendTc()
	{
		/* Not malicious daps donot send tc messages in order to avoid to became part of a path to another DAP
		* Malicious DAPs, on the other hand, do want to attract network flow and will send tc messages
		*/
		if (!IsMalicious())
		{
			return;
		}

		RoutingProtocol::SendTc();
	}
  }
}
