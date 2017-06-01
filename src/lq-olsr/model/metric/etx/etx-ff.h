/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INESC Porto
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt>
 */

#ifndef ETX_H
#define ETX_H

#include <stdint.h>
#include <map>
#include "ns3/header.h"
#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/lq-olsr-header.h"
#include "data-structures.h"
#include <limits>
#include "ns3/lq-metric.h"
#include "ns3/packet.h"
#include "ns3/event-garbage-collector.h"
#include "ns3/lq-olsr-routing-protocol.h"


namespace ns3 {
namespace lqmetric {

using namespace ns3::lqolsr;

/**
 *
*/
class Etx: public LqAbstractMetric
{
  public:

    //Constructors and destructors
    Etx ();
    virtual ~Etx ();


    //Public methods
    static TypeId GetTypeId (void);

    /*
     * Gets the cost of the link to the specified neighbor ifaddress
     */
    virtual float GetCost(const Ipv4Address & neighborIfaceAddress);

    /*
     * Gets the cost calculated based on the specified metricInfo
     */
    virtual float GetCost(uint32_t metricInfo);

    virtual uint32_t GetHelloInfo(const Ipv4Address & neighborIfaceAddress);

    /*
     * Get the highest link cost possible.
     */
    virtual float GetInfinityCostValue();

    /*
     * Notifies the metric about the reception of a message, so it can take the appropriate
     * actions.
     */
    virtual void NotifyMessageReceived(uint16_t packetSeqNumber,
				       const MessageList & messages,
		                       const Ipv4Address &receiverIface,
		                       const Ipv4Address &senderIface);


    /*
     * Compares two costs regarding the quality of link/path. Some metrics give higher
     * values to higher quality links, while other metrics give higher values to lower
     * quality links.
     * This method returns an int > 0 if cost1 represents a better quality link/path than
     * cost2. If the returned value is < 0, cost2 represents a better quality link/path than
     * cost1. Otherwise, if the returned value is equal to zero, both costs represents a
     * link/path of the same quality.
     *
    */
    virtual int CompareBest(float cost1, float cost2);

    /*
     * When computing the cost of a path, we need to calculate the resulting cost from a
     * composition of the cost from node A to node B with the cost of the path from node B
     * to the destination. It is not true that for all existing quality-aware metrics this
     * composition consists in simple summation.
     * In this sense, this method allows a specific metric to implement the way it compound
     * two costs.
     *
    */
    virtual float Compound(float cost1, float cost2);

    virtual float Decompound(float compoundedCost, float partialCost);

    virtual MetricType GetMetricType();

    //This cost is achieved when the link has a delivery probability lower than 1% in both directions
    static constexpr float INFINITY_COST = 124;

    static constexpr float UNDEFINED_R_ETX = -1;
    static const uint16_t MAX_SEQ_NUM = 65535;

private:

    std::map<Ipv4Address, EtxInfo> m_links_info;
    uint16_t etx_memory_length;
    Time etx_metric_interval;
    uint16_t etx_seqno_restart_detection;
    float etx_hello_timeout_factor;
    float etx_perfect_metric;
    //Timer m_helloTimer;
    //Timer m_calculationTimer;
    EventGarbageCollector m_events;

    void HelloProcessing( const lqolsr::MessageHeader::LqHello &hello, const Ipv4Address &receiverIface, EtxInfo * info);
    void Compute(EtxInfo * info);
    void PacketProcessing(uint16_t packetSeqNumber, EtxInfo * info);
    void Timeout(EtxInfo * info, const Time & expirationTime);
};



}
}  // namespace lqolsr, ns3

#endif /* ETX_H */
