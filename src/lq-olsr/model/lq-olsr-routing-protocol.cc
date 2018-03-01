/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2004 Francisco J. Ros
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
 * Authors: Francisco J. Ros  <fjrm@dif.um.es>
 *          Gustavo J. A. M. Carneiro <gjc@inescporto.pt>
 */


///
/// \brief Implementation of OLSR agent and related classes.
///
/// This is the main file of this software because %OLSR's behaviour is
/// implemented here.
///

#define NS_LOG_APPEND_CONTEXT std::clog << "[node " << m_mainAddress << "] (" << Simulator::Now().GetSeconds() << " s) ";


#include "lq-olsr-routing-protocol.h"
#include "lq-olsr-util.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-route.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4-header.h"

#include <algorithm>



///
/// \brief Period at which a node must cite every link and every neighbor.
///
/// We only use this value in order to define OLSR_NEIGHB_HOLD_TIME.
///
#define OLSR_REFRESH_INTERVAL   m_helloInterval


/********** Holding times **********/

/// Neighbor holding time.
#define OLSR_NEIGHB_HOLD_TIME   Time (3 * OLSR_REFRESH_INTERVAL)
/// Top holding time.
#define OLSR_TOP_HOLD_TIME      Time (3 * m_tcInterval)
/// Dup holding time.
#define OLSR_DUP_HOLD_TIME      Seconds (30)
/// MID holding time.
#define OLSR_MID_HOLD_TIME      Time (3 * m_midInterval)
/// HNA holding time.
#define OLSR_HNA_HOLD_TIME      Time (3 * m_hnaInterval)

/********** Link types **********/

/// Unspecified link type.
#define OLSR_UNSPEC_LINK        0
/// Asymmetric link type.
#define OLSR_ASYM_LINK          1
/// Symmetric link type.
#define OLSR_SYM_LINK           2
/// Lost link type.
#define OLSR_LOST_LINK          3

/********** Neighbor types **********/

/// Not neighbor type.
#define OLSR_NOT_NEIGH          0
/// Symmetric neighbor type.
#define OLSR_SYM_NEIGH          1
/// Asymmetric neighbor type.
#define OLSR_MPR_NEIGH          2


/********** Willingness **********/

/// Willingness for forwarding packets from other nodes: never.
#define OLSR_WILL_NEVER         0
/// Willingness for forwarding packets from other nodes: low.
#define OLSR_WILL_LOW           1
/// Willingness for forwarding packets from other nodes: medium.
#define OLSR_WILL_DEFAULT       3
/// Willingness for forwarding packets from other nodes: high.
#define OLSR_WILL_HIGH          6
/// Willingness for forwarding packets from other nodes: always.
#define OLSR_WILL_ALWAYS        7


/********** Miscellaneous constants **********/

/// Maximum allowed jitter.
#define OLSR_MAXJITTER          (m_helloInterval.GetSeconds () / 4)
/// Maximum allowed sequence number.
#define OLSR_MAX_SEQ_NUM        65535
/// Random number between [0-OLSR_MAXJITTER] used to jitter OLSR packet transmission.
#define JITTER (Seconds (m_uniformRandomVariable->GetValue (0, OLSR_MAXJITTER)))


#define OLSR_PORT_NUMBER 698
/// Maximum number of messages per packet.
#define OLSR_MAX_MSGS           64

/// Maximum number of hellos per message (4 possible link types * 3 possible nb types).
#define OLSR_MAX_HELLOS         12

/// Maximum number of addresses advertised on a message.
#define OLSR_MAX_ADDRS          64


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LqOlsrRoutingProtocol");

namespace lqolsr {

/********** OLSR class **********/

NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::lqolsr::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("LqOlsr")
    .AddConstructor<RoutingProtocol> ()
    .AddAttribute ("HelloInterval", "HELLO/LQ_HELLO messages emission interval.",
                   TimeValue (Seconds (2)),
                   MakeTimeAccessor (&RoutingProtocol::m_helloInterval),
                   MakeTimeChecker ())
    .AddAttribute ("TcInterval", "TC/LQ_TC messages emission interval.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&RoutingProtocol::m_tcInterval),
                   MakeTimeChecker ())
    .AddAttribute ("MidInterval", "MID messages emission interval.  Normally it is equal to TcInterval.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&RoutingProtocol::m_midInterval),
                   MakeTimeChecker ())
    .AddAttribute ("HnaInterval", "HNA messages emission interval.  Normally it is equal to TcInterval.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&RoutingProtocol::m_hnaInterval),
                   MakeTimeChecker ())
    .AddAttribute ("Willingness", "Willingness of a node to carry and forward traffic for other nodes.",
                   EnumValue (OLSR_WILL_ALWAYS),
                   MakeEnumAccessor (&RoutingProtocol::m_willingness),
                   MakeEnumChecker (OLSR_WILL_NEVER, "never",
                                    OLSR_WILL_LOW, "low",
                                    OLSR_WILL_DEFAULT, "default",
                                    OLSR_WILL_HIGH, "high",
                                    OLSR_WILL_ALWAYS, "always"))
    .AddTraceSource ("Rx", "Receive OLSR packet.",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_rxPacketTrace),
                     "ns3::lqolsr::RoutingProtocol::PacketTxRxTracedCallback")
    .AddTraceSource ("Tx", "Send OLSR packet.",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_txPacketTrace),
                     "ns3::lqolsr::RoutingProtocol::PacketTxRxTracedCallback")
    .AddTraceSource ("RoutingTableChanged", "The OLSR routing table has changed.",
                     MakeTraceSourceAccessor (&RoutingProtocol::m_routingTableChanged),
                     "ns3::lqolsr::RoutingProtocol::TableChangeTracedCallback")
  ;
  return tid;
}


RoutingProtocol::RoutingProtocol ()
  : m_routingTableAssociation (0),
  m_ipv4 (0),
  m_helloTimer (Timer::CANCEL_ON_DESTROY),
  m_tcTimer (Timer::CANCEL_ON_DESTROY),
  m_midTimer (Timer::CANCEL_ON_DESTROY),
  m_hnaTimer (Timer::CANCEL_ON_DESTROY),
  m_queuedMessagesTimer (Timer::CANCEL_ON_DESTROY)
{
  m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();

  m_hnaRoutingTable = Create<Ipv4StaticRouting> ();
}

RoutingProtocol::~RoutingProtocol ()
{
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);
  NS_LOG_DEBUG ("Created lqolsr::RoutingProtocol");
  m_helloTimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
  m_tcTimer.SetFunction (&RoutingProtocol::TcTimerExpire, this);
  m_midTimer.SetFunction (&RoutingProtocol::MidTimerExpire, this);
  m_hnaTimer.SetFunction (&RoutingProtocol::HnaTimerExpire, this);
  m_queuedMessagesTimer.SetFunction (&RoutingProtocol::SendQueuedMessages, this);

  m_packetSequenceNumber = OLSR_MAX_SEQ_NUM;
  m_messageSequenceNumber = OLSR_MAX_SEQ_NUM;
  m_ansn = OLSR_MAX_SEQ_NUM;

  m_linkTupleTimerFirstTime = true;

  m_ipv4 = ipv4;

  m_hnaRoutingTable->SetIpv4 (ipv4);
}

void RoutingProtocol::DoDispose ()
{
  m_ipv4 = 0;
  m_hnaRoutingTable = 0;
  m_routingTableAssociation = 0;

  for (std::map< Ptr<Socket>, Ipv4InterfaceAddress >::iterator iter = m_socketAddresses.begin ();
       iter != m_socketAddresses.end (); iter++)
    {
      iter->first->Close ();
    }
  m_socketAddresses.clear ();

  Ipv4RoutingProtocol::DoDispose ();
}

void
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
  std::ostream* os = stream->GetStream ();

  uint32_t myNodeId = m_ipv4->GetObject<Node> ()->GetId ();

  *os << "Node: " << myNodeId
      << ", Time: " << Now ().As (Time::S)
      << ", Local time: " << GetObject<Node> ()->GetLocalTime ().As (Time::S)
      << ", OLSR Routing table" << std::endl;

  *os << "Destination\t\tNextHop\t\tInterface\tCost\n";

  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator iter = m_table.begin ();
       iter != m_table.end (); iter++)
    {
      *os << iter->first << "\t\t";
      *os << iter->second.nextAddr << "\t\t";
      if (Names::FindName (m_ipv4->GetNetDevice (iter->second.interface)) != "")
        {
          *os << Names::FindName (m_ipv4->GetNetDevice (iter->second.interface)) << "\t\t";
        }
      else
        {
          *os << iter->second.interface << "\t\t";
        }

      *os << iter->second.cost << "\t";

      *os << "\n";
    }

  // Also print the HNA routing table
  if (m_hnaRoutingTable->GetNRoutes () > 0)
    {
      *os << " HNA Routing Table: ";
      m_hnaRoutingTable->PrintRoutingTable (stream);
    }
  else
    {
      *os << " HNA Routing Table: empty" << std::endl;
    }
}

void
RoutingProtocol::SetLqMetric(Ptr<lqmetric::LqAbstractMetric> metric)
  {
    if (m_metric == NULL)
      {
	NS_LOG_DEBUG ("The metric is defined as: " << metric->GetTypeId().GetName());
	m_metric = metric;
      }
  }

void RoutingProtocol::DoInitialize ()
{
  if (m_mainAddress == Ipv4Address ())
    {
      Ipv4Address loopback ("127.0.0.1");
      for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
        {
          // Use primary address, if multiple
          Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
          if (addr != loopback)
            {
              m_mainAddress = addr;
              break;
            }
        }

      NS_ASSERT (m_mainAddress != Ipv4Address ());
    }

  NS_ASSERT(m_metric != NULL);

  NS_LOG_DEBUG ("Starting LQ_OLSR on node " << m_mainAddress);

  Ipv4Address loopback ("127.0.0.1");

  bool canRunOlsr = false;
  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
    {
      Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
      if (addr == loopback)
        {
          continue;
        }

      if (addr != m_mainAddress)
        {
          // Create never expiring interface association tuple entries for our
          // own network interfaces, so that GetMainAddress () works to
          // translate the node's own interface addresses into the main address.
          IfaceAssocTuple tuple;
          tuple.ifaceAddr = addr;
          tuple.mainAddr = m_mainAddress;
          AddIfaceAssocTuple (tuple);
          NS_ASSERT (GetMainAddress (addr) == m_mainAddress);
        }

      if (m_interfaceExclusions.find (i) != m_interfaceExclusions.end ())
        {
          continue;
        }

      // Create a socket to listen only on this interface
      Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                 UdpSocketFactory::GetTypeId ());
      socket->SetAllowBroadcast (true);
      InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), OLSR_PORT_NUMBER);
      socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvOlsr,  this));
      if (socket->Bind (inetAddr))
        {
          NS_FATAL_ERROR ("Failed to bind() OLSR socket");
        }
      socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
      m_socketAddresses[socket] = m_ipv4->GetAddress (i, 0);

      canRunOlsr = true;
    }

  if (canRunOlsr)
    {
      HelloTimerExpire ();
      TcTimerExpire ();
      MidTimerExpire ();
      HnaTimerExpire ();

      NS_LOG_DEBUG ("LQ_OLSR on node " << m_mainAddress << " started");
    }
}

lqmetric::LqAbstractMetric::MetricType
RoutingProtocol::getMetricType()
{
  if (m_metric != NULL)
    {
      return m_metric->GetMetricType();
    }

  return lqmetric::LqAbstractMetric::MetricType::NOT_DEF;
}

Ptr<lqmetric::LqAbstractMetric>
RoutingProtocol::GetMetric()
{
  return m_metric;
}

void RoutingProtocol::SetMainInterface (uint32_t interface)
{
  m_mainAddress = m_ipv4->GetAddress (interface, 0).GetLocal ();
}

void RoutingProtocol::SetInterfaceExclusions (std::set<uint32_t> exceptions)
{
  m_interfaceExclusions = exceptions;
}

MessageList
RoutingProtocol::ExtractCorrectMessagesFromPacket(Ptr<Packet> packet, const lqolsr::PacketHeader & olsrPacketHeader)
{
  NS_ASSERT (olsrPacketHeader.GetPacketLength () >= olsrPacketHeader.GetSerializedSize ());

  uint32_t sizeLeft = olsrPacketHeader.GetPacketLength () - olsrPacketHeader.GetSerializedSize ();
  MessageList messages;

  while (sizeLeft)
   {
     MessageHeader messageHeader;
     if (packet->RemoveHeader (messageHeader) == 0)
       {
	 NS_ASSERT (false);
       }

     sizeLeft -= messageHeader.GetSerializedSize ();

     // If ttl is less than or equal to zero, or
     // the receiver is the same as the originator,
     // the message must be silently dropped
     if (messageHeader.GetTimeToLive () == 0 || messageHeader.GetOriginatorAddress () == m_mainAddress)
      {
	packet->RemoveAtStart (messageHeader.GetSerializedSize () - messageHeader.GetSerializedSize ());
	continue;
      }

     NS_LOG_DEBUG ("Olsr Msg received with type "
		   << std::dec << int (messageHeader.GetMessageType ())
		   << " TTL=" << int (messageHeader.GetTimeToLive ())
		   << " origAddr=" << messageHeader.GetOriginatorAddress ()
		   << " Seq Number = " << messageHeader.GetMessageSequenceNumber());

     messages.push_back (messageHeader);
   }

  m_rxPacketTrace (olsrPacketHeader, messages);

  return messages;
}

MessageList
RoutingProtocol::FilterNonDuplicatedMessages(const MessageList & messages)
{
  MessageList nonDuplicatedMessages;
  for (MessageList::const_iterator messageIter = messages.begin (); messageIter != messages.end (); messageIter++)
    {
      const MessageHeader &messageHeader = *messageIter;

      DuplicateTuple *duplicated = m_state.FindDuplicateTuple(messageHeader.GetOriginatorAddress (), messageHeader.GetMessageSequenceNumber ());

      if (duplicated == NULL)
	{
	  nonDuplicatedMessages.push_back(messageHeader);
	}
    }

  return nonDuplicatedMessages;
}

void
RoutingProtocol::ProcessNonDuplicatedMessage(const MessageHeader & messageHeader, const Ipv4Address & senderIfaceAddr, const Ipv4Address & receiverIfaceAddr)
{
  switch (messageHeader.GetMessageType ())
    {
      case lqolsr::MessageHeader::LQ_HELLO_MESSAGE:
	  NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
			<< "s OLSR node " << m_mainAddress
			<< " received HELLO message of size " << messageHeader.GetSerializedSize ());

	  ProcessHello (messageHeader, receiverIfaceAddr, senderIfaceAddr);
	  break;

      case lqolsr::MessageHeader::LQ_TC_MESSAGE:
	  NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
			<< "s OLSR node " << m_mainAddress
			<< " received TC message of size " << messageHeader.GetSerializedSize ());

	  ProcessTc (messageHeader, senderIfaceAddr);
	  break;

      case lqolsr::MessageHeader::MID_MESSAGE:
	NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
		      << "s OLSR node " << m_mainAddress
		      <<  " received MID message of size " << messageHeader.GetSerializedSize ());

	ProcessMid (messageHeader, senderIfaceAddr);
	break;
      case lqolsr::MessageHeader::HNA_MESSAGE:
	NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
		      << "s OLSR node " << m_mainAddress
		      <<  " received HNA message of size " << messageHeader.GetSerializedSize ());

	ProcessHna (messageHeader, senderIfaceAddr);
	break;

      default:
	NS_LOG_DEBUG ("OLSR message type " << int (messageHeader.GetMessageType ()) << " not implemented");
    }
}

void
RoutingProtocol::ProcessMessage(const MessageList & messages, const Ipv4Address & senderIfaceAddr, const Ipv4Address & receiverIfaceAddr, const InetSocketAddress & inetSourceAddr)
{
  for (MessageList::const_iterator messageIter = messages.begin (); messageIter != messages.end (); messageIter++)
    {
      const MessageHeader &messageHeader = *messageIter;

      bool do_forwarding = true;
      DuplicateTuple *duplicated = m_state.FindDuplicateTuple(messageHeader.GetOriginatorAddress (), messageHeader.GetMessageSequenceNumber ());

      // If the message has been processed it must not be processed again
      if (duplicated == NULL)
	{
	  ProcessNonDuplicatedMessage(messageHeader, senderIfaceAddr, receiverIfaceAddr);
	}
      else
	{
	  NS_LOG_DEBUG ("OLSR message is duplicated, not reading it.");

	  // If the message has been considered for forwarding, it should
	  // not be retransmitted again
	  for (std::vector<Ipv4Address>::const_iterator it = duplicated->ifaceList.begin ();
	       it != duplicated->ifaceList.end (); it++)
	    {
	      if (*it == receiverIfaceAddr)
		{
		  do_forwarding = false;
		  break;
		}
	    }
	}

      if (do_forwarding)
	{
	  // HELLO messages are never forwarded.
	  // TC and MID messages are forwarded using the default algorithm.
	  // Remaining messages are also forwarded using the default algorithm.
	  if (messageHeader.GetMessageType ()  != lqolsr::MessageHeader::LQ_HELLO_MESSAGE)
	    {
	      ForwardDefault (messageHeader, duplicated, receiverIfaceAddr, inetSourceAddr.GetIpv4 ());
	    }
	}
    }
}

//
// \brief Processes an incoming %OLSR packet following \RFC{3626} specification.
void
RoutingProtocol::RecvOlsr (Ptr<Socket> socket)
{
  Address sourceAddress;
  Ptr<Packet> receivedPacket = socket->RecvFrom (sourceAddress);

  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address senderIfaceAddr = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiverIfaceAddr = m_socketAddresses[socket].GetLocal ();

  NS_ASSERT (receiverIfaceAddr != Ipv4Address ());
  NS_ASSERT (inetSourceAddr.GetPort () == OLSR_PORT_NUMBER);

  lqolsr::PacketHeader olsrPacketHeader;
  receivedPacket->RemoveHeader (olsrPacketHeader);

  NS_LOG_DEBUG ("OLSR node " << m_mainAddress << " received a OLSR packet from " << senderIfaceAddr << " to " << receiverIfaceAddr);
  MessageList messages = ExtractCorrectMessagesFromPacket(receivedPacket, olsrPacketHeader);

  ProcessMessage(messages, senderIfaceAddr, receiverIfaceAddr, inetSourceAddr);

  MessageList correctNonDuplicatedMessages = FilterNonDuplicatedMessages(messages);
  m_metric->NotifyMessageReceived(olsrPacketHeader.GetPacketSequenceNumber(), correctNonDuplicatedMessages, receiverIfaceAddr, senderIfaceAddr);

  RoutingTableComputation ();
}

bool
RoutingProtocol::HasLinkTo(const Ipv4Address & neighbor)
{
  LinkSet links = m_state.GetLinks();

  for (std::vector<LinkTuple>::iterator it = links.begin(); it != links.end(); it++)
    {
      if (it->neighborIfaceAddr == neighbor)
	{
	  return true;
	}
    }

  return false;
}

//This Mpr implementation simply enables all symetric neighbors as Mpr
void
RoutingProtocol::MprComputation ()
{
  NS_LOG_FUNCTION(this);
  NeighborSet neighbors = m_state.GetNeighbors();
  MprSet newMprSet;

  for(NeighborSet::iterator neighbor = neighbors.begin(); neighbor != neighbors.end(); neighbor++)
    {
      if (neighbor->status != NeighborTuple::STATUS_NOT_SYM)
      {
	newMprSet.insert(neighbor->neighborMainAddr);
      }
    }

  m_state.SetMprSet(newMprSet);

  NS_LOG_DEBUG ("Mpr Computation concluded for: " << m_mainAddress);

}

Ipv4Address
RoutingProtocol::GetMainAddress (Ipv4Address iface_addr) const
{
  const IfaceAssocTuple *tuple =
    m_state.FindIfaceAssocTuple (iface_addr);

  if (tuple != NULL)
    {
      return tuple->mainAddr;
    }
  else
    {
      return iface_addr;
    }
}

void
RoutingProtocol::InitializeDestinations()
{
  bool mainAddrSelected = false;

  m_destinations.clear();
  m_costs.clear();

  const NeighborSet &neighborSet = m_state.GetNeighbors ();
  for (NeighborSet::const_iterator it = neighborSet.begin (); it != neighborSet.end (); it++)
    {
      if (it->neighborMainAddr == m_mainAddress)
	{
	  continue;
	}

      if (it->status == NeighborTuple::STATUS_SYM)
	{
	  const LinkSet &linkSet = m_state.GetLinks ();

	  bool added = false;

	  DestinationTuple dest;

	  for (LinkSet::const_iterator it2 = linkSet.begin (); it2 != linkSet.end (); it2++)
	  {
	    if ((GetMainAddress (it2->neighborIfaceAddr) == it->neighborMainAddr) && it2->time >= Simulator::Now ())
	      {
		dest.destAddress = it2->neighborIfaceAddr;
		dest.accessLink = &(*it2);
		m_destinations.push_back(dest);
		m_costs[it2->neighborIfaceAddr] = m_metric->GetCost(it2->neighborIfaceAddr);//it2->cost;
		NS_LOG_DEBUG("Added destination " << dest.destAddress << "with cost = " << m_costs[it2->neighborIfaceAddr]);
		added = true;
	      }

	    if (it2->neighborIfaceAddr == it->neighborMainAddr)
	      {
		mainAddrSelected = true;
	      }
	  }

	  if (added && !mainAddrSelected)
	    {
	      DestinationTuple lastAdded = m_destinations.back();
	      m_costs[it->neighborMainAddr] = m_costs[lastAdded.destAddress];
	      lastAdded.destAddress = it->neighborMainAddr;
	      m_destinations.push_back(lastAdded);

	      NS_LOG_DEBUG("Added destination " << dest.destAddress << "with cost = " << m_costs[lastAdded.destAddress]);
	    }
	}
    }

  const TopologySet &topology = m_state.GetTopologySet ();

  for (TopologySet::const_iterator top = topology.begin ();
      top != topology.end (); top++)
    {
	DestinationTuple dest;
	dest.destAddress = top->destAddr;
	dest.accessLink = NULL;

	bool myself = top->destAddr == m_mainAddress;
	bool alreadyAdded = std::find(m_destinations.begin(), m_destinations.end(), dest) != m_destinations.end();

	if (!myself and !alreadyAdded)
	  {
	    m_destinations.push_back(dest);
	    m_costs[top->destAddr] = m_metric->GetInfinityCostValue();

	    NS_LOG_DEBUG("Added destination " << dest.destAddress << "with cost = " << m_costs[top->destAddr]);
	  }
    }
}

int
RoutingProtocol::GetMinDestination()
{
  int index = 0;
  int selectedIndex = -1;
  float bestCost = m_metric->GetInfinityCostValue();

  for(std::vector<DestinationTuple>::iterator it = m_destinations.begin(); it != m_destinations.end(); it++)
    {
      float costTest = m_costs[it->destAddress];

      if (selectedIndex < 0 || m_metric->CompareBest(costTest, bestCost) > 0)
      {
	  selectedIndex = index;
	  bestCost = costTest;
      }

      index++;
    }

  return selectedIndex;
}

void
RoutingProtocol::UpdateDestinationNeighbors(const DestinationTuple & dest)
{

  const TopologySet &topology = m_state.GetTopologySet ();
  for (TopologySet::const_iterator top = topology.begin ();
      top != topology.end (); top++)
    {
      DestinationTuple destNeighbor;
      destNeighbor.destAddress = top->destAddr;

      std::vector<DestinationTuple>::iterator it = std::find(m_destinations.begin(),
							     m_destinations.end(),
							     destNeighbor);
      if (it == m_destinations.end())
	{
	  continue;
	}

      if (top->lastAddr == dest.destAddress)
	{
	  float currentCost = m_costs[top->destAddr];
	  float newCost = m_metric->Compound(m_costs[dest.destAddress], top->cost);

	  if (m_metric->CompareBest(newCost, currentCost) > 0)
	    {
	      m_costs[top->destAddr] = newCost;
	      it->accessLink = dest.accessLink;
	    }
	}
    }
}

void
RoutingProtocol::RoutingTableComputation ()
{
  double now = Simulator::Now ().GetSeconds ();

  NS_LOG_DEBUG ("Routing Table Computaion at time " << now << " in node " << m_mainAddress);

  Clear();

  //Time now = Simulator::Now ();

  InitializeDestinations();

  bool done = false;

  while (!done)
    {
      int selectedIndex = GetMinDestination();

      if (selectedIndex >= 0)
	{
	  std::vector<DestinationTuple>::iterator selectedIt = m_destinations.begin() + selectedIndex;
	  DestinationTuple selectedDest = *selectedIt;

	  float selectedCost = m_costs[selectedDest.destAddress];
	  int validCost = m_metric->CompareBest(selectedCost, m_metric->GetInfinityCostValue());
	  if (selectedDest.accessLink != NULL && validCost > 0)
	    {
	      AddLqEntry(selectedDest.destAddress, selectedDest.accessLink->neighborIfaceAddr,
			 selectedDest.accessLink->localIfaceAddr,
			 m_costs[selectedDest.destAddress]);

	      m_destinations.erase(selectedIt);

	      UpdateDestinationNeighbors(selectedDest);
	    }
	  else
	    {
	      done = true;
	    }
	}
      else
	{
	  done = true;
	}
    }

    // For each entry in the multiple interface association base
    // where there exists a routing entry such that:
    // R_dest_addr == I_main_addr (of the multiple interface association entry)
    // AND there is no routing entry such that:
    // R_dest_addr == I_iface_addr
    AddInterfaceAssociationsToRoutingTable();

    // For each tuple in the association set,
    // If there is no entry in the routing table with:
    // R_dest_addr     == A_network_addr/A_netmask
    // and if the announced network is not announced by the node itself,
    // then a new routing entry is created.
    CalculateHNARoutingTable();

    NS_LOG_DEBUG ("Node " << m_mainAddress << ": RoutingTableComputation end.");

    if (g_log.IsEnabled(LOG_LEVEL_DEBUG))
      {

	NS_LOG_DEBUG ("Dumping Routing Table");

	Ptr<OutputStreamWrapper> outStream = Create<OutputStreamWrapper>(&std::clog);
	PrintRoutingTable(outStream);
      }

    m_routingTableChanged (GetSize ());
}

void
RoutingProtocol::AddInterfaceAssociationsToRoutingTable()
{
  const IfaceAssocSet &ifaceAssocSet = m_state.GetIfaceAssocSet ();
  for (IfaceAssocSet::const_iterator it = ifaceAssocSet.begin ();
       it != ifaceAssocSet.end (); it++)
    {
      IfaceAssocTuple const &tuple = *it;
      RoutingTableEntry entry1, entry2;
      bool have_entry1 = Lookup (tuple.mainAddr, entry1);
      bool have_entry2 = Lookup (tuple.ifaceAddr, entry2);
      if (have_entry1 && !have_entry2)
	{
	  // then a route entry is created in the routing table with:
	  //       R_dest_addr  =  I_iface_addr (of the multiple interface
	  //                                     association entry)
	  //       R_next_addr  =  R_next_addr  (of the recorded route entry)
	  //       R_dist       =  R_dist       (of the recorded route entry)
	  //       R_iface_addr =  R_iface_addr (of the recorded route entry).
	  AddEntry (tuple.ifaceAddr,
		    entry1.nextAddr,
		    entry1.interface,
		    entry1.distance);
	}
    }
}

Ipv4Address
RoutingProtocol::GetBestHnaGateway()
{
  Ipv4Address gwAddress = Ipv4Address::GetBroadcast();
  float bestCost = m_metric->GetInfinityCostValue();

  const AssociationSet &associationSet = m_state.GetAssociationSet ();

  for (AssociationSet::const_iterator it = associationSet.begin (); it != associationSet.end (); it++)
    {
      AssociationTuple const &tuple = *it;

      RoutingTableEntry gatewayEntry;

      if (Lookup(tuple.gatewayAddr, gatewayEntry) && m_metric->CompareBest(gatewayEntry.cost, bestCost) > 0)
	{
	  gwAddress = tuple.gatewayAddr;
	  bestCost = gatewayEntry.cost;
	}
    }

  return gwAddress;
}


void
RoutingProtocol::CalculateHNARoutingTable()
{
  const AssociationSet &associationSet = m_state.GetAssociationSet ();

  // Clear HNA routing table
  for (uint32_t i = 0; i < m_hnaRoutingTable->GetNRoutes (); i++)
    {
      m_hnaRoutingTable->RemoveRoute (0);
    }

  for (AssociationSet::const_iterator it = associationSet.begin ();
       it != associationSet.end (); it++)
    {
      AssociationTuple const &tuple = *it;

      // Test if HNA associations received from other gateways
      // are also announced by this node. In such a case, no route
      // is created for this association tuple (go to the next one).
      bool goToNextAssociationTuple = false;
      const Associations &localHnaAssociations = m_state.GetAssociations ();
      NS_LOG_DEBUG ("Nb local associations: " << localHnaAssociations.size ());
      for (Associations::const_iterator assocIterator = localHnaAssociations.begin ();
	   assocIterator != localHnaAssociations.end (); assocIterator++)
	{
	  Association const &localHnaAssoc = *assocIterator;
	  if (localHnaAssoc.networkAddr == tuple.networkAddr && localHnaAssoc.netmask == tuple.netmask)
	    {
	      NS_LOG_DEBUG ("HNA association received from another GW is part of local HNA associations: no route added for network "
			    << tuple.networkAddr << "/" << tuple.netmask);
	      goToNextAssociationTuple = true;
	    }
	}
      if (goToNextAssociationTuple)
	{
	  continue;
	}

      RoutingTableEntry gatewayEntry;

      bool gatewayEntryExists = Lookup (tuple.gatewayAddr, gatewayEntry);
      bool addRoute = false;

      uint32_t routeIndex = 0;

      for (routeIndex = 0; routeIndex < m_hnaRoutingTable->GetNRoutes (); routeIndex++)
	{
	  Ipv4RoutingTableEntry route = m_hnaRoutingTable->GetRoute (routeIndex);
	  if (route.GetDestNetwork () == tuple.networkAddr
	      && route.GetDestNetworkMask () == tuple.netmask)
	    {
	      break;
	    }
	}

      if (routeIndex == m_hnaRoutingTable->GetNRoutes ())
	{
	  addRoute = true;
	}
      else //if (gatewayEntryExists && m_hnaRoutingTable->GetMetric (routeIndex) > gatewayEntry.distance)
	{
	  float gwRouteCost = unpack754_32(m_hnaRoutingTable->GetMetric (routeIndex));

	  if (gatewayEntryExists && m_metric->CompareBest(gatewayEntry.cost, gwRouteCost) > 0)
	    {
	      NS_LOG_DEBUG("Removing hna entry because new one's cost (" << gatewayEntry.cost << ") is lower than the old one (" << gwRouteCost << ")");
	      m_hnaRoutingTable->RemoveRoute (routeIndex);
	      addRoute = true;
	    }
	}

      if (addRoute && gatewayEntryExists)
	{
	  uint32_t cost = pack754_32(gatewayEntry.cost);
	  NS_LOG_DEBUG("Hna entry added: dest = " << tuple.networkAddr << ", net mask = " << tuple.netmask << ", gw = " << tuple.gatewayAddr << ", next hop = " << gatewayEntry.nextAddr);
	  m_hnaRoutingTable->AddNetworkRouteTo (tuple.networkAddr,
						tuple.netmask,
						gatewayEntry.nextAddr,
						gatewayEntry.interface,
						cost);
	}
    }
}

void
RoutingProtocol::ProcessHello (const lqolsr::MessageHeader &msg,
                               const Ipv4Address &receiverIface,
                               const Ipv4Address &senderIface)
{
  NS_LOG_FUNCTION (msg << receiverIface << senderIface);


  const lqolsr::MessageHeader::LqHello &hello = msg.GetLqHello();

  LinkSensing (msg, hello, receiverIface, senderIface);

#ifdef NS3_LOG_ENABLE
  {
    const LinkSet &links = m_state.GetLinks ();
    NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                  << "s ** BEGIN dump Link Set for OLSR Node " << m_mainAddress);
    for (LinkSet::const_iterator link = links.begin (); link != links.end (); link++)
      {
        NS_LOG_DEBUG (*link);
      }
    NS_LOG_DEBUG ("** END dump Link Set for OLSR Node " << m_mainAddress);

    const NeighborSet &neighbors = m_state.GetNeighbors ();
    NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                  << "s ** BEGIN dump Neighbor Set for OLSR Node " << m_mainAddress);
    for (NeighborSet::const_iterator neighbor = neighbors.begin (); neighbor != neighbors.end (); neighbor++)
      {
        NS_LOG_DEBUG (*neighbor);
      }
    NS_LOG_DEBUG ("** END dump Neighbor Set for OLSR Node " << m_mainAddress);
  }
#endif // NS3_LOG_ENABLE

  PopulateNeighborSet (msg, hello);
  PopulateTwoHopNeighborSetLq(msg, hello);
  MprComputation ();
  PopulateMprSelectorSet (msg, hello);
}

void
RoutingProtocol::CreateTopologyTuple(TopologyTuple & tuple, const Ipv4Address & destAddress,
				     const Ipv4Address & lastAddress, uint16_t seqNumber, Time expirationTime, float cost)
{
  tuple.destAddr = destAddress;
  tuple.lastAddr = lastAddress;
  tuple.sequenceNumber = seqNumber;
  tuple.expirationTime = expirationTime;
  tuple.cost = cost;
}

void
RoutingProtocol::ProcessTc (const lqolsr::MessageHeader &msg,
                            const Ipv4Address &senderIface)
{
  const lqolsr::MessageHeader::LqTc &tc = msg.GetLqTc ();
  Time now = Simulator::Now ();

  // 1. If the sender interface of this message is not in the symmetric
  // 1-hop neighborhood of this node, the message MUST be discarded.
  const LinkTuple *link_tuple = m_state.FindSymLinkTuple (senderIface, now);
  if (link_tuple == NULL)
    {
      return;
    }

  // 2. If there exist some tuple in the topology set where:
  //    T_last_addr == originator address AND
  //    T_seq       >  ANSN,
  // then further processing of this TC message MUST NOT be
  // performed.
  const TopologyTuple *topologyTuple = m_state.FindNewerTopologyTuple (msg.GetOriginatorAddress (), tc.ansn);

  if (topologyTuple != NULL)
    {
      return;
    }

  // 3. All tuples in the topology set where:
  //    T_last_addr == originator address AND
  //    T_seq       <  ANSN
  // MUST be removed from the topology set.
  m_state.EraseOlderTopologyTuples (msg.GetOriginatorAddress (), tc.ansn);

  // 4. For each of the advertised neighbor interface information received in the TC message:

  for (std::vector<MessageHeader::NeighborInterfaceInfo>::const_iterator info = tc.neighborAddresses.begin();
      info != tc.neighborAddresses.end(); info++)
    {
      const Ipv4Address &addr = info->neighborInterfaceAddress;

      // 4.1. If there exist some tuple in the topology set where:
      //      T_dest_addr == advertised neighbor main address, AND
      //      T_last_addr == originator address,
      // then the holding time of that tuple MUST be set to:
      //      T_time      =  current time + validity time.

      TopologyTuple *topologyTuple = m_state.FindTopologyTuple (addr, msg.GetOriginatorAddress());

      float cost = unpack754_32(info->metricInfo);

      if (topologyTuple != NULL)
	{
	  topologyTuple->expirationTime = msg.GetVTime() + now;
	  topologyTuple->cost = cost;
	}
      else
	{
	  // 4.2. Otherwise, a new tuple MUST be recorded in the topology
	  // set where:
	  //      T_dest_addr = advertised neighbor main address,
	  //      T_last_addr = originator address,
	  //      T_seq       = ANSN,
	  //      T_time      = current time + validity time.

	  TopologyTuple topologyTuple;

	  CreateTopologyTuple(topologyTuple, addr, msg.GetOriginatorAddress(), tc.ansn, msg.GetVTime() + now, cost);

	  AddTopologyTuple (topologyTuple);

	  // Schedules topology tuple deletion
	  m_events.Track (Simulator::Schedule (DELAY (topologyTuple.expirationTime),
					       &RoutingProtocol::TopologyTupleTimerExpire,
					       this,
					       topologyTuple.destAddr,
					       topologyTuple.lastAddr));
	}
    }

  #ifdef NS3_LOG_ENABLE
    {
      const TopologySet &topology = m_state.GetTopologySet ();
      NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
		    << "s ** BEGIN dump TopologySet for OLSR Node " << m_mainAddress);
      for (TopologySet::const_iterator tuple = topology.begin ();
	   tuple != topology.end (); tuple++)
	{
	  NS_LOG_DEBUG (*tuple);
	}
      NS_LOG_DEBUG ("** END dump TopologySet Set for OLSR Node " << m_mainAddress);
    }
  #endif // NS3_LOG_ENABLE
}

void
RoutingProtocol::ProcessMid (const lqolsr::MessageHeader &msg,
                             const Ipv4Address &senderIface)
{
  const lqolsr::MessageHeader::Mid &mid = msg.GetMid ();
  Time now = Simulator::Now ();

  NS_LOG_DEBUG ("Node " << m_mainAddress << " ProcessMid from " << senderIface);
  // 1. If the sender interface of this message is not in the symmetric
  // 1-hop neighborhood of this node, the message MUST be discarded.
  const LinkTuple *linkTuple = m_state.FindSymLinkTuple (senderIface, now);
  if (linkTuple == NULL)
    {
      NS_LOG_LOGIC ("Node " << m_mainAddress <<
                    ": the sender interface of this message is not in the "
                    "symmetric 1-hop neighborhood of this node,"
                    " the message MUST be discarded.");
      return;
    }

  // 2. For each interface address listed in the MID message
  for (std::vector<Ipv4Address>::const_iterator i = mid.interfaceAddresses.begin ();
       i != mid.interfaceAddresses.end (); i++)
    {
      bool updated = false;
      IfaceAssocSet &ifaceAssoc = m_state.GetIfaceAssocSetMutable ();
      for (IfaceAssocSet::iterator tuple = ifaceAssoc.begin ();
           tuple != ifaceAssoc.end (); tuple++)
        {
          if (tuple->ifaceAddr == *i
              && tuple->mainAddr == msg.GetOriginatorAddress ())
            {
              NS_LOG_LOGIC ("IfaceAssoc updated: " << *tuple);
              tuple->time = now + msg.GetVTime ();
              updated = true;
            }
        }
      if (!updated)
        {
          IfaceAssocTuple tuple;
          tuple.ifaceAddr = *i;
          tuple.mainAddr = msg.GetOriginatorAddress ();
          tuple.time = now + msg.GetVTime ();
          AddIfaceAssocTuple (tuple);
          NS_LOG_LOGIC ("New IfaceAssoc added: " << tuple);
          // Schedules iface association tuple deletion
          Simulator::Schedule (DELAY (tuple.time),
                               &RoutingProtocol::IfaceAssocTupleTimerExpire, this, tuple.ifaceAddr);
        }
    }

  // 3. (not part of the RFC) iterate over all NeighborTuple's and
  // TwoHopNeighborTuples, update the neighbor addresses taking into account
  // the new MID information.
  NeighborSet &neighbors = m_state.GetNeighbors ();
  for (NeighborSet::iterator neighbor = neighbors.begin (); neighbor != neighbors.end (); neighbor++)
    {
      neighbor->neighborMainAddr = GetMainAddress (neighbor->neighborMainAddr);
    }

  TwoHopNeighborSet &twoHopNeighbors = m_state.GetTwoHopNeighbors ();
  for (TwoHopNeighborSet::iterator twoHopNeighbor = twoHopNeighbors.begin ();
       twoHopNeighbor != twoHopNeighbors.end (); twoHopNeighbor++)
    {
      twoHopNeighbor->neighborMainAddr = GetMainAddress (twoHopNeighbor->neighborMainAddr);
      twoHopNeighbor->twoHopNeighborAddr = GetMainAddress (twoHopNeighbor->twoHopNeighborAddr);
    }
  NS_LOG_DEBUG ("Node " << m_mainAddress << " ProcessMid from " << senderIface << " -> END.");
}

void
RoutingProtocol::ProcessHna (const lqolsr::MessageHeader &msg,
                             const Ipv4Address &senderIface)
{
  NS_LOG_DEBUG("Received HNA message from " << msg.GetOriginatorAddress());
  const lqolsr::MessageHeader::Hna &hna = msg.GetHna ();
  Time now = Simulator::Now ();

  // 1. If the sender interface of this message is not in the symmetric
  // 1-hop neighborhood of this node, the message MUST be discarded.
  const LinkTuple *link_tuple = m_state.FindSymLinkTuple (senderIface, now);
  if (link_tuple == NULL)
    {
      return;
    }

  // 2. Otherwise, for each (network address, netmask) pair in the
  // message:

  for (std::vector<lqolsr::MessageHeader::Hna::Association>::const_iterator it = hna.associations.begin ();
       it != hna.associations.end (); it++)
    {
      AssociationTuple *tuple = m_state.FindAssociationTuple (msg.GetOriginatorAddress (),it->address,it->mask);

      // 2.1  if an entry in the association set already exists, where:
      //          A_gateway_addr == originator address
      //          A_network_addr == network address
      //          A_netmask      == netmask
      //      then the holding time for that tuple MUST be set to:
      //          A_time         =  current time + validity time
      if (tuple != NULL)
        {
          tuple->expirationTime = now + msg.GetVTime ();
        }

      // 2.2 otherwise, a new tuple MUST be recorded with:
      //          A_gateway_addr =  originator address
      //          A_network_addr =  network address
      //          A_netmask      =  netmask
      //          A_time         =  current time + validity time
      else
        {
          AssociationTuple assocTuple = {
            msg.GetOriginatorAddress (),
            it->address,
            it->mask,
            now + msg.GetVTime ()
          };
          AddAssociationTuple (assocTuple);

          //Schedule Association Tuple deletion
          Simulator::Schedule (DELAY (assocTuple.expirationTime),
                               &RoutingProtocol::AssociationTupleTimerExpire, this,
                               assocTuple.gatewayAddr,assocTuple.networkAddr,assocTuple.netmask);
        }

    }
}

void
RoutingProtocol::ForwardDefault (lqolsr::MessageHeader olsrMessage,
                                 DuplicateTuple *duplicated,
                                 const Ipv4Address &localIface,
                                 const Ipv4Address &senderAddress)
{
  Time now = Simulator::Now ();

  // If the sender interface address is not in the symmetric
  // 1-hop neighborhood the message must not be forwarded
  const LinkTuple *linkTuple = m_state.FindSymLinkTuple (senderAddress, now);
  if (linkTuple == NULL)
    {
      return;
    }

  // If the message has already been considered for forwarding,
  // it must not be retransmitted again
  if (duplicated != NULL && duplicated->retransmitted)
    {
      NS_LOG_LOGIC (Simulator::Now () << "Node " << m_mainAddress << " does not forward a message received"
                    " from " << olsrMessage.GetOriginatorAddress () << " because it is duplicated");
      return;
    }

  // If the sender interface address is an interface address
  // of a MPR selector of this node and ttl is greater than 1,
  // the message must be retransmitted
  bool retransmitted = false;
  if (olsrMessage.GetTimeToLive () > 1)
    {
      const MprSelectorTuple *mprselTuple =
        m_state.FindMprSelectorTuple (GetMainAddress (senderAddress));
      if (mprselTuple != NULL)
        {
          olsrMessage.SetTimeToLive (olsrMessage.GetTimeToLive () - 1);
          olsrMessage.SetHopCount (olsrMessage.GetHopCount () + 1);
          // We have to introduce a random delay to avoid
          // synchronization with neighbors.
          QueueMessage (olsrMessage, JITTER);
          retransmitted = true;
        }
    }

  // Update duplicate tuple...
  if (duplicated != NULL)
    {
      duplicated->expirationTime = now + OLSR_DUP_HOLD_TIME;
      duplicated->retransmitted = retransmitted;
      duplicated->ifaceList.push_back (localIface);
    }
  // ...or create a new one
  else
    {
      DuplicateTuple newDup;
      newDup.address = olsrMessage.GetOriginatorAddress ();
      newDup.sequenceNumber = olsrMessage.GetMessageSequenceNumber ();
      newDup.expirationTime = now + OLSR_DUP_HOLD_TIME;
      newDup.retransmitted = retransmitted;
      newDup.ifaceList.push_back (localIface);
      AddDuplicateTuple (newDup);
      // Schedule dup tuple deletion
      Simulator::Schedule (OLSR_DUP_HOLD_TIME,
                           &RoutingProtocol::DupTupleTimerExpire, this,
                           newDup.address, newDup.sequenceNumber);
    }
}

void
RoutingProtocol::QueueMessage (const lqolsr::MessageHeader &message, Time delay)
{
  m_queuedMessages.push_back (message);
  if (not m_queuedMessagesTimer.IsRunning ())
    {
      m_queuedMessagesTimer.SetDelay (delay);
      m_queuedMessagesTimer.Schedule ();
    }
}

void
RoutingProtocol::SendPacket (Ptr<Packet> packet,
                             const MessageList &containedMessages)
{
  NS_LOG_DEBUG ("OLSR node " << m_mainAddress << " sending a OLSR packet");

  // Add a header
  lqolsr::PacketHeader header;
  header.SetPacketLength (header.GetSerializedSize () + packet->GetSize ());
  header.SetPacketSequenceNumber (GetPacketSequenceNumber ());
  packet->AddHeader (header);

  // Trace it
  m_txPacketTrace (header, containedMessages);

  // Send it
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
         m_socketAddresses.begin (); i != m_socketAddresses.end (); i++)
    {
      Ipv4Address bcast = i->second.GetLocal ().GetSubnetDirectedBroadcast (i->second.GetMask ());
      i->first->SendTo (packet, 0, InetSocketAddress (bcast, OLSR_PORT_NUMBER));
    }
}

void
RoutingProtocol::SendQueuedMessages ()
{
  Ptr<Packet> packet = Create<Packet> ();
  int numMessages = 0;

  NS_LOG_DEBUG ("Olsr node " << m_mainAddress << ": SendQueuedMessages");

  MessageList msglist;

  for (std::vector<lqolsr::MessageHeader>::const_iterator message = m_queuedMessages.begin ();
       message != m_queuedMessages.end ();
       message++)
    {
      Ptr<Packet> p = Create<Packet> ();
      p->AddHeader (*message);
      packet->AddAtEnd (p);
      msglist.push_back (*message);
      if (++numMessages == OLSR_MAX_MSGS)
        {
          SendPacket (packet, msglist);
          msglist.clear ();
          // Reset variables for next packet
          numMessages = 0;
          packet = Create<Packet> ();
        }
    }

  if (packet->GetSize ())
    {
      SendPacket (packet, msglist);
    }

  m_queuedMessages.clear ();
}

lqolsr::MessageHeader
RoutingProtocol::CreateNewMessage(Time vTime, uint8_t timeToLive)
{
  lqolsr::MessageHeader msg;

  msg.SetVTime (vTime);
  msg.SetOriginatorAddress (m_mainAddress);
  msg.SetTimeToLive (timeToLive);
  msg.SetHopCount (0);
  msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());

  return msg;
}

uint8_t
RoutingProtocol::GetLinkCode(const LinkTuple & link_tuple, Time now)
{
  if (!(GetMainAddress (link_tuple.localIfaceAddr) == m_mainAddress
	&& link_tuple.time >= now))
    {
      return 0;
    }

  uint8_t link_type, nb_type = 0xff;

  // Establishes link type
  if (link_tuple.symTime >= now)
    {
      link_type = OLSR_SYM_LINK;
    }
  else if (link_tuple.asymTime >= now)
    {
      link_type = OLSR_ASYM_LINK;
    }
  else
    {
      link_type = OLSR_LOST_LINK;
    }
  // Establishes neighbor type.
  if (m_state.FindMprAddress (GetMainAddress (link_tuple.neighborIfaceAddr)))
    {
      nb_type = OLSR_MPR_NEIGH;
      NS_LOG_DEBUG ("I consider neighbor " << GetMainAddress (link_tuple.neighborIfaceAddr)
		    << " to be MPR_NEIGH.");
    }
  else
    {
      bool ok = false;
      for (NeighborSet::const_iterator nb_tuple = m_state.GetNeighbors ().begin ();
	   nb_tuple != m_state.GetNeighbors ().end ();
	   nb_tuple++)
	{
	  if (nb_tuple->neighborMainAddr == GetMainAddress (link_tuple.neighborIfaceAddr))
	    {
	      if (nb_tuple->status == NeighborTuple::STATUS_SYM)
		{
		  NS_LOG_DEBUG ("I consider neighbor " << GetMainAddress (link_tuple.neighborIfaceAddr)
				<< " to be SYM_NEIGH.");
		  nb_type = OLSR_SYM_NEIGH;
		}
	      else if (nb_tuple->status == NeighborTuple::STATUS_NOT_SYM)
		{
		  nb_type = OLSR_NOT_NEIGH;
		  NS_LOG_DEBUG ("I consider neighbor " << GetMainAddress (link_tuple.neighborIfaceAddr)
				<< " to be NOT_NEIGH.");
		}
	      else
		{
		  NS_FATAL_ERROR ("There is a neighbor tuple with an unknown status!\n");
		}
	      ok = true;
	      break;
	    }
	}
      if (!ok)
	{
	  NS_LOG_WARN ("I don't know the neighbor " << GetMainAddress (link_tuple.neighborIfaceAddr) << "!!!");
	  return 0;
	}
    }

  return (link_type & 0x03) | ((nb_type << 2) & 0x0f);
}

uint32_t
RoutingProtocol::GetHelloInfoToSendHello(Ipv4Address neiAddress)
{
  return m_metric->GetHelloInfo(neiAddress);
}

void
RoutingProtocol::SendHello ()
{
  NS_LOG_FUNCTION (this);

  Time now = Simulator::Now ();

  lqolsr::MessageHeader msg = CreateNewMessage(OLSR_NEIGHB_HOLD_TIME, 1);

  lqolsr::MessageHeader::LqHello &lqhello = msg.GetLqHello ();

  lqhello.SetHTime (m_helloInterval);
  lqhello.willingness = m_willingness;

  std::vector<lqolsr::MessageHeader::LqHello::LinkMessage> &linkMessages = lqhello.linkMessages;

  const LinkSet &links = m_state.GetLinks ();
  for (LinkSet::const_iterator link_tuple = links.begin ();
       link_tuple != links.end (); link_tuple++)
    {
      lqolsr::MessageHeader::LqHello::LinkMessage linkMessage;
      uint8_t linkCode = GetLinkCode((*link_tuple), now);

      if (linkCode == 0)
      {
	continue;
      }

      linkMessage.linkCode = linkCode;

      lqolsr::MessageHeader::NeighborInterfaceInfo neigh_info;
      neigh_info.neighborInterfaceAddress = link_tuple->neighborIfaceAddr;
      neigh_info.metricInfo = GetHelloInfoToSendHello(link_tuple->neighborIfaceAddr);

      linkMessage.neighborInterfaceInformation.push_back(neigh_info);

      std::vector<Ipv4Address> interfaces = m_state.FindNeighborInterfaces (link_tuple->neighborIfaceAddr);

      std::vector< lqolsr::MessageHeader::NeighborInterfaceInfo> neighInfos;

      for (std::vector<Ipv4Address>::iterator it = interfaces.begin(); it != interfaces.end();
	  it++)
      {
	neigh_info.neighborInterfaceAddress = (*it);
	neigh_info.metricInfo = m_metric->GetHelloInfo((*it));

	neighInfos.push_back(neigh_info);
      }

      linkMessage.neighborInterfaceInformation.insert(linkMessage.neighborInterfaceInformation.end(),
						    neighInfos.begin (), neighInfos.end ());
      linkMessages.push_back (linkMessage);
    }

    NS_LOG_DEBUG ("Prepare to send OLSR LQ_HELLO message size: " << int (msg.GetSerializedSize ())
                                                << " (with " << int (linkMessages.size ()) << " link messages)");
    QueueMessage (msg, JITTER);
}

float
RoutingProtocol::GetCostToTcSend(LinkTuple *link_tuple)
{
  RoutingTableEntry outEntry;
  float cost;
  if(Lookup (link_tuple->neighborIfaceAddr, outEntry))
    {
      cost = outEntry.cost;
    }
  else
    {
      cost = m_metric->GetCost(link_tuple->neighborIfaceAddr);
    }

  return cost;
}

void
RoutingProtocol::SendTc ()
{
  NS_LOG_FUNCTION (this);

  lqolsr::MessageHeader msg = CreateNewMessage(OLSR_TOP_HOLD_TIME, 255);

  lqolsr::MessageHeader::LqTc &lqtc = msg.GetLqTc ();
  lqtc.ansn = m_ansn;

  NS_LOG_DEBUG("Sending TC. Seq Number = " << msg.GetMessageSequenceNumber());

  for (MprSelectorSet::const_iterator mprsel_tuple = m_state.GetMprSelectors ().begin ();
       mprsel_tuple != m_state.GetMprSelectors ().end (); mprsel_tuple++)
    {
      lqolsr::MessageHeader::NeighborInterfaceInfo neigh_info;
      neigh_info.neighborInterfaceAddress = mprsel_tuple->mainAddr;

      LinkTuple *link_tuple = m_state.FindLinkTuple ( mprsel_tuple->mainAddr);

      if (link_tuple != NULL)
	{
	  float cost = GetCostToTcSend(link_tuple);

	  neigh_info.metricInfo = pack754_32(cost);//link_tuple->cost);
	  lqtc.neighborAddresses.push_back (neigh_info);
	  NS_LOG_DEBUG("Neighbor " << neigh_info.neighborInterfaceAddress << " Cost = " << cost);
	}
    }
    QueueMessage (msg, JITTER);
}

void
RoutingProtocol::SendMid ()
{
  lqolsr::MessageHeader msg;
  lqolsr::MessageHeader::Mid &mid = msg.GetMid ();

  // A node which has only a single interface address participating in
  // the MANET (i.e., running OLSR), MUST NOT generate any MID
  // message.

  // A node with several interfaces, where only one is participating
  // in the MANET and running OLSR (e.g., a node is connected to a
  // wired network as well as to a MANET) MUST NOT generate any MID
  // messages.

  // A node with several interfaces, where more than one is
  // participating in the MANET and running OLSR MUST generate MID
  // messages as specified.

  // [ Note: assuming here that all interfaces participate in the
  // MANET; later we may want to make this configurable. ]

  Ipv4Address loopback ("127.0.0.1");
  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
    {
      Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
      if (addr != m_mainAddress && addr != loopback && m_interfaceExclusions.find (i) == m_interfaceExclusions.end ())
        {
	  NS_LOG_DEBUG("Mid address: " << addr << "Main address: " << m_mainAddress);
          mid.interfaceAddresses.push_back (addr);
        }
    }
  if (mid.interfaceAddresses.size () == 0)
    {
      NS_LOG_DEBUG ("Not sending any MID, node has only one olsr interface.");
      return;
    }

  msg.SetVTime (OLSR_MID_HOLD_TIME);
  msg.SetOriginatorAddress (m_mainAddress);
  msg.SetTimeToLive (255);
  msg.SetHopCount (0);
  msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());

  QueueMessage (msg, JITTER);
}

void
RoutingProtocol::SendHna ()
{

  lqolsr::MessageHeader msg;

  msg.SetVTime (OLSR_HNA_HOLD_TIME);
  msg.SetOriginatorAddress (m_mainAddress);
  msg.SetTimeToLive (255);
  msg.SetHopCount (0);
  msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
  lqolsr::MessageHeader::Hna &hna = msg.GetHna ();

  std::vector<lqolsr::MessageHeader::Hna::Association> &associations = hna.associations;

  // Add all local HNA associations to the HNA message
  const Associations &localHnaAssociations = m_state.GetAssociations ();
  for (Associations::const_iterator it = localHnaAssociations.begin ();
       it != localHnaAssociations.end (); it++)
    {
      lqolsr::MessageHeader::Hna::Association assoc = { it->networkAddr, it->netmask};
      associations.push_back (assoc);
    }
  // If there is no HNA associations to send, return without queuing the message
  if (associations.size () == 0)
    {
      return;
    }

  // Else, queue the message to be sent later on
  NS_LOG_DEBUG("Sending HNA message(seqNumber = " << msg.GetMessageSequenceNumber());
  QueueMessage (msg, JITTER);
}

void
RoutingProtocol::AddHostNetworkAssociation (Ipv4Address networkAddr, Ipv4Mask netmask)
{
  // Check if the (networkAddr, netmask) tuple already exist
  // in the list of local HNA associations
  const Associations &localHnaAssociations = m_state.GetAssociations ();
  for (Associations::const_iterator assocIterator = localHnaAssociations.begin ();
       assocIterator != localHnaAssociations.end (); assocIterator++)
    {
      Association const &localHnaAssoc = *assocIterator;
      if (localHnaAssoc.networkAddr == networkAddr && localHnaAssoc.netmask == netmask)
        {
          NS_LOG_INFO ("HNA association for network " << networkAddr << "/" << netmask << " already exists.");
          return;
        }
    }
  // If the tuple does not already exist, add it to the list of local HNA associations.
  NS_LOG_INFO ("Adding HNA association for network " << networkAddr << "/" << netmask << ".");
  m_state.InsertAssociation ( (Association) { networkAddr, netmask} );
}

void
RoutingProtocol::RemoveHostNetworkAssociation (Ipv4Address networkAddr, Ipv4Mask netmask)
{
  NS_LOG_INFO ("Removing HNA association for network " << networkAddr << "/" << netmask << ".");
  m_state.EraseAssociation ( (Association) { networkAddr, netmask} );
}

void
RoutingProtocol::SetRoutingTableAssociation (Ptr<Ipv4StaticRouting> routingTable)
{
  // If a routing table has already been associated, remove
  // corresponding entries from the list of local HNA associations
  if (m_routingTableAssociation != 0)
    {
      NS_LOG_INFO ("Removing HNA entries coming from the old routing table association.");
      for (uint32_t i = 0; i < m_routingTableAssociation->GetNRoutes (); i++)
        {
          Ipv4RoutingTableEntry route = m_routingTableAssociation->GetRoute (i);
          // If the outgoing interface for this route is a non-olsr interface
          if (UsesNonOlsrOutgoingInterface (route))
            {
              // remove the corresponding entry
              RemoveHostNetworkAssociation (route.GetDestNetwork (), route.GetDestNetworkMask ());
            }
        }
    }

  // Sets the routingTableAssociation to its new value
  m_routingTableAssociation = routingTable;

  // Iterate over entries of the associated routing table and
  // add the routes using non-olsr outgoing interfaces to the list
  // of local HNA associations
  NS_LOG_DEBUG ("Nb local associations before adding some entries from"
                " the associated routing table: " << m_state.GetAssociations ().size ());
  for (uint32_t i = 0; i < m_routingTableAssociation->GetNRoutes (); i++)
    {
      Ipv4RoutingTableEntry route = m_routingTableAssociation->GetRoute (i);
      Ipv4Address destNetworkAddress = route.GetDestNetwork ();
      Ipv4Mask destNetmask = route.GetDestNetworkMask ();

      // If the outgoing interface for this route is a non-olsr interface,
      if (UsesNonOlsrOutgoingInterface (route))
        {
          // Add this entry's network address and netmask to the list of local HNA entries
          AddHostNetworkAssociation (destNetworkAddress, destNetmask);
        }
    }
  NS_LOG_DEBUG ("Nb local associations after having added some entries from "
                "the associated routing table: " << m_state.GetAssociations ().size ());
}

bool
RoutingProtocol::UsesNonOlsrOutgoingInterface (const Ipv4RoutingTableEntry &route)
{
  std::set<uint32_t>::const_iterator ci = m_interfaceExclusions.find (route.GetInterface ());
  // The outgoing interface is a non-OLSR interface if a match is found
  // before reaching the end of the list of excluded interfaces
  return ci != m_interfaceExclusions.end ();
}

void
RoutingProtocol::CreateNewLinkTuple(LinkTuple &newLinkTuple, const Ipv4Address &senderIface, const Ipv4Address &receiverIface, Time now, Time vtime)
{
  newLinkTuple.neighborIfaceAddr = senderIface;
  newLinkTuple.localIfaceAddr = receiverIface;
  newLinkTuple.symTime = now - Seconds (1);
  newLinkTuple.time = now + vtime;
  //newLinkTuple.cost = m_metric->GetInfinityCostValue();
}

void
RoutingProtocol::LogLinkSensing(int linkType, int neighborType)
{
  #ifdef NS3_LOG_ENABLE
	const char *linkTypeName;
	switch (linkType)
	  {
	  case OLSR_UNSPEC_LINK:
	    linkTypeName = "UNSPEC_LINK";
	    break;
	  case OLSR_ASYM_LINK:
	    linkTypeName = "ASYM_LINK";
	    break;
	  case OLSR_SYM_LINK:
	    linkTypeName = "SYM_LINK";
	    break;
	  case OLSR_LOST_LINK:
	    linkTypeName = "LOST_LINK";
	    break;
	    /*  no default, since lt must be in 0..3, covered above
	  default: linkTypeName = "(invalid value!)";
	    */
	  }

	const char *neighborTypeName;
	switch (neighborType)
	  {
	  case OLSR_NOT_NEIGH:
	    neighborTypeName = "NOT_NEIGH";
	    break;
	  case OLSR_SYM_NEIGH:
	    neighborTypeName = "SYM_NEIGH";
	    break;
	  case OLSR_MPR_NEIGH:
	    neighborTypeName = "MPR_NEIGH";
	    break;
	  default:
	    neighborTypeName = "(invalid value!)";
	  }

	NS_LOG_DEBUG ("Looking at HELLO link messages with Link Type "
		      << linkType << " (" << linkTypeName
		      << ") and Neighbor Type " << neighborType
		      << " (" << neighborTypeName << ")");
  #endif // NS3_LOG_ENABLE
}

bool
RoutingProtocol::ProcessLqHelloLinkMessages(LinkTuple *link_tuple,  const lqolsr::MessageHeader::LqHello &lqhello,
				     const Ipv4Address &receiverIface, Time now, Time vTime)
{
  bool updated = false;

  for (std::vector<lqolsr::MessageHeader::LqHello::LinkMessage>::const_iterator linkMessage =
      lqhello.linkMessages.begin ();
         linkMessage != lqhello.linkMessages.end ();
         linkMessage++)
      {
        int lt = linkMessage->linkCode & 0x03; // Link Type
        int nt = (linkMessage->linkCode >> 2) & 0x03; // Neighbor Type

        LogLinkSensing(lt, nt);

        // We must not process invalid advertised links
        if ((lt == OLSR_SYM_LINK && nt == OLSR_NOT_NEIGH)
            || (nt != OLSR_SYM_NEIGH && nt != OLSR_MPR_NEIGH
                && nt != OLSR_NOT_NEIGH))
          {
            NS_LOG_LOGIC ("HELLO link code is invalid => IGNORING");
            //link_tuple->cost = m_metric->GetInfinityCostValue();
            continue;
          }

        for (std::vector<lqolsr::MessageHeader::NeighborInterfaceInfo>::const_iterator neighIfaceInfo =
               linkMessage->neighborInterfaceInformation.begin ();
            neighIfaceInfo != linkMessage->neighborInterfaceInformation.end ();
            neighIfaceInfo++)
          {
            NS_LOG_DEBUG ("   -> Neighbor: " << neighIfaceInfo->neighborInterfaceAddress);

            if (neighIfaceInfo->neighborInterfaceAddress == receiverIface)
              {
                if (lt == OLSR_LOST_LINK)
                  {
                    NS_LOG_LOGIC ("link is LOST => expiring it");
                    link_tuple->symTime = now - Seconds (1);
                    //link_tuple->cost = m_metric->GetInfinityCostValue();
                    updated = true;
                  }
                else if (lt == OLSR_SYM_LINK || lt == OLSR_ASYM_LINK)
                  {
                    NS_LOG_DEBUG (*link_tuple << ": link is SYM or ASYM => should become SYM now"
                                  " (symTime being increased to " << now + vTime);
                    link_tuple->symTime = now + vTime;
                    link_tuple->time = link_tuple->symTime + OLSR_NEIGHB_HOLD_TIME;
                    //link_tuple->cost = m_metric->GetCost(link_tuple->neighborIfaceAddr);
                    updated = true;
                  }
                else
                  {
                    NS_FATAL_ERROR ("bad link type");
                    //link_tuple->cost = m_metric->GetInfinityCostValue();
                  }
                break;
              }
            else
              {
                NS_LOG_DEBUG ("     \\-> *neighIfaceAddr (" << neighIfaceInfo->neighborInterfaceAddress
                                                            << " != receiverIface (" << receiverIface << ") => IGNORING!");
              }
          }
        NS_LOG_DEBUG ("Link tuple updated: " << int (updated));
      }
  return updated;
}

void
RoutingProtocol::LinkSensing (const lqolsr::MessageHeader &msg,
                              const lqolsr::MessageHeader::LqHello &hello,
                              const Ipv4Address &receiverIface,
                              const Ipv4Address &senderIface)
{
  Time now = Simulator::Now ();
  bool updated = false;
  bool created = false;
  NS_LOG_DEBUG ("@" << now.GetSeconds () << ": Olsr node " << m_mainAddress
                    << ": LinkSensing(receiverIface=" << receiverIface
                    << ", senderIface=" << senderIface << ") BEGIN");

  NS_ASSERT (msg.GetVTime () > Seconds (0));
  LinkTuple *link_tuple = m_state.FindLinkTuple (senderIface);
  if (link_tuple == NULL)
    {
      LinkTuple newLinkTuple;
      CreateNewLinkTuple(newLinkTuple, senderIface, receiverIface, now, msg.GetVTime ());
      link_tuple = &m_state.InsertLinkTuple (newLinkTuple);
      created = true;
      NS_LOG_LOGIC ("Existing link tuple did not exist => creating new one");
    }
  else
    {
      NS_LOG_LOGIC ("Existing link tuple already exists => will update it");
      updated = true;
    }

  link_tuple->asymTime = now + msg.GetVTime ();

  updated = ProcessLqHelloLinkMessages(link_tuple, hello , receiverIface, now,  msg.GetVTime ());


  link_tuple->time = std::max (link_tuple->time, link_tuple->asymTime);

  if (updated)
    {
      LinkTupleUpdated (*link_tuple, hello.willingness);
    }

  // Schedules link tuple deletion
  if (created)
    {
      LinkTupleAdded (*link_tuple, hello.willingness);
      Time t = DELAY (std::min (link_tuple->time, link_tuple->symTime));
      NS_LOG_DEBUG("Expiration shcedule delay = " << t.GetSeconds());
      m_events.Track (Simulator::Schedule (t,
                                           &RoutingProtocol::LinkTupleTimerExpire, this,
                                           link_tuple->neighborIfaceAddr));
    }
  NS_LOG_DEBUG ("@" << now.GetSeconds () << ": Olsr node " << m_mainAddress
                    << ": LinkSensing END");
}

void
RoutingProtocol::PopulateNeighborSet (const lqolsr::MessageHeader &msg,
                                      const lqolsr::MessageHeader::LqHello &hello)
{
  NeighborTuple *nb_tuple = m_state.FindNeighborTuple (msg.GetOriginatorAddress ());
  if (nb_tuple != NULL)
    {
      nb_tuple->willingness = hello.willingness;
    }
}

void
RoutingProtocol::LogPopulateTwoHopNeighborSet(int neighborType)
{
  #ifdef NS3_LOG_ENABLE
    const char *neighborTypeNames[3] = { "NOT_NEIGH", "SYM_NEIGH", "MPR_NEIGH" };
    const char *neighborTypeName = ((neighborType < 3) ?
				    neighborTypeNames[neighborType]
				    : "(invalid value)");
    NS_LOG_DEBUG ("Looking at Link Message from HELLO message: neighborType="
		  << neighborType << " (" << neighborTypeName << ")");
  #endif // NS3_LOG_ENABLE
}

void
RoutingProtocol::PopulateTwoHopNeighborSetLq(const lqolsr::MessageHeader &msg,
					     const lqolsr::MessageHeader::LqHello &lqhello)
{
  //In this implementation we do not use the two-hop neighbors for link-quality metric
}

void
RoutingProtocol::PopulateMprSelectorSet (const lqolsr::MessageHeader &msg,
                                         const lqolsr::MessageHeader::LqHello &hello)
{
  NS_LOG_FUNCTION (this);

  Time now = Simulator::Now ();

  typedef std::vector<lqolsr::MessageHeader::LqHello::LinkMessage> LinkMessageVec;
  for (LinkMessageVec::const_iterator linkMessage = hello.linkMessages.begin ();
      linkMessage != hello.linkMessages.end ();
      linkMessage++)
  {
      int nt = linkMessage->linkCode >> 2;
      if (nt == OLSR_MPR_NEIGH)
	{
	  NS_LOG_DEBUG ("Processing a link message with neighbor type MPR_NEIGH");

	  for (std::vector<MessageHeader::NeighborInterfaceInfo>::const_iterator nb_iface_info =
		linkMessage->neighborInterfaceInformation.begin ();
		nb_iface_info != linkMessage->neighborInterfaceInformation.end ();
		nb_iface_info++)
	      {
		if (GetMainAddress (nb_iface_info->neighborInterfaceAddress) == m_mainAddress)
		  {
		    NS_LOG_DEBUG ("Adding entry to mpr selector set for neighbor " << nb_iface_info->neighborInterfaceAddress);

		    MprSelectorTuple *existing_mprsel_tuple = m_state.FindMprSelectorTuple (msg.GetOriginatorAddress());

		    if (existing_mprsel_tuple == NULL)
		      {
			MprSelectorTuple new_mprsel_tuple;

			MprSelectorTuple mprsel_tuple;

			mprsel_tuple.mainAddr = msg.GetOriginatorAddress();
			mprsel_tuple.expirationTime = now + msg.GetVTime();

			AddMprSelectorTuple (mprsel_tuple);

			// Schedules mpr selector tuple deletion
			m_events.Track (Simulator::Schedule
					  (DELAY (mprsel_tuple.expirationTime),
					  &RoutingProtocol::MprSelTupleTimerExpire, this,
					  mprsel_tuple.mainAddr));
		      }
		    else
		      {
			existing_mprsel_tuple->expirationTime = now + msg.GetVTime();
		      }
		  }
	      }
	}
  }

  NS_LOG_DEBUG ("Populate Mpr selectors terminated for: " << m_mainAddress);
}


#if 0
///
/// \brief Drops a given packet because it couldn't be delivered to the corresponding
/// destination by the MAC layer. This may cause a neighbor loss, and appropriate
/// actions are then taken.
///
/// \param p the packet which couldn't be delivered by the MAC layer.
///
void
lqolsr::mac_failed (Ptr<Packet> p)
{
  double now              = Simulator::Now ();
  struct hdr_ip* ih       = HDR_IP (p);
  struct hdr_cmn* ch      = HDR_CMN (p);

  debug ("%f: Node %d MAC Layer detects a breakage on link to %d\n",
         now,
         LqOLSR::node_id (ra_addr ()),
         LqOLSR::node_id (ch->next_hop ()));

  if ((u_int32_t)ih->daddr () == IP_BROADCAST)
    {
      drop (p, DROP_RTR_MAC_CALLBACK);
      return;
    }

  LqOLSR_link_tuple* link_tuple = state_.find_link_tuple (ch->next_hop ());
  if (link_tuple != NULL)
    {
      link_tuple->lost_time () = now + OLSR_NEIGHB_HOLD_TIME;
      link_tuple->time ()      = now + OLSR_NEIGHB_HOLD_TIME;
      nb_loss (link_tuple);
    }
  drop (p, DROP_RTR_MAC_CALLBACK);
}
#endif


void
RoutingProtocol::NeighborLoss (const LinkTuple &tuple)
{
  NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                << "s: OLSR Node " << m_mainAddress
                << " LinkTuple " << tuple.neighborIfaceAddr << " -> neighbor loss.");
  LinkTupleUpdated (tuple, OLSR_WILL_DEFAULT);
  m_state.EraseTwoHopNeighborTuples (GetMainAddress (tuple.neighborIfaceAddr));
  m_state.EraseMprSelectorTuples (GetMainAddress (tuple.neighborIfaceAddr));

  MprComputation ();
  RoutingTableComputation ();
}

void
RoutingProtocol::AddDuplicateTuple (const DuplicateTuple &tuple)
{
  /*debug("%f: Node %d adds dup tuple: addr = %d seq_num = %d\n",
          Simulator::Now (),
          OLSR::node_id(ra_addr()),
          OLSR::node_id(tuple->addr()),
          tuple->seq_num());*/
  m_state.InsertDuplicateTuple (tuple);
}

void
RoutingProtocol::RemoveDuplicateTuple (const DuplicateTuple &tuple)
{
  /*debug("%f: Node %d removes dup tuple: addr = %d seq_num = %d\n",
    Simulator::Now (),
    OLSR::node_id(ra_addr()),
    OLSR::node_id(tuple->addr()),
    tuple->seq_num());*/
  m_state.EraseDuplicateTuple (tuple);
}

void
RoutingProtocol::LinkTupleAdded (const LinkTuple &tuple, uint8_t willingness)
{
  // Creates associated neighbor tuple
  NeighborTuple nb_tuple;
  nb_tuple.neighborMainAddr = GetMainAddress (tuple.neighborIfaceAddr);
  nb_tuple.willingness = willingness;

  if (tuple.symTime >= Simulator::Now ())
    {
      nb_tuple.status = NeighborTuple::STATUS_SYM;
    }
  else
    {
      nb_tuple.status = NeighborTuple::STATUS_NOT_SYM;
    }

  AddNeighborTuple (nb_tuple);
}

void
RoutingProtocol::RemoveLinkTuple (const LinkTuple &tuple)
{
  NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                << "s: OLSR Node " << m_mainAddress
                << " LinkTuple " << tuple << " REMOVED.");

  m_state.EraseNeighborTuple (GetMainAddress (tuple.neighborIfaceAddr));
  m_state.EraseLinkTuple (tuple);
}

void
RoutingProtocol::LinkTupleUpdated (const LinkTuple &tuple, uint8_t willingness)
{
  // Each time a link tuple changes, the associated neighbor tuple must be recomputed

  NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                << "s: OLSR Node " << m_mainAddress
                << " LinkTuple " << tuple << " UPDATED.");

  NeighborTuple *nb_tuple =
    m_state.FindNeighborTuple (GetMainAddress (tuple.neighborIfaceAddr));

  if (nb_tuple == NULL)
    {
      LinkTupleAdded (tuple, willingness);
      nb_tuple = m_state.FindNeighborTuple (GetMainAddress (tuple.neighborIfaceAddr));
    }

  if (nb_tuple != NULL)
    {
      int statusBefore = nb_tuple->status;

      bool hasSymmetricLink = false;

      const LinkSet &linkSet = m_state.GetLinks ();
      for (LinkSet::const_iterator it = linkSet.begin ();
           it != linkSet.end (); it++)
        {
          const LinkTuple &link_tuple = *it;
          if (GetMainAddress (link_tuple.neighborIfaceAddr) == nb_tuple->neighborMainAddr
              && link_tuple.symTime >= Simulator::Now ())
            {
              hasSymmetricLink = true;
              break;
            }
        }

      if (hasSymmetricLink)
        {
          nb_tuple->status = NeighborTuple::STATUS_SYM;
          NS_LOG_DEBUG (*nb_tuple << "->status = STATUS_SYM; changed:"
                                  << int (statusBefore != nb_tuple->status));
        }
      else
        {
          nb_tuple->status = NeighborTuple::STATUS_NOT_SYM;
          NS_LOG_DEBUG (*nb_tuple << "->status = STATUS_NOT_SYM; changed:"
                                  << int (statusBefore != nb_tuple->status));
        }
    }
  else
    {
      NS_LOG_WARN ("ERROR! Wanted to update a NeighborTuple but none was found!");
    }
}

void
RoutingProtocol::AddNeighborTuple (const NeighborTuple &tuple)
{
//   debug("%f: Node %d adds neighbor tuple: nb_addr = %d status = %s\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->neighborMainAddr),
//         ((tuple->status() == OLSR_STATUS_SYM) ? "sym" : "not_sym"));

  m_state.InsertNeighborTuple (tuple);
  IncrementAnsn ();
}

void
RoutingProtocol::RemoveNeighborTuple (const NeighborTuple &tuple)
{
//   debug("%f: Node %d removes neighbor tuple: nb_addr = %d status = %s\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->neighborMainAddr),
//         ((tuple->status() == OLSR_STATUS_SYM) ? "sym" : "not_sym"));

  m_state.EraseNeighborTuple (tuple);
  IncrementAnsn ();
}

void
RoutingProtocol::AddTwoHopNeighborTuple (const TwoHopNeighborTuple &tuple)
{
//   debug("%f: Node %d adds 2-hop neighbor tuple: nb_addr = %d nb2hop_addr = %d\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->neighborMainAddr),
//         OLSR::node_id(tuple->twoHopNeighborAddr));

  m_state.InsertTwoHopNeighborTuple (tuple);
}

void
RoutingProtocol::RemoveTwoHopNeighborTuple (const TwoHopNeighborTuple &tuple)
{
//   debug("%f: Node %d removes 2-hop neighbor tuple: nb_addr = %d nb2hop_addr = %d\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->neighborMainAddr),
//         OLSR::node_id(tuple->twoHopNeighborAddr));

  m_state.EraseTwoHopNeighborTuple (tuple);
}

void
RoutingProtocol::IncrementAnsn ()
{
  m_ansn = (m_ansn + 1) % (OLSR_MAX_SEQ_NUM + 1);
}

void
RoutingProtocol::AddMprSelectorTuple (const MprSelectorTuple  &tuple)
{
//   debug("%f: Node %d adds MPR selector tuple: nb_addr = %d\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->main_addr()));

  m_state.InsertMprSelectorTuple (tuple);
  IncrementAnsn ();
}

void
RoutingProtocol::RemoveMprSelectorTuple (const MprSelectorTuple &tuple)
{
//   debug("%f: Node %d removes MPR selector tuple: nb_addr = %d\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->main_addr()));

  m_state.EraseMprSelectorTuple (tuple);
  IncrementAnsn ();
}

void
RoutingProtocol::AddTopologyTuple (const TopologyTuple &tuple)
{
//   debug("%f: Node %d adds topology tuple: dest_addr = %d last_addr = %d seq = %d\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->dest_addr()),
//         OLSR::node_id(tuple->last_addr()),
//         tuple->seq());

  m_state.InsertTopologyTuple (tuple);
}

void
RoutingProtocol::RemoveTopologyTuple (const TopologyTuple &tuple)
{
//   debug("%f: Node %d removes topology tuple: dest_addr = %d last_addr = %d seq = %d\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->dest_addr()),
//         OLSR::node_id(tuple->last_addr()),
//         tuple->seq());

  m_state.EraseTopologyTuple (tuple);
}

void
RoutingProtocol::AddIfaceAssocTuple (const IfaceAssocTuple &tuple)
{
//   debug("%f: Node %d adds iface association tuple: main_addr = %d iface_addr = %d\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->main_addr()),
//         OLSR::node_id(tuple->iface_addr()));

  m_state.InsertIfaceAssocTuple (tuple);
}

void
RoutingProtocol::RemoveIfaceAssocTuple (const IfaceAssocTuple &tuple)
{
//   debug("%f: Node %d removes iface association tuple: main_addr = %d iface_addr = %d\n",
//         Simulator::Now (),
//         OLSR::node_id(ra_addr()),
//         OLSR::node_id(tuple->main_addr()),
//         OLSR::node_id(tuple->iface_addr()));

  m_state.EraseIfaceAssocTuple (tuple);
}

void
RoutingProtocol::AddAssociationTuple (const AssociationTuple &tuple)
{
  m_state.InsertAssociationTuple (tuple);
}

void
RoutingProtocol::RemoveAssociationTuple (const AssociationTuple &tuple)
{
  m_state.EraseAssociationTuple (tuple);
}

uint16_t RoutingProtocol::GetPacketSequenceNumber ()
{
  m_packetSequenceNumber = (m_packetSequenceNumber + 1) % (OLSR_MAX_SEQ_NUM + 1);
  return m_packetSequenceNumber;
}

uint16_t RoutingProtocol::GetMessageSequenceNumber ()
{
  m_messageSequenceNumber = (m_messageSequenceNumber + 1) % (OLSR_MAX_SEQ_NUM + 1);
  return m_messageSequenceNumber;
}

void
RoutingProtocol::HelloTimerExpire ()
{
  SendHello ();

  m_helloTimer.Schedule (m_helloInterval);
}

void
RoutingProtocol::TcTimerExpire ()
{
  if (m_state.GetMprSelectors ().size () > 0)
    {
      SendTc ();
    }
  else
    {
      NS_LOG_DEBUG ("Not sending any TC, no one selected me as MPR.");
    }

  m_tcTimer.Schedule (m_tcInterval);
}

void
RoutingProtocol::MidTimerExpire ()
{
  SendMid ();
  m_midTimer.Schedule (m_midInterval);
}

void
RoutingProtocol::HnaTimerExpire ()
{
  if (m_state.GetAssociations ().size () > 0)
    {
      SendHna ();
    }
  else
    {
      NS_LOG_DEBUG ("Not sending any HNA, no associations to advertise.");
    }
  m_hnaTimer.Schedule (m_hnaInterval);
}

void
RoutingProtocol::DupTupleTimerExpire (Ipv4Address address, uint16_t sequenceNumber)
{
  DuplicateTuple *tuple =
    m_state.FindDuplicateTuple (address, sequenceNumber);
  if (tuple == NULL)
    {
      return;
    }
  if (tuple->expirationTime < Simulator::Now ())
    {
      RemoveDuplicateTuple (*tuple);
    }
  else
    {
      m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                           &RoutingProtocol::DupTupleTimerExpire, this,
                                           address, sequenceNumber));
    }
}

void
RoutingProtocol::LinkTupleTimerExpire (Ipv4Address neighborIfaceAddr)
{
  Time now = Simulator::Now ();

  // the tuple parameter may be a stale copy; get a newer version from m_state
  LinkTuple *tuple = m_state.FindLinkTuple (neighborIfaceAddr);
  if (tuple == NULL)
    {
      return;
    }
  if (tuple->time < now)
    {
      NS_LOG_DEBUG("Neighbor Expired: " << neighborIfaceAddr);
      RemoveLinkTuple (*tuple);
      m_metric->NotifyLinkExpired(neighborIfaceAddr);
    }
  else if (tuple->symTime < now)
    {
      if (m_linkTupleTimerFirstTime)
        {
          m_linkTupleTimerFirstTime = false;
        }
      else
        {
          NeighborLoss (*tuple);
        }

      m_events.Track (Simulator::Schedule (DELAY (tuple->time),
                                           &RoutingProtocol::LinkTupleTimerExpire, this,
                                           neighborIfaceAddr));
    }
  else
    {
      m_events.Track (Simulator::Schedule (DELAY (std::min (tuple->time, tuple->symTime)),
                                           &RoutingProtocol::LinkTupleTimerExpire, this,
                                           neighborIfaceAddr));
    }
}

void
RoutingProtocol::Nb2hopTupleTimerExpire (Ipv4Address neighborMainAddr, Ipv4Address twoHopNeighborAddr)
{
  TwoHopNeighborTuple *tuple;
  tuple = m_state.FindTwoHopNeighborTuple (neighborMainAddr, twoHopNeighborAddr);
  if (tuple == NULL)
    {
      return;
    }
  if (tuple->expirationTime < Simulator::Now ())
    {
      RemoveTwoHopNeighborTuple (*tuple);
    }
  else
    {
      m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                           &RoutingProtocol::Nb2hopTupleTimerExpire,
                                           this, neighborMainAddr, twoHopNeighborAddr));
    }
}

void
RoutingProtocol::MprSelTupleTimerExpire (Ipv4Address mainAddr)
{
  MprSelectorTuple *tuple = m_state.FindMprSelectorTuple (mainAddr);
  if (tuple == NULL)
    {
      return;
    }
  if (tuple->expirationTime < Simulator::Now ())
    {
      RemoveMprSelectorTuple (*tuple);
    }
  else
    {
      m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                           &RoutingProtocol::MprSelTupleTimerExpire,
                                           this, mainAddr));
    }
}

void
RoutingProtocol::TopologyTupleTimerExpire (Ipv4Address destAddr, Ipv4Address lastAddr)
{
  TopologyTuple *tuple = m_state.FindTopologyTuple (destAddr, lastAddr);
  if (tuple == NULL)
    {
      return;
    }
  if (tuple->expirationTime < Simulator::Now ())
    {
      RemoveTopologyTuple (*tuple);
    }
  else
    {
      m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                           &RoutingProtocol::TopologyTupleTimerExpire,
                                           this, tuple->destAddr, tuple->lastAddr));
    }
}

void
RoutingProtocol::IfaceAssocTupleTimerExpire (Ipv4Address ifaceAddr)
{
  IfaceAssocTuple *tuple = m_state.FindIfaceAssocTuple (ifaceAddr);
  if (tuple == NULL)
    {
      return;
    }
  if (tuple->time < Simulator::Now ())
    {
      RemoveIfaceAssocTuple (*tuple);
    }
  else
    {
      m_events.Track (Simulator::Schedule (DELAY (tuple->time),
                                           &RoutingProtocol::IfaceAssocTupleTimerExpire,
                                           this, ifaceAddr));
    }
}

void
RoutingProtocol::AssociationTupleTimerExpire (Ipv4Address gatewayAddr, Ipv4Address networkAddr, Ipv4Mask netmask)
{
  AssociationTuple *tuple = m_state.FindAssociationTuple (gatewayAddr, networkAddr, netmask);
  if (tuple == NULL)
    {
      return;
    }
  if (tuple->expirationTime < Simulator::Now ())
    {
      RemoveAssociationTuple (*tuple);
      //NS_LOG_DEBUG("DAP " << gatewayAddr << " Expired");
    }
  else
    {
      m_events.Track (Simulator::Schedule (DELAY (tuple->expirationTime),
                                           &RoutingProtocol::AssociationTupleTimerExpire,
                                           this, gatewayAddr, networkAddr, netmask));
    }
}

void
RoutingProtocol::Clear ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_table.clear ();
}

void
RoutingProtocol::RemoveEntry (Ipv4Address const &dest)
{
  m_table.erase (dest);
}

bool
RoutingProtocol::Lookup (Ipv4Address const &dest,
                         RoutingTableEntry &outEntry) const
{
  // Get the iterator at "dest" position
  std::map<Ipv4Address, RoutingTableEntry>::const_iterator it = m_table.find (dest);

  // If there is no route to "dest", return NULL
  if (it == m_table.end ())
    {
      return false;
    }

  outEntry = it->second;

  return true;
}

bool
RoutingProtocol::FindSendEntry (RoutingTableEntry const &entry,
                                RoutingTableEntry &outEntry) const
{
  outEntry = entry;
  while (outEntry.destAddr != outEntry.nextAddr)
    {
      if (not Lookup (outEntry.nextAddr, outEntry))
        {
          return false;
        }
    }
  return true;
}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId () << " " << header.GetDestination () << " " << oif);
  Ptr<Ipv4Route> rtentry;
  RoutingTableEntry entry1, entry2;
  bool found = false;

  if (Lookup (header.GetDestination (), entry1) != 0)
    {
      bool foundSendEntry = FindSendEntry (entry1, entry2);

      if (!foundSendEntry)
        {
          NS_FATAL_ERROR ("FindSendEntry failure");
        }
      uint32_t interfaceIdx = entry2.interface;
      if (oif && m_ipv4->GetInterfaceForDevice (oif) != static_cast<int> (interfaceIdx))
        {
          // We do not attempt to perform a constrained routing search
          // if the caller specifies the oif; we just enforce that
          // that the found route matches the requested outbound interface
          NS_LOG_DEBUG ("Olsr node " << m_mainAddress
                                     << ": RouteOutput for dest=" << header.GetDestination ()
                                     << " Route interface " << interfaceIdx
                                     << " does not match requested output interface "
                                     << m_ipv4->GetInterfaceForDevice (oif));
          sockerr = Socket::ERROR_NOROUTETOHOST;
          return rtentry;
        }

      rtentry = Create<Ipv4Route> ();
      rtentry->SetDestination (header.GetDestination ());

      // the source address is the interface address that matches
      // the destination address (when multiple are present on the
      // outgoing interface, one is selected via scoping rules)
      NS_ASSERT (m_ipv4);
      uint32_t numOifAddresses = m_ipv4->GetNAddresses (interfaceIdx);
      NS_ASSERT (numOifAddresses > 0);
      Ipv4InterfaceAddress ifAddr;

      if (numOifAddresses == 1)
        {
          ifAddr = m_ipv4->GetAddress (interfaceIdx, 0);
        }
      else
        {
          /// \todo Implment IP aliasing and OLSR
          NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing and OLSR");
        }

      rtentry->SetSource (ifAddr.GetLocal ());
      rtentry->SetGateway (entry2.nextAddr);
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));
      sockerr = Socket::ERROR_NOTERROR;
      NS_LOG_DEBUG ("Olsr node " << m_mainAddress
                                 << ": RouteOutput for dest=" << header.GetDestination ()
                                 << " --> nextHop=" << entry2.nextAddr
                                 << " interface=" << entry2.interface);
      NS_LOG_DEBUG ("Found route to " << rtentry->GetDestination () << " via nh " << rtentry->GetGateway () << " with source addr " << rtentry->GetSource () << " and output dev " << rtentry->GetOutputDevice ());
      found = true;
    }
  else
    {
      rtentry = m_hnaRoutingTable->RouteOutput (p, header, oif, sockerr);

      if (rtentry)
        {
          found = true;
          NS_LOG_DEBUG ("Found route to " << rtentry->GetDestination () << " via nh " << rtentry->GetGateway () << " with source addr " << rtentry->GetSource () << " and output dev " << rtentry->GetOutputDevice ());
        }
    }

  if (!found)
    {
      NS_LOG_DEBUG ("Olsr node " << m_mainAddress
                                 << ": RouteOutput for dest=" << header.GetDestination ()
                                 << " No route to host");
      sockerr = Socket::ERROR_NOROUTETOHOST;
    }
  return rtentry;
}

bool RoutingProtocol::RouteInput  (Ptr<const Packet> p,
                                   const Ipv4Header &header, Ptr<const NetDevice> idev,
                                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                   LocalDeliverCallback lcb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId () << " " << header.GetDestination ());

  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();

  // Consume self-originated packets
  if (IsMyOwnAddress (origin) == true)
    {
      return true;
    }

  // Local delivery
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);
  if (m_ipv4->IsDestinationAddress (dst, iif))
    {
      if (!lcb.IsNull ())
        {
          NS_LOG_LOGIC ("Local delivery to " << dst);
          lcb (p, header, iif);
          return true;
        }
      else
        {
          // The local delivery callback is null.  This may be a multicast
          // or broadcast packet, so return false so that another
          // multicast routing protocol can handle it.  It should be possible
          // to extend this to explicitly check whether it is a unicast
          // packet, and invoke the error callback if so
          return false;
        }
    }

  // Forwarding
  Ptr<Ipv4Route> rtentry;
  RoutingTableEntry entry1, entry2;
  if (Lookup (header.GetDestination (), entry1))
    {
      bool foundSendEntry = FindSendEntry (entry1, entry2);
      if (!foundSendEntry)
        {
          NS_FATAL_ERROR ("FindSendEntry failure");
        }
      rtentry = Create<Ipv4Route> ();
      rtentry->SetDestination (header.GetDestination ());
      uint32_t interfaceIdx = entry2.interface;
      // the source address is the interface address that matches
      // the destination address (when multiple are present on the
      // outgoing interface, one is selected via scoping rules)
      NS_ASSERT (m_ipv4);
      uint32_t numOifAddresses = m_ipv4->GetNAddresses (interfaceIdx);
      NS_ASSERT (numOifAddresses > 0);
      Ipv4InterfaceAddress ifAddr;
      if (numOifAddresses == 1)
        {
          ifAddr = m_ipv4->GetAddress (interfaceIdx, 0);
        }
      else
        {
          /// \todo Implment IP aliasing and OLSR
          NS_FATAL_ERROR ("XXX Not implemented yet:  IP aliasing and OLSR");
        }
      rtentry->SetSource (ifAddr.GetLocal ());
      rtentry->SetGateway (entry2.nextAddr);
      rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIdx));

      NS_LOG_DEBUG ("Olsr node " << m_mainAddress
                                 << ": RouteInput for dest=" << header.GetDestination ()
                                 << " --> nextHop=" << entry2.nextAddr
                                 << " interface=" << entry2.interface);

      ucb (rtentry, p, header);
      return true;
    }
  else
    {
      if (m_hnaRoutingTable->RouteInput (p, header, idev, ucb, mcb, lcb, ecb))
        {
          return true;
        }
      else
        {

#ifdef NS3_LOG_ENABLE
          NS_LOG_DEBUG ("Olsr node " << m_mainAddress
                                     << ": RouteInput for dest=" << header.GetDestination ()
                                     << " --> NOT FOUND; ** Dumping routing table...");

          for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator iter = m_table.begin ();
               iter != m_table.end (); iter++)
            {
              NS_LOG_DEBUG ("dest=" << iter->first << " --> next=" << iter->second.nextAddr
                                    << " via interface " << iter->second.interface);
            }

          NS_LOG_DEBUG ("** Routing table dump end.");
#endif // NS3_LOG_ENABLE

          return false;
        }
    }
}
void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
}
void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
}
void
RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
}
void
RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
}

uint32_t
RoutingProtocol::GetInterfaceNumberByAddress(const Ipv4Address & intAddress)
{
  NS_ASSERT (m_ipv4);

  uint32_t interface = m_ipv4->GetNInterfaces();

  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
      {
        for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++)
          {
            if (m_ipv4->GetAddress (i,j).GetLocal () == intAddress)
              {
                interface = i;
              }
          }
      }

  return interface;
}


RoutingTableEntry
RoutingProtocol::CreateLqEntry (Ipv4Address const &dest,
                           Ipv4Address const &next,
			   Ipv4Address const &interfaceAddress,
                           float cost)
{
  RoutingTableEntry entry;

  entry.destAddr = dest;
  entry.nextAddr = next;
  entry.interface = GetInterfaceNumberByAddress(interfaceAddress);
  entry.distance = 0;
  entry.cost = cost;

  return entry;
}

void
RoutingProtocol::AddLqEntry (Ipv4Address const &dest,
                           Ipv4Address const &next,
                           uint32_t interface,
                           float cost)
{
  NS_LOG_FUNCTION (this << dest << next << interface << cost << m_mainAddress);

  // Creates a new rt entry with specified values
  RoutingTableEntry &entry = m_table[dest];

  entry.destAddr = dest;
  entry.nextAddr = next;
  entry.interface = interface;
  entry.distance = 0;
  entry.cost = cost;
}

void
RoutingProtocol::AddEntry (Ipv4Address const &dest,
                           Ipv4Address const &next,
                           uint32_t interface,
                           uint32_t distance)
{
  NS_LOG_FUNCTION (this << dest << next << interface << distance << m_mainAddress);

  NS_ASSERT (distance > 0);

  // Creates a new rt entry with specified values
  RoutingTableEntry &entry = m_table[dest];

  entry.destAddr = dest;
  entry.nextAddr = next;
  entry.interface = interface;
  entry.distance = distance;
}

void
RoutingProtocol::AddLqEntry (Ipv4Address const &dest,
                           Ipv4Address const &next,
                           Ipv4Address const &interfaceAddress,
                           float cost)
{
  NS_LOG_FUNCTION (this << dest << next << interfaceAddress << cost << m_mainAddress);

  NS_ASSERT (m_ipv4);

  RoutingTableEntry entry;
  uint32_t intId = GetInterfaceNumberByAddress(interfaceAddress);
  AddLqEntry (dest, next, intId, cost);
}

void
RoutingProtocol::AddEntry (Ipv4Address const &dest,
                           Ipv4Address const &next,
                           Ipv4Address const &interfaceAddress,
                           uint32_t distance)
{
  NS_LOG_FUNCTION (this << dest << next << interfaceAddress << distance << m_mainAddress);

  NS_ASSERT (distance > 0);
  NS_ASSERT (m_ipv4);

  RoutingTableEntry entry;
  uint32_t intId = GetInterfaceNumberByAddress(interfaceAddress);
  AddEntry (dest, next, intId, distance);
}

bool
RoutingProtocol::UpdateEntry(Ipv4Address const &dest,
	                     Ipv4Address const &next,
			     Ipv4Address const &intAddress,
	                     float cost)
{
  RoutingTableEntry outEntry;
  bool found = Lookup (dest, outEntry);

  if (found)
    {
      m_table[dest].destAddr = dest;
      m_table[dest].nextAddr = next;
      m_table[dest].interface = GetInterfaceNumberByAddress(intAddress);
      m_table[dest].cost = cost;
    }

  return found;
}


std::vector<RoutingTableEntry>
RoutingProtocol::GetRoutingTableEntries () const
{
  std::vector<RoutingTableEntry> retval;
  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator iter = m_table.begin ();
       iter != m_table.end (); iter++)
    {
      retval.push_back (iter->second);
    }
  return retval;
}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uniformRandomVariable->SetStream (stream);
  return 1;
}

bool
RoutingProtocol::IsMyOwnAddress (const Ipv4Address & a) const
{
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (a == iface.GetLocal ())
        {
          return true;
        }
    }
  return false;
}

Ipv4Address
RoutingProtocol::GetMyMainAddress()
{
  return m_mainAddress;
}

void
RoutingProtocol::Dump (void)
{
#ifdef NS3_LOG_ENABLE
  Time now = Simulator::Now ();
  NS_LOG_DEBUG ("Dumping for node with main address " << m_mainAddress);
  NS_LOG_DEBUG (" Neighbor set");
  for (NeighborSet::const_iterator iter = m_state.GetNeighbors ().begin ();
       iter != m_state.GetNeighbors ().end (); iter++)
    {
      NS_LOG_DEBUG ("  " << *iter);
    }
  NS_LOG_DEBUG (" Two-hop neighbor set");
  for (TwoHopNeighborSet::const_iterator iter = m_state.GetTwoHopNeighbors ().begin ();
       iter != m_state.GetTwoHopNeighbors ().end (); iter++)
    {
      if (now < iter->expirationTime)
        {
          NS_LOG_DEBUG ("  " << *iter);
        }
    }
  NS_LOG_DEBUG (" Routing table");
  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator iter = m_table.begin (); iter != m_table.end (); iter++)
    {
      NS_LOG_DEBUG ("  dest=" << iter->first << " --> next=" << iter->second.nextAddr << " via interface " << iter->second.interface);
    }
  NS_LOG_DEBUG ("");
#endif  //NS3_LOG_ENABLE
}

Ptr<const Ipv4StaticRouting>
RoutingProtocol::GetRoutingTableAssociation () const
{
  return m_hnaRoutingTable;
}

} // namespace olsr
} // namespace ns3


