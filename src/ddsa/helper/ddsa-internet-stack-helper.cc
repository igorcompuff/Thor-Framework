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
 * Author: Faker Moatamri <faker.moatamri@sophia.inria.fr>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/ipv4-l3-protocol-ddsa-adapter.h"
#include "ddsa-internet-stack-helper.h"



namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DdsaInternetStackHelper");


DdsaInternetStackHelper::DdsaInternetStackHelper (): InternetStackHelper()

{
  nodeType = ddsa::Ipv4L3ProtocolDdsaAdapter::NodeType::METER;
}

DdsaInternetStackHelper::~DdsaInternetStackHelper ()
{

}

DdsaInternetStackHelper::DdsaInternetStackHelper (const DdsaInternetStackHelper &o): InternetStackHelper(o)
{
  nodeType = o.nodeType;
}

DdsaInternetStackHelper &
DdsaInternetStackHelper::operator = (const DdsaInternetStackHelper &o)
{
  InternetStackHelper::operator=(o);
  return *this;
}

void
DdsaInternetStackHelper::SetNodeType(ddsa::Ipv4L3ProtocolDdsaAdapter::NodeType nt)
{
  nodeType = nt;
}


void
DdsaInternetStackHelper::InstallIpv4Protocols (Ptr<Node> node) const
{
  CreateAndAggregateObjectFromTypeId (node, "ns3::ArpL3Protocol");
  CreateAndAggregateObjectFromTypeId (node, "ns3::ddsa::Ipv4L3ProtocolDdsaAdapter");
  CreateAndAggregateObjectFromTypeId (node, "ns3::Icmpv4L4Protocol");

  Ptr<ns3::ddsa::Ipv4L3ProtocolDdsaAdapter> ipv4L3 = node->GetObject<ns3::ddsa::Ipv4L3ProtocolDdsaAdapter>();
  NS_ASSERT (ipv4L3);
  ipv4L3->SetNodeType(nodeType);
}

} // namespace ns3
