/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#define NS_LOG_APPEND_CONTEXT std::clog << "[node " << GetMyMainAddress() << "] (" << Simulator::Now().GetSeconds() << " s) ";
#define MIN_REDUNDANCY 2

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "dynamic-retrans-ddsa-routing-protocol.h"
#include <vector>
#include <cmath>

namespace ns3 {

	NS_LOG_COMPONENT_DEFINE ("DynamicRetransDdsaRoutingProtocol");

	namespace ddsa {

		NS_OBJECT_ENSURE_REGISTERED (DynamicRetransDdsaRoutingProtocol);

		TypeId
		DynamicRetransDdsaRoutingProtocol::GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::ddsa::DynamicRetransDdsaRoutingProtocol")
				.SetParent<OriginalDdsaRoutingProtocol> ()
				.SetGroupName ("Ddsa")
				.AddConstructor<DynamicRetransDdsaRoutingProtocol>()
				.AddAttribute ("Prop", "Target Delivery Probability",
								DoubleValue (0.99),
								MakeDoubleAccessor (&DynamicRetransDdsaRoutingProtocol::m_targetProbability),
								MakeDoubleChecker<double> (0, 1))
				.AddAttribute ("tprob", "All DAPs must have at least tprob delivery probability in order to be considered available to selection.",
								DoubleValue (0.1),
								MakeDoubleAccessor (&DynamicRetransDdsaRoutingProtocol::m_thresholdProbability),
								MakeDoubleChecker<double> (0, 1))
				.AddAttribute ("ltrans", "Total Link Layer Transmissions",
								IntegerValue (4),
								MakeIntegerAccessor (&DynamicRetransDdsaRoutingProtocol::linkLayerTrans),
								MakeIntegerChecker<int> (1));
			return tid;
		}

		DynamicRetransDdsaRoutingProtocol::DynamicRetransDdsaRoutingProtocol()
		{
			linkLayerTrans = 4;
			m_targetProbability = 0.99;
			m_thresholdProbability = 0.1;
		}

		DynamicRetransDdsaRoutingProtocol::~DynamicRetransDdsaRoutingProtocol(){}


		bool
		DynamicRetransDdsaRoutingProtocol::MustExclude(Dap dap)
		{
			return m_delivery_probs[dap.GetAddress()] < m_thresholdProbability && ((m_gateways.size() - 1) >= GetMinRedundancy());
		}

		int
		DynamicRetransDdsaRoutingProtocol::GetTotalRetransmissions()
		{
			int totalRetrans = (int)std::ceil(std::log10(1 - m_targetProbability) / std::log10(1 - CalculateOneShootDeliveryProbability()));

			return std::max(OriginalDdsaRoutingProtocol::GetTotalRetransmissions(), totalRetrans);
		}

		double
		DynamicRetransDdsaRoutingProtocol::CalculateLinkDeliveryProbability(float linkEtx)
		{
			double linkSuccessProbability = 1 / linkEtx;
			double linkFailureProbAfterRetrans = std::pow(1 - linkSuccessProbability, linkLayerTrans);

			return 1 - linkFailureProbAfterRetrans;
		}

		double
		DynamicRetransDdsaRoutingProtocol::CalculateOneShootDeliveryProbability()
		{
			double summation = 0.0;
			for(std::vector<Dap>::iterator dap = m_gateways.begin(); dap != m_gateways.end(); dap++)
			{
				summation += dap->GetProbability() * m_delivery_probs[dap->GetAddress()];
			}

			return summation;
		}


		void
		DynamicRetransDdsaRoutingProtocol::InitializeDeliveryProbabilities()
		{
			std::vector<lqolsr::DestinationTuple> destinations = GetDestinations();

			for(std::vector<lqolsr::DestinationTuple>::iterator it = destinations.begin(); it != destinations.end(); it++)
			{
				float cost = GetCostToDestination(it->destAddress);

				if (cost != m_metric->GetInfinityCostValue())
				{
					m_delivery_probs[it->destAddress] = CalculateLinkDeliveryProbability(cost);
				}
				else
				{
					m_delivery_probs[it->destAddress] = 0.0;
				}
			}
		}

		void
		DynamicRetransDdsaRoutingProtocol::InitializeDestinations()
		{
			OriginalDdsaRoutingProtocol::InitializeDestinations();

			InitializeDeliveryProbabilities();
		}

		void
		DynamicRetransDdsaRoutingProtocol::UpdateDestination(lqolsr::DestinationTuple & tuple, const Ipv4Address & destAddress, float newCost, const lqolsr::LinkTuple * accessLink, int hopCount)
		{
			OriginalDdsaRoutingProtocol::UpdateDestination(tuple, destAddress, newCost, accessLink, hopCount);

			m_delivery_probs[destAddress] = m_delivery_probs[tuple.destAddress] * CalculateLinkDeliveryProbability(newCost);
		}
	}
}
