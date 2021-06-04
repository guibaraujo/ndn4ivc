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
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());

  CheckNewTimeWindow ();

  if (m_traci->TraCIAPI::vehicle.getVehicleClass (m_traci->GetVehicleId (thisNode))
              .compare ("emergency") != 0 &&
      m_interestList.size () > 0)
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
  NS_LOG_INFO ("DATA received for: " << data.getName ()
                                     << " from faceId: " << ExtractIncomingFace (data));
  NS_LOG_INFO ("DATA packet size: " << data.wireEncode ().size () << " octets (payload="
                                    << data.getContent ().value_size () << ")");

  std::string strContent;
  strContent.assign ((char *) data.getContent ().value (), data.getContent ().value_size ());

  using nlohmann::json;
  auto jContent = json::parse (strContent);
  //std::cout << "\t:" << jContent.at ("roadId") << std::endl;
  //std::cout << "\t:" << jContent.at ("speedLevel") << std::endl;
  //std::cout << "\t:" << jContent.at ("occupancyLevel") << std::endl;

  // simple congestion level estimation/verification
  if (jContent.at ("speedLevel").get<double> () <= 0.2 ||
      jContent.at ("occupancyLevel").get<double> () >= 0.8)
    {
      ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
      std::vector<std::string> vNewRoute;

      CheckAlternativeRoute (data.getName ().at (-2).toUri (), vNewRoute);

      if (vNewRoute.size ())
        { //&& m_traci->GetVehicleId (thisNode).compare ("passenger5") == 0
          libsumo::TraCIColor vColorStruct;
          vColorStruct.r = 0;
          vColorStruct.g = 128;
          vColorStruct.b = 255;
          vColorStruct.a = 205;

          m_traci->TraCIAPI::vehicle.setColor (m_traci->GetVehicleId (thisNode), vColorStruct);
          m_traci->TraCIAPI::vehicle.setRoute (m_traci->GetVehicleId (thisNode), vNewRoute);
        }
    }
}

void
TmsConsumer::CheckAlternativeRoute (const std::string edge,
                                    std::vector<std::string> &alternativeRoute)
{
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
  std::vector<std::string> vehCurrRoute =
      m_traci->TraCIAPI::vehicle.getRoute (m_traci->GetVehicleId (thisNode));
  std::string vehicleRoad = m_traci->TraCIAPI::vehicle.getRoadID (m_traci->GetVehicleId (thisNode));

  // a simple structure for illustrating how to change vehicle route ...
  // NOTE: further actions cloud consider changing the map structure to a graph
  if (edge.compare ("B0C0") == 0)
    {
      bool flag = false;
      for (auto e : vehCurrRoute)
        if (e.compare (vehicleRoad) == 0)
          {
            flag = true;
            alternativeRoute.push_back (vehicleRoad);
          }
        else if (flag)
          {
            if (e.compare ("B0C0") == 0)
              {
                alternativeRoute.push_back ("B0B1");
                alternativeRoute.push_back ("B1C1");
                alternativeRoute.push_back ("C1C0");
              }
            else
              {
                // simple road loop detector
                if (e.compare ("C0C1") == 0 && alternativeRoute.back ().compare ("C1C0") == 0)
                  alternativeRoute.pop_back ();
                else
                  alternativeRoute.push_back (e);
              }
          }
    }
}

void
TmsConsumer::OnNack (const ndn::Interest &interest, const ndn::lp::Nack &nack)
{
  NS_LOG_INFO ("NACK received for Name: " << nack.getInterest ().getName ()
                                          << ", reason: " << nack.getReason ());
  m_interestList.push_back (interest.getName ().at (-2).toUri ());
}

void
TmsConsumer::OnTimedOut (const ndn::Interest &interest)
{
  NS_LOG_INFO ("Interest TIME OUT for Name: " << interest.getName ());

  // Is it in the same time window?
  CheckNewTimeWindow ();
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
  std::string vehicleRoad = m_traci->TraCIAPI::vehicle.getRoadID (m_traci->GetVehicleId (thisNode));

  if (vehicleRoad.compare (interest.getName ().at (-2).toUri ()) != 0)
    if (stoi (interest.getName ().at (-1).toUri ()) == m_timeWindow)
      ResendInterest (interest.getName ());
}

uint64_t
TmsConsumer::ExtractIncomingFace (const ndn::Interest &interest)
{
  /** Incoming Face Indication
   * NDNLPv2 says "Incoming face indication feature allows the forwarder to inform local applications
   * about the face on which a packet is received." and also warns "application MUST be prepared to
   * receive a packet without IncomingFaceId field". From our tests, only internal faces seems to not
   * initialize it and this can even be used to filter out our own interests. See more:
   *  - FibManager::setFaceForSelfRegistration (ndnSIM/NFD/daemon/mgmt/fib-manager.cpp)
   *  - https://redmine.named-data.net/projects/nfd/wiki/NDNLPv2#Incoming-Face-Indication
   */
  shared_ptr<lp::IncomingFaceIdTag> incomingFaceIdTag = interest.getTag<lp::IncomingFaceIdTag> ();
  if (incomingFaceIdTag == nullptr)
    {
      return 0;
    }
  return *incomingFaceIdTag;
}

uint64_t
TmsConsumer::ExtractIncomingFace (const ndn::Data &data)
{
  shared_ptr<lp::IncomingFaceIdTag> incomingFaceIdTag = data.getTag<lp::IncomingFaceIdTag> ();
  if (incomingFaceIdTag == nullptr)
    {
      return 0;
    }
  return *incomingFaceIdTag;
}

void
TmsConsumer::CheckNewTimeWindow ()
{
  if (m_timeWindow != (int) ns3::Simulator::Now ().GetSeconds () / m_windowSize)
    {
      m_timeWindow = (int) ns3::Simulator::Now ().GetSeconds () / m_windowSize;
      m_interestList.clear ();

      ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
      std::string vehicleRoad =
          m_traci->TraCIAPI::vehicle.getRoadID (m_traci->GetVehicleId (thisNode));
      std::vector<std::string> vehicleRoute =
          m_traci->TraCIAPI::vehicle.getRoute (m_traci->GetVehicleId (thisNode));

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
  NS_LOG_INFO ("TMS Consumer application started!");
  ScheduleNextPacket ();
}

void
TmsConsumer::stop ()
{
  NS_LOG_INFO ("Stopping TMS Consumer application...");
  m_sendInterestEvent.cancel ();
  m_scheduler.cancelAllEvents ();
}

} // namespace ndn
