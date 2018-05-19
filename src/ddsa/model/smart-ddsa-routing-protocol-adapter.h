/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef SMART_DDSA_ROUTING_PROTOCOL_H
#define SMART_DDSA_ROUTING_PROTOCOL_H

//#include "ns3/lq-olsr-routing-protocol.h"
#include "malicious-lq-routing-protocol.h"
#include "ns3/simulator.h"
#include "ns3/lq-olsr-repositories.h"

namespace ns3 {

	namespace ddsa{

		struct SmartDap
		{
			Ipv4Address address; //!< Address of the destination node.
			float cost;
			bool excluded;
			Time expirationTime;
			int totalCopies;
			int hops;
	
			SmartDap ()
			{
				address = Ipv4Address::GetBroadcast();
				totalCopies = 0;
				cost = -1;
				excluded = false;
				hops = 0;
			}
	
			bool operator==(const SmartDap& rhs){ return (address == rhs.address); }
		};


		class SmartDdsaRoutingProtocolAdapter : public MaliciousLqRoutingProtocol
		{
			public:

				enum NodeType
				{
					METER, DAP
				};

				/**
				* \brief Get the type ID.
				* \return The object TypeId.
				*/
				static TypeId GetTypeId (void);

				SmartDdsaRoutingProtocolAdapter ();
				virtual ~SmartDdsaRoutingProtocolAdapter ();
				double GetAlpha();
				int GetTotalCurrentEligibleDaps();
				void SetControllerAddress(Ipv4Address address);
				Ipv4Address GetControllerAddress();
				SmartDap GetBestCostDap();
				void SetNodeType(NodeType nType);
				NodeType GetNodeType();
				float GetMeanDapCost();
				std::vector<SmartDap> GetNotExcludedDaps();

				typedef void (* NewRouteComputedTracedCallback)
				(std::vector<SmartDap> daps, const Ipv4Address & address);

				typedef void (* SelectedTracedCallback)
				(const Ipv4Header & header, Ptr<Packet> packet);

				typedef void (* NeighborExpiredCallback)
				(Ipv4Address neighborIfaceAddr);

			protected:

				virtual void SendTc ();
				virtual void ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface);
				virtual void ExcludeDaps();
				virtual void LinkTupleTimerExpire (Ipv4Address neighborIfaceAddr);
				void RoutingTableComputation ();

			private:

				void AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask);
				bool DapExists(const Ipv4Address & dapAddress);
				void UpdateDapCosts();
				void PrintDaps (Ptr<OutputStreamWrapper> stream) const;
				void ClearDapExclusions();

				std::map<Ipv4Address, SmartDap> m_gateways;
				double alpha;
				Ptr<UniformRandomVariable> m_rnd;
				Ipv4Address controllerAddress;

				TracedCallback<std::vector<SmartDap>, const Ipv4Address & > m_newRouteComputedTrace;
				TracedCallback<Ipv4Address> m_neighborExpiredTrace;
				NodeType m_type;
		};

  }
}

#endif /* SMART_DDSA_ROUTING_PROTOCOL_H */

