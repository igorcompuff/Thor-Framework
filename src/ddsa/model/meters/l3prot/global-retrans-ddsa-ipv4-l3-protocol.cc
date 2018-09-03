/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "global-retrans-ddsa-ipv4-l3-protocol.h"
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
			.AddConstructor<GlobalRetransDdsaIpv4L3Protocol> ();
//			.AddTraceSource ("L3tx", "Packet is sent forward",
//							 MakeTraceSourceAccessor (&GlobalRetransDdsaIpv4L3Protocol::m_txTrace),
//							 "ns3::ddsa::PacketSentTracedCallback");

			return tid;
		}

		GlobalRetransDdsaIpv4L3Protocol::GlobalRetransDdsaIpv4L3Protocol()
		{

		}

		GlobalRetransDdsaIpv4L3Protocol::~GlobalRetransDdsaIpv4L3Protocol(){}

		Ptr<Ipv4Route>
		GlobalRetransDdsaIpv4L3Protocol::GetNewRoute(const Ipv4Address & dapAddress, Ptr<Packet> packet, uint8_t protocol)
		{
			Ptr<GlobalRetransDdsaRoutingProtocolBase> rp = GetMyNode()->GetObject<GlobalRetransDdsaRoutingProtocolBase>();

			NS_ASSERT(rp);

			Ipv4Header header;
			header.SetDestination (dapAddress);
			header.SetProtocol (protocol);

			Socket::SocketErrno errno_;

			return rp->RouteOutput(packet, header, 0, errno_);
		}

		void
		GlobalRetransDdsaIpv4L3Protocol::SendToDap(Ptr<Packet> packet, Ipv4Address source, uint8_t protocol)
		{
			Ptr<GlobalRetransDdsaRoutingProtocolBase> rp = GetMyNode()->GetObject<GlobalRetransDdsaRoutingProtocolBase>();

			NS_ASSERT(rp);

			std::vector<Dap> selectedDaps = rp->SelectDaps();

			NS_ASSERT(selectedDaps.size() == 1);

			Dap selectedDap = selectedDaps[0];

			Ptr<Ipv4Route> newroute = GetNewRoute(selectedDap.GetAddress(), packet, protocol);

			//The meter must have routes to all available DAPS.
			NS_ASSERT(newroute);

			DdsaIpv4L3ProtocolBase::DoSend(packet, source, selectedDap.GetAddress(), protocol, newroute);
			m_txTrace(packet, selectedDap.GetAddress());
		}

		void
		GlobalRetransDdsaIpv4L3Protocol::DoSend (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
		{
			Ptr<GlobalRetransDdsaRoutingProtocolBase> rp = GetMyNode()->GetObject<GlobalRetransDdsaRoutingProtocolBase>();

			NS_ASSERT(rp);

			int totalTransmissions = rp->GetTotalTransmissions();

			NS_LOG_DEBUG("Meter " << GetMyNode()->GetId() << " will send "
								  << totalTransmissions << " copies at "
								  << Simulator::Now().GetSeconds() << " s\n");

			for (int i = 0; i < totalTransmissions; i++)
			{
				if (rp->HasAvailableDaps())
				{
					SendToDap(packet, source, protocol);
				}
				else
				{
					DdsaIpv4L3ProtocolBase::DoSend(packet, source, destination, protocol, route);
					m_txTrace(packet, Ipv4Address::GetBroadcast());
				}
			}
		}

	}
}
