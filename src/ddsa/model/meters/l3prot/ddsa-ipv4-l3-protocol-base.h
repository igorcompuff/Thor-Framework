/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DDSA_IPV4_L3_PROTOCOL_BASE_H
#define DDSA_IPV4_L3_PROTOCOL_BASE_H

#include "ns3/ipv4-l3-protocol.h"

namespace ns3 {

	namespace ddsa{

		class DdsaIpv4L3ProtocolBase : public Ipv4L3Protocol
		{
			public:

				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				DdsaIpv4L3ProtocolBase ();
				virtual ~DdsaIpv4L3ProtocolBase ();

				virtual void Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);
				void SetControllerAddress(Ipv4Address address);
				Ipv4Address GetControllerAddress();

			protected:
				virtual void DoSend(Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);

			private:
				Ipv4Address m_controllerAddress;
		};

	}
}

#endif /* DDSA_IPV4_L3_PROTOCOL_BASE_H */

