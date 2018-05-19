/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#define NS_LOG_APPEND_CONTEXT std::clog << "[node " << GetMyMainAddress() << "] (" << Simulator::Now().GetSeconds() << " s) ";
#define MIN_REDUNDANCY 2

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "original-ddsa-routing-protocol.h"

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("OriginalDdsaRoutingProtocol");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (OriginalDdsaRoutingProtocol);

		TypeId
		OriginalDdsaRoutingProtocol::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::OriginalDdsaRoutingProtocol")
				.SetParent<GlobalRetransDdsaRoutingProtocolBase> ()
				.SetGroupName ("Ddsa")
				.AddConstructor<OriginalDdsaRoutingProtocol>()
				.AddAttribute ("Alpha", "The Alpha value used in the DAP exclusion algorithm",
								DoubleValue (0.0),
								MakeDoubleAccessor (&OriginalDdsaRoutingProtocol::m_alpha),
								MakeDoubleChecker<double> (0))
				.AddAttribute ("Retrans", "Number of retransmissions.",
								IntegerValue (0),
								MakeIntegerAccessor (&OriginalDdsaRoutingProtocol::m_retrans),
								MakeIntegerChecker<int> (0))
				.AddAttribute ("MinRed", "Minimum redundancy acceptable. Cannot exclude a DAP if the total remaining available DAPs is below this value.",
								UintegerValue (2),
								MakeUintegerAccessor (&OriginalDdsaRoutingProtocol::m_minRedundancy),
								MakeUintegerChecker<unsigned int> (2));

			return tid;
		}

		OriginalDdsaRoutingProtocol::OriginalDdsaRoutingProtocol()
		{
			m_alpha = 0.0;
			m_retrans = 0;
			m_minRedundancy = 2;
			m_rnd = CreateObject<UniformRandomVariable>();
			m_rnd->SetAttribute("Min", DoubleValue(0));
			m_rnd->SetAttribute("Max", DoubleValue(1));
		}

		OriginalDdsaRoutingProtocol::~OriginalDdsaRoutingProtocol(){}

		Dap
		OriginalDdsaRoutingProtocol::GetBestDap()
		{
			Dap bestDap;

			for (std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (it->GetProbability() > bestDap.GetProbability())
				{
					bestDap = *it;
				}
			}

			return bestDap;
		}

		bool
		OriginalDdsaRoutingProtocol::MustExclude(Dap dap)
		{
			Dap bestDap = GetBestDap();
			double lambda = m_alpha * bestDap.GetProbability();

			return (dap.GetProbability() < lambda) && ((m_gateways.size() - 1) >= m_minRedundancy);
		}

		void
		OriginalDdsaRoutingProtocol::BuildEligibleGateways()
		{
			bool processDapComputation = true;

			while (processDapComputation)
			{
				CalculateProbabilities();
				processDapComputation = ExcludeDaps();
			}
		}

		void
		OriginalDdsaRoutingProtocol::CalculateProbabilities()
		{
			double costSomatory = SumUpNotExcludedDapCosts();

			for (std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				double probability = 0.0;

				if (!it->IsExcluded())
				{
					float cost = it->GetCost();

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

					it->SetProbability(probability);
				}
			}
		}

		double
		OriginalDdsaRoutingProtocol::SumUpNotExcludedDapCosts()
		{
			double costSomatory = 0.0;

			for (std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (!it->IsExcluded())
				{
					if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_HIGHER)
					{
						costSomatory += it->GetCost();
					}
					else if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_LOWER)
					{
						costSomatory += (1 / it->GetCost());
					}
				}
			}

			return costSomatory;
		}

		std::vector<Dap>
		OriginalDdsaRoutingProtocol::SelectDaps()
		{
			std::vector<Dap> selectedDaps;

			double rand = m_rnd->GetValue();
			double prob_sum = 0;
			bool selected = false;

			std::vector<Dap>::iterator it = m_gateways.begin();

			for (std::vector<Dap>::iterator it = m_gateways.begin(); (it != m_gateways.end() && !selected); it++)
			{
				if (!it->IsExcluded())
				{
					prob_sum += it->GetProbability();

					if (prob_sum >= rand)
					{
						selectedDaps.push_back(*it);
						selected = true;
						NS_LOG_DEBUG("Dap: " << it->GetAddress() << " selected by " << GetMyMainAddress());
					}
				}
			}

			return selectedDaps;
		}

		int
		OriginalDdsaRoutingProtocol::GetTotalRetransmissions()
		{
			return m_retrans;
		}

		void
		OriginalDdsaRoutingProtocol::SetAlpha(double alpha)
		{
			m_alpha = alpha;
		}

		double
		OriginalDdsaRoutingProtocol::GetAlpha()
		{
			return m_alpha;
		}

		unsigned int
		OriginalDdsaRoutingProtocol::GetMinRedundancy()
		{
			return m_minRedundancy;
		}
	}
}
