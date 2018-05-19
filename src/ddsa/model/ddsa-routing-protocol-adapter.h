/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DDSA_ROUTING_PROTOCOL_H
#define DDSA_ROUTING_PROTOCOL_H

//#include "ns3/lq-olsr-routing-protocol.h"
#include "malicious-lq-routing-protocol.h"
#include "ns3/simulator.h"
#include "ns3/lq-olsr-repositories.h"
#include "ns3/dap.h"

namespace ns3 {

  namespace ddsa{

    class DdsaRoutingProtocolAdapter : public MaliciousLqRoutingProtocol
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

			DdsaRoutingProtocolAdapter ();
			virtual ~DdsaRoutingProtocolAdapter ();
			double GetAlpha();
			int GetTotalCurrentEligibleDaps();
			void SetControllerAddress(Ipv4Address address);
			Ipv4Address GetControllerAddress();
			Dap GetBestCostDap();
			void SetNodeType(NodeType nType);
			float GetMeanDapCost();

			typedef void (* NewRouteComputedTracedCallback)
			(std::vector<Dap> daps, const Ipv4Address & address);

			typedef void (* SelectedTracedCallback)
			(const Ipv4Header & header, Ptr<Packet> packet);

			typedef void (* NeighborExpiredCallback)
			(Ipv4Address neighborIfaceAddr);

			virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

		protected:

			virtual void SendTc ();
			virtual void ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface);
			virtual void CalculateProbabilities();
			virtual bool ExcludeDaps();
			virtual void LinkTupleTimerExpire (Ipv4Address neighborIfaceAddr);
			void RoutingTableComputation ();

		private:

			double SumUpNotExcludedDapCosts();
			void AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask);
			void BuildEligibleGateways();
			bool DapExists(const Ipv4Address & dapAddress);
			void UpdateDapCosts();
			void PrintDaps (Ptr<OutputStreamWrapper> stream);
			void ClearDapExclusions();
			Dap SelectDap();

			std::map<Ipv4Address, Dap> m_gateways;
			double alpha;
			Ptr<UniformRandomVariable> m_rnd;
			bool dumb;
			Ipv4Address controllerAddress;

			TracedCallback<std::vector<Dap>, const Ipv4Address & > m_newRouteComputedTrace;
			TracedCallback<const Ipv4Address &, Ptr<Packet> > m_dapSelectionTrace;
			TracedCallback<Ipv4Address> m_neighborExpiredTrace;
			NodeType m_type;
			double m_desiredDeliveryProbability;
    };

  }
}

#endif /* DDSA_ROUTING_PROTOCOL_H */

