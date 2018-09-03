/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ddsa-ipv4-l3-protocol-base.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("DdsaIpv4L3ProtocolBase");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (DdsaIpv4L3ProtocolBase);

		TypeId
		DdsaIpv4L3ProtocolBase::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::DdsaIpv4L3ProtocolBase")
			.SetParent<Ipv4L3Protocol> ()
			.SetGroupName ("Ddsa")
			.AddConstructor<DdsaIpv4L3ProtocolBase> ()
			.AddTraceSource ("L3tx", "Packet is sent forward",
							  MakeTraceSourceAccessor (&DdsaIpv4L3ProtocolBase::m_txTrace),
							  "ns3::ddsa::PacketSentTracedCallback");

			return tid;
		}

		DdsaIpv4L3ProtocolBase::DdsaIpv4L3ProtocolBase()
		{
		}

		DdsaIpv4L3ProtocolBase::~DdsaIpv4L3ProtocolBase(){}

		void
		DdsaIpv4L3ProtocolBase::Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
		{
			if (destination == m_controllerAddress)
			{
				DoSend(packet, source, destination, protocol, route);
			}
			else
			{
				Ipv4L3Protocol::Send(packet, source, destination, protocol, route);
			}
		}

		void
		DdsaIpv4L3ProtocolBase::SetControllerAddress(Ipv4Address address)
		{
			m_controllerAddress = address;
		}

		Ipv4Address
		DdsaIpv4L3ProtocolBase::GetControllerAddress()
		{
			return m_controllerAddress;
		}

		void
		DdsaIpv4L3ProtocolBase::DoSend (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
		{
			Ipv4L3Protocol::Send(packet, source, destination, protocol, route);
		}

	}
}
