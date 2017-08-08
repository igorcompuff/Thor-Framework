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

#ifndef READING_HEADER_H
#define READING_HEADER_H

#include <stdint.h>
#include "ns3/header.h"


namespace ns3 {
namespace ami {

double EmfToSeconds (uint8_t emf);
uint8_t SecondsToEmf (double seconds);

/**
  \verbatim
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Sequence Number       |         Reading Type          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             Reading                           |                                   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  \endverbatim
  */
class ReadingHeader : public Header
{
public:

  enum ReadingType
  {
    POWER_COMSUMPTION = 1,
  };

  ReadingHeader ();
  virtual ~ReadingHeader ();
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  void SetReadingType (ReadingType type);
  ReadingType GetReadingType () const;
  void SetPacketSequenceNumber (uint16_t seqnum);
  uint16_t GetPacketSequenceNumber () const;
  void SetReadingInfo(uint32_t info);
  uint32_t GetReadingInfo() const;

  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  ReadingType m_readingType;
  uint16_t m_sequenceNumber;  //!< The packet sequence number.
  uint32_t m_readingInfo;
};
}
}  // namespace ami, ns3

#endif /* READING_HEADER_H */

