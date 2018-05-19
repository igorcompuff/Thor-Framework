/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "dap.h"

namespace ns3 {

	namespace ddsa {

		Dap::Dap(Ipv4Address address, float cost, Time expirationTime)
		{
			m_address = address;
			m_cost = cost;
			m_expirationTime = expirationTime;
			excluded = false;
			m_probability = 0.0;
		}

		Dap::Dap(Ipv4Address address): Dap(address, -1, Time::Max())
		{

		}

		Dap::Dap(): Dap(Ipv4Address::GetBroadcast())
		{

		}

		Dap::~Dap(){}

		Ipv4Address
		Dap::GetAddress()
		{
			return m_address;
		}

		void
		Dap::SetProbability(double probability)
		{
			m_probability = probability;
		}

		double
		Dap::GetProbability()
		{
			return m_probability;
		}

		void
		Dap::SetCost(float cost)
		{
			m_cost = cost;
		}

		float
		Dap::GetCost()
		{
			return m_cost;
		}

		void
		Dap::Exclude(bool option)
		{
			excluded = option;
		}

		bool
		Dap::IsExcluded()
		{
			return excluded;
		}

		void
		Dap::SetExpirationTime(Time expirationTime)
		{
			m_expirationTime = expirationTime;
		}

		Time
		Dap::GetExpirationTime()
		{
			return m_expirationTime;
		}
	}
}
