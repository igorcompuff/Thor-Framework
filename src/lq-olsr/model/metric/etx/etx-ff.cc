#include "etx-ff.h"
#include "ns3/lq-olsr-util.h"
#include "ns3/simulator.h"
#include "ns3/lq-olsr-routing-protocol.h"

namespace ns3{
namespace lqmetric{

Etx::Etx()
{
  etx_memory_length = 32;
  etx_metric_interval = Seconds(1.0);
  etx_seqno_restart_detection = 256;
  etx_hello_timeout_factor = 1.5;
  etx_perfect_metric = DEFAULT_METRIC * 0.1;
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
  return cost1 + cost2;
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
      info->metricHelloTime = info->metricHelloTime + info->metricHelloInterval;

      m_events.Track (Simulator::Schedule (DELAY (info->metricHelloTime),
         					       &Etx::Timeout,
         					       this,
         					       info,
         					       info->metricHelloTime));
    }

}

void
Etx::NotifyMessageReceived(Ptr<Packet> packet,
                           const Ipv4Address &receiverIface,
                           const Ipv4Address &senderIface)
{
  bool created = false;
  lqolsr::PacketHeader olsrPacketHeader;
  packet->RemoveHeader (olsrPacketHeader);

  std::map<Ipv4Address, EtxInfo>::iterator it = m_links_info.find(senderIface);

  EtxInfo * info;

  if (it == m_links_info.end())
    {
      EtxInfo newInfo(etx_memory_length);
      info = &newInfo;
      created = true;
    }
  else
    {
      info = &(it->second);
    }

  PacketProcessing(olsrPacketHeader, info);

  uint32_t sizeLeft = olsrPacketHeader.GetPacketLength () - olsrPacketHeader.GetSerializedSize ();

  MessageList messages;

  while (sizeLeft)
    {
      MessageHeader messageHeader;
      if (packet->RemoveHeader (messageHeader) == 0)
	{
	  NS_ASSERT (false);
	}

      sizeLeft -= messageHeader.GetSerializedSize ();

      messages.push_back (messageHeader);
    }

  for (MessageList::const_iterator messageIter = messages.begin ();
         messageIter != messages.end (); messageIter++)
    {
      const MessageHeader &messageHeader = *messageIter;

      if (messageHeader.GetMessageType () == lqolsr::MessageHeader::LQ_HELLO_MESSAGE)
	{
	  const lqolsr::MessageHeader::LqHello &hello = messageHeader.GetLqHello();
	  HelloProcessing(hello, receiverIface, info);
	}
    }

  if (created)
    {
      m_events.Track (Simulator::Schedule (DELAY (info->metricHelloTime), &Etx::Timeout, this, info, info->metricHelloTime));
      m_events.Track (Simulator::Schedule (DELAY (etx_metric_interval), &Etx::Compute, this, info));
    }
}

void
Etx::PacketProcessing(const lqolsr::PacketHeader &pkt, EtxInfo * info)
{
  if (info->metricLastPktSeqno.isUndefined)
    {
      info->metricReceivedLifo.SetCurent(1);
      info->metricTotalLifo.SetCurent(1);
    }
  else
    {
      info->metricReceivedLifo.IncrementCurrent();
      uint16_t diff = (pkt.GetPacketSequenceNumber() - info->metricLastPktSeqno.m_sequenceNumber);

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

  info->metricLastPktSeqno.m_sequenceNumber = pkt.GetPacketSequenceNumber();
  info->metricLastPktSeqno.isUndefined = false;

}



void
Etx::HelloProcessing( const lqolsr::MessageHeader::LqHello &hello, const Ipv4Address &receiverIface, EtxInfo * info)
{
  Time currentTime = Simulator::Now();

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
              if (info != NULL)
        	{
        	  if (neighIfaceInfo->neighborInterfaceAddress == receiverIface)
		    {
        	      info->metric_d_etx = unpack754_32(neighIfaceInfo->metricInfo / info->metric_r_etx);
		    }
        	  else
        	    {
        	      info->metric_d_etx = UNDEFINED_R_ETX;
        	    }
        	}

            }
        }

  info->metricHelloInterval = hello.GetHTime();
  info->metricHelloTime = currentTime + etx_hello_timeout_factor * hello.GetHTime();
  info->metricLostHellos = 0;

}

void
Etx::Compute(EtxInfo * info)
{
  int sum_received = info->metricReceivedLifo.Sum();
  int sum_total = info->metricTotalLifo.Sum();
  float sum_penalty = sum_received;
  float penalty = 0;

  if (info->metricHelloInterval.Compare(Time::Min()) != 0 && info->metricLostHellos > 0)
    {
      penalty = ((float)info->metricHelloInterval.GetSeconds() * info->metricLostHellos) / etx_memory_length;

      sum_penalty -= sum_received * penalty;
    }

  if (sum_penalty < 1)
    {
      info->metric_r_etx = UNDEFINED_R_ETX;
      info->metricValue = MAXIMUM_METRIC;
    }
  else
    {
      info->metric_r_etx = sum_total / sum_penalty;

      if (info->metric_d_etx == UNDEFINED_R_ETX)
	{
	  info->metricValue = DEFAULT_METRIC;
	}
      else
	{
	  info->metricValue = etx_perfect_metric * info->metric_d_etx * info->metric_r_etx;
	}
    }

  info->metricReceivedLifo.Push(0);
  info->metricTotalLifo.Push(0);

  m_events.Track (Simulator::Schedule (DELAY (etx_metric_interval), &Etx::Compute, this, info));

}

uint32_t
Etx::GetMetricInfo(const Ipv4Address & neighborIfaceAddress)
{
  std::map<Ipv4Address, EtxInfo>::iterator it = m_links_info.find(neighborIfaceAddress);

  if (it != m_links_info.end())
    {
      return pack754_32(it->second.metric_r_etx * it->second.metric_d_etx);
    }

  return pack754_32(UNDEFINED_R_ETX);
}

float
Etx::GetCost(const Ipv4Address & neighborIfaceAddress)
{
  std::map<Ipv4Address, EtxInfo>::iterator it = m_links_info.find(neighborIfaceAddress);

  if (it != m_links_info.end())
    {
      return 1/it->second.metricValue;
    }

  return MAXIMUM_METRIC;
}

float
Etx::GetCost(uint32_t metricInfo)
{
  return 1/(unpack754_32(metricInfo) * etx_perfect_metric);
}

float
Etx::GetInfinityCostValue()
{
  return MAXIMUM_METRIC;
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
