/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GLOBAL_RETRANS_DDSA_ROUTING_PROTOCOL_H
#define GLOBAL_RETRANS_DDSA_ROUTING_PROTOCOL_H

#include "ns3/ddsa-routing-protocol-base.h"
#include "ns3/simulator.h"

namespace ns3 {

	namespace ddsa{

		class GlobalRetransDdsaRoutingProtocolBase: public DdsaRoutingProtocolBase
		{
			public:

				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				GlobalRetransDdsaRoutingProtocolBase ();
				virtual ~GlobalRetransDdsaRoutingProtocolBase ();
				virtual int GetTotalTransmissions() = 0;
		};

	}
}

#endif /* GLOBAL_RETRANS_DDSA_ROUTING_PROTOCOL_H */

