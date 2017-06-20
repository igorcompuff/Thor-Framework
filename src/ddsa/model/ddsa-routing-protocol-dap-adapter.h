/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DDSA_ROUTING_PROTOCOL_DAP_ADAPTER_H
#define DDSA_ROUTING_PROTOCOL_DAP_ADAPTER_H

#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/simulator.h"

namespace ns3 {

  namespace ddsa{

    class DdsaRoutingProtocolDapAdapter : public ns3::lqolsr::RoutingProtocol
    {
      public:

	/**
	 * \brief Get the type ID.
	 * \return The object TypeId.
	 */
	static TypeId GetTypeId (void);

	DdsaRoutingProtocolDapAdapter ();
	virtual ~DdsaRoutingProtocolDapAdapter ();

	void setControllerAddress(const Ipv4Address & address);

      protected:

	virtual bool RouteInput (Ptr<const Packet> p,
	                           const Ipv4Header &header,
	                           Ptr<const NetDevice> idev,
	                           UnicastForwardCallback ucb,
	                           MulticastForwardCallback mcb,
	                           LocalDeliverCallback lcb,
	                           ErrorCallback ecb);
      private:
	Ipv4Address controllerAddress;
    };

  }
}

#endif /* DDSA_ROUTING_PROTOCOL_DAP_ADAPTER_H */

