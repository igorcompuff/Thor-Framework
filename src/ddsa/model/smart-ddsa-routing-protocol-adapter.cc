/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#define NS_LOG_APPEND_CONTEXT std::clog << "[node " << GetMyMainAddress() << "] (" << Simulator::Now().GetSeconds() << " s) ";

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"
#include "ns3/lq-olsr-repositories.h"
#include <algorithm>
#include "smart-ddsa-routing-protocol-adapter.h"
#include "ns3/udp-header.h"
#include "ns3/simple-header.h"
#include "ns3/lq-olsr-util.h"

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("SmartDdsaRoutingProtocolAdapter");

  namespace ddsa {

    NS_OBJECT_ENSURE_REGISTERED (SmartDdsaRoutingProtocolAdapter);

    TypeId
	SmartDdsaRoutingProtocolAdapter::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::ddsa::SmartDdsaRoutingProtocolAdapter")
        .SetParent<MaliciousLqRoutingProtocol> ()
        .SetGroupName ("Ddsa")
        .AddConstructor<SmartDdsaRoutingProtocolAdapter> ()
        .AddAttribute ("Alpha", "The Alpha value used in the DAP exclusion algorithm",
                       DoubleValue (0.0),
                       MakeDoubleAccessor (&SmartDdsaRoutingProtocolAdapter::alpha),
		       MakeDoubleChecker<double> (0))
	.AddTraceSource ("RouteComputed", "Called when a new route computation is performed",
			 MakeTraceSourceAccessor (&SmartDdsaRoutingProtocolAdapter::m_newRouteComputedTrace),
			 "ns3::ddsa::DdsaRoutingProtocolAdapter::NewRouteComputedTracedCallback")
	 .AddTraceSource ("NeighborExpired", "Called when the linktuple timer expires for some neighbor",
					 MakeTraceSourceAccessor (&SmartDdsaRoutingProtocolAdapter::m_neighborExpiredTrace),
					 "ns3::ddsa::DdsaRoutingProtocolAdapter::NeighborExpiredCallback");
      return tid;
    }

    SmartDdsaRoutingProtocolAdapter::SmartDdsaRoutingProtocolAdapter(): alpha (0.0)
    {
      m_rnd = CreateObject<UniformRandomVariable>();
      m_rnd->SetAttribute("Min", DoubleValue(0));
      m_rnd->SetAttribute("Max", DoubleValue(1));
      controllerAddress = Ipv4Address::GetBroadcast();
      m_type = NodeType::METER;
    }

    SmartDdsaRoutingProtocolAdapter::~SmartDdsaRoutingProtocolAdapter(){}

    void
	SmartDdsaRoutingProtocolAdapter::ClearDapExclusions()
    {
		for (std::map<Ipv4Address, SmartDap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
		{
			it->second.excluded = false;
		}
    }

    void
	SmartDdsaRoutingProtocolAdapter::ExcludeDaps()
    {
		if (alpha > 0)
		{
			SmartDap dapBestCost = GetBestCostDap();

			float lambda = m_metric->Compound(dapBestCost.cost, alpha * dapBestCost.cost);;

			NS_LOG_DEBUG("SmartDap " << dapBestCost.address << " has the lowest cost (" << dapBestCost.cost << ")");

			NS_LOG_DEBUG("Lambda = " << lambda);

			for (std::map<Ipv4Address, SmartDap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
			{
				if (m_metric->CompareBest(it->second.cost, lambda) < 0)
				{
					if (!it->second.excluded && (GetTotalCurrentEligibleDaps() - 1) >= 2 )
					{
						it->second.excluded = true;
						NS_LOG_DEBUG("Dap " << it->second.address << " excluded.");
					}
				}
				else
				{
					it->second.excluded = false;
				}
			}
		}
    }

    void
	SmartDdsaRoutingProtocolAdapter::PrintDaps (Ptr<OutputStreamWrapper> stream) const
    {
		std::ostream* os = stream->GetStream ();

		*os << "Dap\t\tTotal copies\tCost\tStatus\n";

		for (std::map<Ipv4Address, SmartDap>::const_iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
		{
			lqolsr::RoutingTableEntry entry;

			if (Lookup (it->second.address, entry))
			{
				std::string status = it->second.excluded ? "Excluded" : "Not Excluded";
				*os << it->second.address << "\t" << it->second.totalCopies << "\t\t" << entry.cost << "\t" << status << "\n";
			}
		}
    }

    std::vector<SmartDap>
    SmartDdsaRoutingProtocolAdapter::GetNotExcludedDaps()
    {
		std::vector<SmartDap> daps;
    	for (std::map<Ipv4Address, SmartDap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
		{
			if (!it->second.excluded)
			{
				daps.push_back(it->second);
			}
		}

    	return daps;
    }

    int
	SmartDdsaRoutingProtocolAdapter::GetTotalCurrentEligibleDaps()
    {
		return GetNotExcludedDaps().size();
    }

    void
	SmartDdsaRoutingProtocolAdapter::AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask)
    {
		std::map<Ipv4Address, SmartDap>::iterator it = m_gateways.find(gatewayAddr);

		if (it != m_gateways.end() && it->second.expirationTime < Simulator::Now ())
		{
			m_gateways.erase(it);

			NS_LOG_DEBUG("DAP " << it->second.address << " expired. Now we have " << m_gateways.size() << " Daps.");

			ClearDapExclusions();
			ExcludeDaps();
		}

		RoutingProtocol::AssociationTupleTimerExpire(gatewayAddr, networkAddr, netmask);
    }

    void
	SmartDdsaRoutingProtocolAdapter::LinkTupleTimerExpire (Ipv4Address neighborIfaceAddr)
    {
		m_neighborExpiredTrace(neighborIfaceAddr);
		RoutingProtocol::LinkTupleTimerExpire(neighborIfaceAddr);
    }

    bool
	SmartDdsaRoutingProtocolAdapter::DapExists(const Ipv4Address & dapAddress)
    {
		return m_gateways.find(dapAddress) != m_gateways.end();
    }

    //A new HNA message is supposed to have been sent by a DAP.
    void
	SmartDdsaRoutingProtocolAdapter::ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface)
    {
		Time now = Simulator::Now ();
		Ipv4Address dapAddress = msg.GetOriginatorAddress();
		lqolsr::RoutingTableEntry entry1;

		std::map<Ipv4Address, SmartDap>::iterator it = m_gateways.find(dapAddress);

		if (Lookup(dapAddress, entry1))
		{
			if (it == m_gateways.end())
			{
				SmartDap gw;
				gw.address = dapAddress;
				gw.expirationTime = now + msg.GetVTime();
				gw.cost = entry1.cost;
				gw.totalCopies = entry1.cost / entry1.distance;
				gw.hops = entry1.distance;
				m_gateways[dapAddress] = gw;

				NS_LOG_DEBUG("DAP " << dapAddress << " is now a possible gateway");
			}
			else
			{
				it->second.expirationTime = now + msg.GetVTime();
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
	SmartDdsaRoutingProtocolAdapter::SendTc()
    {
		if (m_type == NodeType::DAP && !IsMalicious())
		{
			return;
		}

		NS_LOG_DEBUG("Sending TC");

		lqolsr::RoutingProtocol::SendTc();
    }

    void
	SmartDdsaRoutingProtocolAdapter::UpdateDapCosts()
    {
		lqolsr::RoutingTableEntry rEntry;

		std::map<Ipv4Address, SmartDap>::iterator it = m_gateways.begin();

		while(it != m_gateways.end())
		{
			if (Lookup(it->first, rEntry))
			{
				it->second.cost = rEntry.cost;
				it->second.totalCopies = rEntry.cost / rEntry.distance;
				it->second.hops = rEntry.distance;
				it++;
			}
			else
			{
				NS_LOG_DEBUG("DAP " << it->second.address << " is no longer available.");
				//it->second.cost = m_metric->GetInfinityCostValue();
				it = m_gateways.erase(it);
				//it++;
			}
		}
    }

    float
	SmartDdsaRoutingProtocolAdapter::GetMeanDapCost()
    {
		float costSum = 0;

		for (std::map<Ipv4Address, SmartDap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
		{
			costSum+= it->second.cost;
		}

		return costSum / m_gateways.size();
    }

    void
	SmartDdsaRoutingProtocolAdapter::RoutingTableComputation ()
	{
		RoutingProtocol::RoutingTableComputation();
		ClearDapExclusions();
		UpdateDapCosts();
		ExcludeDaps();

		if (g_log.IsEnabled(LOG_LEVEL_DEBUG) && GetTotalCurrentEligibleDaps() > 0)
		{
			NS_LOG_DEBUG("Dumping all DAPS.");
			Ptr<OutputStreamWrapper> outStream = Create<OutputStreamWrapper>(&std::clog);
			PrintDaps(outStream);
		}

		std::vector<SmartDap> daps;

		for (std::map<Ipv4Address, SmartDap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
		{
			daps.push_back(it->second);
		}

		m_newRouteComputedTrace(daps, GetMyMainAddress());
    }

    double
	SmartDdsaRoutingProtocolAdapter::GetAlpha()
    {
    	return alpha;
    }

    void
	SmartDdsaRoutingProtocolAdapter::SetControllerAddress(Ipv4Address address)
    {
		controllerAddress = address;
		NS_LOG_DEBUG("Controller address set: " << controllerAddress);
    }

    Ipv4Address
	SmartDdsaRoutingProtocolAdapter::GetControllerAddress()
    {
    	return controllerAddress;
    }

    SmartDap
	SmartDdsaRoutingProtocolAdapter::GetBestCostDap()
    {
		SmartDap bestDap;
		bestDap.cost = m_metric->GetInfinityCostValue();

		for (std::map<Ipv4Address, SmartDap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
		{
			if (m_metric->CompareBest(it->second.cost, bestDap.cost) > 0)
			{
				bestDap = it->second;
			}
		}

		return bestDap;
    }

    void
	SmartDdsaRoutingProtocolAdapter::SetNodeType(NodeType nType)
    {
    	m_type = nType;
    }

    SmartDdsaRoutingProtocolAdapter::NodeType
	SmartDdsaRoutingProtocolAdapter::GetNodeType()
    {
    	return m_type;
    }

  }
}
