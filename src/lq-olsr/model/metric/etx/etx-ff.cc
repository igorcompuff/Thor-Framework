#define NS_LOG_APPEND_CONTEXT std::clog << "[node " << m_address << "] (" << Simulator::Now().GetSeconds() << " s) ";

#include "etx-ff.h"
#include "ns3/lq-olsr-util.h"
#include "ns3/simulator.h"
#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/log.h"
#include <string>

namespace ns3{

  NS_LOG_COMPONENT_DEFINE ("Etx");

namespace lqmetric{

  NS_OBJECT_ENSURE_REGISTERED (Etx);

Etx::Etx()
{
  etx_memory_length = 32;
  etx_metric_interval = Seconds(3.0);
  //etx_seqno_restart_detection = 256;
  etx_hello_timeout_factor = 1.5;
  etx_perfect_metric = 1;
  m_address = Ipv4Address::GetBroadcast();
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

LqAbstractMetric::MetricType
Etx::GetMetricType()
{
  return LqAbstractMetric::MetricType::BETTER_LOWER;
}


void
Etx::Timeout(EtxInfo * info)
{
  info->metricLostHellos++;

  info->metricHelloTime = CalculateHelloTime(info->metricHelloInterval);
  info->timeoutEvtId = Simulator::Schedule (DELAY (info->metricHelloTime), &Etx::Timeout, this, info);

  NS_LOG_DEBUG("Timeout for " << info->senderAddress << ". N Lost hellos = " << info->metricLostHellos << " at " << Simulator::Now().GetSeconds());
}

Time
Etx::CalculateHelloTime(Time HelloInterval)
{
  double timeToAdd = HelloInterval.GetSeconds() * etx_hello_timeout_factor;

  return Simulator::Now() + Seconds(timeToAdd);
}

void
Etx::NotifyMessageReceived(uint16_t packetSeqNumber,
			   const lqolsr::MessageList & messages,
                           const Ipv4Address &receiverIface,
                           const Ipv4Address &senderIface)
{
  NS_LOG_DEBUG("New Packet Received: " << senderIface << " --> " << receiverIface << " (sqNum = " <<  packetSeqNumber << ")");

  m_address = receiverIface;

  bool created = false;

  std::map<Ipv4Address, EtxInfo>::iterator it = m_links_info.find(senderIface);
  EtxInfo * info;

  if (it == m_links_info.end())
    {
      m_links_info[senderIface].SetMaxQueueSize(etx_memory_length);
      m_links_info[senderIface].senderAddress = senderIface;
      m_links_info[senderIface].receiverAddress = receiverIface;
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
      info->computationEventId = Simulator::Schedule (DELAY (etx_metric_interval + Simulator::Now()), &Etx::Compute, this, info);
    }

  LogInfo(info);
}

void
Etx::NotifyLinkExpired(const Ipv4Address & neighborAddress)
{
  std::map<Ipv4Address, EtxInfo>::iterator it = m_links_info.find(neighborAddress);

  if (it != m_links_info.end())
    {
      NS_LOG_DEBUG("Erasing metric info for neighbor " << neighborAddress);

      if (it->second.computationEventId.IsRunning())
        {
	  it->second.computationEventId.Cancel();
        }

      if (it->second.timeoutEvtId.IsRunning())
        {
	  it->second.timeoutEvtId.Cancel();
        }

      m_links_info.erase(it);
    }
}

void
Etx::PacketProcessing(uint16_t packetSeqNumber, EtxInfo * info)
{
  NS_LOG_DEBUG("Processing packet " << packetSeqNumber);

  if (info->metricLastPktSeqno.isUndefined)
    {
      info->metricReceivedLifo.SetCurent(1);

      //I suppose pachet sequence number starts at 0
      info->metricTotalLifo.SetCurent(packetSeqNumber + 1);

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

//      if (diff > etx_seqno_restart_detection)
//	{
//	  diff = 1;
//	}

      info->metricTotalLifo.IncrementCurrent(diff);
    }

  info->metricLastPktSeqno.m_sequenceNumber = packetSeqNumber;
}

void
Etx::HelloProcessing( const lqolsr::MessageHeader::LqHello &hello, const Ipv4Address &receiverIface, EtxInfo * info)
{
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
      info->metric_d_etx = UNDEFINED_VALUE;
    }

  info->metricHelloInterval = hello.GetHTime();
  info->metricHelloTime = CalculateHelloTime(info->metricHelloInterval);
  info->metricLostHellos = 0;

  //If we have already a timeout event scheduled, we must cancel it and schedule it again.
  if (info->timeoutEvtId.IsRunning())
  {
    info->timeoutEvtId.Cancel();
  }

  info->timeoutEvtId = Simulator::Schedule (DELAY (info->metricHelloTime), &Etx::Timeout, this, info);
}

void
Etx::LogInfo(EtxInfo * info)
{
  NS_LOG_DEBUG("Current Received = " << info->metricReceivedLifo.GetCurrent());
  NS_LOG_DEBUG("Total = " << info->metricTotalLifo.GetCurrent());
  NS_LOG_DEBUG("Last Seq Number = " << info->metricLastPktSeqno.m_sequenceNumber);

  if (info->metric_d_etx == UNDEFINED_VALUE)
    {
      NS_LOG_DEBUG("D_x = UNDEFINED_VALUE");
    }
  else
    {
      NS_LOG_DEBUG("D_x = " << info->metric_d_etx);
    }

  NS_LOG_DEBUG("Hello Interval: " << info->metricHelloInterval.GetSeconds());
  NS_LOG_DEBUG("Hello Time: " << info->metricHelloTime.GetSeconds());
}

void
Etx::Compute(EtxInfo * info)
{


  NS_LOG_DEBUG("Computing ETX at " << Simulator::Now().GetSeconds() << "s.");

  int sum_received = info->metricReceivedLifo.Sum();
  int sum_total = info->metricTotalLifo.Sum();
  float sum_penalty = sum_received;
  float penalty = 0;

  NS_LOG_DEBUG("Sum Received = " << sum_received << " Sum Total = " << sum_total);

  if (info->metricLostHellos > 0)
    {
      penalty = (float)info->metricHelloInterval.GetSeconds() * ((double)info->metricLostHellos / etx_memory_length);

      sum_penalty -= sum_received * penalty;

      NS_LOG_DEBUG("Has penalty = " << penalty << " for " << info->senderAddress << ". Lost Hellos =  " << info->metricLostHellos);
      NS_LOG_DEBUG("Sum Penalty = " << sum_penalty);
    }

  //info->metric_r_etx = (double)sum_received / (sum_total + info->metricLostHellos);


  if (sum_penalty >= 1 && sum_total > 0)
    {
      info->metric_r_etx = sum_penalty / sum_total;
    }
  else
    {
      info->metric_r_etx = UNDEFINED_VALUE;
    }

  if (info->metric_r_etx == UNDEFINED_VALUE || info->metric_d_etx == UNDEFINED_VALUE)
    {
      info->metricValue = INFINITY_COST;
    }
  else
    {
      float x = (etx_perfect_metric * info->metric_d_etx * info->metric_r_etx);
      info->metricValue = x > 0 ? 1 / x : INFINITY_COST;
    }

//  if (info->metric_r_etx < 0.01)
//    {
//      info->metric_r_etx = UNDEFINED_VALUE;
//      info->metricValue = INFINITY_COST;
//    }
//  else if (info->metric_d_etx == UNDEFINED_VALUE)
//    {
//      info->metricValue = INFINITY_COST;
//    }
//  else
//    {
//      float x = (etx_perfect_metric * info->metric_d_etx * info->metric_r_etx);
//
//      if (x > 0)
//	{
//	  info->metricValue = 1 / x;
//	}
//      else
//	{
//	  info->metricValue = INFINITY_COST;
//	}
//    }

  info->metricReceivedLifo.Push(0);
  info->metricTotalLifo.Push(0);

  NS_LOG_DEBUG("Computation finished for link " << info->senderAddress << " -------" << info->receiverAddress << ". New cost = " << info->metricValue << "(Rx = " << info->metric_r_etx << ", Dx = " << info->metric_d_etx << ").");
  NS_LOG_DEBUG("Sender = " << info->senderAddress << " Sum_Total = " << sum_total << " Sum_received = " << sum_received << " Lost helllos = " << info->metricLostHellos << " LastSeqNumber = " << info->metricLastPktSeqno.m_sequenceNumber);

  NS_LOG_DEBUG("Total:");
  std::string msg = "";

  for (int i = 0; i < info->metricTotalLifo.GetSize(); i++)
    {
      msg += (std::to_string(info->metricTotalLifo.At(i)) + ", ");

    }
  NS_LOG_DEBUG(msg);

  Time nextSched = Simulator::Now() +  etx_metric_interval;
  info->computationEventId = Simulator::Schedule (DELAY (nextSched), &Etx::Compute, this, info);

}

uint32_t
Etx::GetHelloInfo(const Ipv4Address & neighborIfaceAddress)
{
  uint32_t info;
  std::map<Ipv4Address, EtxInfo>::iterator it = m_links_info.find(neighborIfaceAddress);
  if (it != m_links_info.end())
    {
      info = pack754_32(it->second.metric_r_etx);
    }
  else
    {
      info = pack754_32(UNDEFINED_VALUE);
    }

  NS_LOG_DEBUG("Requested Hello Info to " << neighborIfaceAddress << " = " << info);

  return info;
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

  NS_LOG_DEBUG("Requested cost to " << neighborIfaceAddress << ": " << cost);

  return cost;
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
Etx::Compound(float cost1, float cost2)
{
  float newCost = cost1 + cost2;
  NS_LOG_DEBUG("Compounded = " << newCost);
  return newCost >= INFINITY_COST ? INFINITY_COST : newCost;
}

float
Etx::Decompound(float compoundedCost, float partialCost)
{
  return compoundedCost - partialCost;
}

}
}
