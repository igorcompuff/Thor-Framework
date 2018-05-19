/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DDSA_ROUTING_PROTOCOL_BASE_H
#define DDSA_ROUTING_PROTOCOL_BASE_H

//#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/simulator.h"
#include "ns3/lq-olsr-repositories.h"
#include "ns3/dap.h"

namespace ns3 {

	namespace ddsa{

		class DdsaRoutingProtocolBase: public lqolsr::RoutingProtocol
		{
			public:
				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				DdsaRoutingProtocolBase ();
				virtual ~DdsaRoutingProtocolBase ();
				virtual std::vector<Dap> SelectDaps() = 0;
				bool HasAvailableDaps();


			protected:
				virtual bool MustExclude(Dap dap) = 0;
				virtual void BuildEligibleGateways() = 0;
				virtual void UpdateDapCosts();
				int GetTotalNotExcludedDaps();
				bool ExcludeDaps();
				virtual Dap AddDap(Ipv4Address address, float cost, Time expirationTime);
				std::vector<Dap>::iterator FindDap(Ipv4Address address);

				std::vector<Dap> m_gateways;

			private:
				void ClearDapExclusions();
				void PrintDaps (Ptr<OutputStreamWrapper> stream);

				//Inherited from ns3::lqplsr::RoutingProtocol
				void RoutingTableComputation ();
				void ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface);
				void AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask);
		};

	}
}

#endif /* DDSA_ROUTING_PROTOCOL_BASE_H */

