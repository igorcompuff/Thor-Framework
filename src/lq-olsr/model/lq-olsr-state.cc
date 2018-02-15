/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "lq-olsr-state.h"

namespace ns3 {
namespace lqolsr {

  /********** MPR Selector Set Manipulation **********/

  MprSelectorTuple*
  LqOlsrState::FindMprSelectorTuple (Ipv4Address const &mainAddr)
  {
    for (MprSelectorSet::iterator it = m_mprSelectorSet.begin ();
         it != m_mprSelectorSet.end (); it++)
      {
        if (it->mainAddr == mainAddr)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  void
  LqOlsrState::EraseMprSelectorTuple (const MprSelectorTuple &tuple)
  {
    for (MprSelectorSet::iterator it = m_mprSelectorSet.begin ();
         it != m_mprSelectorSet.end (); it++)
      {
        if (*it == tuple)
          {
            m_mprSelectorSet.erase (it);
            break;
          }
      }
  }

  void
  LqOlsrState::EraseMprSelectorTuples (const Ipv4Address &mainAddr)
  {
    for (MprSelectorSet::iterator it = m_mprSelectorSet.begin ();
         it != m_mprSelectorSet.end (); )
      {
        if (it->mainAddr == mainAddr)
          {
            it = m_mprSelectorSet.erase (it);
          }
        else
          {
            it++;
          }
      }
  }

  void
  LqOlsrState::InsertMprSelectorTuple (MprSelectorTuple const &tuple)
  {
    m_mprSelectorSet.push_back (tuple);
  }

  std::string
  LqOlsrState::PrintMprSelectorSet () const
  {
    std::ostringstream os;
    os << "[";
    for (MprSelectorSet::const_iterator iter = m_mprSelectorSet.begin ();
         iter != m_mprSelectorSet.end (); iter++)
      {
        MprSelectorSet::const_iterator next = iter;
        next++;
        os << iter->mainAddr;
        if (next != m_mprSelectorSet.end ())
          {
            os << ", ";
          }
      }
    os << "]";
    return os.str ();
  }


  /********** Neighbor Set Manipulation **********/

  NeighborTuple*
  LqOlsrState::FindNeighborTuple (Ipv4Address const &mainAddr)
  {
    for (NeighborSet::iterator it = m_neighborSet.begin ();
         it != m_neighborSet.end (); it++)
      {
        if (it->neighborMainAddr == mainAddr)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  const NeighborTuple*
  LqOlsrState::FindSymNeighborTuple (Ipv4Address const &mainAddr) const
  {
    for (NeighborSet::const_iterator it = m_neighborSet.begin ();
         it != m_neighborSet.end (); it++)
      {
        if (it->neighborMainAddr == mainAddr && it->status == NeighborTuple::STATUS_SYM)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  NeighborTuple*
  LqOlsrState::FindNeighborTuple (Ipv4Address const &mainAddr, uint8_t willingness)
  {
    for (NeighborSet::iterator it = m_neighborSet.begin ();
         it != m_neighborSet.end (); it++)
      {
        if (it->neighborMainAddr == mainAddr && it->willingness == willingness)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  void
  LqOlsrState::EraseNeighborTuple (const NeighborTuple &tuple)
  {
    for (NeighborSet::iterator it = m_neighborSet.begin ();
         it != m_neighborSet.end (); it++)
      {
        if (*it == tuple)
          {
            m_neighborSet.erase (it);
            break;
          }
      }
  }

  void
  LqOlsrState::EraseNeighborTuple (const Ipv4Address &mainAddr)
  {
    for (NeighborSet::iterator it = m_neighborSet.begin ();
         it != m_neighborSet.end (); it++)
      {
        if (it->neighborMainAddr == mainAddr)
          {
            it = m_neighborSet.erase (it);
            break;
          }
      }
  }

  void
  LqOlsrState::InsertNeighborTuple (NeighborTuple const &tuple)
  {
    for (NeighborSet::iterator it = m_neighborSet.begin ();
         it != m_neighborSet.end (); it++)
      {
        if (it->neighborMainAddr == tuple.neighborMainAddr)
          {
            // Update it
            *it = tuple;
            return;
          }
      }
    m_neighborSet.push_back (tuple);
  }

  /********** Neighbor 2 Hop Set Manipulation **********/

  void
  LqOlsrState::FindSameNodeTwoHopNeighbors(const Ipv4Address & twoHopNeighborAddress, TwoHopNeighborSet & sameTwoHopSet)
  {
    for (TwoHopNeighborSet::iterator twoHopNeighbor = m_twoHopNeighborSet.begin();
	 twoHopNeighbor != m_twoHopNeighborSet.end(); twoHopNeighbor++)
      {
	if (twoHopNeighbor->twoHopNeighborAddr == twoHopNeighborAddress)
	  {
	    sameTwoHopSet.push_back(*twoHopNeighbor);
	  }
      }
  }

  TwoHopNeighborTuple*
  LqOlsrState::FindTwoHopNeighborTuple (Ipv4Address const &neighborMainAddr,
                                      Ipv4Address const &twoHopNeighborAddr)
  {
    for (TwoHopNeighborSet::iterator it = m_twoHopNeighborSet.begin ();
         it != m_twoHopNeighborSet.end (); it++)
      {
        if (it->neighborMainAddr == neighborMainAddr
            && it->twoHopNeighborAddr == twoHopNeighborAddr)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  void
  LqOlsrState::EraseTwoHopNeighborTuple (const TwoHopNeighborTuple &tuple)
  {
    for (TwoHopNeighborSet::iterator it = m_twoHopNeighborSet.begin ();
         it != m_twoHopNeighborSet.end (); it++)
      {
        if (*it == tuple)
          {
            m_twoHopNeighborSet.erase (it);
            break;
          }
      }
  }

  void
  LqOlsrState::EraseTwoHopNeighborTuples (const Ipv4Address &neighborMainAddr,
                                        const Ipv4Address &twoHopNeighborAddr)
  {
    for (TwoHopNeighborSet::iterator it = m_twoHopNeighborSet.begin ();
         it != m_twoHopNeighborSet.end (); )
      {
        if (it->neighborMainAddr == neighborMainAddr
            && it->twoHopNeighborAddr == twoHopNeighborAddr)
          {
            it = m_twoHopNeighborSet.erase (it);
          }
        else
          {
            it++;
          }
      }
  }

  void
  LqOlsrState::EraseTwoHopNeighborTuples (const Ipv4Address &neighborMainAddr)
  {
    for (TwoHopNeighborSet::iterator it = m_twoHopNeighborSet.begin ();
         it != m_twoHopNeighborSet.end (); )
      {
        if (it->neighborMainAddr == neighborMainAddr)
          {
            it = m_twoHopNeighborSet.erase (it);
          }
        else
          {
            it++;
          }
      }
  }

  void
  LqOlsrState::InsertTwoHopNeighborTuple (TwoHopNeighborTuple const &tuple)
  {
    m_twoHopNeighborSet.push_back (tuple);
  }

  /********** MPR Set Manipulation **********/

  bool
  LqOlsrState::FindMprAddress (Ipv4Address const &addr)
  {
    MprSet::iterator it = m_mprSet.find (addr);
    return (it != m_mprSet.end ());
  }

  void
  LqOlsrState::SetMprSet (MprSet mprSet)
  {
    m_mprSet = mprSet;
  }
  MprSet
  LqOlsrState::GetMprSet () const
  {
    return m_mprSet;
  }

  /********** Duplicate Set Manipulation **********/

  DuplicateTuple*
  LqOlsrState::FindDuplicateTuple (Ipv4Address const &addr, uint16_t sequenceNumber)
  {
    for (DuplicateSet::iterator it = m_duplicateSet.begin ();
         it != m_duplicateSet.end (); it++)
      {
        if (it->address == addr && it->sequenceNumber == sequenceNumber)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  void
  LqOlsrState::EraseDuplicateTuple (const DuplicateTuple &tuple)
  {
    for (DuplicateSet::iterator it = m_duplicateSet.begin ();
         it != m_duplicateSet.end (); it++)
      {
        if (*it == tuple)
          {
            m_duplicateSet.erase (it);
            break;
          }
      }
  }

  void
  LqOlsrState::InsertDuplicateTuple (DuplicateTuple const &tuple)
  {
    m_duplicateSet.push_back (tuple);
  }

  /********** Link Set Manipulation **********/

//  LinkTuple *
//  LqOlsrState::FindBestLinkToNeighbor(const Ipv4Address & neighborMainAddress, Time now, Ptr<lqmetric::LqAbstractMetric> metric)
//  {
//    LinkTuple* bestLink = NULL;
//    LinkTuple* backupLink = NULL;
//    IfaceAssocTuple* tuple = NULL;
//
//    float currentBestCost = metric->GetInfinityCostValue();
//
//    for (LinkSet::iterator link = m_linkSet.begin(); link != m_linkSet.end(); link++)
//      {
//	tuple = FindIfaceAssocTuple(link->neighborIfaceAddr);
//
//	if (tuple == NULL || tuple->mainAddr != neighborMainAddress)
//	  {
//	    continue;
//	  }
//
//	if (metric->CompareBest(link->cost, currentBestCost) > 0)
//	  {
//	    currentBestCost = link->cost;
//	    if (link->symTime < now)
//	      {
//		backupLink = &(*link);
//	      }
//	    else
//	      {
//		bestLink = &(*link);
//	      }
//	  }
//      }
//
//    return bestLink != NULL ? bestLink : backupLink;
//
//  }

  LinkTuple*
  LqOlsrState::FindLinkTuple (Ipv4Address const & ifaceAddr)
  {
    for (LinkSet::iterator it = m_linkSet.begin ();
         it != m_linkSet.end (); it++)
      {
        if (it->neighborIfaceAddr == ifaceAddr)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  LinkTuple*
  LqOlsrState::FindSymLinkTuple (Ipv4Address const &ifaceAddr, Time now)
  {
    for (LinkSet::iterator it = m_linkSet.begin ();
         it != m_linkSet.end (); it++)
      {
        if (it->neighborIfaceAddr == ifaceAddr)
          {
            if (it->symTime > now)
              {
                return &(*it);
              }
            else
              {
                break;
              }
          }
      }
    return NULL;
  }

  void
  LqOlsrState::EraseLinkTuple (const LinkTuple &tuple)
  {
    for (LinkSet::iterator it = m_linkSet.begin ();
         it != m_linkSet.end (); it++)
      {
        if (*it == tuple)
          {
            m_linkSet.erase (it);
            break;
          }
      }
  }

  LinkTuple&
  LqOlsrState::InsertLinkTuple (LinkTuple const &tuple)
  {
    m_linkSet.push_back (tuple);
    return m_linkSet.back ();
  }

  /********** Topology Set Manipulation **********/

  TopologyTuple*
  LqOlsrState::FindTopologyTuple (Ipv4Address const &destAddr,
                                Ipv4Address const &lastAddr)
  {
    for (TopologySet::iterator it = m_topologySet.begin ();
         it != m_topologySet.end (); it++)
      {
        if (it->destAddr == destAddr && it->lastAddr == lastAddr)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  TopologyTuple*
  LqOlsrState::FindNewerTopologyTuple (Ipv4Address const & lastAddr, uint16_t ansn)
  {
    for (TopologySet::iterator it = m_topologySet.begin ();
         it != m_topologySet.end (); it++)
      {
        if (it->lastAddr == lastAddr && it->sequenceNumber > ansn)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  void
  LqOlsrState::EraseTopologyTuple (const TopologyTuple &tuple)
  {
    for (TopologySet::iterator it = m_topologySet.begin ();
         it != m_topologySet.end (); it++)
      {
        if (*it == tuple)
          {
            m_topologySet.erase (it);
            break;
          }
      }
  }

  void
  LqOlsrState::EraseOlderTopologyTuples (const Ipv4Address &lastAddr, uint16_t ansn)
  {
    for (TopologySet::iterator it = m_topologySet.begin ();
         it != m_topologySet.end (); )
      {
        if (it->lastAddr == lastAddr && it->sequenceNumber < ansn)
          {
            it = m_topologySet.erase (it);
          }
        else
          {
            it++;
          }
      }
  }

  void
  LqOlsrState::InsertTopologyTuple (TopologyTuple const &tuple)
  {
    m_topologySet.push_back (tuple);
  }

  /********** Interface Association Set Manipulation **********/

  IfaceAssocTuple*
  LqOlsrState::FindIfaceAssocTuple (Ipv4Address const &ifaceAddr)
  {
    for (IfaceAssocSet::iterator it = m_ifaceAssocSet.begin ();
         it != m_ifaceAssocSet.end (); it++)
      {
        if (it->ifaceAddr == ifaceAddr)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  const IfaceAssocTuple*
  LqOlsrState::FindIfaceAssocTuple (Ipv4Address const &ifaceAddr) const
  {
    for (IfaceAssocSet::const_iterator it = m_ifaceAssocSet.begin ();
         it != m_ifaceAssocSet.end (); it++)
      {
        if (it->ifaceAddr == ifaceAddr)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  void
  LqOlsrState::EraseIfaceAssocTuple (const IfaceAssocTuple &tuple)
  {
    for (IfaceAssocSet::iterator it = m_ifaceAssocSet.begin ();
         it != m_ifaceAssocSet.end (); it++)
      {
        if (*it == tuple)
          {
            m_ifaceAssocSet.erase (it);
            break;
          }
      }
  }

  void
  LqOlsrState::InsertIfaceAssocTuple (const IfaceAssocTuple &tuple)
  {
    m_ifaceAssocSet.push_back (tuple);
  }

  std::vector<Ipv4Address>
  LqOlsrState::FindNeighborInterfaces (const Ipv4Address &neighborMainAddr) const
  {
    std::vector<Ipv4Address> retval;
    for (IfaceAssocSet::const_iterator it = m_ifaceAssocSet.begin ();
         it != m_ifaceAssocSet.end (); it++)
      {
        if (it->mainAddr == neighborMainAddr)
          {
            retval.push_back (it->ifaceAddr);
          }
      }
    return retval;
  }

  /********** Host-Network Association Set Manipulation **********/

  AssociationTuple*
  LqOlsrState::FindAssociationTuple (const Ipv4Address &gatewayAddr, const Ipv4Address &networkAddr, const Ipv4Mask &netmask)
  {
    for (AssociationSet::iterator it = m_associationSet.begin ();
         it != m_associationSet.end (); it++)
      {
        if (it->gatewayAddr == gatewayAddr and it->networkAddr == networkAddr and it->netmask == netmask)
          {
            return &(*it);
          }
      }
    return NULL;
  }

  void
  LqOlsrState::EraseAssociationTuple (const AssociationTuple &tuple)
  {
    for (AssociationSet::iterator it = m_associationSet.begin ();
         it != m_associationSet.end (); it++)
      {
        if (*it == tuple)
          {
            m_associationSet.erase (it);
            break;
          }
      }
  }

  void
  LqOlsrState::InsertAssociationTuple (const AssociationTuple &tuple)
  {
    m_associationSet.push_back (tuple);
  }

  void
  LqOlsrState::EraseAssociation (const Association &tuple)
  {
    for (Associations::iterator it = m_associations.begin ();
         it != m_associations.end (); it++)
      {
        if (*it == tuple)
          {
            m_associations.erase (it);
            break;
          }
      }
  }

  void
  LqOlsrState::InsertAssociation (const Association &tuple)
  {
    m_associations.push_back (tuple);
  }

}
}

