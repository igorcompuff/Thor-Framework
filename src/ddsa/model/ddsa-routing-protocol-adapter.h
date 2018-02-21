/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DDSA_ROUTING_PROTOCOL_H
#define DDSA_ROUTING_PROTOCOL_H

#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/simulator.h"

namespace ns3 {

  namespace ddsa{

    struct Dap
    {
      Ipv4Address address; //!< Address of the destination node.
      double probability; //!< Address of the next hop.
      float cost;
      bool excluded;
      Time expirationTime;

      Dap () : // default values
	address (), probability (0),
	cost (-1), excluded (false)
      {
      }

      bool operator==(const Dap& rhs){ return (address == rhs.address); }
    };

    class DdsaRoutingProtocolAdapter : public ns3::lqolsr::RoutingProtocol
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

	typedef void (* NewRouteComputedTracedCallback)
		    (std::vector<Dap> daps);

	typedef void (* SelectedTracedCallback)
		    (const Ipv4Header & header, Ptr<Packet> packet);

      protected:

	virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
	virtual void SendTc ();
	virtual void ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface);
	virtual void CalculateProbabilities();
	virtual bool ExcludeDaps();
	void RoutingTableComputation ();

      private:


	double SumUpNotExcludedDapCosts();

	void AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask);
	void BuildEligibleGateways();
	bool DapExists(const Ipv4Address & dapAddress);
	void UpdateDapCosts();
	void PrintDaps (Ptr<OutputStreamWrapper> stream) const;
	void ClearDapExclusions();
	Dap SelectDap();

	std::map<Ipv4Address, Dap> m_gateways;
	double alpha;
	Ptr<UniformRandomVariable> m_rnd;
	bool dumb;
	Ipv4Address controllerAddress;

	TracedCallback<std::vector<Dap> > m_newRouteComputedTrace;
	TracedCallback<const Ipv4Address &, Ptr<Packet> > m_dapSelectionTrace;
	NodeType m_type;
    };

  }
}

#endif /* DDSA_ROUTING_PROTOCOL_H */

