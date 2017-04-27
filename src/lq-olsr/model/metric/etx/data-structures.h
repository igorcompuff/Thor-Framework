/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef LQ_METRIC_DATA_STRUCTURES_H
#define LQ_METRIC_DATA_STRUCTURES_H

#include <set>
#include <vector>
#include <queue>
#include <map>

#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

namespace ns3 {
namespace lqmetric {

  struct SequenceNumber
  {
    uint16_t m_sequenceNUmber;
    bool isUndefined;
  };

  class LifoQueue
  {
    private:
    std::vector<int>::size_type max_size;
    std::vector<int> list;

    public:
      LifoQueue(std::vector<int>::size_type max_size);
      ~LifoQueue();
      void Push(int value);
      int GetCurrent();
      void IncrementCurrent();
      void IncrementCurrent(int amount);
      void SetCurent(int value);
      int Sum();
  };

  struct EtxInfo
    {
      EtxInfo(std::vector<int>::size_type max_size);

      LifoQueue metricReceivedLifo; //L_METRIC_received_lifo
      LifoQueue metricTotalLifo; //L_METRIC_received_lifo
      SequenceNumber metricLastPktSeqno; // Sequence Number of the last received packet
      float metric_r_etx;
      float metric_d_etx;
      Time metricHelloTime;
      Time metricHelloInterval;
      int metricLostHellos;
      float metricValue;
    };
}
}

#endif /* LQ_OLSR_REPOSITORIES_H */

