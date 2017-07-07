/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/lq-olsr-repositories.h"
#include <algorithm>
#include "ddsa-routing-protocol-dap-adapter.h"

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("DdsaRoutingProtocolDapAdapter");

  namespace ddsa {

    NS_OBJECT_ENSURE_REGISTERED (DdsaRoutingProtocolDapAdapter);

    TypeId
    DdsaRoutingProtocolDapAdapter::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::ddsa::DdsaRoutingProtocolDapAdapter")
        .SetParent<RoutingProtocol> ()
        .SetGroupName ("Ddsa")
        .AddConstructor<DdsaRoutingProtocolDapAdapter> ();
      return tid;
    }

    DdsaRoutingProtocolDapAdapter::DdsaRoutingProtocolDapAdapter()
    {
      controllerAddress = Ipv4Address::GetBroadcast();
    }

    DdsaRoutingProtocolDapAdapter::~DdsaRoutingProtocolDapAdapter(){}

    void
    DdsaRoutingProtocolDapAdapter::setControllerAddress(const Ipv4Address & address)
    {
      controllerAddress = address;
    }

    bool
    DdsaRoutingProtocolDapAdapter::RouteInput (Ptr<const Packet> p,
					       const Ipv4Header &header,
					       Ptr<const NetDevice> idev,
					       UnicastForwardCallback ucb,
					       MulticastForwardCallback mcb,
					       LocalDeliverCallback lcb,
					       ErrorCallback ecb)
    {
//      NS_LOG_DEBUG("RouteInput DAP");
//      Ipv4Header copyHeader(header);
//      if (header.GetDestination() == GetMyMainAddress())
//	{
//	  copyHeader.SetDestination(controllerAddress);
//	}

      return false;//RoutingProtocol::RouteInput(p, copyHeader, idev, ucb, mcb, lcb, ecb);
    }


  }
}

