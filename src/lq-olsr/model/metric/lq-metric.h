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

#ifndef LQ_ABSTRACT_METRIC_H
#define LQ_ABSTRACT_METRIC_H

#include <stdint.h>
#include <vector>
#include "ns3/header.h"
#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/lq-olsr-header.h"


namespace ns3 {
namespace lqmetric {

using namespace ns3::lqolsr;

/**
 *
*/
class LqAbstractMetric: public Object
{
public:

    //Constructors and destructors
    LqAbstractMetric ();
    virtual ~LqAbstractMetric ();


    //Public methods
    static TypeId GetTypeId (void);

    /*
     * Gets the cost of the link/path to the specified ifaddress
     */
    virtual float GetCost(const Ipv4Address & neighborIfaceAddress) const = 0;

    virtual uint32_t GetMetricInfo(const Ipv4Address & neighborIfaceAddress) const = 0;

    /*
     * Get the highest link cost possible.
     */
    virtual float GetInfinityCostValue() = 0;

    /*
     * Notifies the metric about the reception of a message, so it can take the appropriate
     * action.
     */
    virtual void NotifyMessageReceived(const MessageHeader& header) = 0;


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
    virtual int CompareBest(double cost1, double cost2) = 0;

    /*
     * When computing the cost of a path, we need to calculate the resulting cost from a
     * composition of the cost from node A to node B with the cost of the path from node B
     * to the destination. It is not true that for all existing quality-aware metrics this
     * composition consists in simple summation.
     * In this sense, this method allows a specific metric to implement the way it compound
     * two costs.
     *
    */
    virtual double CompoundMetrics(double cost1, double cost2) = 0;
};



}
}  // namespace lqolsr, ns3

#endif /* LQ_ABSTRACT_SYMMETRIC_METRIC_H */
