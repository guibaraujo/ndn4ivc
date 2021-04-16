/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <ns3/tms-consumer.h>

#include <ns3/ptr.h>
#include <limits>
#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/node-list.h>
#include <ns3/ndnSIM/helper/ndn-stack-helper.hpp>
#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/util/time.hpp>

#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"

#include "ns3/mobility-model.h"

#include <vector>
#include <stdlib.h>
#include <stdio.h>

NS_LOG_COMPONENT_DEFINE ("ndn.TmsConsumer");

namespace ndn {

TmsConsumer::TmsConsumer (uint32_t frequency, ns3::Ptr<ns3::TraciClient> &traci)
    : m_scheduler (m_face.getIoService ()),
      m_rand (ns3::CreateObject<ns3::UniformRandomVariable> ()),
      m_frequency (frequency),
      m_RTTimeout (1),
      m_timeWindow (-1),
      m_windowSize (30),
      m_traci (traci)
{
  m_interestList.clear ();
}

void
TmsConsumer::ScheduleNextPacket ()
{
  ns3::Ptr<ns3::UniformRandomVariable> delay = ns3::CreateObject<ns3::UniformRandomVariable> ();
  // 500 is a magic number - change % num nodes here
  m_sendInterestEvent =
      m_scheduler.schedule (time::milliseconds (m_frequency + delay->GetInteger (0.0, 500.0)),
                            [this] { SendInterest (); });
}

void
TmsConsumer::SendInterest ()
{
  CheckNewTimeWindow ();

  if (m_interestList.size () > 0)
    {
      // app name schema >> /service/traffic/<road-id>/<time-window>
      Name name = Name (PREFIX); ///service/traffic
      name.append (m_interestList.at (0)); ///<road-id>
      name.append (std::to_string (m_timeWindow)); ///<time-window>
      m_interestList.erase (m_interestList.begin ());

      Interest interest = Interest ();
      interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
      interest.setName (name);
      interest.setCanBePrefix (false);
      interest.setInterestLifetime (time::seconds (m_RTTimeout));

      NS_LOG_INFO ("Sending an interest name: " << name);
      m_face.expressInterest (interest, std::bind (&TmsConsumer::OnData, this, _1, _2),
                              std::bind (&TmsConsumer::OnNack, this, _1, _2),
                              std::bind (&TmsConsumer::OnTimedOut, this, _1));
      //   m_face.expressInterest (
      //       interest, [] (const Interest &, const Data &) {},
      //       [] (const Interest &, const lp::Nack &) {}, [] (const Interest &) {});
    }
  ScheduleNextPacket ();
}

void
TmsConsumer::ResendInterest (const ndn::Name name)
{
  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (name);
  interest.setCanBePrefix (false);
  interest.setInterestLifetime (time::seconds (m_RTTimeout));

  NS_LOG_INFO ("Retransmitting the interest name: " << name);
  m_face.expressInterest (interest, std::bind (&TmsConsumer::OnData, this, _1, _2),
                          std::bind (&TmsConsumer::OnNack, this, _1, _2),
                          std::bind (&TmsConsumer::OnTimedOut, this, _1));
}

void
TmsConsumer::OnData (const ndn::Interest &interest, const ndn::Data &data)
{
  NS_LOG_INFO ("DATA received for: " << data.getName ());
}

void
TmsConsumer::OnNack (const ndn::Interest &interest, const ndn::lp::Nack &nack)
{
  NS_LOG_INFO ("NACK received for Name: " << nack.getInterest ().getName ()
                                          << ", reason: " << nack.getReason ());
  m_interestList.push_back (interest.getName ().get (interest.getName ().size () - 2).toUri ());
}

void
TmsConsumer::OnTimedOut (const ndn::Interest &interest)
{
  NS_LOG_INFO ("Interest TIME OUT for Name: " << interest.getName ());
  // Is it in the same time window?
  if (stoi (interest.getName ().get (interest.getName ().size () - 1).toUri ()) == m_timeWindow)
    ResendInterest (interest.getName ()); // retransmitir
}

void
TmsConsumer::CheckNewTimeWindow ()
{
  if (m_timeWindow != (int) ns3::Simulator::Now ().GetSeconds () / m_windowSize)
    {
      m_timeWindow = (int) ns3::Simulator::Now ().GetSeconds () / m_windowSize;
      m_interestList.clear ();

      ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
      std::vector<std::string> vehicleRoute =
          m_traci->TraCIAPI::vehicle.getRoute (m_traci->GetVehicleId (thisNode));
      std::string vehicleRoad =
          m_traci->TraCIAPI::vehicle.getRoadID (m_traci->GetVehicleId (thisNode));

      bool ctrlCurrentRoute = false;
      for (std::string roadId : vehicleRoute)
        {
          if (ctrlCurrentRoute)
            m_interestList.push_back (roadId);

          if (roadId.compare (vehicleRoad) == 0)
            ctrlCurrentRoute = true;
        }
    }
}

void
TmsConsumer::run ()
{
  m_face.processEvents ();
}

void
TmsConsumer::start ()
{
  ScheduleNextPacket ();
}

void
TmsConsumer::stop ()
{
  NS_LOG_DEBUG ("Stopping TmsConsumer application...");
}

} // namespace ndn
