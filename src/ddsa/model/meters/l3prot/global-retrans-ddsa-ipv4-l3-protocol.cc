/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "global-retrans-ddsa-ipv4-l3-protocol.h"
#include "ns3/global-retrans-ddsa-routing-protocol-base.h"
#include "ns3/simple-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/dap.h"
#include <vector>

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("GlobalRetransDdsaIpv4L3Protocol");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (GlobalRetransDdsaIpv4L3Protocol);

		TypeId
		GlobalRetransDdsaIpv4L3Protocol::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::GlobalRetransDdsaIpv4L3Protocol")
			.SetParent<DdsaIpv4L3ProtocolBase> ()
			.SetGroupName ("Ddsa")
			.AddConstructor<GlobalRetransDdsaIpv4L3Protocol> ()
			.AddTraceSource ("L3tx", "Packet is sent forward",
							 MakeTraceSourceAccessor (&GlobalRetransDdsaIpv4L3Protocol::m_txTrace),
							 "ns3::ddsa::PacketSentTracedCallback");

			return tid;
		}

		GlobalRetransDdsaIpv4L3Protocol::GlobalRetransDdsaIpv4L3Protocol()
		{

		}

		GlobalRetransDdsaIpv4L3Protocol::~GlobalRetransDdsaIpv4L3Protocol(){}


		void
		GlobalRetransDdsaIpv4L3Protocol::DoSend (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
		{
			Ptr<GlobalRetransDdsaRoutingProtocolBase> rp = GetMyNode()->GetObject<GlobalRetransDdsaRoutingProtocolBase>();


			bool processed = false;


			if (destination == GetControllerAddress() && rp)
			{
				if (rp->HasAvailableDaps())
				{
					int globalRetrans = rp->GetTotalRetransmissions();

					for (int i = 0; i < globalRetrans; i++)
					{
						std::vector<Dap> selectedDaps = rp->SelectDaps();

						Dap selectedDap = selectedDaps[0];

						Ipv4Header header;
						header.SetDestination (selectedDap.GetAddress());
						header.SetProtocol (protocol);

						Socket::SocketErrno errno_;

						Ptr<Ipv4Route> newroute = rp->RouteOutput(packet, header, 0, errno_);


						//The meter must have routes to all available DAPS.
						NS_ASSERT(newroute);

						m_txTrace(packet, selectedDap.GetAddress());
						DdsaIpv4L3ProtocolBase::DoSend(packet, source, selectedDap.GetAddress(), protocol, newroute);
					}

					processed = true;

				}
			}

			if (!processed)
			{
				DdsaIpv4L3ProtocolBase::DoSend(packet, source, destination, protocol, route);
			}
		}

	}
}
