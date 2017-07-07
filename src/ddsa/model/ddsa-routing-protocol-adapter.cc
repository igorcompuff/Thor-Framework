/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/double.h"
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
                       MakeIntegerChecker<int> (0));
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
    DdsaRoutingProtocolAdapter::CalculateProbabilities()
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
    DdsaRoutingProtocolAdapter::ExcludeDaps()
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
    DdsaRoutingProtocolAdapter::SelectDap()
    {
      double rand = m_rnd->GetValue();
      double prob_sum = 0;
      Dap selectedDap;
      selectedDap.address = Ipv4Address::GetBroadcast();
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

    void
    DdsaRoutingProtocolAdapter::AssociationTupleTimerExpire (Ipv4Address address)
    {
      Dap gw;
      gw.address = address;
      std::vector<Dap>::iterator it = std::find(m_gateways.begin(), m_gateways.end(), gw);

      if (it == m_gateways.end())
        {
          return;
        }

      if (it->expirationTime <= Simulator::Now ())
        {
	  m_gateways.erase(it);
        }
      else
        {
          m_events.Track (Simulator::Schedule (DELAY (it->expirationTime),
                                               &DdsaRoutingProtocolAdapter::AssociationTupleTimerExpire,
                                               this, it->address));
        }
    }

    bool
    DdsaRoutingProtocolAdapter::DapExists(const Ipv4Address & dapAddress)
    {
      bool dapFund = false;
      std::vector<Dap>::iterator it = m_gateways.begin();

      while (!dapFund && it != m_gateways.end())
	{
	  dapFund = it->address == dapAddress;
	  it++;
	}

      return dapFund;
    }

    //A new HNA message is supposed to have been sent by a DAP.
    void
    DdsaRoutingProtocolAdapter::ProcessHna (const lqolsr::MessageHeader &msg, const Ipv4Address &senderIface)
    {
      Time now = Simulator::Now ();
      Ipv4Address dapAddress = msg.GetOriginatorAddress();

      lqolsr::RoutingTableEntry entry1;

      if (!DapExists(dapAddress))
	{
	  if (Lookup(dapAddress, entry1))
	    {
	      Dap gw;
	      gw.address = dapAddress;
	      gw.expirationTime = now + msg.GetVTime();
	      gw.cost = entry1.cost;
	      m_gateways.push_back(gw);

	      Simulator::Schedule (DELAY (gw.expirationTime), &DdsaRoutingProtocolAdapter::AssociationTupleTimerExpire, this, gw.address);

	      BuildEligibleGateways();
	    }
	}


      RoutingProtocol::ProcessHna(msg, senderIface);
    }

    void
    DdsaRoutingProtocolAdapter::UpdateDapCosts()
    {
      lqolsr::RoutingTableEntry rEntry;

      for (std::vector<Dap>::iterator it = m_gateways.begin(); it != m_gateways.end(); it++)
	{
	  if (Lookup(it->address, rEntry))
	    {
	      it->cost = rEntry.cost;
	    }
	}
    }

    void
    DdsaRoutingProtocolAdapter::RoutingTableComputation ()
    {
      RoutingProtocol::RoutingTableComputation();
      UpdateDapCosts();
      BuildEligibleGateways();
    }

    Ptr<Ipv4Route>
    DdsaRoutingProtocolAdapter::RouteOutput (Ptr<Packet> p, Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
    {
//      Dap selectedDap = SelectDap();
//      if (selectedDap.address != Ipv4Address::GetBroadcast())
//	{
//	  header.SetDestination(selectedDap.address);
//	}

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

