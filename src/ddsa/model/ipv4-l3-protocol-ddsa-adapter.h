/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_L3_PROTOCOL_DDSA_ADAPTER_H
#define IPV4_L3_PROTOCOL_DDSA_ADAPTER_H

#include "ns3/ipv4-l3-protocol.h"

namespace ns3 {

	namespace ddsa{

		class Ipv4L3ProtocolDdsaAdapter : public Ipv4L3Protocol
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

				Ipv4L3ProtocolDdsaAdapter ();
				virtual ~Ipv4L3ProtocolDdsaAdapter ();

				virtual void Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);
				virtual void Receive ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from, const Address &to, NetDevice::PacketType packetType);
				void MakeFail();
				void SetFailureType(FailureType fType);

			protected:
				bool IsApplicationPacket(Ptr<Packet> packet, bool receiveMethod);
				bool ShouldFail(Ptr<Packet> packet, bool received);
			private:
				bool mustFail;
				FailureType m_failureType;
				/// and will be changed to \c Ptr<const Ipv4> in a future release.
				TracedCallback<Ptr<const Packet>, Ptr<Ipv4>, uint32_t> m_receivedTrace;
		};

	}
}

#endif /* IPV4_L3_PROTOCOL_DDSA_ADAPTER_H */

