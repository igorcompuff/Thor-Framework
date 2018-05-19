/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ddsa-routing-protocol-base.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("DdsaRoutingProtocolBase");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (DdsaRoutingProtocolBase);

		TypeId
		DdsaRoutingProtocolBase::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::DdsaRoutingProtocolBase")
				.SetParent<lqolsr::RoutingProtocol> ()
				.SetGroupName ("Ddsa");

			return tid;
		}

		DdsaRoutingProtocolBase::DdsaRoutingProtocolBase(){}

		DdsaRoutingProtocolBase::~DdsaRoutingProtocolBase(){}

		void
		DdsaRoutingProtocolBase::ClearDapExclusions()
		{
			for (std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				it->Exclude(false);
			}
		}

		bool
		DdsaRoutingProtocolBase::ExcludeDaps()
		{
			bool excluded = false;

			for (std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{

				if (MustExclude(*it))
				{
					if (!it->IsExcluded())
					{
						NS_LOG_DEBUG("Dap " << it->GetAddress() << " excluded.");

						it->Exclude(true);
						excluded = true;
					}
				}
				else
				{
					if (it->IsExcluded())
					{
						NS_LOG_DEBUG("Dap " << it->GetAddress() << " is no longer excluded.");
						excluded = false;
					}
				}
			}

			return excluded;
		}

		void
		DdsaRoutingProtocolBase::PrintDaps (Ptr<OutputStreamWrapper> stream)
		{
			std::ostream* os = stream->GetStream ();

			*os << "Dap\t\tProbability\tCost\tStatus\n";

			for (std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				std::string status = it->IsExcluded() ? "Excluded" : "Not Excluded";
				*os << it->GetAddress() << "\t" << it->GetProbability() << "\t\t" << it->GetCost() << "\t" << status << "\n";
			}
		}

		std::vector<Dap>::iterator
		DdsaRoutingProtocolBase::FindDap(Ipv4Address address)
		{
			std::vector<Dap>::iterator found = m_gateways.begin();

			while (found != m_gateways.end() && found->GetAddress() != address)
			{
				found++;
			}

			return found;
		}

		void
		DdsaRoutingProtocolBase::AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask)
		{
			std::vector<Dap>::iterator it = FindDap(gatewayAddr);

			if (it != m_gateways.end() && it->GetExpirationTime() < Simulator::Now ())
			{
				m_gateways.erase(it);

				NS_LOG_DEBUG("DAP " << it->GetAddress() << " expired. Now we have " << m_gateways.size() << " Daps.");

				ClearDapExclusions();
				BuildEligibleGateways();
			}

			lqolsr::RoutingProtocol::AssociationTupleTimerExpire(gatewayAddr, networkAddr, netmask);
		}

		Dap
		DdsaRoutingProtocolBase::AddDap(Ipv4Address address, float cost, Time expirationTime)
		{
			Dap gw(address, cost, expirationTime);
			m_gateways.push_back(gw);

			return gw;
		}


		/*
		 * HNA messages are supposed to be sent only by DAPs in order to announce the MDMS network.
		 * When an HNA message is received, a new DAP should be added if it is not present in the gateway list or it expiration must be updated otherwise.
		 */
		void
		DdsaRoutingProtocolBase::ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface)
		{
			Time now = Simulator::Now ();
			Ipv4Address dapAddress = msg.GetOriginatorAddress();
			lqolsr::RoutingTableEntry entry1;

			//If the current node has no route to the advertised DAP, then it cannot be a gateway anyway.
			if (Lookup(dapAddress, entry1))
			{
				std::vector<Dap>::iterator it = FindDap(dapAddress);

				if (it == m_gateways.end())
				{
					Dap addedDap = AddDap(dapAddress, entry1.cost, now + msg.GetVTime());

					NS_LOG_DEBUG("DAP " << addedDap.GetAddress() << " is now a possible gateway");
				}
				else
				{
					it->SetExpirationTime(now + msg.GetVTime());
					NS_LOG_DEBUG("Expiration Time for Gateway " << it->GetAddress() << " updated");
				}
			}
			else
			{
				NS_LOG_DEBUG("DAP " << dapAddress << " cannot be a gateway (no route found)");
			}

			lqolsr::RoutingProtocol::ProcessHna(msg, senderIface);
		}

		void
		DdsaRoutingProtocolBase::UpdateDapCosts()
		{
			lqolsr::RoutingTableEntry rEntry;

			std::vector<Dap>::iterator it = m_gateways.begin();

			while(it != m_gateways.end())
			{
				if (Lookup(it->GetAddress(), rEntry))
				{
					it->SetCost(rEntry.cost);
					it++;
				}
				else
				{
					NS_LOG_DEBUG("DAP " << it->GetAddress() << " is no longer available.");
					it = m_gateways.erase(it);
				}
			}
		}

		void
		DdsaRoutingProtocolBase::RoutingTableComputation ()
		{
			lqolsr::RoutingProtocol::RoutingTableComputation();
			ClearDapExclusions();
			UpdateDapCosts();
			BuildEligibleGateways();

			if (g_log.IsEnabled(LOG_LEVEL_DEBUG))
			{
				NS_LOG_DEBUG("Dumping all DAPS.");
				Ptr<OutputStreamWrapper> outStream = Create<OutputStreamWrapper>(&std::clog);
				PrintDaps(outStream);
			}
		}

		int
		DdsaRoutingProtocolBase::GetTotalNotExcludedDaps()
		{
			int count = 0;
			for(std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (!it->IsExcluded())
				{
					count++;
				}
			}

			return count;
		}

		bool
		DdsaRoutingProtocolBase::HasAvailableDaps()
		{
			return GetTotalNotExcludedDaps() > 0;
		}
	}
}
