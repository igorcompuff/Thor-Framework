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

#ifndef DDSA_INTERNET_STACK_HELPER_H
#define DDSA_INTERNET_STACK_HELPER_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/internet-trace-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv6-routing-helper.h"
#include "ns3/internet-trace-helper.h"
#include "ns3/node.h"
#include "ns3/ipv4-l3-protocol-ddsa-adapter.h"

namespace ns3 {


class DdsaInternetStackHelper : public InternetStackHelper
{
public:

  DdsaInternetStackHelper();

  virtual ~DdsaInternetStackHelper(void);

  DdsaInternetStackHelper (const DdsaInternetStackHelper &);

  DdsaInternetStackHelper &operator = (const DdsaInternetStackHelper &o);

  void SetNodeType(ddsa::Ipv4L3ProtocolDdsaAdapter::NodeType nt);

private:
  virtual void InstallIpv4Protocols (Ptr<Node> node) const;
  ddsa::Ipv4L3ProtocolDdsaAdapter::NodeType nodeType;


};
} // namespace ns3

#endif /* DDSA_INTERNET_STACK_HELPER_H */
