/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#define NS_LOG_APPEND_CONTEXT std::clog << "[node " << GetMyMainAddress() << "] (" << Simulator::Now().GetSeconds() << " s) ";
#define MIN_REDUNDANCY 2

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "dumb-ddsa-routing-protocol.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("DumbDdsaRoutingProtocol");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (DumbDdsaRoutingProtocol);

		TypeId
		DumbDdsaRoutingProtocol::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::DumbDdsaRoutingProtocol")
				.SetParent<OriginalDdsaRoutingProtocol> ()
				.SetGroupName ("Ddsa")
				.AddConstructor<DumbDdsaRoutingProtocol>();

			return tid;
		}

		DumbDdsaRoutingProtocol::DumbDdsaRoutingProtocol()
		{
		}

		DumbDdsaRoutingProtocol::~DumbDdsaRoutingProtocol(){}

		bool
		DumbDdsaRoutingProtocol::MustExclude(Dap dap)
		{
			return false;
		}

		void
		DumbDdsaRoutingProtocol::CalculateProbabilities()
		{
			for (std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				double probability = 1 / (double)m_gateways.size();;

				it->SetProbability(probability);
				//NS_LOG_DEBUG("Dap " << it->GetAddress() << ": " << it->GetProbability() << "\n");
			}
		}
	}
}
