/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ipv4-l3-protocol-smartddsa-adapter.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/simple-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "smart-ddsa-routing-protocol-adapter.h"

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("Ipv4L3ProtocolSmartDdsaAdapter");

  namespace ddsa {

    NS_OBJECT_ENSURE_REGISTERED (Ipv4L3ProtocolSmartDdsaAdapter);

    TypeId
	Ipv4L3ProtocolSmartDdsaAdapter::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::ddsa::Ipv4L3ProtocolSmartDdsaAdapter")
        .SetParent<Ipv4L3ProtocolDdsaAdapter> ()
        .SetGroupName ("Ddsa")
        .AddConstructor<Ipv4L3ProtocolSmartDdsaAdapter> ();
      return tid;
    }

    Ipv4L3ProtocolSmartDdsaAdapter::Ipv4L3ProtocolSmartDdsaAdapter()
    {

    }

    Ipv4L3ProtocolSmartDdsaAdapter::~Ipv4L3ProtocolSmartDdsaAdapter(){}



    void
	Ipv4L3ProtocolSmartDdsaAdapter::Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
    {
		if (!ShouldFail(packet->Copy(), false))
		{
			Ptr<SmartDdsaRoutingProtocolAdapter> rp = GetMyNode()->GetObject<SmartDdsaRoutingProtocolAdapter>();//DynamicCast<Ipv4ListRouting>(this->GetRoutingProtocol());

			if (rp)
			{
				std::vector<SmartDap> daps = rp->GetNotExcludedDaps();

				if (rp->GetNodeType() == SmartDdsaRoutingProtocolAdapter::NodeType::METER &&
					rp->GetControllerAddress() == destination &&
					daps.size() > 0)
				{
					for (std::vector<SmartDap>::iterator it = daps.begin(); it != daps.end(); it++)
					{
						SmartDap selectedDap = *(it);

						Ipv4Header header;
						header.SetDestination (selectedDap.address);
						header.SetProtocol (protocol);

						Socket::SocketErrno errno_;

						Ptr<Ipv4Route> newroute = rp->RouteOutput(packet, header, 0, errno_);

						if (newroute != 0)
						{
							for (int i = 0; i < selectedDap.totalCopies; i++)
							{
								Ipv4L3Protocol::Send(packet, source, selectedDap.address, protocol, newroute);
							}
						}
					}
				}
				else
				{
					Ipv4L3Protocol::Send(packet, source, destination, protocol, route);
				}
			}
			else
			{
				NS_LOG_DEBUG("There is no routing protocol: Not sending" << "\n");
			}
		}
		else
		{
			NS_LOG_DEBUG("Sending: Node " << source << " is failing, so the packet will be discarded!.(t = " << Simulator::Now() << "\n");
		}
    }

  }
}
