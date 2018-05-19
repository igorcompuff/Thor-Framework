/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#define NS_LOG_APPEND_CONTEXT std::clog << "[node " << GetMyMainAddress() << "] (" << Simulator::Now().GetSeconds() << " s) ";

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"
#include "ns3/lq-olsr-repositories.h"
#include <algorithm>
#include "ddsa-routing-protocol-adapter.h"
#include "ns3/udp-header.h"
#include "ns3/simple-header.h"
#include "ns3/lq-olsr-util.h"
#include <cmath>

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("DdsaRoutingProtocolAdapter");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (DdsaRoutingProtocolAdapter);

		TypeId
		DdsaRoutingProtocolAdapter::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::DdsaRoutingProtocolAdapter")
				.SetParent<MaliciousLqRoutingProtocol> ()
				.SetGroupName ("Ddsa")
				.AddConstructor<DdsaRoutingProtocolAdapter> ()
				.AddAttribute ("Alpha", "The Alpha value used in the DAP exclusion algorithm",
								DoubleValue (0.0),
								MakeDoubleAccessor (&DdsaRoutingProtocolAdapter::alpha),
								MakeDoubleChecker<double> (0))
				.AddAttribute ("Dumb", "Dumb mode",
								BooleanValue(false),
								MakeBooleanAccessor(&DdsaRoutingProtocolAdapter::dumb),
								MakeBooleanChecker())
				.AddAttribute ("prop", "Desired probability",
								DoubleValue(0.99),
								MakeDoubleAccessor(&DdsaRoutingProtocolAdapter::m_desiredDeliveryProbability),
								MakeDoubleChecker<double>(0.0, 1.0))
				.AddTraceSource ("RouteComputed", "Called when a new route computation is performed",
								MakeTraceSourceAccessor (&DdsaRoutingProtocolAdapter::m_newRouteComputedTrace),
								"ns3::ddsa::DdsaRoutingProtocolAdapter::NewRouteComputedTracedCallback")
				.AddTraceSource ("DapSelection", "Called when a new dap is selected",
								MakeTraceSourceAccessor (&DdsaRoutingProtocolAdapter::m_dapSelectionTrace),
								"ns3::ddsa::DdsaRoutingProtocolAdapter::SelectedTracedCallback")
				.AddTraceSource ("NeighborExpired", "Called when the linktuple timer expires for some neighbor",
								MakeTraceSourceAccessor (&DdsaRoutingProtocolAdapter::m_neighborExpiredTrace),
								"ns3::ddsa::DdsaRoutingProtocolAdapter::NeighborExpiredCallback");

			return tid;
		}

		DdsaRoutingProtocolAdapter::DdsaRoutingProtocolAdapter(): alpha (0.0)
		{
			m_rnd = CreateObject<UniformRandomVariable>();
			m_rnd->SetAttribute("Min", DoubleValue(0));
			m_rnd->SetAttribute("Max", DoubleValue(1));
			controllerAddress = Ipv4Address::GetBroadcast();
			dumb = false;
			m_type = NodeType::METER;
			m_desiredDeliveryProbability = 0.99;
		}

		DdsaRoutingProtocolAdapter::~DdsaRoutingProtocolAdapter(){}


		double
		DdsaRoutingProtocolAdapter::SumUpNotExcludedDapCosts()
		{
			double costSomatory = 0.0;
			int totalDaps = 0;
			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (!it->second.IsExcluded())
				{
					totalDaps++;
					if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_HIGHER)
					{
						costSomatory += it->second.GetCost();
					}
					else if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_LOWER)
					{
						costSomatory += (1 / it->second.GetCost());
					}
				}
			}

			NS_LOG_DEBUG("Cost somatory = " << costSomatory << " Total DAPS = " << totalDaps);

			return costSomatory;
		}

		void
		DdsaRoutingProtocolAdapter::CalculateProbabilities()
		{
			double costSomatory = SumUpNotExcludedDapCosts();

			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				double probability = 0.0;
				if (dumb)
				{
					// In dumb mode all daps have the same selection probability regardless of their costs
					probability = 1 / (double)m_gateways.size();
					it->second.SetProbability(probability);
				}
				else if (!it->second.IsExcluded())
				{
					//In normal mode we only account for non-excluded Daps

					float cost = it->second.GetCost();

					if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_HIGHER)
					{
						//If the cost is the highest possible, the probability is set to the highest value (p = 1) as well
						probability = cost == m_metric->GetInfinityCostValue() ? 1 : cost / costSomatory;
					}
					else if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_LOWER)
					{
						//If the cost is the lowest possible, the probability is set to the highest value (p = 1)
						probability = cost == m_metric->GetInfinityCostValue() ? 0 : 1 / (cost * costSomatory);
					}

					it->second.SetProbability(probability);
				}
			}
		}

		void
		DdsaRoutingProtocolAdapter::ClearDapExclusions()
		{
			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				it->second.Exclude(false);
			}
		}

		bool
		DdsaRoutingProtocolAdapter::ExcludeDaps()
		{
			//In dumb mode no daps are excluded
			if (dumb)
			{
				return false;
			}

			double highestProb = 0;
			double lambda = 0;
			bool excluded = false;

			Ipv4Address dapHighestProbAddress;

			if (alpha == 0)
			{
				return false;
			}

			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (it->second.GetProbability() > highestProb)
				{
					highestProb = it->second.GetProbability();
					dapHighestProbAddress = it->first;
				}
			}

			NS_LOG_DEBUG("DAP " << dapHighestProbAddress << " has the highest probability (" << highestProb << ")");

			lambda = alpha * highestProb;

			NS_LOG_DEBUG("Lambda = " << lambda);

			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (it->second.GetProbability() < lambda)
				{
					if (!it->second.IsExcluded() && (GetTotalCurrentEligibleDaps() -1) >= 2 )
					{
						it->second.Exclude(true);
						excluded = true;
						NS_LOG_DEBUG("Dap " << it->first << " excluded.");
						NS_LOG_DEBUG("Prob = " << it->second.GetProbability() << ", Lambda = " << lambda);
					}
				}
				else
				{
					if (it->second.IsExcluded())
					{
						NS_LOG_DEBUG("Dap " << it->first << " is no longer excluded.");
					}

					it->second.Exclude(false);
				}
			}

			return excluded;
		}

		void
		DdsaRoutingProtocolAdapter::PrintDaps (Ptr<OutputStreamWrapper> stream)
		{
			std::ostream* os = stream->GetStream ();

			*os << "Dap\t\tProbability\tCost\tStatus\n";

			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				lqolsr::RoutingTableEntry entry;

				if (Lookup (it->first, entry))
				{
					std::string status = it->second.IsExcluded() ? "Excluded" : "Not Excluded";
					*os << it->first << "\t" << it->second.GetProbability() << "\t\t" << entry.cost << "\t" << status << "\n";
				}
			}
		}

		Dap
		DdsaRoutingProtocolAdapter::SelectDap()
		{
			double rand = m_rnd->GetValue();
			double prob_sum = 0;

			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (!it->second.IsExcluded())
				{
					prob_sum += it->second.GetProbability();

					if (prob_sum >= rand)
					{
						NS_LOG_DEBUG("Dap: " << it->first << " selected by " << GetMyMainAddress());

						return it->second;
					}
				}
			}

			return Dap(Ipv4Address::GetBroadcast());
		}

		int
		DdsaRoutingProtocolAdapter::GetTotalCurrentEligibleDaps()
		{
			int count = 0;
			for(std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (!it->second.IsExcluded())
				{
					count++;
				}
			}

			return count;
		}

		void
		DdsaRoutingProtocolAdapter::AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask)
		{
			std::map<Ipv4Address, Dap>::iterator it = m_gateways.find(gatewayAddr);

			if (it != m_gateways.end() && it->second.GetExpirationTime() < Simulator::Now ())
			{
				m_gateways.erase(it);

				NS_LOG_DEBUG("DAP " << it->first << " expired. Now we have " << m_gateways.size() << " Daps.");

				ClearDapExclusions();
				BuildEligibleGateways();
			}

			RoutingProtocol::AssociationTupleTimerExpire(gatewayAddr, networkAddr, netmask);
		}

		void
		DdsaRoutingProtocolAdapter::LinkTupleTimerExpire (Ipv4Address neighborIfaceAddr)
		{
			m_neighborExpiredTrace(neighborIfaceAddr);
			RoutingProtocol::LinkTupleTimerExpire(neighborIfaceAddr);
		}

		bool
		DdsaRoutingProtocolAdapter::DapExists(const Ipv4Address & dapAddress)
		{
			return m_gateways.find(dapAddress) != m_gateways.end();
		}

		//A new HNA message is supposed to have been sent by a DAP.
		void
		DdsaRoutingProtocolAdapter::ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface)
		{
			Time now = Simulator::Now ();
			Ipv4Address dapAddress = msg.GetOriginatorAddress();
			lqolsr::RoutingTableEntry entry1;

			std::map<Ipv4Address, Dap>::iterator it = m_gateways.find(dapAddress);

			if (Lookup(dapAddress, entry1))
			{
				if (it == m_gateways.end())
				{
					Dap gw(dapAddress, entry1.cost, now + msg.GetVTime());
					m_gateways[dapAddress] = gw;

					NS_LOG_DEBUG("DAP " << dapAddress << " is now a possible gateway");
				}
				else
				{
					it->second.SetExpirationTime(now + msg.GetVTime());
					NS_LOG_DEBUG("Expiration Time for Gateway " << dapAddress << " updated");
				}
			}
			else
			{
				NS_LOG_DEBUG("DAP " << dapAddress << " cannot be a gateway (no route found)");
			}

			RoutingProtocol::ProcessHna(msg, senderIface);
		}

		void
		DdsaRoutingProtocolAdapter::SendTc()
		{
			if (m_type == NodeType::DAP && !IsMalicious())
			{
				return;
			}

			NS_LOG_DEBUG("Sending TC");

			lqolsr::RoutingProtocol::SendTc();
		}

		void
		DdsaRoutingProtocolAdapter::UpdateDapCosts()
		{
			lqolsr::RoutingTableEntry rEntry;

			std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin();

			while(it != m_gateways.end())
			{
				if (Lookup(it->first, rEntry))
				{
					it->second.SetCost(rEntry.cost);
					it++;
				}
				else
				{
					NS_LOG_DEBUG("DAP " << it->first << " is no longer available.");
					it = m_gateways.erase(it);
				}
			}
		}

		float
		DdsaRoutingProtocolAdapter::GetMeanDapCost()
		{
			float costSum = 0;

			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				costSum+= it->second.GetCost();
			}

			return costSum / m_gateways.size();
		}

		void
		DdsaRoutingProtocolAdapter::RoutingTableComputation ()
		{
			RoutingProtocol::RoutingTableComputation();
			ClearDapExclusions();
			UpdateDapCosts();
			BuildEligibleGateways();

			if (g_log.IsEnabled(LOG_LEVEL_DEBUG) && GetTotalCurrentEligibleDaps() > 0)
			{
			NS_LOG_DEBUG("Dumping all DAPS.");
			Ptr<OutputStreamWrapper> outStream = Create<OutputStreamWrapper>(&std::clog);
			PrintDaps(outStream);
			}

			std::vector<Dap> daps;

			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				daps.push_back(it->second);
			}

			m_newRouteComputedTrace(daps, GetMyMainAddress());
		}

		Ptr<Ipv4Route>
		DdsaRoutingProtocolAdapter::RouteOutput (Ptr<Packet> p, Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
		{
			Ptr<Ipv4Route> route = 0;

			/*
			* 1 - The controller is the destination. All packets directed to the controller should be redirected
			* to the selected DAP, so DDSA should process it.
			*/
			if (header.GetDestination() == controllerAddress && m_type == NodeType::METER)
			{
				if (m_gateways.size() > 0)
				{
					Dap selectedDap = SelectDap();

					m_dapSelectionTrace(selectedDap.GetAddress(), p->Copy());

					header.SetDestination (selectedDap.GetAddress());

					route = RoutingProtocol::RouteOutput(p, header, oif, sockerr);
				}
				else
				{
					NS_LOG_DEBUG("No dap available. Trying to find a route.");

					lqolsr::RoutingTableEntry gatewayEntry;
					route = RoutingProtocol::RouteOutput(p, header, oif, sockerr);
				}
			}
			/*
			* 2 - In case the destination is not the controller, DDSA does not perform any action in addition to
			* OLSR.
			*/
			else
			{
				route = RoutingProtocol::RouteOutput(p, header, oif, sockerr);
			}


			return route;
		}

		void
		DdsaRoutingProtocolAdapter::BuildEligibleGateways()
		{
			bool processDapComputation = true;

			while (processDapComputation)
			{
				CalculateProbabilities();
				processDapComputation = ExcludeDaps();
			}
		}

		double
		DdsaRoutingProtocolAdapter::GetAlpha()
		{
			return alpha;
		}

		void
		DdsaRoutingProtocolAdapter::SetControllerAddress(Ipv4Address address)
		{
			controllerAddress = address;
			NS_LOG_DEBUG("Controller address set: " << controllerAddress);
		}

		Ipv4Address
		DdsaRoutingProtocolAdapter::GetControllerAddress()
		{
			return controllerAddress;
		}

		Dap
		DdsaRoutingProtocolAdapter::GetBestCostDap()
		{
			Dap bestDap(Ipv4Address::GetBroadcast(), m_metric->GetInfinityCostValue(), Time::Max());

			for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (m_metric->CompareBest(it->second.GetCost(), bestDap.GetCost()) > 0)
				{
					bestDap = it->second;
				}
			}

			return bestDap;
		}

		void
		DdsaRoutingProtocolAdapter::SetNodeType(NodeType nType)
		{
			m_type = nType;
		}
	}
}
