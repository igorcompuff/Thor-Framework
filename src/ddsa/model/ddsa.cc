/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ddsa.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/lq-olsr-repositories.h"
#include <algorithm>

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("DdsaAdapter");

  namespace ddsa {

    NS_OBJECT_ENSURE_REGISTERED (DdsaAdapter);

    TypeId
    DdsaAdapter::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::ddsa::DdsaAdapter")
        .SetParent<RoutingProtocol> ()
        .SetGroupName ("Ddsa")
        .AddConstructor<DdsaAdapter> ()
        .AddAttribute ("Alpha", "The Alpha value used in the DAP exclusion algorithm",
                       DoubleValue (0.0),
                       MakeDoubleAccessor (&DdsaAdapter::alpha),
		       MakeDoubleChecker<double> (0))
        .AddAttribute ("Retrans", "Number of Retransmissions",
                       IntegerValue (1),
                       MakeIntegerAccessor(&DdsaAdapter::n_retransmissions),
                       MakeIntegerChecker<int> (1));
      return tid;
    }

    DdsaAdapter::DdsaAdapter(): alpha (0.0), n_retransmissions (1)
    {

    }

    DdsaAdapter::~DdsaAdapter(){}


    double
    DdsaAdapter::SumUpNotExcludedDapCosts()
    {
      double costSomatory = 0.0;

      for(std::vector<Dap>::const_iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
      	{
      	  if (!it->excluded)
      	    {
      	      if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_HIGHER)
      		{
      		  costSomatory += it->cost;
      		}
      	      else if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_LOWER)
      		{
      		  costSomatory += (1 / it->cost);
      		}
      	    }
      	}

      return costSomatory;
    }

    void
    DdsaAdapter::CalculateProbabilities()
    {
      double costSomatory = SumUpNotExcludedDapCosts();

      for(std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  //We only account for non-excluded Daps
	  if (!it->excluded)
	    {
	      if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_HIGHER)
		{
		  //If the cost is the highest possible, the probability is set to the highest value (p = 1) as well
		  it->probability = it->cost == m_metric->GetInfinityCostValue() ? 1 : it->cost / costSomatory;
		}
	      else if (getMetricType() == lqmetric::LqAbstractMetric::MetricType::BETTER_LOWER)
		{
		  //If the cost is the lowest possible, the probability is set to the highest value (p = 1)
		  it->probability = it->cost == 0 ? 0 : 1 / (it->cost * costSomatory);
		}
	    }
	}
    }

    bool
    DdsaAdapter::ExcludeDaps()
    {
    	double highestProb = 0;
    	double lambda = 0;
    	bool excluded = false;

    	if (alpha == 0)
    	{
    		return false;
    	}

    	for(std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); ++it)
    	{
	  if (it->probability > highestProb)
	  {
		  highestProb = it->probability;
	  }
    	}

    	lambda = alpha * highestProb;

    	for(std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); ++it)
    	{
	  if (it->probability < lambda)
	  {
	      it->excluded = true;
	      excluded = true;
	  }
	  else
	    {
	      it->excluded = false;
	    }
    	}

    	return excluded;
    }

    Dap
    DdsaAdapter::SelectDap()
    {
      double rand = m_rnd->GetValue(0, 1);
      double prob_sum = 0;
      Dap selectedDap;
      bool dapFound = false;

      std::vector<Dap>::iterator it = m_gateways.begin();

      while (it != m_gateways.end() && !dapFound)
	{
	  prob_sum += it->probability;

	  if (!it->excluded && prob_sum >= rand)
	    {
	      selectedDap = *it;
	      dapFound = true;
	    }

	  it++;
	}

      return selectedDap;
    }

    void
    DdsaAdapter::AssociationTupleTimerExpire (Ipv4Address address)
    {
      Dap gw;
      gw.address = address;
      std::vector<Dap>::iterator it = std::find(m_gateways.begin(), m_gateways.end(), gw);

      if (it == m_gateways.end())
        {
          return;
        }

      if (it->expirationTime < Simulator::Now ())
        {
	  m_gateways.erase(it);
        }
      else
        {
          m_events.Track (Simulator::Schedule (DELAY (it->expirationTime),
                                               &DdsaAdapter::AssociationTupleTimerExpire,
                                               this, it->address));
        }
    }

    //A new HNA message is supposed to have been sent by a DAP.
    void
    DdsaAdapter::ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface)
    {
      Time now = Simulator::Now ();

      //This code is repeated in the ProcessHna of the base class. In the future, this should be cleaned up in a refactoring process
      const lqolsr::LinkTuple *link_tuple = m_state.FindSymLinkTuple (senderIface, now);
      if (link_tuple == NULL)
	{
	  return;
	}

      Dap gw;
      gw.address = msg.GetOriginatorAddress();
      gw.expirationTime = now + msg.GetVTime();

      std::vector<Dap>::iterator it = std::find(m_gateways.begin(), m_gateways.end(), gw);
      lqolsr::RoutingTableEntry entry1;

      if (it == m_gateways.end())
	{
	  if (Lookup(gw.address, entry1) != 0)
	    {
	      gw.cost = entry1.cost;
	      m_gateways.push_back(gw);

	      Simulator::Schedule (DELAY (gw.expirationTime), &DdsaAdapter::AssociationTupleTimerExpire, this, gw.address);

	      BuildEligibleGateways();
	    }
	}

      RoutingProtocol::ProcessHna(msg, senderIface);
    }

    void
    DdsaAdapter::LqRoutingTableComputation ()
    {
      RoutingProtocol::LqRoutingTableComputation();
      BuildEligibleGateways();
    }

    Ptr<Ipv4Route>
    DdsaAdapter::RouteOutput (Ptr<Packet> p, Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
    {
      Dap selectedDap = SelectDap();
      header.SetDestination(selectedDap.address);

      return RoutingProtocol::RouteOutput(p, header, oif, sockerr);
    }

    void
    DdsaAdapter::BuildEligibleGateways()
    {
    	bool processDapComputation = true;

    	while (processDapComputation)
    	{
    		CalculateProbabilities();
    		processDapComputation = ExcludeDaps();
    	}
    }

  }
}

