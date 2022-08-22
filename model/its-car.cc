/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "its-car.h"

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

#define MYLOG_COMPONENT "its.car"
#include "../helper/mylog-helper.h"

namespace ndn {
namespace its {

ItsCar::ItsCar (Name appPrefix, Name nodeName, ns3::Ptr<ns3::TraciClient> &traci)
    : m_scheduler (m_face.getIoService ()),
      m_keyChain ("pib-memory:", "tpm-memory:"),
      m_validator (m_face),
      m_appPrefix (appPrefix),
      m_nodeName (nodeName),
      m_rand (ns3::CreateObject<ns3::UniformRandomVariable> ()),
      m_traci (traci),
      m_beaconInterval (1000), //miliseconds
      m_beaconRTTimeout (5000), //miliseconds
      m_defaultRTTimeout (5000), //miliseconds
      m_defaultInterval (1000), //miliseconds
      m_defaultFreshnessPeriod (1000 * 3600) //miliseconds
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

  /* Register the BEACON prefix in the FIB */
  Name appHelloPrefix = Name (kCarBeaconPrefix);
  appHelloPrefix.append (kCarHelloType);
  m_face.setInterestFilter (appHelloPrefix, std::bind (&ItsCar::OnBeaconHelloInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  /* Register the MSG application prefix in the FIB */
  Name appMsgPrefix = Name (kCarBeaconPrefix);
  appMsgPrefix.append (m_nodeName);

  m_face.setInterestFilter (appMsgPrefix, std::bind (&ItsCar::OnMsgInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  // Set Interest Filter for KEY (m_nodeName + KEY)
  /* Register the KEY prefix in the FIB so other can fetch the certificate */
  Name nodeKeyPrefix = Name (m_nodeName);
  nodeKeyPrefix.append ("KEY");
  m_face.setInterestFilter (nodeKeyPrefix, std::bind (&ItsCar::OnKeyInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  m_scheduler.schedule (ndn::time::milliseconds (m_defaultInterval), [this] { PrintFib (); });
}

void
ItsCar::Start ()
{
  m_newRouteInterest = true; //node interested in faster routes

  m_scheduler.schedule (time::milliseconds (m_beaconInterval), [this] { SendBeaconInterest (); });
  m_scheduler.schedule (time::milliseconds (m_defaultInterval), [this] { SendItsInterest (); });
}

void
ItsCar::Stop ()
{
}

void
ItsCar::run ()
{
  m_face.processEvents ();
}

void
ItsCar::cleanup ()
{
}

void
ItsCar::SendItsInterest ()
{
  if (!m_newRouteInterest)
    return;

  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
  std::vector<std::string> vehCurrRoute =
      m_traci->TraCIAPI::vehicle.getRoute (m_traci->GetVehicleId (thisNode));
  std::string vehicleRoad = m_traci->TraCIAPI::vehicle.getRoadID (m_traci->GetVehicleId (thisNode));
  std::string vehicleRoad_src = vehCurrRoute.front ();
  std::string vehicleRoad_dst = vehCurrRoute.back ();

  //MYLOG_INFO ("vehicle vehCurrRoute size: " << vehCurrRoute.size ());
  //MYLOG_INFO ("vehicle vehCurrRoute front: " << vehCurrRoute.front ());
  //MYLOG_INFO ("vehicle vehCurrRoute back: " << vehCurrRoute.back ());
  //MYLOG_INFO ("vehicle vehicleRoad: " << vehicleRoad);

  //name schema >> /service/.../<time-window>  note: <time-window> ~ <timetamp> || time-window = 1 = timetamp
  Name name = Name (m_appPrefix);
  name.append (vehicleRoad_src);
  name.append (vehicleRoad_dst);
  name.append (std::to_string (CalcTimeWindow (10))); //<time-window>
  MYLOG_INFO ("Sending Navigo Interest " << name);

  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (name);
  interest.setCanBePrefix (false);
  interest.setInterestLifetime (time::milliseconds (m_defaultRTTimeout));

  m_face.expressInterest (interest, std::bind (&ItsCar::OnItsContent, this, _1, _2),
                          std::bind (&ItsCar::OnItsNack, this, _1, _2),
                          std::bind (&ItsCar::OnItsTimedOut, this, _1));

  m_scheduler.schedule (time::milliseconds (m_defaultInterval), [this] { SendItsInterest (); });
  //m_newRouteInterest = false;
}

void
ItsCar::SendBeaconInterest ()
{
  Name name = Name (kCarBeaconPrefix);
  name.append (kCarHelloType);
  name.append (m_nodeName);
  MYLOG_INFO ("Sending BEACON " << name);

  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (name);
  interest.setCanBePrefix (false);
  interest.setInterestLifetime (time::milliseconds (m_beaconRTTimeout));

  m_face.expressInterest (
      interest, [] (const Interest &, const Data &) {}, [] (const Interest &, const lp::Nack &) {},
      [] (const Interest &) {});

  m_scheduler.schedule (time::milliseconds (m_beaconInterval), [this] { SendBeaconInterest (); });
}

void
ItsCar::OnBeaconHelloInterest (const ndn::Interest &interest)
{
  Name appHelloPrefix = Name (kCarBeaconPrefix);
  appHelloPrefix.append (kCarHelloType);
  // Sanity check to avoid hello interest from myself
  std::string neighName = interest.getName ().getSubName (appHelloPrefix.size ()).toUri ();
  if (neighName == m_nodeName)
    {
      return;
    }
  MYLOG_INFO ("Received BEACON (Hello Interest) " << interest.getName ());
  //SendMsgInterest (neighName);
}

void
ItsCar::SendMsgInterest (const std::string &neighName)
{
  Name msgName = Name (kCarBeaconPrefix);
  msgName.append (neighName);
  //name.append ("/1");
  MYLOG_INFO ("Sending MSG Interest to neighbor=" << neighName << " >>> " << msgName << "");

  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (msgName);
  interest.setCanBePrefix (false);
  interest.setMustBeFresh (true);
  interest.setInterestLifetime (time::milliseconds (m_defaultRTTimeout));

  m_face.expressInterest (interest, std::bind (&ItsCar::OnMsgContent, this, _1, _2),
                          std::bind (&ItsCar::OnMsgNack, this, _1, _2),
                          std::bind (&ItsCar::OnMsgTimedOut, this, _1));
}

void
ItsCar::OnMsgInterest (const ndn::Interest &interest)
{
  MYLOG_INFO ("Received MSG Interest " << interest.getName ());

  // ReplyMsgInterest
  auto data = std::make_shared<ndn::Data> (interest.getName ());
  data->setFreshnessPeriod (ndn::time::milliseconds (m_defaultFreshnessPeriod));

  // Set ALERT answer
  std::string alert_str = "<< TEST CAR MSG >>";
  data->setContent (reinterpret_cast<const uint8_t *> (alert_str.c_str ()), alert_str.size ());
  m_keyChain.sign (*data, m_signingInfo);
  m_face.put (*data);

  MYLOG_INFO ("MSG sent! Data content: " << alert_str);
}

void
ItsCar::OnMsgContent (const ndn::Interest &interest, const ndn::Data &data)
{
  // Call the Validator upon receiving a data packet
  MYLOG_INFO ("Data received: " << data.getName ());

  /* Security validation */
  if (data.getSignature ().hasKeyLocator ())
    {
      MYLOG_DEBUG ("Data signed with: " << data.getSignature ().getKeyLocator ().getName ());
    }

  // Validating data
  MYLOG_DEBUG ("Validating package...");
  m_validator.validate (data, std::bind (&ItsCar::OnMsgValidated, this, _1),
                        std::bind (&ItsCar::OnMsgValidationFailed, this, _1, _2));
}

void
ItsCar::OnMsgTimedOut (const ndn::Interest &interest)
{
  MYLOG_DEBUG ("Interest timed out for Name: " << interest.getName ());
}

void
ItsCar::OnMsgNack (const ndn::Interest &interest, const ndn::lp::Nack &nack)
{
  MYLOG_DEBUG ("Received Nack with reason: " << nack.getReason ());
}

void
ItsCar::OnMsgValidated (const ndn::Data &data)
{
  MYLOG_DEBUG ("Validated data: " << data.getName ());

  std::string neighName = data.getName ().getSubName (m_appPrefix.size () + 1).toUri ();

  /* Get data content */
  size_t content_size = data.getContent ().value_size ();
  std::string content_value ((char *) data.getContent ().value (), content_size);
  MYLOG_INFO ("Received MSG from=" << neighName << " size=" << content_size
                                   << " msg=" << content_value);

  m_neighMap.emplace (neighName, content_value);
}

void
ItsCar::OnMsgValidationFailed (const ndn::Data &data, const ndn::security::v2::ValidationError &ve)
{
  MYLOG_DEBUG ("Fail to validate data: " << data.getName () << " error: " << ve);
}

ndn::security::v2::Certificate
ItsCar::GetCertificate ()
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
  myCertName.append ("Test"); //default <issuer-info>
  myCertName.appendVersion ();
  myCert.setName (myCertName);
  MYLOG_INFO (">> myCert name: " << myCert.getName ());

  return myCert;
}

void
ItsCar::InstallSignedCertificate (ndn::security::v2::Certificate signedCert)
{
  auto myIdentity = m_keyChain.getPib ().getIdentity (m_nodeName);
  auto myKey = myIdentity.getKey (signedCert.getKeyName ());
  m_keyChain.addCertificate (myKey, signedCert);
  m_keyChain.setDefaultCertificate (myKey, signedCert);
  m_signingInfo =
      ndn::security::SigningInfo (ndn::security::SigningInfo::SIGNER_TYPE_ID, m_nodeName);
}

void
ItsCar::OnKeyInterest (const ndn::Interest &interest)
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
ItsCar::OnItsContent (const ndn::Interest &interest, const ndn::Data &data)
{
  // Call the Validator upon receiving a data packet
  MYLOG_INFO ("Data received: " << data.getName ());

  /* Security validation */
  if (data.getSignature ().hasKeyLocator ())
    {
      MYLOG_DEBUG ("Data signed with: " << data.getSignature ().getKeyLocator ().getName ());
    }

  // Validating data
  MYLOG_DEBUG ("Validating package...");
  m_validator.validate (data, std::bind (&ItsCar::OnMsgItsValidated, this, _1),
                        std::bind (&ItsCar::OnMsgValidationFailed, this, _1, _2));
}

void
ItsCar::OnItsNack (const ndn::Interest &interest, const ndn::lp::Nack &nack)
{
  MYLOG_DEBUG ("Received Nack with reason: " << nack.getReason ());
}

void
ItsCar::OnItsTimedOut (const ndn::Interest &interest)
{
  MYLOG_DEBUG ("Interest timed out for Name: " << interest.getName ());
}

void
ItsCar::OnMsgItsValidated (const ndn::Data &data)
{
  MYLOG_DEBUG ("Validated data: " << data.getName ());

  /* Get data content */
  size_t content_size = data.getContent ().value_size ();
  std::string content_value ((char *) data.getContent ().value (), content_size);
  if (content_value.size () < 30)
    {
      MYLOG_INFO ("Received MSG ITS size=" << content_size << " msg=" << content_value);
    }
  else
    {
      MYLOG_INFO ("Received MSG ITS size=" << content_size
                                           << " msg=" << content_value.substr (0, 30) << "...");
    }

  nlohmann::json jsonRes = nlohmann::json::parse (content_value);
  //std::cout << "\t attr_1:" << jsonRes.at ("vehiclePath") << std::endl;
  //std::cout << "\t attr_2:" << jsonRes.at ("pathTime").get<double> () << std::endl;

  auto new_routepath = [] (std::string roads_str) -> std::vector<std::string> {
    std::vector<std::string> roads;
    std::string token;
    std::stringstream ss (roads_str);
    while (getline (ss, token, ' '))
      roads.push_back (token);
    return roads;
  };

  //**** Caution ****
  //ALERT: Sumo routes must respect the PATTERN: road_0_(current) road_1_(next) ... road_N_(destination)
  //simulation crash (error): "... Route replacement failed for ..."
  MYLOG_INFO ("Path (roads) has changed for a faster route. Estimated arrival time ~ "
              << jsonRes.at ("pathTime").get<double> () << " (s)");
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
  m_traci->TraCIAPI::vehicle.setRoute (m_traci->GetVehicleId (thisNode),
                                       new_routepath (jsonRes.at ("vehiclePath")));

  m_newRouteInterest = false; //true: vehicle will continue to search for faster routes
}

void
ItsCar::PrintFib ()
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
ItsCar::RegisterPrefixes ()
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

      Name appHelloPrefix = Name (kCarBeaconPrefix);
      appHelloPrefix.append (kCarHelloType);
      MYLOG_DEBUG ("FibHelper::AddRoute prefix=" << appHelloPrefix
                                                 << " via faceId=" << face->getId ());
      // add Fib entry for TMSPREFIX with properly faceId
      FibHelper::AddRoute (thisNode, appHelloPrefix, face, metric);
    }
}

int
ItsCar::CalcTimeWindow (int window_size_in_seconds)
{
  if (window_size_in_seconds < 0)
    return -1;
  return (int) ns3::Simulator::Now ().GetSeconds () / window_size_in_seconds;
}

} // namespace its
} // namespace ndn