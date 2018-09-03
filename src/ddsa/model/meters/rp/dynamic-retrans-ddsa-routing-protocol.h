/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DYNAMIC_RETRANS_DDSA_ROUTING_PROTOCOL_H
#define DYNAMIC_RETRANS_DDSA_ROUTING_PROTOCOL_H


#include "original-ddsa-routing-protocol.h"
#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/lq-olsr-repositories.h"

namespace ns3 {

	namespace ddsa{

		class DynamicRetransDdsaRoutingProtocol: public OriginalDdsaRoutingProtocol
		{
			public:
				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				DynamicRetransDdsaRoutingProtocol ();
				virtual ~DynamicRetransDdsaRoutingProtocol ();
				virtual int GetTotalTransmissions();

			protected:
				virtual bool MustExclude(Dap dap);
				virtual void InitializeDestinations();
				virtual float UpdateDestination(lqolsr::DestinationTuple & tupleToUpdate, const lqolsr::DestinationTuple & destinationFound, const lqolsr::TopologyTuple & topTuple);

			private:

				void InitializeDeliveryProbabilities();
				double CalculateLinkDeliveryProbability(float linkEtx);
				double CalculateOneShootDeliveryProbability();
				bool ShouldConsiderDap(const Ipv4Address & dapAddress);
				bool DapExists(const Ipv4Address & dapAddress);

				double m_targetProbability;
				double m_thresholdProbability;
				int linkLayerTrans;
				std::map<Ipv4Address, double> m_delivery_probs;

		};

	}
}

#endif /* DYNAMIC_RETRANS_DDSA_ROUTING_PROTOCOL_H */

