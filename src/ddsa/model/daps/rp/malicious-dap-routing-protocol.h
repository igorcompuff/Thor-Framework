/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef MALICIOUS_DAP_ROUTING_PROTOCOL_H
#define MALICIOUS_DAP_ROUTING_PROTOCOL_H

#include "ns3/lq-olsr-routing-protocol.h"
#include "ns3/simulator.h"
#include "ns3/lq-olsr-repositories.h"

namespace ns3 {

  namespace ddsa{

    class MaliciousDapRoutingProtocol : public ns3::lqolsr::RoutingProtocol
    {
		public:

    		/**
			* \brief Get the type ID.
			* \return The object TypeId.
			*/
			static TypeId GetTypeId (void);

			MaliciousDapRoutingProtocol ();
			virtual ~MaliciousDapRoutingProtocol ();

			void StartMalicious();
			void StopMalicious();
			bool IsMalicious();

		protected:
			virtual float GetCostToTcSend(lqolsr::LinkTuple *link_tuple);
			virtual uint32_t GetHelloInfoToSendHello(Ipv4Address neiAddress);
			virtual void SendTc ();

		private:
			bool malicious;
    };

  }
}

#endif /* MALICIOUS_DAP_ROUTING_PROTOCOL_H */

