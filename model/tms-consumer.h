/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef TMS_CONSUMER_H
#define TMS_CONSUMER_H

#include <iostream>
#include <map>
#include <string>
#include <random>
#include <vector>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ns3/core-module.h>
#include <ns3/random-variable-stream.h>

#include "ns3/network-module.h"
#include "ns3/wifi-phy.h"
#include "ns3/traci-client.h"

#include "../../src/json/single_include/nlohmann/json.hpp"

#include "neighbor-info.h"

#include <memory>
#include <map>

namespace ndn {

static const Name PREFIX = Name ("/service/traffic/");

/** \brief Transportation Management Service - Consumer app
 */
class TmsConsumer
{
public:
  TmsConsumer (uint32_t m_frequency, ns3::Ptr<ns3::TraciClient> &m_traci);
  void run ();
  void start ();
  void stop ();

private:
  void CheckNewTimeWindow ();

  void OnData (const ndn::Interest &interest, const ndn::Data &data);
  void OnNack (const ndn::Interest &interest, const ndn::lp::Nack &nack);
  void OnTimedOut (const ndn::Interest &interest);

  void SendInterest ();
  void ResendInterest (const ndn::Name name);

  uint64_t ExtractIncomingFace (const ndn::Interest &interest);
  uint64_t ExtractIncomingFace (const ndn::Data &data);

  void ScheduleNextPacket ();
  void CheckAlternativeRoute (const std::string edge, std::vector<std::string> &newRoute);

private:
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;

  ns3::Ptr<ns3::UniformRandomVariable> m_rand;

  uint32_t m_frequency; // @brief interest (in milliseconds)
  uint32_t m_RTTimeout;

  int m_timeWindow;
  int m_windowSize; // @brief window size interval (in seconds)

  ns3::Ptr<ns3::TraciClient> m_traci; // @brief sumo client - TraCI

  scheduler::EventId m_sendInterestEvent; /* async send hello event scheduler */

  std::vector<std::string> m_interestList; // @brief roadIds of interest
};
} // namespace ndn

#endif // TMS_CONSUMER_H