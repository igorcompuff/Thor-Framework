#include "ns3/log.h"
#include "ami-app.h"
#include "ns3/simple-header.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/double.h"
#include "ns3/integer.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AmiApplication");

NS_OBJECT_ENSURE_REGISTERED (AmiApplication);

TypeId
AmiApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AmiApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<AmiApplication> ()
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&AmiApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&AmiApplication::m_socketTid),
                   MakeTypeIdChecker ())
     .AddAttribute ("Retrans", "Number of Retransmissions",
			    IntegerValue (0),
			    MakeIntegerAccessor(&AmiApplication::m_nRetransmissions),
			    MakeIntegerChecker<int> (0))
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&AmiApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

AmiApplication::AmiApplication()
{
  m_socket = NULL;
  m_seqNumber = 0;
  m_rnd = CreateObject<UniformRandomVariable>();
  m_rnd->SetAttribute("Min", DoubleValue(0));
  m_rnd->SetAttribute("Max", DoubleValue(10000));
  m_nRetransmissions = 0;
}

AmiApplication::~AmiApplication(){}

// Application Methods

void AmiApplication::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_socketTid);
      if (Inet6SocketAddress::IsMatchingType (m_peer))
	{
	  m_socket->Bind6 ();
	}
      else if (InetSocketAddress::IsMatchingType (m_peer) ||
	       PacketSocketAddress::IsMatchingType (m_peer))
	{
	  m_socket->Bind ();
	}
      m_socket->Connect (m_peer);
    }

  Simulator::Schedule (Seconds(1), &AmiApplication::SendPacket, this);
}

void AmiApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel(m_sendEvent);

  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("AmiApplication found null socket to close in StopApplication");
    }
}

void
AmiApplication::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());
  Ptr<Packet> packet = Create<Packet> ();

  ami::AmiHeader header;
  header.SetPacketSequenceNumber(m_seqNumber++);
  header.SetNodeId(m_node->GetId());
  header.SetReadingInfo(1);//m_rnd->GetInteger());

  packet->AddHeader(header);

  for (int i = 1; i<= (m_nRetransmissions + 1); i++)
    {
      m_socket->Send (packet);
      m_txTrace (packet);
      NS_LOG_DEBUG("(" << Simulator::Now().GetSeconds() << ") Sent packet copy " << header.GetPacketSequenceNumber());
    }

  m_sendEvent = Simulator::Schedule (Seconds(1), &AmiApplication::SendPacket, this);
}

int64_t
AmiApplication::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rnd->SetStream (stream);

  return 1;
}


}



