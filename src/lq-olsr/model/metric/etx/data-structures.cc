#include "data-structures.h"
#include "etx-ff.h"

namespace ns3{
namespace lqmetric{

EtxInfo::EtxInfo(std::vector<int>::size_type max_size): metricReceivedLifo(max_size), metricTotalLifo(max_size)
{

  metricReceivedLifo.Push(0);
  metricLostHellos = 0;
  metric_d_etx = 0;
  metric_r_etx = Etx::UNDEFINED_R_ETX;
  metricHelloInterval = Time::Min();
  metricTotalLifo.Push(0);
  metricValue = Etx::MAXIMUM_METRIC;
}

LifoQueue::LifoQueue(std::vector<int>::size_type max_size)
{
  this->max_size = max_size;

}

LifoQueue::~LifoQueue(){}

void
LifoQueue::Push(int value)
{
  list.push_back(value);

  if (list.size() >= max_size)
    {
      list.erase(list.begin());
    }
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
