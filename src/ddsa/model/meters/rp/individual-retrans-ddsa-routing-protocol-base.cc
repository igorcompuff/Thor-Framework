/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "individual-retrans-ddsa-routing-protocol-base.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("IndividualRetransDdsaRoutingProtocolBase");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (IndividualRetransDdsaRoutingProtocolBase);

		TypeId
		IndividualRetransDdsaRoutingProtocolBase::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::IndividualRetransDdsaRoutingProtocolBase")
				.SetParent<DdsaRoutingProtocolBase> ()
				.SetGroupName ("Ddsa");

			return tid;
		}

		IndividualRetransDdsaRoutingProtocolBase::IndividualRetransDdsaRoutingProtocolBase(){}

		IndividualRetransDdsaRoutingProtocolBase::~IndividualRetransDdsaRoutingProtocolBase(){}
	}
}
