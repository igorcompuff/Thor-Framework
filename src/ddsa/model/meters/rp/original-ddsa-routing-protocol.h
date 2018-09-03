/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef ORIGINAL_DDSA_ROUTING_PROTOCOL_H
#define ORIGINAL_DDSA_ROUTING_PROTOCOL_H


#include "ns3/simulator.h"
#include "ns3/dap.h"
#include "ns3/global-retrans-ddsa-routing-protocol-base.h"

namespace ns3 {

	namespace ddsa{

		class OriginalDdsaRoutingProtocol: public GlobalRetransDdsaRoutingProtocolBase
		{
			public:
				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				OriginalDdsaRoutingProtocol ();
				virtual ~OriginalDdsaRoutingProtocol ();
				void SetAlpha(double alpha);
				double GetAlpha();
				virtual int GetTotalTransmissions();
				virtual std::vector<Dap> SelectDaps();

			protected:
				virtual bool MustExclude(Dap dap);
				virtual void BuildEligibleGateways();
				virtual void CalculateProbabilities();
				unsigned int GetMinRedundancy();

			private:
				Dap GetBestDap();
				double SumUpNotExcludedDapCosts();

				double m_alpha;
				int m_retrans;
				unsigned int m_minRedundancy;
				Ptr<UniformRandomVariable> m_rnd;
		};

	}
}

#endif /* ORIGINAL_DDSA_ROUTING_PROTOCOL_H */

