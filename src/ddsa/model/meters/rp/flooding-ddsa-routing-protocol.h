/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef FLOODING_DDSA_ROUTING_PROTOCOL_H
#define FLOODING_DDSA_ROUTING_PROTOCOL_H

//#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/individual-retrans-ddsa-routing-protocol-base.h"
#include "ns3/simulator.h"

namespace ns3 {

	namespace ddsa{

		class FloodingDdsaRoutingProtocol: public IndividualRetransDdsaRoutingProtocolBase
		{
			public:

				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				FloodingDdsaRoutingProtocol ();
				virtual ~FloodingDdsaRoutingProtocol ();
				virtual std::vector<Dap> SelectDaps();
				virtual int GetTotalTransmissionsFor(const Ipv4Address & destAddress);

			protected:
				virtual bool MustExclude(Dap dap);
				virtual void BuildEligibleGateways();
		};

	}
}

#endif /* FLOODING_DDSA_ROUTING_PROTOCOL_H */

