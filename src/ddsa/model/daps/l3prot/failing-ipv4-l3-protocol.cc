/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "failing-ipv4-l3-protocol.h"
#include "ns3/simple-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("FailingIpv4L3Protocol");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (FailingIpv4L3Protocol);

		TypeId
		FailingIpv4L3Protocol::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::FailingIpv4L3Protocol")
			.SetParent<Ipv4L3Protocol> ()
			.SetGroupName ("Ddsa")
			.AddConstructor<FailingIpv4L3Protocol> ();

			return tid;
		}

		FailingIpv4L3Protocol::FailingIpv4L3Protocol()
		{
			mustFail = false;
			m_failureType = FailureType::FULL;
		}

		FailingIpv4L3Protocol::~FailingIpv4L3Protocol(){}

		void
		FailingIpv4L3Protocol::MakeFail()
		{
			mustFail = true;
		}

		bool
		FailingIpv4L3Protocol::IsApplicationPacket(Ptr<Packet> packet, bool received)
		{
			Ipv4Header ipHeader;
			UdpHeader udpHeader;
			TcpHeader tcpHeader;
			ami::AmiHeader amiHeader;


			/*
			* Only received packets have Ip header aggregated. Sent packets have only the application and the
			* transport headers
			*/
			if (received)
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

		bool
		FailingIpv4L3Protocol::ShouldFail(Ptr<Packet> packet, bool received)
		{
			bool shouldFail = false;

			if (mustFail)
			{
				switch(m_failureType)
				{
					case FailureType::FULL: shouldFail = true; break;
					case FailureType::MALICIOUS: shouldFail = IsApplicationPacket(packet->Copy(), received); break;
				}
			}

			return shouldFail;
		}

		void
		FailingIpv4L3Protocol::Receive (Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from, const Address &to, NetDevice::PacketType packetType)
		{
			if (!ShouldFail(p->Copy(), true))
			{
				DoReceive(device, p, protocol, from, to, packetType);
			}
			else
			{
				NS_LOG_DEBUG("Receiveing: Node is failing, so the packet will be discarded!.(t = " << Simulator::Now() << "\n");
			}
		}

		void
		FailingIpv4L3Protocol::DoReceive ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from, const Address &to, NetDevice::PacketType packetType)
		{
			Ipv4L3Protocol::Receive(device, p, protocol, from, to, packetType);
		}

		void
		FailingIpv4L3Protocol::Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
		{
			if (!ShouldFail(packet->Copy(), false))
			{
				DoSend(packet, source, destination, protocol, route);
			}
			else
			{
				NS_LOG_DEBUG("Sending: Node " << source << " is failing, so the packet will be discarded!.(t = " << Simulator::Now() << "\n");
			}
		}

		void
		FailingIpv4L3Protocol::DoSend (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
		{
			Ipv4L3Protocol::Send(packet, source, destination, protocol, route);
		}

		void
		FailingIpv4L3Protocol::SetFailureType(FailureType fType)
		{
			m_failureType = fType;
		}

		FailingIpv4L3Protocol::FailureType
		FailingIpv4L3Protocol::GetFailureType()
		{
			return m_failureType;
		}
	}
}
