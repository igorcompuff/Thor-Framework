/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GLOBAL_RETRANS_DDSA_IPV4_L3_PROTOCOL_H
#define GLOBAL_RETRANS_DDSA_IPV4_L3_PROTOCOL_H

#include "ns3/ddsa-ipv4-l3-protocol-base.h"
#include "ns3/traced-callback.h"

namespace ns3 {

	namespace ddsa{

		class GlobalRetransDdsaIpv4L3Protocol : public DdsaIpv4L3ProtocolBase
		{
			public:

				static TypeId GetTypeId (void);

				GlobalRetransDdsaIpv4L3Protocol ();
				virtual ~GlobalRetransDdsaIpv4L3Protocol ();

				typedef void (* PacketSentTracedCallback)
				    (Ptr<const Packet> packet, const Ipv4Address &address);

			protected:
				virtual void DoSend(Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);

			private:
				bool MustProcess(Ipv4Address destination);
				TracedCallback<Ptr<const Packet>, const Ipv4Address &> m_txTrace;
		};

	}
}

#endif /* GLOBAL_RETRANS_DDSA_IPV4_L3_PROTOCOL_H */

