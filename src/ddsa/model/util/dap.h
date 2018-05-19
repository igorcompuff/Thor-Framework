/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DAP_H
#define DAP_H

#include "ns3/ipv4-address.h"
#include "ns3/simulator.h"

namespace ns3 {

	namespace ddsa{

		class Dap
		{
			public:

				Dap (Ipv4Address address, float cost, Time expirationTime);
				Dap (Ipv4Address address);
				Dap ();
				virtual ~Dap ();

				Ipv4Address GetAddress();
				void SetProbability(double probability);
				double GetProbability();
				void SetCost(float cost);
				float GetCost();
				void Exclude(bool option);
				bool IsExcluded();
				void SetExpirationTime(Time expirationTime);
				Time GetExpirationTime();

				bool operator==(const Dap& rhs){ return (m_address == rhs.m_address); }

			private:

				Ipv4Address m_address;
				double m_probability;
				float m_cost;
				bool excluded;
				Time m_expirationTime;
		};

	}
}

#endif /* DAP_H */

