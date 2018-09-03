/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "individual-retrans-ddsa-ipv4-l3-protocol.h"
#include "ns3/simple-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/dap.h"
#include <vector>

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("IndividualRetransDdsaIpv4L3Protocol");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (IndividualRetransDdsaIpv4L3Protocol);

		TypeId
		IndividualRetransDdsaIpv4L3Protocol::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::IndividualRetransDdsaIpv4L3Protocol")
			.SetParent<DdsaIpv4L3ProtocolBase> ()
			.SetGroupName ("Ddsa")
			.AddConstructor<IndividualRetransDdsaIpv4L3Protocol> ();

			return tid;
		}

		IndividualRetransDdsaIpv4L3Protocol::IndividualRetransDdsaIpv4L3Protocol()
		{

		}

		IndividualRetransDdsaIpv4L3Protocol::~IndividualRetransDdsaIpv4L3Protocol(){}

		void
		IndividualRetransDdsaIpv4L3Protocol::SendToDap(const Ipv4Address dapAddress, Ptr<Packet> packet, Ipv4Address source, uint8_t protocol, Ptr<Ipv4Route> route)
		{
			Ptr<IndividualRetransDdsaRoutingProtocolBase> rp = GetMyNode()->GetObject<IndividualRetransDdsaRoutingProtocolBase>();

			NS_ASSERT(rp);

			int transmissions = rp->GetTotalTransmissionsFor(dapAddress);

			NS_LOG_DEBUG("Meter " << GetMyNode()->GetId() << " will send "
								  << transmissions << " copies to " << dapAddress << " at "
								  << Simulator::Now().GetSeconds() << " s\n");


			for(int i = 1; i <= transmissions; i++)
			{
				DdsaIpv4L3ProtocolBase::DoSend(packet, source, dapAddress, protocol, route);
				m_txTrace(packet, dapAddress);
			}
		}

		Ptr<Ipv4Route>
		IndividualRetransDdsaIpv4L3Protocol::GetNewRoute(const Ipv4Address & dapAddress, Ptr<Packet> packet, uint8_t protocol)
		{
			Ptr<IndividualRetransDdsaRoutingProtocolBase> rp = GetMyNode()->GetObject<IndividualRetransDdsaRoutingProtocolBase>();

			NS_ASSERT(rp);

			Ipv4Header header;
			header.SetDestination (dapAddress);
			header.SetProtocol (protocol);

			Socket::SocketErrno errno_;

			return rp->RouteOutput(packet, header, 0, errno_);
		}

		void
		IndividualRetransDdsaIpv4L3Protocol::DoSend (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
		{
			Ptr<IndividualRetransDdsaRoutingProtocolBase> rp = GetMyNode()->GetObject<IndividualRetransDdsaRoutingProtocolBase>();

			NS_ASSERT(rp);

			std::vector<Dap> selectedDaps = rp->SelectDaps();

			for(std::vector<Dap>::iterator it = selectedDaps.begin(); it != selectedDaps.end(); it++)
			{
				Dap selectedDap = *it;

				Ptr<Ipv4Route> newRoute = GetNewRoute(selectedDap.GetAddress(), packet, protocol);
				NS_ASSERT(newRoute); //The meter must have routes to all available DAPS.

				SendToDap(selectedDap.GetAddress(), packet, source, protocol, newRoute);
			}

			if (selectedDaps.size() == 0)
			{
				//Trying to send in ETX fashion
				DdsaIpv4L3ProtocolBase::DoSend(packet, source, destination, protocol, route);
				m_txTrace(packet, Ipv4Address::GetBroadcast());
			}
		}

	}
}
