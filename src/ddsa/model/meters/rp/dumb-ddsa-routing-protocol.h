/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DUMB_DDSA_ROUTING_PROTOCOL_H
#define DUMB_DDSA_ROUTING_PROTOCOL_H


#include "ns3/simulator.h"
#include "ns3/dap.h"
#include "ns3/original-ddsa-routing-protocol.h"

namespace ns3 {

	namespace ddsa{

		class DumbDdsaRoutingProtocol: public OriginalDdsaRoutingProtocol
		{
			public:
				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				DumbDdsaRoutingProtocol ();
				virtual ~DumbDdsaRoutingProtocol ();

			protected:
				virtual bool MustExclude(Dap dap);
				virtual void CalculateProbabilities();

			private:

		};

	}
}

#endif /* DUMB_DDSA_ROUTING_PROTOCOL_H */

