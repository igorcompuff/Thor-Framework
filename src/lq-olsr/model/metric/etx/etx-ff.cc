#include "etx-ff.h"
#include "ns3/lq-olsr-util.h"
#include "ns3/simulator.h"
#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/log.h"

namespace ns3{

  NS_LOG_COMPONENT_DEFINE ("Etx");

namespace lqmetric{

  NS_OBJECT_ENSURE_REGISTERED (Etx);

Etx::Etx()
{
  etx_memory_length = 32;
  etx_metric_interval = Seconds(2.0);
  etx_seqno_restart_detection = 256;
  etx_hello_timeout_factor = 1.5;
  etx_perfect_metric = 1;
}


Etx::~Etx() { }

TypeId
Etx::GetTypeId()
{
	static TypeId tid = TypeId ("ns3::lqmetric::Etx")
	    .SetParent<LqAbstractMetric> ()
	    .SetGroupName ("LqMetric")
	    .AddConstructor<Etx> ();

	return tid;
}

float
Etx::Compound(float cost1, float cost2)
{
  float newCost = cost1 + cost2;
  return newCost >= INFINITY_COST ? INFINITY_COST : newCost;
}

LqAbstractMetric::MetricType
Etx::GetMetricType()
{
  return LqAbstractMetric::MetricType::BETTER_LOWER;
}


void
Etx::Timeout(EtxInfo * info, const Time & expirationTime)
{

  if (info->metricHelloTime == expirationTime)
    {
      info->metricLostHellos++;
      NS_LOG_DEBUG("Hello Expired. N Lost hellos = " << info->metricLostHellos);
      info->metricHelloTime = info->metricHelloTime + info->metricHelloInterval;

      Time nextSched = Simulator::Now() + info->metricHelloTime;
      m_events.Track (Simulator::Schedule (DELAY (nextSched), &Etx::Timeout, this, info, info->metricHelloTime));
    }

}

void
Etx::NotifyMessageReceived(uint16_t packetSeqNumber,
			   const lqolsr::MessageList & messages,
                           const Ipv4Address &receiverIface,
                           const Ipv4Address &senderIface)
{
  NS_LOG_DEBUG("New Packet Received: " << senderIface << " --> " << receiverIface
	       << " (sqNum = " <<  packetSeqNumber << ")");


  bool created = false;

  std::map<Ipv4Address, EtxInfo>::iterator it = m_links_info.find(senderIface);

  EtxInfo * info;

  if (it == m_links_info.end())
    {
      m_links_info[senderIface].SetMaxQueueSize(etx_memory_length);
      it = m_links_info.find(senderIface);
      created = true;
    }

  info = &(it->second);

  PacketProcessing(packetSeqNumber, info);

  for (MessageList::const_iterator messageIter = messages.begin ();
         messageIter != messages.end (); messageIter++)
      {
	if (messageIter->GetMessageType () == lqolsr::MessageHeader::LQ_HELLO_MESSAGE)
	  {
	    HelloProcessing(messageIter->GetLqHello(), receiverIface, info);
	  }
      }

  if (created)
    {
      Time now = Simulator::Now();
      m_events.Track (Simulator::Schedule (DELAY (info->metricHelloTime + now), &Etx::Timeout, this, info, info->metricHelloTime));
      m_events.Track (Simulator::Schedule (DELAY (etx_metric_interval + now), &Etx::Compute, this, info));
    }
}

void
Etx::PacketProcessing(uint16_t packetSeqNumber, EtxInfo * info)
{
  NS_LOG_DEBUG("Processing packet " << packetSeqNumber);

  if (info->metricLastPktSeqno.isUndefined)
    {
      info->metricReceivedLifo.SetCurent(1);
      info->metricTotalLifo.SetCurent(1);

      info->metricLastPktSeqno.isUndefined = false;
    }
  else
    {
      info->metricReceivedLifo.IncrementCurrent();
      uint16_t diff = (packetSeqNumber - info->metricLastPktSeqno.m_sequenceNumber);

      if (diff < 0)
	{
	  diff += MAX_SEQ_NUM;
	}

      if (diff > etx_seqno_restart_detection)
	{
	  diff = 1;
	}

      info->metricTotalLifo.IncrementCurrent(diff);
    }

  info->metricLastPktSeqno.m_sequenceNumber = packetSeqNumber;

  NS_LOG_DEBUG("Received = " << info->metricReceivedLifo.GetCurrent() <<
	       " Total = " << info->metricTotalLifo.GetCurrent() <<
	       " Last Seq Number = " << info->metricLastPktSeqno.m_sequenceNumber);

}

void
Etx::HelloProcessing( const lqolsr::MessageHeader::LqHello &hello, const Ipv4Address &receiverIface, EtxInfo * info)
{
  Time currentTime = Simulator::Now();

  NS_LOG_DEBUG("Processing Hello Message");

  const lqolsr::MessageHeader::NeighborInterfaceInfo * foundInfo = NULL;

  for (std::vector<lqolsr::MessageHeader::LqHello::LinkMessage>::const_iterator linkMessage =
      hello.linkMessages.begin ();
           linkMessage != hello.linkMessages.end ();
           linkMessage++)
        {
          for (std::vector<lqolsr::MessageHeader::NeighborInterfaceInfo>::const_iterator neighIfaceInfo =
               linkMessage->neighborInterfaceInformation.begin ();
	       neighIfaceInfo != linkMessage->neighborInterfaceInformation.end ();
               neighIfaceInfo++)
            {
              if (neighIfaceInfo->neighborInterfaceAddress == receiverIface)
		{
		  foundInfo = &(*neighIfaceInfo);
		}

            }
        }

  if (foundInfo != NULL)
    {
      info->metric_d_etx = unpack754_32(foundInfo->metricInfo);
    }
  else
    {
      info->metric_d_etx = UNDEFINED_R_ETX;
    }

  info->metricHelloInterval = hello.GetHTime();
  info->metricHelloTime = currentTime + etx_hello_timeout_factor * hello.GetHTime();
  info->metricLostHellos = 0;

  NS_LOG_DEBUG("Dx = " << info->metric_d_etx);

}

void
Etx::Compute(EtxInfo * info)
{
  NS_LOG_DEBUG("Computing ETX at " << Simulator::Now().GetSeconds());
  int sum_received = info->metricReceivedLifo.Sum();
  int sum_total = info->metricTotalLifo.Sum();
  float sum_penalty = sum_received;
  float penalty = 0;

  if (info->metricHelloInterval.Compare(Time::Min()) != 0 && info->metricLostHellos > 0)
    {
      penalty = ((float)info->metricHelloInterval.GetSeconds() * info->metricLostHellos) / etx_memory_length;

      sum_penalty -= sum_received * penalty;

      NS_LOG_DEBUG("Has penalty =  " << penalty << " And sum_penalty = " << sum_penalty);
    }

  if (sum_penalty < 1)
    {
      info->metric_r_etx = UNDEFINED_R_ETX;
      info->metricValue = INFINITY_COST;
    }
  else
    {
      info->metric_r_etx = sum_total / sum_penalty;

      if (info->metric_d_etx == UNDEFINED_R_ETX)
	{
	  info->metricValue = INFINITY_COST;
	}
      else
	{
	  float x = (etx_perfect_metric * info->metric_d_etx * info->metric_r_etx);

	  if (x > 0)
	    {
	      info->metricValue = 1 / x;
	    }
	  else
	    {
	      info->metricValue = INFINITY_COST;
	    }
	}
    }

  info->metricReceivedLifo.Push(0);
  info->metricTotalLifo.Push(0);

  NS_LOG_DEBUG("Computation finished. New cost = " << info->metricValue);

  Time nextSched = Simulator::Now() +  etx_metric_interval;
  m_events.Track (Simulator::Schedule (DELAY (nextSched), &Etx::Compute, this, info));

}

uint32_t
Etx::GetHelloInfo(const Ipv4Address & neighborIfaceAddress)
{
  std::map<Ipv4Address, EtxInfo>::iterator it = m_links_info.find(neighborIfaceAddress);
  if (it != m_links_info.end())
    {
      return pack754_32(it->second.metric_r_etx);
    }

  return pack754_32(UNDEFINED_R_ETX);
}

float
Etx::GetCost(const Ipv4Address & neighborIfaceAddress)
{
  std::map<Ipv4Address, EtxInfo>::iterator it = m_links_info.find(neighborIfaceAddress);

  float cost = INFINITY_COST;

  if (it != m_links_info.end())
    {
      cost = it->second.metricValue;
    }

  NS_LOG_DEBUG("Cost to " << neighborIfaceAddress << " is " << cost);

  return cost;
}

float
Etx::GetCost(uint32_t metricInfo)
{
  return 1/(unpack754_32(metricInfo) * etx_perfect_metric);
}

float
Etx::GetInfinityCostValue()
{
  return INFINITY_COST;
}

int
Etx::CompareBest(float cost1, float cost2)
{
  return cost1 < cost2 ? 1 : cost1 == cost2 ? 0 : -1;
}

float
Etx::Decompound(float compoundedCost, float partialCost)
{
  return compoundedCost - partialCost;
}

}
}
