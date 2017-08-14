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

#ifndef AMI_HEADER_H
#define AMI_HEADER_H

#include <stdint.h>
#include <vector>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

namespace ns3 {
namespace ami {

  enum ReadingType
    {
      POWER_COMSUMPTION = 1,
    };

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
  class AmiHeader : public Header
  {
  public:
    AmiHeader ();
    virtual ~AmiHeader ();

    void SetReadingType (ReadingType type)
    {
      m_readingType = type;
    }

    ReadingType GetReadingType () const
    {
      return m_readingType;
    }

    void SetReadingInfo(uint32_t info)
    {
      m_readingInfo = info;
    }

    uint32_t GetReadingInfo() const
    {
      return m_readingInfo;
    }

    /**
     * Set the packet sequence number.
     * \param seqnum The packet sequence number.
     */
    void SetPacketSequenceNumber (uint16_t seqnum)
    {
      m_packetSequenceNumber = seqnum;
    }

    /**
     * Get the packet sequence number.
     * \returns The packet sequence number.
     */
    uint16_t GetPacketSequenceNumber () const
    {
      return m_packetSequenceNumber;
    }

  private:
    uint16_t m_packetSequenceNumber;  //!< The packet sequence number.
    ReadingType m_readingType;
    uint32_t m_readingInfo;

  public:
    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual uint32_t GetSerializedSize (void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
  };
}
}

#endif /* AMI_HEADER_H */

