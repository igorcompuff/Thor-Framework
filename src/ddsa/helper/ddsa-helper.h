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
#ifndef LQ_DDSA_HELPER_H
#define LQ_DDSA_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/lq-olsr-helper.h"
#include <map>
#include <set>

namespace ns3 {

/**
 * \ingroup olsr
 *
 * \brief Helper class that adds OLSR routing to nodes.
 *
 * This class is expected to be used in conjunction with
 * ns3::InternetStackHelper::SetRoutingHelper
 */
class DDsaHelper : public LqOlsrHelper
{
public:
  /**
   * Create an OlsrHelper that makes life easier for people who want to install
   * OLSR routing to nodes.
   */
  DDsaHelper ();

  DDsaHelper (const TypeId & metricTid);

  DDsaHelper (const TypeId & metricTid, const TypeId & routingProtocolTid);

  /**
   * \brief Construct an OlsrHelper from another previously initialized instance
   * (Copy Constructor).
   */
  DDsaHelper (const DDsaHelper &);

  /**
   * \returns pointer to clone of this OlsrHelper
   *
   * This method is mainly for internal use by the other helpers;
   * clients are expected to free the dynamic memory allocated by this method
   */
  virtual LqOlsrHelper* Copy (void) const;
};

} // namespace ns3

#endif /* LQ_DDSA_HELPER_H */
