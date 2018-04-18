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

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("DdsaRoutingProtocolAdapter");

  namespace ddsa {

    NS_OBJECT_ENSURE_REGISTERED (DdsaRoutingProtocolAdapter);

    TypeId
    DdsaRoutingProtocolAdapter::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::ddsa::DdsaRoutingProtocolAdapter")
        .SetParent<RoutingProtocol> ()
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
      malicious = false;
    }

    DdsaRoutingProtocolAdapter::~DdsaRoutingProtocolAdapter(){}


    double
    DdsaRoutingProtocolAdapter::SumUpNotExcludedDapCosts()
    {
      double costSomatory = 0.0;
      int totalDaps = 0;
      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (!it->second.excluded)
	    {
	      totalDaps++;
	      if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_HIGHER)
		{
		  costSomatory += it->second.cost;
		}
	      else if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_LOWER)
		{
		  costSomatory += (1 / it->second.cost);
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
      double probabilitySum = 0.0;

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
      	{
	  if (dumb)
	    {
	      // In dumb mode all daps have the same selection probability regardless of their costs
	      it->second.probability = 1 / (double)m_gateways.size();
	    }
	  else if (!it->second.excluded)
	    {
	      //In normal mode we only account for non-excluded Daps

	      if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_HIGHER)
		{
		  //If the cost is the highest possible, the probability is set to the highest value (p = 1) as well
		  it->second.probability = it->second.cost == m_metric->GetInfinityCostValue() ? 1 : it->second.cost / costSomatory;
		}
	      else if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_LOWER)
		{
		  //If the cost is the lowest possible, the probability is set to the highest value (p = 1)
		  it->second.probability = it->second.cost == m_metric->GetInfinityCostValue() ? 0 : 1 / (it->second.cost * costSomatory);
		}

	      probabilitySum+= it->second.probability;
	    }
      	}
    }

    void
    DdsaRoutingProtocolAdapter::ClearDapExclusions()
    {
      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
      	{
	  it->second.excluded = false;
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

      Dap dapHighestProb;

      if (alpha == 0)
      {
	      return false;
      }

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (it->second.probability > highestProb)
	    {
	      highestProb = it->second.probability;
	      dapHighestProb = it->second;
	    }
	}

      NS_LOG_DEBUG("DAP " << dapHighestProb.address << " has the highest probability (" << highestProb << ")");

      lambda = alpha * highestProb;

      NS_LOG_DEBUG("Lambda = " << lambda);

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (it->second.probability < lambda)
	    {
	      if (!it->second.excluded && (GetTotalCurrentEligibleDaps() -1) >= 2 )
		{
		  it->second.excluded = true;
		  excluded = true;
		  NS_LOG_DEBUG("Dap " << it->second.address << " excluded.");
		  NS_LOG_DEBUG("Prob = " << it->second.probability << ", Lambda = " << lambda);
		}
	    }
	  else
	    {
	      if (it->second.excluded)
		{
		  NS_LOG_DEBUG("Dap " << it->second.address << " is no longer excluded.");
		}

	      it->second.excluded = false;
	    }
	}

      return excluded;
    }

    void
    DdsaRoutingProtocolAdapter::PrintDaps (Ptr<OutputStreamWrapper> stream) const
    {
      std::ostream* os = stream->GetStream ();

      *os << "Dap\t\tProbability\tCost\tStatus\n";

      for (std::map<Ipv4Address, Dap>::const_iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
      	{
	  lqolsr::RoutingTableEntry entry;

	  if (Lookup (it->second.address, entry))
	    {
	      std::string status = it->second.excluded ? "Excluded" : "Not Excluded";
	      *os << it->second.address << "\t" << it->second.probability << "\t\t" << entry.cost << "\t" << status << "\n";
	    }
      	}
    }

    Dap
    DdsaRoutingProtocolAdapter::SelectDap()
    {
      double rand = m_rnd->GetValue();
      double prob_sum = 0;

      Dap selectedDap;
      selectedDap.address = Ipv4Address::GetBroadcast();

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
      	{
	  if (!it->second.excluded)
	    {
	      prob_sum += it->second.probability;

	      if (prob_sum >= rand)
		{
		  selectedDap = it->second;
		  NS_LOG_DEBUG("Dap: " << selectedDap.address << " selected by " << GetMyMainAddress());
		  break;
		}
	    }
      	}

      return selectedDap;
    }

    int
    DdsaRoutingProtocolAdapter::GetTotalCurrentEligibleDaps()
    {
      int count = 0;
      for(std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (!it->second.excluded)
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

      if (it != m_gateways.end() && it->second.expirationTime < Simulator::Now ())
		{
		  m_gateways.erase(it);

		  NS_LOG_DEBUG("DAP " << it->second.address << " expired. Now we have " << m_gateways.size() << " Daps.");

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
	      Dap gw;
	      gw.address = dapAddress;
	      gw.expirationTime = now + msg.GetVTime();
	      gw.cost = entry1.cost;
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
    DdsaRoutingProtocolAdapter::SendTc()
    {
      if (m_type == NodeType::DAP && !malicious)
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
	      it->second.cost = rEntry.cost;
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
    DdsaRoutingProtocolAdapter::GetMeanDapCost()
    {
      float costSum = 0;

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  costSum+= it->second.cost;
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

				m_dapSelectionTrace(selectedDap.address, p->Copy());

				header.SetDestination (selectedDap.address);

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
      Dap bestDap;
      bestDap.cost = m_metric->GetInfinityCostValue();

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (m_metric->CompareBest(it->second.cost, bestDap.cost) > 0)
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

    float
    DdsaRoutingProtocolAdapter::GetCostToTcSend(lqolsr::LinkTuple *link_tuple)
    {
      return malicious ? 1.0f : RoutingProtocol::GetCostToTcSend(link_tuple);
    }

    uint32_t
    DdsaRoutingProtocolAdapter::GetHelloInfoToSendHello(Ipv4Address neiAddress)
    {
      uint32_t info;
      if (malicious)
	{
	  info = pack754_32(1.0f);
	}
      else
	{
	  info = RoutingProtocol::GetHelloInfoToSendHello(neiAddress);
	}

      return info;
    }

    void
    DdsaRoutingProtocolAdapter::SetMalicious(bool mal)
    {
      malicious = mal;
    }

  }
}
