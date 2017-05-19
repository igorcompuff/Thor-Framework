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

#include "ns3/test.h"
#include "ns3/lq-olsr-header.h"
#include "ns3/packet.h"

using namespace ns3;

class OlsrEmfTestCase : public TestCase
{
public:
  OlsrEmfTestCase ();
  virtual void DoRun (void);
};

OlsrEmfTestCase::OlsrEmfTestCase ()
  : TestCase ("Check Emf olsr time conversion")
{
}
void
OlsrEmfTestCase::DoRun (void)
{
  for (int time = 1; time <= 30; time++)
    {
      uint8_t emf = lqolsr::SecondsToEmf (time);
      double seconds = lqolsr::EmfToSeconds (emf);
      NS_TEST_ASSERT_MSG_EQ ((seconds < 0 || std::fabs (seconds - time) > 0.1), false,
                             "100");
    }
}


class OlsrMidTestCase : public TestCase
{
public:
  OlsrMidTestCase ();
  virtual void DoRun (void);
};

OlsrMidTestCase::OlsrMidTestCase ()
  : TestCase ("Check Mid olsr messages")
{
}
void
OlsrMidTestCase::DoRun (void)
{
  Packet packet;

  {
    lqolsr::PacketHeader hdr;
    lqolsr::MessageHeader msg1;
    lqolsr::MessageHeader::Mid &mid1 = msg1.GetMid ();
    lqolsr::MessageHeader msg2;
    lqolsr::MessageHeader::Mid &mid2 = msg2.GetMid ();

    // MID message #1
    {
      std::vector<Ipv4Address> &addresses = mid1.interfaceAddresses;
      addresses.clear ();
      addresses.push_back (Ipv4Address ("1.2.3.4"));
      addresses.push_back (Ipv4Address ("1.2.3.5"));
    }

    msg1.SetTimeToLive (255);
    msg1.SetOriginatorAddress (Ipv4Address ("11.22.33.44"));
    msg1.SetVTime (Seconds (9));
    msg1.SetMessageSequenceNumber (7);

    // MID message #2
    {
      std::vector<Ipv4Address> &addresses = mid2.interfaceAddresses;
      addresses.clear ();
      addresses.push_back (Ipv4Address ("2.2.3.4"));
      addresses.push_back (Ipv4Address ("2.2.3.5"));
    }

    msg2.SetTimeToLive (254);
    msg2.SetOriginatorAddress (Ipv4Address ("12.22.33.44"));
    msg2.SetVTime (Seconds (10));
    msg2.SetMessageType (lqolsr::MessageHeader::MID_MESSAGE);
    msg2.SetMessageSequenceNumber (7);

    // Build an OLSR packet header
    hdr.SetPacketLength (hdr.GetSerializedSize () + msg1.GetSerializedSize () + msg2.GetSerializedSize ());
    hdr.SetPacketSequenceNumber (123);


    // Now add all the headers in the correct order
    packet.AddHeader (msg2);
    packet.AddHeader (msg1);
    packet.AddHeader (hdr);
  }

  {
    lqolsr::PacketHeader hdr;
    packet.RemoveHeader (hdr);
    NS_TEST_ASSERT_MSG_EQ (hdr.GetPacketSequenceNumber (), 123, "200");
    uint32_t sizeLeft = hdr.GetPacketLength () - hdr.GetSerializedSize ();
    {
      lqolsr::MessageHeader msg1;

      packet.RemoveHeader (msg1);

      NS_TEST_ASSERT_MSG_EQ (msg1.GetTimeToLive (),  255, "201");
      NS_TEST_ASSERT_MSG_EQ (msg1.GetOriginatorAddress (), Ipv4Address ("11.22.33.44"), "202");
      NS_TEST_ASSERT_MSG_EQ (msg1.GetVTime (), Seconds (9), "203");
      NS_TEST_ASSERT_MSG_EQ (msg1.GetMessageType (), lqolsr::MessageHeader::MID_MESSAGE, "204");
      NS_TEST_ASSERT_MSG_EQ (msg1.GetMessageSequenceNumber (), 7, "205");

      lqolsr::MessageHeader::Mid &mid1 = msg1.GetMid ();
      NS_TEST_ASSERT_MSG_EQ (mid1.interfaceAddresses.size (), 2, "206");
      NS_TEST_ASSERT_MSG_EQ (*mid1.interfaceAddresses.begin (), Ipv4Address ("1.2.3.4"), "207");

      sizeLeft -= msg1.GetSerializedSize ();
      NS_TEST_ASSERT_MSG_EQ ((sizeLeft > 0), true, "208");
    }
    {
      // now read the second message
      lqolsr::MessageHeader msg2;

      packet.RemoveHeader (msg2);

      NS_TEST_ASSERT_MSG_EQ (msg2.GetTimeToLive (),  254, "209");
      NS_TEST_ASSERT_MSG_EQ (msg2.GetOriginatorAddress (), Ipv4Address ("12.22.33.44"), "210");
      NS_TEST_ASSERT_MSG_EQ (msg2.GetVTime (), Seconds (10), "211");
      NS_TEST_ASSERT_MSG_EQ (msg2.GetMessageType (), lqolsr::MessageHeader::MID_MESSAGE, "212");
      NS_TEST_ASSERT_MSG_EQ (msg2.GetMessageSequenceNumber (), 7, "213");

      lqolsr::MessageHeader::Mid mid2 = msg2.GetMid ();
      NS_TEST_ASSERT_MSG_EQ (mid2.interfaceAddresses.size (), 2, "214");
      NS_TEST_ASSERT_MSG_EQ (*mid2.interfaceAddresses.begin (), Ipv4Address ("2.2.3.4"), "215");

      sizeLeft -= msg2.GetSerializedSize ();
      NS_TEST_ASSERT_MSG_EQ (sizeLeft, 0, "216");
    }
  }
}

class LqOlsrHelloTestCase : public TestCase
{
public:
  LqOlsrHelloTestCase ();
  virtual void DoRun (void);
};

LqOlsrHelloTestCase::LqOlsrHelloTestCase ()
  : TestCase ("Check LqHello olsr messages")
{
}
void
LqOlsrHelloTestCase::DoRun (void)
{
  Packet packet;
  lqolsr::MessageHeader msgIn;
  lqolsr::MessageHeader::LqHello &lqhelloIn = msgIn.GetLqHello ();

  lqhelloIn.SetHTime (Seconds (7));
  lqhelloIn.willingness = 66;

  {
    lqolsr::MessageHeader::LqHello::LinkMessage lm1;
    lqolsr::MessageHeader::NeighborInterfaceInfo neighInfo;

    lm1.linkCode = 2;
    neighInfo.neighborInterfaceAddress = Ipv4Address ("1.2.3.4");
    neighInfo.metricInfo = 50;
    lm1.neighborInterfaceInformation.push_back (neighInfo);

    neighInfo.neighborInterfaceAddress = Ipv4Address ("1.2.3.5");
    neighInfo.metricInfo = 60;
    lm1.neighborInterfaceInformation.push_back (neighInfo);

    lqhelloIn.linkMessages.push_back (lm1);

    lqolsr::MessageHeader::LqHello::LinkMessage lm2;
    lm2.linkCode = 3;

    neighInfo.neighborInterfaceAddress = Ipv4Address ("2.2.3.4");
    neighInfo.metricInfo = 50;
    lm2.neighborInterfaceInformation.push_back (neighInfo);

    neighInfo.neighborInterfaceAddress = Ipv4Address ("2.2.3.5");
    neighInfo.metricInfo = 60;
    lm2.neighborInterfaceInformation.push_back (neighInfo);

    lqhelloIn.linkMessages.push_back (lm2);
  }

  packet.AddHeader (msgIn);

  lqolsr::MessageHeader msgOut;
  packet.RemoveHeader (msgOut);
  lqolsr::MessageHeader::LqHello &lqHelloOut = msgOut.GetLqHello ();

  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.GetHTime (), Seconds (7), "300");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.willingness, 66, "301");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages.size (), 2, "302");

  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[0].linkCode, 2, "303");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[0].neighborInterfaceInformation[0].neighborInterfaceAddress,
                         Ipv4Address ("1.2.3.4"), "304.a");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[0].neighborInterfaceInformation[0].metricInfo,
                           50, "304.b");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[0].neighborInterfaceInformation[1].neighborInterfaceAddress,
                         Ipv4Address ("1.2.3.5"), "305.a");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[0].neighborInterfaceInformation[1].metricInfo,
                           60, "305.b");

  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[1].linkCode, 3, "306");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[1].neighborInterfaceInformation[0].neighborInterfaceAddress,
                         Ipv4Address ("2.2.3.4"), "307.a");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[1].neighborInterfaceInformation[0].metricInfo,
                           50, "307.b");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[1].neighborInterfaceInformation[1].neighborInterfaceAddress,
                         Ipv4Address ("2.2.3.5"), "308.a");
  NS_TEST_ASSERT_MSG_EQ (lqHelloOut.linkMessages[1].neighborInterfaceInformation[1].metricInfo,
                           60, "308.b");

  NS_TEST_ASSERT_MSG_EQ (packet.GetSize (), 0, "All bytes in packet were not read");

}

class LqOlsrTcTestCase : public TestCase
{
public:
  LqOlsrTcTestCase ();
  virtual void DoRun (void);
};

LqOlsrTcTestCase::LqOlsrTcTestCase ()
  : TestCase ("Check LqTc olsr messages")
{
}
void
LqOlsrTcTestCase::DoRun (void)
{
  Packet packet;
  lqolsr::MessageHeader msgIn;
  lqolsr::MessageHeader::LqTc &tcIn = msgIn.GetLqTc ();

  tcIn.ansn = 0x1234;
  lqolsr::MessageHeader::NeighborInterfaceInfo neighInfo;
  neighInfo.neighborInterfaceAddress = Ipv4Address ("1.2.3.4");
  neighInfo.metricInfo = 50;
  tcIn.neighborAddresses.push_back (neighInfo);

  neighInfo.neighborInterfaceAddress = Ipv4Address ("1.2.3.5");
  neighInfo.metricInfo = 60;
  tcIn.neighborAddresses.push_back (neighInfo);
  packet.AddHeader (msgIn);

  lqolsr::MessageHeader msgOut;
  packet.RemoveHeader (msgOut);
  lqolsr::MessageHeader::LqTc &lqTcOut = msgOut.GetLqTc ();

  NS_TEST_ASSERT_MSG_EQ (lqTcOut.ansn, 0x1234, "400");
  NS_TEST_ASSERT_MSG_EQ (lqTcOut.neighborAddresses.size (), 2, "401");

  NS_TEST_ASSERT_MSG_EQ (lqTcOut.neighborAddresses[0].neighborInterfaceAddress, Ipv4Address("1.2.3.4"), "402.a");
  NS_TEST_ASSERT_MSG_EQ (lqTcOut.neighborAddresses[0].metricInfo, 50, "402.b");

  NS_TEST_ASSERT_MSG_EQ (lqTcOut.neighborAddresses[1].neighborInterfaceAddress, Ipv4Address("1.2.3.5"), "403.a");
  NS_TEST_ASSERT_MSG_EQ (lqTcOut.neighborAddresses[1].metricInfo, 60, "403.b");

  NS_TEST_ASSERT_MSG_EQ (packet.GetSize (), 0, "404");

}

class OlsrHnaTestCase : public TestCase
{
public:
  OlsrHnaTestCase ();
  virtual void DoRun (void);
};

OlsrHnaTestCase::OlsrHnaTestCase ()
  : TestCase ("Check Hna olsr messages")
{
}

void
OlsrHnaTestCase::DoRun (void)
{
  Packet packet;
  lqolsr::MessageHeader msgIn;
  lqolsr::MessageHeader::Hna &hnaIn = msgIn.GetHna ();

  hnaIn.associations.push_back ((lqolsr::MessageHeader::Hna::Association)
                                { Ipv4Address ("1.2.3.4"), Ipv4Mask ("255.255.255.0")});
  hnaIn.associations.push_back ((lqolsr::MessageHeader::Hna::Association)
                                { Ipv4Address ("1.2.3.5"), Ipv4Mask ("255.255.0.0")});
  packet.AddHeader (msgIn);

  lqolsr::MessageHeader msgOut;
  packet.RemoveHeader (msgOut);
  lqolsr::MessageHeader::Hna &hnaOut = msgOut.GetHna ();

  NS_TEST_ASSERT_MSG_EQ (hnaOut.associations.size (), 2, "500");

  NS_TEST_ASSERT_MSG_EQ (hnaOut.associations[0].address,
                         Ipv4Address ("1.2.3.4"), "501");
  NS_TEST_ASSERT_MSG_EQ (hnaOut.associations[0].mask,
                         Ipv4Mask ("255.255.255.0"), "502");

  NS_TEST_ASSERT_MSG_EQ (hnaOut.associations[1].address,
                         Ipv4Address ("1.2.3.5"), "503");
  NS_TEST_ASSERT_MSG_EQ (hnaOut.associations[1].mask,
                         Ipv4Mask ("255.255.0.0"), "504");

  NS_TEST_ASSERT_MSG_EQ (packet.GetSize (), 0, "All bytes in packet were not read");

}


static class OlsrTestSuite : public TestSuite
{
public:
  OlsrTestSuite ();
} g_olsrTestSuite;

OlsrTestSuite::OlsrTestSuite ()
  : TestSuite ("routing-lqolsr-header", UNIT)
{
  AddTestCase (new OlsrHnaTestCase (), TestCase::QUICK);
  AddTestCase (new LqOlsrTcTestCase (), TestCase::QUICK);
  AddTestCase (new LqOlsrHelloTestCase (), TestCase::QUICK);
  AddTestCase (new OlsrMidTestCase (), TestCase::QUICK);
  AddTestCase (new OlsrEmfTestCase (), TestCase::QUICK);
}
