/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "flooding-ddsa-routing-protocol.h"
#include "ns3/lq-olsr-routing-protocol.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("FloodingDdsaRoutingProtocol");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (FloodingDdsaRoutingProtocol);

		TypeId
		FloodingDdsaRoutingProtocol::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::FloodingDdsaRoutingProtocol")
				.SetParent<IndividualRetransDdsaRoutingProtocolBase> ()
				.AddConstructor<FloodingDdsaRoutingProtocol>()
				.SetGroupName ("Ddsa");

			return tid;
		}

		FloodingDdsaRoutingProtocol::FloodingDdsaRoutingProtocol(){}

		FloodingDdsaRoutingProtocol::~FloodingDdsaRoutingProtocol(){}

		std::vector<Dap>
		FloodingDdsaRoutingProtocol::SelectDaps()
		{
			std::vector<Dap> daps;

			for(std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (!it->IsExcluded())
				{
					daps.push_back(*it);
				}
			}

			return daps;
		}

		int
		FloodingDdsaRoutingProtocol::GetTotalTransmissionsFor(const Ipv4Address & destAddress)
		{
			int totalTrans = 0;
			lqolsr::RoutingTableEntry entry;

			if (Lookup(destAddress, entry))
			{
				totalTrans = entry.cost / entry.distance;
			}

			return totalTrans;
		}

		bool
		FloodingDdsaRoutingProtocol::MustExclude(Dap dap)
		{
			int totalTrans = GetTotalTransmissionsFor(dap.GetAddress());
			int totalAvailableDaps = GetTotalNotExcludedDaps();

			return totalAvailableDaps >= 2 && totalTrans > 4;
		}

		void
		FloodingDdsaRoutingProtocol::BuildEligibleGateways()
		{
			ExcludeDaps();
		}
	}
}
