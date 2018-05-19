/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "global-retrans-ddsa-routing-protocol-base.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("GlobalRetransDdsaRoutingProtocolBase");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (GlobalRetransDdsaRoutingProtocolBase);

		TypeId
		GlobalRetransDdsaRoutingProtocolBase::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::GlobalRetransDdsaRoutingProtocolBase")
				.SetParent<DdsaRoutingProtocolBase> ()
				.SetGroupName ("Ddsa");

			return tid;
		}

		GlobalRetransDdsaRoutingProtocolBase::GlobalRetransDdsaRoutingProtocolBase(){}

		GlobalRetransDdsaRoutingProtocolBase::~GlobalRetransDdsaRoutingProtocolBase(){}
	}
}
