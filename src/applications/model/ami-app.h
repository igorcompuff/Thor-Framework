/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#ifndef AMI_APPLICATION_H
#define AMI_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;


class AmiApplication : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);

    AmiApplication ();

    virtual ~AmiApplication();

    /**
    * \brief Assign a fixed random variable stream number to the random variables
    * used by this model.
    *
    * \param stream first stream index to use
    * \return the number of stream indices assigned by this model
    */
    int64_t AssignStreams (int64_t stream);


  private:
    uint16_t m_seqNumber;
    Ptr<Socket> m_socket;       //!< Associated socket
    Address m_peer;         //!< Peer address
    TypeId m_socketTid;
    EventId m_sendEvent;
    Ptr<UniformRandomVariable> m_rnd;

    TracedCallback<Ptr<const Packet> > m_txTrace;

    virtual void SendPacket ();

    // inherited from Application base class.
    virtual void StartApplication (void);    // Called at time specified by Start
    virtual void StopApplication (void);     // Called at time specified by Stop
};

} // namespace ns3

#endif /* AMI_APPLICATION_H */
