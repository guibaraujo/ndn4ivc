/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef NDN_DEMO_H
#define NDN_DEMO_H

#include <iostream>
#include <map>
#include <unordered_map>
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

#include "ns3/network-module.h"
#include "ns3/traci-client.h"

namespace ndn {
namespace demo {

static const std::string kHelloType = "hi-i-am";

class NdnDemo
{
public:
  NdnDemo (Name appPrefix, Name nodeName, ns3::Ptr<ns3::TraciClient> &m_traci);
  void run ();
  void cleanup ();
  void Start ();
  void Stop ();

  ndn::security::v2::Certificate GetCertificate ();
  void InstallSignedCertificate (ndn::security::v2::Certificate signedCert);

private:
  void SendHelloInterest ();
  void OnHelloInterest (const ndn::Interest &interest);
  void OnMsgInterest (const ndn::Interest &interest);
  void ReplyMsgInterest (const ndn::Interest &interest);
  void OnMsgContent (const ndn::Interest &interest, const ndn::Data &data);
  void OnMsgTimedOut (const ndn::Interest &interest);
  void OnMsgNack (const ndn::Interest &interest, const ndn::lp::Nack &nack);
  void SendMsgInterest (const std::string &neighName);
  void OnMsgValidated (const ndn::Data &data);
  void OnMsgValidationFailed (const ndn::Data &data, const ndn::security::v2::ValidationError &ve);
  void OnKeyInterest (const ndn::Interest &interest);
  void PrintFib ();
  void RegisterPrefixes ();

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

  std::map<std::string, std::string> m_neighMap;
};

} // namespace demo
} // namespace ndn

#endif