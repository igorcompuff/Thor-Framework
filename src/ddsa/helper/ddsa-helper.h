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
#include "ns3/lq-olsr-helper.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-routing-helper.h"
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
  DDsaHelper* Copy (void) const;

  /**
   * \param node the node on which the routing protocol will run
   * \returns a newly-created routing protocol
   *
   * This method will be called by ns3::InternetStackHelper::Install
   */
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.  The Install() method of the InternetStackHelper
   * should have previously been called by the user.
   *
   * \param stream first stream index to use
   * \param c NodeContainer of the set of nodes for which the OlsrRoutingProtocol
   *          should be modified to use a fixed stream
   * \return the number of stream indices assigned by this helper
   */
  virtual int64_t AssignStreams (NodeContainer c, int64_t stream);

  void SetAttribute(std::string name, const AttributeValue &value);

private:
  /**
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   */
  DDsaHelper &operator = (const DDsaHelper &);
  ObjectFactory m_agentFactory; //!< Object factory

  std::map< Ptr<Node>, std::set<uint32_t> > m_interfaceExclusions; //!< container of interfaces excluded from OLSR operations
};

} // namespace ns3

#endif /* LQ_DDSA_HELPER_H */
