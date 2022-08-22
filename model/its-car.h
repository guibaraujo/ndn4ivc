/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef ITS_CAR_H
#define ITS_CAR_H

#include <iostream>
#include <map>
#include <unordered_map>
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

#include "ns3/network-module.h"
#include "ns3/traci-client.h"

#include "../src/json/single_include/nlohmann/json.hpp"

namespace ndn {
namespace its {

static const std::string kCarBeaconPrefix = "/localhop/beacon";
static const std::string kCarHelloType = "/hi-i-am";
//static const std::string kCarTag = "\%CAR";

class ItsCar
{
public:
  ItsCar (Name appPrefix, Name nodeName, ns3::Ptr<ns3::TraciClient> &m_traci);
  void run ();
  void cleanup ();
  void Start ();
  void Stop ();

  ndn::security::v2::Certificate GetCertificate ();
  void InstallSignedCertificate (ndn::security::v2::Certificate signedCert);

private:
  void SendBeaconInterest ();
  void OnBeaconHelloInterest (const ndn::Interest &interest);
  void OnMsgInterest (const ndn::Interest &interest);
  void OnMsgContent (const ndn::Interest &interest, const ndn::Data &data);
  void OnMsgTimedOut (const ndn::Interest &interest);
  void OnMsgNack (const ndn::Interest &interest, const ndn::lp::Nack &nack);
  void SendMsgInterest (const std::string &neighName);
  void OnMsgValidated (const ndn::Data &data);
  void OnMsgValidationFailed (const ndn::Data &data, const ndn::security::v2::ValidationError &ve);
  void OnKeyInterest (const ndn::Interest &interest);

  void SendItsInterest ();
  void OnItsContent (const ndn::Interest &interest, const ndn::Data &data);
  void OnItsNack (const ndn::Interest &interest, const ndn::lp::Nack &nack);
  void OnItsTimedOut (const ndn::Interest &interest);

  void PrintFib ();
  void RegisterPrefixes ();
  int CalcTimeWindow (int window_size);

  void OnMsgItsValidated (const ndn::Data &data);

private:
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;
  ndn::KeyChain m_keyChain;
  ndn::ValidatorConfig m_validator;
  ndn::security::SigningInfo m_signingInfo = ndn::security::SigningInfo ();

  Name m_appPrefix;
  Name m_nodeName;
  ns3::Ptr<ns3::UniformRandomVariable> m_rand;
  ns3::Ptr<ns3::TraciClient> m_traci;

  /*All in miliseconds*/
  int m_beaconInterval;
  int m_beaconRTTimeout;
  int m_defaultRTTimeout;
  int m_defaultInterval;
  int m_defaultFreshnessPeriod;

  std::map<std::string, std::string> m_neighMap;

  // vehicle trip/path info
  std::vector<std::string> m_defaultRoute;
  bool m_newRouteInterest;
};

} // namespace its
} // namespace ndn

#endif