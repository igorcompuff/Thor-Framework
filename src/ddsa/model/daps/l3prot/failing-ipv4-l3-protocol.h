/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef FAILING_IPV4_L3_PROTOCOL_H
#define FAILING_IPV4_L3_PROTOCOL_H

#include "ns3/ipv4-l3-protocol.h"

namespace ns3 {

	namespace ddsa{

		class FailingIpv4L3Protocol : public  Ipv4L3Protocol
		{
			public:

				enum FailureType
				{
					FULL, MALICIOUS
				};

				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				FailingIpv4L3Protocol ();
				virtual ~FailingIpv4L3Protocol ();

				void Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);
				void Receive (Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from, const Address &to, NetDevice::PacketType packetType);
				void MakeFail();
				void SetFailureType(FailureType fType);
				FailureType GetFailureType();

			protected:
				bool IsApplicationPacket(Ptr<Packet> packet, bool receiveMethod);
				bool ShouldFail(Ptr<Packet> packet, bool received);
				virtual void DoSend(Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);
				virtual void DoReceive(Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from, const Address &to, NetDevice::PacketType packetType);

			private:

				bool mustFail;
				FailureType m_failureType;
		};

	}
}

#endif /* FAILING_IPV4_L3_PROTOCOL_H */

