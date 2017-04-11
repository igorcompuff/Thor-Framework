/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DDSA_H
#define DDSA_H

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

    class DdsaAdapter : public ns3::lqolsr::RoutingProtocol
    {
      public:

	/**
	 * \brief Get the type ID.
	 * \return The object TypeId.
	 */
	static TypeId GetTypeId (void);

	DdsaAdapter ();
	virtual ~DdsaAdapter ();

      protected:

	virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
	virtual void ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface);
	void LqRoutingTableComputation ();

      private:

	void CalculateProbabilities();
	double SumUpNotExcludedDapCosts();
	bool ExcludeDaps();
	Dap SelectDap();
	void AssociationTupleTimerExpire (Ipv4Address address);
	void BuildEligibleGateways();

	//std::vector<Ipv4Address> mdmsList; To be implemented multiple mdms endpoints

	std::vector<Dap> m_gateways;
	double alpha;
	Ptr<UniformRandomVariable> m_rnd;
	int n_retransmissions;


    };

  }
}

#endif /* DDSA_H */

