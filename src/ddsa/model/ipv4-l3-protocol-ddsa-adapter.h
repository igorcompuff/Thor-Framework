/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IPV4_L3_PROTOCOL_DDSA_ADAPTER_H
#define IPV4_L3_PROTOCOL_DDSA_ADAPTER_H

#include "ns3/ipv4-l3-protocol.h"

namespace ns3 {

  namespace ddsa{

    class Ipv4L3ProtocolDdsaAdapter : public Ipv4L3Protocol
    {
      public:

      enum FailureType
          {
            FULL, MALICIOUS
          };

	/**
	 * \brief Get the type ID.
	 * \return The object TypeId.
	 */
	static TypeId GetTypeId (void);

	Ipv4L3ProtocolDdsaAdapter ();
	virtual ~Ipv4L3ProtocolDdsaAdapter ();

	virtual void Send (Ptr<Packet> packet, Ipv4Address source, Ipv4Address destination, uint8_t protocol,
			   Ptr<Ipv4Route> route);
	virtual void Receive ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
	                 const Address &to, NetDevice::PacketType packetType);
	void MakeFail();
	void SetFailureType(FailureType fType);

      private:
	bool ShouldFailMalicious(Ptr<Packet> packet, bool receiveMethod);

	bool mustFail;
	FailureType m_failureType;
    };

  }
}

#endif /* IPV4_L3_PROTOCOL_DDSA_ADAPTER_H */

