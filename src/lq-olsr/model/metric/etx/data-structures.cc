#include "data-structures.h"
#include "etx-ff.h"

namespace ns3{
namespace lqmetric{

EtxInfo::EtxInfo()
{
  Initialize(1);
}

EtxInfo::EtxInfo(std::vector<int>::size_type max_size)
{

  Initialize(max_size);
}

void
EtxInfo::Initialize(std::vector<int>::size_type max_size)
{
  metricReceivedLifo.SetMaxSize(max_size);
  metricTotalLifo.SetMaxSize(max_size);
  metricReceivedLifo.Push(0);
  metricLostHellos = 0;
  metric_d_etx = 0;
  metric_r_etx = Etx::UNDEFINED_R_ETX;
  metricHelloInterval = Time::Min();
  metricTotalLifo.Push(0);
  metricValue = Etx::INFINITY_COST;
  metricLastPktSeqno.isUndefined = true;
  metricLastPktSeqno.m_sequenceNumber = 0;
}

void
EtxInfo::SetMaxQueueSize(std::vector<int>::size_type max_size)
{
  metricReceivedLifo.SetMaxSize(max_size);
  metricTotalLifo.SetMaxSize(max_size);
}

LifoQueue::LifoQueue()
{

}

LifoQueue::LifoQueue(std::vector<int>::size_type max_size)
{
  this->max_size = max_size;

}

LifoQueue::~LifoQueue(){}

int
LifoQueue::GetSize()
{
  return (int)list.size();
}

int
LifoQueue::SetMaxSize(std::vector<int>::size_type max_size)
{
  return this->max_size = max_size;
}

void
LifoQueue::Push(int value)
{
  if (list.size() >= max_size)
    {
      list.erase(list.begin());
    }

  list.push_back(value);
}

int
LifoQueue::GetCurrent()
{
  return list[list.size() - 1];
}

void
LifoQueue::IncrementCurrent()
{
  list[list.size() - 1]++;
}

void
LifoQueue::IncrementCurrent(int amount)
{
  list[list.size() - 1]+= amount;
}

void
LifoQueue::SetCurent(int value)
{
  list[list.size() - 1] = value;
}

int
LifoQueue::Sum()
{
  int sum = 0;
  for (std::vector<int>::const_iterator it = list.begin(); it != list.end(); it++)
    {
      sum += *it;
    }

  return sum;
}

}
}
