/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef BEACON_H
#define BEACON_H

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

#include "neighbor-info.h"

#include <memory>

namespace ndn {

static const Name BEACONPREFIX = Name ("/localhop/beacon/");

/** \brief Beacon for simple intervehicle communication
 */
class Beacon
{
public:
  Beacon (uint32_t m_frequency, ns3::Ptr<ns3::TraciClient> &m_traci);
  void run ();
  void start ();
  void stop ();

private:
  typedef std::map<uint32_t, NeighborInfo> NeighborMap;

  bool isValidBeacon (const Name &name, NeighborInfo &neighbor);

  /** @brief Processing beacon interest. The name schema is:
   *  /localhop/beacon/<node-id>/<node-pos-x>/<node-pos-y>/<node-pos-z>
   */
  void OnBeaconInterest (const ndn::Interest &interest, uint64_t inFaceId);

  void PrintFib ();

  void PrintNeighbors ();

  void ProcessInterest (const ndn::Interest &interest);

  void RegisterPrefixes ();

  void SendBeaconInterest ();

  void setBeaconInterval (uint32_t frequencyInMillisecond);

  uint64_t ExtractIncomingFace (const ndn::Interest &interest);
  uint64_t CreateUnicastFace (std::string mac);

private:
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;

  ns3::Ptr<ns3::UniformRandomVariable> m_rand;

  uint32_t m_seq;
  uint32_t m_frequency; // @brief frequency of beacons (in milliseconds)

  ns3::Time m_lastSpeedChange;

  ns3::Ptr<ns3::TraciClient> m_traci; // @brief sumo client - TraCI

  NeighborMap m_neighbors; // @brief neighbors' info*/

  scheduler::EventId m_sendBeaconEvent; /* async send hello event scheduler */
};
} // namespace ndn

#endif // BEACON_H