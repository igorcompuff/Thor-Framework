/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef INDIVIDUAL_RETRANS_DDSA_ROUTING_PROTOCOL_H
#define INDIVIDUAL_RETRANS_DDSA_ROUTING_PROTOCOL_H

#include "ns3/ddsa-routing-protocol-base.h"
#include "ns3/simulator.h"

namespace ns3 {

	namespace ddsa{

		class IndividualRetransDdsaRoutingProtocolBase: public DdsaRoutingProtocolBase
		{
			public:

				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				IndividualRetransDdsaRoutingProtocolBase ();
				virtual ~IndividualRetransDdsaRoutingProtocolBase ();
				virtual int GetTotalTransmissionsFor(const Ipv4Address & address) = 0;
		};

	}
}

#endif /* INDIVIDUAL_RETRANS_DDSA_ROUTING_PROTOCOL_H */

