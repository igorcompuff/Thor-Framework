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
#include "meter-reading-header.h"
#include "ns3/log.h"

#define READING_HEADER_SIZE 8

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ReadingHeader");

namespace ami {

NS_OBJECT_ENSURE_REGISTERED (ReadingHeader);

ReadingHeader::ReadingHeader (){}

ReadingHeader::~ReadingHeader (){}

TypeId
ReadingHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ami::ReadingHeader")
    .SetParent<Header> ()
    .SetGroupName ("Ami")
    .AddConstructor<ReadingHeader> ()
  ;
  return tid;
}

void
ReadingHeader::SetReadingType (ReadingType type)
{
  m_readingType = type;
}

ReadingHeader::ReadingType
ReadingHeader::GetReadingType () const
{
  return m_readingType;
}

void
ReadingHeader::SetPacketSequenceNumber (uint16_t seqnum)
{
  m_sequenceNumber = seqnum;
}

uint16_t
ReadingHeader::GetPacketSequenceNumber () const
{
  return m_sequenceNumber;
}

void
ReadingHeader::setReadingInfo(uint32_t info)
{
  m_readingInfo = info;
}

uint32_t
ReadingHeader::getReadingInfo() const
{
  return m_readingInfo;
}

TypeId
ReadingHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
ReadingHeader::GetSerializedSize (void) const
{
  return READING_HEADER_SIZE;
}

void
ReadingHeader::Print (std::ostream &os) const
{
  /// \todo
}

void
ReadingHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_sequenceNumber);
  i.WriteHtonU16 (m_readingType);
  i.WriteHtonU32 (m_readingInfo);
}

uint32_t
ReadingHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_sequenceNumber  = i.ReadNtohU16 ();
  m_readingType = (ReadingHeader::ReadingType)i.ReadNtohU16 ();
  m_readingInfo = i.ReadNtohU32 ();
  return GetSerializedSize ();
}
}
}  // namespace ami, ns3

