/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef BEACON_H
#define BEACON_H

#include <iostream>
#include <map>
#include <string>
#include <random>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ns3/core-module.h>
#include <ns3/random-variable-stream.h>

namespace ndn {

static const Name BEACONPREFIX = Name ("/localhop/beacon/");

/** \brief Beacon for simple intervehicle communication.
 */
class Beacon
{
public:
  Beacon (uint32_t m_frequency);
  void run ();

private:
  /** @brief process interest as a beacon for IVC.  
   *  The name schema for beacon, received from a neighbor, should be:
   *  /localhop/beacon/<node-id>/<node-pos-x>/<node-pos-y>/<node-pos-z>
   */
  void OnBeaconInterest (const ndn::Interest &interest, uint64_t inFaceId);

  void PrintFib ();

  void ProcessInterest (const ndn::Interest &interest);

  void RegisterPrefixes ();

  void SendBeaconInterest ();

  void setBeaconInterval (uint32_t frequencyInMillisecond);

  uint64_t ExtractIncomingFace (const ndn::Interest &interest);

private:
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;

  ns3::Ptr<ns3::UniformRandomVariable> m_rand; ///< @brief nonce generator

  uint32_t m_seq;
  uint32_t m_nodeid;
  uint32_t m_frequency; // @brief frequency of beacons (in milliseconds)

  std::string m_macaddr;

  scheduler::EventId send_beacon_event; /* async send hello event scheduler */
};
} // namespace ndn

#endif // BEACON_H