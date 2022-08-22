/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ndn-demo.h"

#include <limits>
#include <cmath>
#include <boost/algorithm/string.hpp>
#include <algorithm>

#include <ns3/ptr.h>
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

#define MYLOG_COMPONENT "ndn.demo"
#include "../helper/mylog-helper.h"

namespace ndn {
namespace demo {

NdnDemo::NdnDemo (Name appPrefix, Name nodeName, ns3::Ptr<ns3::TraciClient> &traci)
    : m_scheduler (m_face.getIoService ()),
      m_keyChain ("pib-memory:", "tpm-memory:"),
      m_validator (m_face),
      m_appPrefix (appPrefix),
      m_nodeName (nodeName),
      m_rand (ns3::CreateObject<ns3::UniformRandomVariable> ()),
      m_traci (traci)
{
  // Loading the Data Validator Config with validation rules
  std::string fileName = "./contrib/ndn4ivc/config/validation1.cfg";
  try
    {
      m_validator.load (fileName);
  } catch (const std::exception &e)
    {
      throw std::runtime_error ("Failed to load validation rules file=" + fileName +
                                " Error=" + e.what ());
  }

  RegisterPrefixes ();

  /* Register the HELLO application prefix in the FIB */
  Name appHelloPrefix = Name (m_appPrefix);
  appHelloPrefix.append (kHelloType);
  m_face.setInterestFilter (appHelloPrefix, std::bind (&NdnDemo::OnHelloInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  /* Register the MSG application prefix in the FIB */
  Name appMsgPrefix = Name (m_appPrefix);
  appMsgPrefix.append (m_nodeName);

  m_face.setInterestFilter (appMsgPrefix, std::bind (&NdnDemo::OnMsgInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  // Set Interest Filter for KEY (m_nodeName + KEY)
  /* Register the KEY prefix in the FIB so other can fetch the certificate */
  Name nodeKeyPrefix = Name (m_nodeName);
  nodeKeyPrefix.append ("KEY");
  m_face.setInterestFilter (nodeKeyPrefix, std::bind (&NdnDemo::OnKeyInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  m_scheduler.schedule (ndn::time::seconds (1), [this] { PrintFib (); });
}

void
NdnDemo::Start ()
{
  m_scheduler.schedule (time::seconds (1), [this] { SendHelloInterest (); });
}

void
NdnDemo::Stop ()
{
}

void
NdnDemo::run ()
{
  m_face.processEvents ();
}

void
NdnDemo::cleanup ()
{
}

void
NdnDemo::SendHelloInterest ()
{
  Name name = Name (m_appPrefix);
  name.append (kHelloType);
  name.append (m_nodeName);
  MYLOG_INFO ("Sending Interest " << name);

  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (name);
  interest.setCanBePrefix (false);
  interest.setInterestLifetime (time::milliseconds (1));

  m_face.expressInterest (
      interest, [] (const Interest &, const Data &) {}, [] (const Interest &, const lp::Nack &) {},
      [] (const Interest &) {});
}

void
NdnDemo::OnHelloInterest (const ndn::Interest &interest)
{
  // Sanity check to avoid hello interest from myself
  std::string neighName = interest.getName ().getSubName (m_appPrefix.size () + 1).toUri ();
  if (neighName == m_nodeName)
    {
      return;
    }
  MYLOG_INFO ("Received HELLO Interest " << interest.getName ());

  auto neigh = m_neighMap.find (neighName);
  if (neigh == m_neighMap.end ())
    {
      SendMsgInterest (neighName);
    }
  else
    {
      MYLOG_INFO ("Skipping already known neighbor" << neighName);
    }
}

void
NdnDemo::SendMsgInterest (const std::string &neighName)
{
  Name name = Name (m_appPrefix);

  name.append (neighName);
  //name.append ("/1");
  MYLOG_INFO ("Sending MSG Interest to neighbor=" << neighName << " >>> " << name << "");

  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (name);
  interest.setCanBePrefix (false);
  interest.setMustBeFresh (true);
  interest.setInterestLifetime (time::seconds (5));

  m_face.expressInterest (interest, std::bind (&NdnDemo::OnMsgContent, this, _1, _2),
                          std::bind (&NdnDemo::OnMsgNack, this, _1, _2),
                          std::bind (&NdnDemo::OnMsgTimedOut, this, _1));
}

void
NdnDemo::OnMsgInterest (const ndn::Interest &interest)
{
  MYLOG_INFO ("Received MSG Interest " << interest.getName ());

  auto data = std::make_shared<ndn::Data> (interest.getName ());
  data->setFreshnessPeriod (ndn::time::milliseconds (1000));

  // Set ALERT answer
  std::string alert_str = " TRAFFIC AHEAD! ";
  data->setContent (reinterpret_cast<const uint8_t *> (alert_str.c_str ()), alert_str.size ());
  m_keyChain.sign (*data, m_signingInfo);
  m_face.put (*data);

  MYLOG_INFO ("MSG sent! ALERT: " << alert_str);
}

void
NdnDemo::OnMsgContent (const ndn::Interest &interest, const ndn::Data &data)
{
  // Call the Validator upon receiving a data packet
  MYLOG_DEBUG ("MSG data received name: " << data.getName ());

  /* Security validation */
  if (data.getSignature ().hasKeyLocator ())
    {
      MYLOG_DEBUG ("Data signed with: " << data.getSignature ().getKeyLocator ().getName ());
    }

  // Validating data
  m_validator.validate (data, std::bind (&NdnDemo::OnMsgValidated, this, _1),
                        std::bind (&NdnDemo::OnMsgValidationFailed, this, _1, _2));
}

void
NdnDemo::OnMsgTimedOut (const ndn::Interest &interest)
{
  MYLOG_DEBUG ("Interest timed out for Name: " << interest.getName ());
}

void
NdnDemo::OnMsgNack (const ndn::Interest &interest, const ndn::lp::Nack &nack)
{
  MYLOG_DEBUG ("Received Nack with reason: " << nack.getReason ());
}

void
NdnDemo::OnMsgValidated (const ndn::Data &data)
{
  MYLOG_DEBUG ("Validated data: " << data.getName ());

  std::string neighName = data.getName ().getSubName (m_appPrefix.size () + 1).toUri ();

  /* Get data content */
  size_t content_size = data.getContent ().value_size ();
  std::string content_value ((char *) data.getContent ().value (), content_size);
  MYLOG_DEBUG ("Received MSG from=" << neighName << " size=" << content_size
                                    << " msg=" << content_value);

  m_neighMap.emplace (neighName, content_value);
}

void
NdnDemo::OnMsgValidationFailed (const ndn::Data &data, const ndn::security::v2::ValidationError &ve)
{
  MYLOG_DEBUG ("Fail to validate data: name: " << data.getName () << " error: " << ve);
}

ndn::security::v2::Certificate
NdnDemo::GetCertificate ()
{
  try
    {
      /* cleanup: remove any possible existing certificate with the same name */
      m_keyChain.deleteIdentity (m_keyChain.getPib ().getIdentity (m_nodeName));
  } catch (const std::exception &e)
    {
  }
  ndn::security::Identity myIdentity = m_keyChain.createIdentity (m_nodeName);
  ndn::security::v2::Certificate myCert = myIdentity.getDefaultKey ().getDefaultCertificate ();
  // NDN cert name convention:  /<prefix>/KEY/<key-id>/<issuer-info>/version
  ndn::Name myCertName = myCert.getKeyName ();
  myCertName.append ("Test");
  myCertName.appendVersion ();
  myCert.setName (myCertName);  
  MYLOG_INFO (">> myCert name: " << myCert.getName ());

  return myCert;
}

void
NdnDemo::InstallSignedCertificate (ndn::security::v2::Certificate signedCert)
{
  auto myIdentity = m_keyChain.getPib ().getIdentity (m_nodeName);
  auto myKey = myIdentity.getKey (signedCert.getKeyName ());
  m_keyChain.addCertificate (myKey, signedCert);
  m_keyChain.setDefaultCertificate (myKey, signedCert);
  m_signingInfo =
      ndn::security::SigningInfo (ndn::security::SigningInfo::SIGNER_TYPE_ID, m_nodeName);
}

void
NdnDemo::OnKeyInterest (const ndn::Interest &interest)
{
  // Reply KEY interest with node's Key
  MYLOG_INFO ("Received KEY Interest " << interest.getName ());
  std::string nameStr = interest.getName ().toUri ();
  std::size_t pos = nameStr.find ("/KEY/");
  ndn::Name identityName = ndn::Name (nameStr.substr (0, pos));

  try
    {
      // Create Data packet
      auto cert = m_keyChain.getPib ()
                      .getIdentity (identityName)
                      .getKey (interest.getName ())
                      .getDefaultCertificate ();

      // Return Data packet to the requester
      m_face.put (cert);
  } catch (const std::exception &)
    {
      MYLOG_DEBUG ("The certificate: " << interest.getName ()
                                       << " does not exist! I was looking for Identity="
                                       << identityName);
  }
}

void
NdnDemo::PrintFib ()
{
  using namespace ns3;
  using namespace ns3::ndn;

  Ptr<Node> thisNode = NodeList::GetNode (Simulator::GetContext ());
  const ::nfd::Fib &fib = thisNode->GetObject<L3Protocol> ()->getForwarder ()->getFib ();
  MYLOG_DEBUG ("FIB Size: " << fib.size ());
  for (const ::nfd::fib::Entry &fibEntry : fib)
    {
      std::string str = "";
      for (const auto &nh : fibEntry.getNextHops ())
        str += std::to_string (nh.getFace ().getId ()) + ",";
      MYLOG_DEBUG ("MyFIB: prefix=" << fibEntry.getPrefix () << " via faceIdList=" << str);
    }
}

void
NdnDemo::RegisterPrefixes ()
{
  using namespace ns3;
  using namespace ns3::ndn;

  MYLOG_DEBUG ("Registering prefixes...");

  int32_t metric = 0;
  Ptr<Node> thisNode = NodeList::GetNode (Simulator::GetContext ());

  for (uint32_t deviceId = 0; deviceId < thisNode->GetNDevices (); deviceId++)
    {
      Ptr<NetDevice> device = thisNode->GetDevice (deviceId);
      Ptr<L3Protocol> ndn = thisNode->GetObject<L3Protocol> ();
      NS_ASSERT_MSG (ndn != nullptr, "Ndn stack should be installed on the node");
      // getting the node faceids
      auto face = ndn->getFaceByNetDevice (device);
      NS_ASSERT_MSG (face != nullptr, "There is no face associated with the net-device");

      Name appHelloPrefix = Name (m_appPrefix);
      appHelloPrefix.append (kHelloType);
      MYLOG_DEBUG ("FibHelper::AddRoute prefix=" << appHelloPrefix
                                                 << " via faceId=" << face->getId ());
      // add Fib entry for TMSPREFIX with properly faceId
      FibHelper::AddRoute (thisNode, appHelloPrefix, face, metric);
    }
}

} // namespace demo
} // namespace ndn