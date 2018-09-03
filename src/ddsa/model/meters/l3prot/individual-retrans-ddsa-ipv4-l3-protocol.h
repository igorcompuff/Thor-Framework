/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef INDIVIDUAL_RETRANS_DDSA_IPV4_L3_PROTOCOL_H
#define INDIVIDUAL_RETRANS_DDSA_IPV4_L3_PROTOCOL_H

#include "ns3/ddsa-ipv4-l3-protocol-base.h"
#include "ns3/individual-retrans-ddsa-routing-protocol-base.h"
#include "ns3/traced-callback.h"

namespace ns3 {

	namespace ddsa{

		class IndividualRetransDdsaIpv4L3Protocol : public DdsaIpv4L3ProtocolBase
		{
			public:

				static TypeId GetTypeId (void);

				IndividualRetransDdsaIpv4L3Protocol ();
				virtual ~IndividualRetransDdsaIpv4L3Protocol ();

			protected:
				virtual void DoSend(Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);

			private:
				void SendToDap(const Ipv4Address dapAddress, Ptr<Packet> packet, Ipv4Address source, uint8_t protocol, Ptr<Ipv4Route> route);
				Ptr<Ipv4Route> GetNewRoute(const Ipv4Address & dapAddress, Ptr<Packet> packet, uint8_t protocol);
		};

	}
}

#endif /* INDIVIDUAL_RETRANS_DDSA_IPV4_L3_PROTOCOL_H */

