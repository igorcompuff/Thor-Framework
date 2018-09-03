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

#include <cmath>
#include "ns3/assert.h"
#include "ns3/log.h"
#include "simple-header.h"

#define READING_HEADER_SIZE 16

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AmiHeader");

namespace ami {

  NS_OBJECT_ENSURE_REGISTERED (AmiHeader);

  AmiHeader::AmiHeader ()
  {
    m_packetSequenceNumber = 0;
    m_readingInfo = 0;
    m_meterId = 0;
    m_dapId = 0;
  }

  AmiHeader::~AmiHeader ()
  {
  }

  TypeId
  AmiHeader::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::ami::AmiHeader")
      .SetParent<Header> ()
      .SetGroupName ("Ami")
      .AddConstructor<AmiHeader> ()
    ;
    return tid;
  }

  TypeId
  AmiHeader::GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  uint32_t
  AmiHeader::GetSerializedSize (void) const
  {
    return READING_HEADER_SIZE;
  }

  void
  AmiHeader::Print (std::ostream &os) const
  {
    /// \todo
  }

  void
  AmiHeader::Serialize (Buffer::Iterator start) const
  {
    Buffer::Iterator i = start;
    i.WriteHtonU32 (m_packetSequenceNumber);
    i.WriteHtonU32 (m_meterId);
    i.WriteHtonU32 (m_dapId);
    i.WriteHtonU32 (m_readingInfo);
  }

  uint32_t
  AmiHeader::Deserialize (Buffer::Iterator start)
  {
    Buffer::Iterator i = start;
      m_packetSequenceNumber  = i.ReadNtohU32 ();
      m_meterId = i.ReadNtohU32 ();
      m_dapId = i.ReadNtohU32 ();
      m_readingInfo = i.ReadNtohU32 ();
      return GetSerializedSize ();
  }
}
}

