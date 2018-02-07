/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/integer.h"
#include "ns3/lq-olsr-repositories.h"
#include <algorithm>
#include "ddsa-routing-protocol-adapter.h"

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
        .AddAttribute ("Retrans", "Number of Retransmissions",
                       IntegerValue (0),
                       MakeIntegerAccessor(&DdsaRoutingProtocolAdapter::n_retransmissions),
                       MakeIntegerChecker<int> (0))
	.AddAttribute ("Dumb", "Dumb mode",
		       BooleanValue(false),
		       MakeBooleanAccessor(&DdsaRoutingProtocolAdapter::dumb),
		       MakeBooleanChecker())
	.AddTraceSource ("RouteComputed", "Called when a new route computation is performed",
			 MakeTraceSourceAccessor (&DdsaRoutingProtocolAdapter::m_newRouteComputedTrace),
			 "ns3::ddsa::DdsaRoutingProtocolAdapter::NewRouteComputedTracedCallback");
      return tid;
    }

    DdsaRoutingProtocolAdapter::DdsaRoutingProtocolAdapter(): alpha (0.0), n_retransmissions (0)
    {
      m_rnd = CreateObject<UniformRandomVariable>();
      m_rnd->SetAttribute("Min", DoubleValue(0));
      m_rnd->SetAttribute("Max", DoubleValue(1));
    }

    DdsaRoutingProtocolAdapter::~DdsaRoutingProtocolAdapter(){}


    double
    DdsaRoutingProtocolAdapter::SumUpNotExcludedDapCosts()
    {
      double costSomatory = 0.0;

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (!it->second.excluded)
	    {
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
		  it->second.probability = it->second.cost == 0 ? 0 : 1 / (it->second.cost * costSomatory);
		}

	      probabilitySum+= it->second.probability;
	    }
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

      if (alpha == 0)
      {
	      return false;
      }

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (it->second.probability > highestProb)
	    {
	      highestProb = it->second.probability;
	    }
	}

      lambda = alpha * highestProb;

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (it->second.probability < lambda)
	    {
	      it->second.excluded = true;
	      excluded = true;
	    }
	  else
	    {
	      it->second.excluded = false;
	    }
	}

      return excluded;
    }

    Dap
    DdsaRoutingProtocolAdapter::SelectDap()
    {
      double rand = m_rnd->GetValue();
      double prob_sum = 0;
      Dap selectedDap;
      selectedDap.address = Ipv4Address::GetBroadcast();
      bool dapFound = false;

      std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin();


      while (it != m_gateways.end() && !dapFound)
      	{

	  prob_sum += it->second.probability;

      	  if ((!it->second.excluded || dumb) && prob_sum >= rand)
      	    {
      	      selectedDap = it->second;
      	      dapFound = true;
      	    }

      	  it++;
      	}

      Ipv4Address m_address = GetMyMainAddress();
      if (selectedDap.address != Ipv4Address::GetBroadcast())
	{
	  NS_LOG_DEBUG("Dap: " << selectedDap.address << " selected by " << m_address);
	}
      else
	{
	  NS_LOG_DEBUG("No DAP Selected by node " << m_address);
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
    DdsaRoutingProtocolAdapter::AssociationTupleTimerExpire (Ipv4Address address)
    {
      std::map<Ipv4Address, Dap>::iterator it = m_gateways.find(address);

      if (it == m_gateways.end())
        {
          return;
        }

      if (it->second.expirationTime <= Simulator::Now ())
        {
	  m_gateways.erase(it);
        }
      else
        {
          m_events.Track (Simulator::Schedule (DELAY (it->second.expirationTime),
                                               &DdsaRoutingProtocolAdapter::AssociationTupleTimerExpire,
                                               this, it->first));
        }
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
    DdsaRoutingProtocolAdapter::UpdateDapCosts()
    {
      lqolsr::RoutingTableEntry rEntry;

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (Lookup(it->first, rEntry))
	    {
	      it->second.cost = rEntry.cost;
	    }
	}
    }

    void
    DdsaRoutingProtocolAdapter::RoutingTableComputation ()
    {
      RoutingProtocol::RoutingTableComputation();
      UpdateDapCosts();
      BuildEligibleGateways();

      std::vector<Dap> daps;

      for (std::map<Ipv4Address, Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
      	{
      	  daps.push_back(it->second);
      	}

      m_newRouteComputedTrace(daps);
    }

    Ptr<Ipv4Route>
    DdsaRoutingProtocolAdapter::RouteOutput (Ptr<Packet> p, Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
    {
      //This method can be removed later
      return RoutingProtocol::RouteOutput(p, header, oif, sockerr);
    }

    Ptr<Ipv4Route>
    DdsaRoutingProtocolAdapter::RouteOutput (Ptr<Packet> p, Ipv4Address dstAddr, Ptr<NetDevice> oif,
					     Socket::SocketErrno &sockerr)
    {
      Ipv4Header header;
      header.SetDestination (dstAddr);

      return RouteOutput(p, header, oif, sockerr);
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

    int
    DdsaRoutingProtocolAdapter::GetNRetransmissions()
    {
      return n_retransmissions;
    }
  }
}

