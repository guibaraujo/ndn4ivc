/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef TMS_PROVIDER_H
#define TMS_PROVIDER_H

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

static const Name TMSPREFIX = Name ("/service/traffic/");

/** \brief Transportation Management Service - Provider app
 */
class TmsProvider
{
public:
  TmsProvider (uint32_t m_frequency, ns3::Ptr<ns3::TraciClient> &m_traci);
  void run ();
  void start ();
  void stop ();

private:
  typedef std::map<uint32_t, NeighborInfo> NeighborMap;

  bool isValidInterest (const Name &name);

  void OnInterest (const ndn::Interest &interest, uint64_t inFaceId);

  void PrintFib ();

  void ProcessInterest (const ndn::Interest &interest);

  void RegisterPrefixes ();

  uint64_t ExtractIncomingFace (const ndn::Interest &interest);
  uint64_t CreateUnicastFace (std::string mac);

  ndn::Face m_face;
  ndn::Scheduler m_scheduler;

  ns3::Ptr<ns3::UniformRandomVariable> m_rand;

  int m_timeWindow;
  int m_windowSize; // @brief window size interval (in seconds)

  ns3::Time m_lastInterestSent;

  ns3::Ptr<ns3::TraciClient> m_traci; // @brief sumo client - TraCI
};
} // namespace ndn

#endif // TMS_PROVIDER_H