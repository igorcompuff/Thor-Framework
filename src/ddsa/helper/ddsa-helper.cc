/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ddsa-helper.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/lq-metric.h"

namespace ns3 {

DDsaHelper::DDsaHelper ()
{
  m_agentFactory.SetTypeId ("ns3::ddsa::DdsaRoutingProtocolAdapter");
  m_metricFactory.SetTypeId("ns3::lqmetric::Etx");
}

DDsaHelper::DDsaHelper (const TypeId & metricTid)
{
  m_agentFactory.SetTypeId ("ns3::ddsa::DdsaRoutingProtocolAdapter");
  m_metricFactory.SetTypeId(metricTid);
}

DDsaHelper::DDsaHelper (const TypeId & metricTid, const TypeId & routingProtocolTid)
{
  m_agentFactory.SetTypeId (routingProtocolTid);
  m_metricFactory.SetTypeId(metricTid);
}

DDsaHelper::DDsaHelper (const DDsaHelper &o): LqOlsrHelper(o)
{

}

LqOlsrHelper*
DDsaHelper::Copy (void) const
{
  return new DDsaHelper (*this);
}

} // namespace ns3
