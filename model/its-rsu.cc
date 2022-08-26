/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "its-rsu.h"

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

#define MYLOG_COMPONENT "its.rsu"
#include "../helper/mylog-helper.h"

namespace ndn {
namespace its {

ItsRsu::ItsRsu (Name appPrefix, Name nodeName, ns3::Ptr<ns3::GraphSumoMap> &graph,
                ns3::Ptr<ns3::TraciClient> &traci)
    : m_scheduler (m_face.getIoService ()),
      m_keyChain ("pib-memory:", "tpm-memory:"),
      m_validator (m_face),
      m_itsPrefix (appPrefix),
      m_nodeName (nodeName),
      m_rand (ns3::CreateObject<ns3::UniformRandomVariable> ()),
      m_graphMap (graph),
      m_traci (traci),
      m_beaconInterval (5000), //miliseconds
      m_beaconRTTimeout (0), //miliseconds
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
  Name appHelloPrefix = Name (kRsuBeaconPrefix);
  appHelloPrefix.append (kRsuHelloType);
  m_face.setInterestFilter (appHelloPrefix, std::bind (&ItsRsu::OnBeaconHelloInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  /* Register the MSG application prefix in the FIB */
  Name appMsgPrefix = Name (kRsuBeaconPrefix);
  appMsgPrefix.append (m_nodeName);

  m_face.setInterestFilter (appMsgPrefix, std::bind (&ItsRsu::OnMsgInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  // Set Interest Filter for KEY (m_nodeName + KEY)
  /* Register the KEY prefix in the FIB so other can fetch the certificate */
  Name nodeKeyPrefix = Name (m_nodeName);
  nodeKeyPrefix.append ("KEY");
  m_face.setInterestFilter (nodeKeyPrefix, std::bind (&ItsRsu::OnKeyInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  /* Register the BEACON prefix in the FIB */
  m_face.setInterestFilter (m_itsPrefix, std::bind (&ItsRsu::OnItsInterest, this, _2),
                            [this] (const Name &, const std::string &reason) {
                              throw std::runtime_error (
                                  "Failed to register sync interest prefix: " + reason);
                            });

  //m_scheduler.schedule (ndn::time::milliseconds (m_defaultInterval), [this] { PrintFib (); });
}

void
ItsRsu::Start ()
{
  m_scheduler.schedule (time::milliseconds (m_defaultInterval),
                        [this] { UpdateRoadTrafficDatabase (); });
  ns3::Ptr<ns3::UniformRandomVariable> delay = ns3::CreateObject<ns3::UniformRandomVariable> ();
  m_scheduler.schedule (time::milliseconds (m_defaultInterval + delay->GetInteger (50.0, 200.0)),
                        [this] { SendBeaconInterest (); });
}

void
ItsRsu::Stop ()
{
}

void
ItsRsu::run ()
{
  m_face.processEvents ();
}

void
ItsRsu::cleanup ()
{
}

void
ItsRsu::SendBeaconInterest ()
{
  Name name = Name (kRsuBeaconPrefix);
  name.append (kRsuHelloType);
  name.append (m_nodeName);
  MYLOG_INFO ("Sending BEACON " << name);

  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (name);
  interest.setCanBePrefix (false);
  interest.setInterestLifetime (time::milliseconds (m_beaconRTTimeout)); //It's hello, thus rtt=0

  m_face.expressInterest (
      interest, [] (const Interest &, const Data &) {}, [] (const Interest &, const lp::Nack &) {},
      [] (const Interest &) {});

  ns3::Ptr<ns3::UniformRandomVariable> delay = ns3::CreateObject<ns3::UniformRandomVariable> ();
  m_scheduler.schedule (time::milliseconds (m_beaconInterval + delay->GetInteger (50.0, 200.0)),
                        [this] { SendBeaconInterest (); });
}

void
ItsRsu::OnBeaconHelloInterest (const ndn::Interest &interest)
{
  Name appHelloPrefix = Name (kRsuBeaconPrefix);
  appHelloPrefix.append (kRsuHelloType);
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
ItsRsu::SendMsgInterest (const std::string &neighName)
{
  Name msgName = Name (kRsuBeaconPrefix);
  msgName.append (neighName);
  //name.append ("/1");
  MYLOG_INFO ("Sending MSG Interest to neighbor=" << neighName << " >>> " << msgName << "");

  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (msgName);
  interest.setCanBePrefix (false);
  interest.setMustBeFresh (true);
  interest.setInterestLifetime (time::milliseconds (m_defaultRTTimeout));

  m_face.expressInterest (interest, std::bind (&ItsRsu::OnMsgContent, this, _1, _2),
                          std::bind (&ItsRsu::OnMsgNack, this, _1, _2),
                          std::bind (&ItsRsu::OnMsgTimedOut, this, _1));
}

void
ItsRsu::OnMsgInterest (const ndn::Interest &interest)
{
  MYLOG_INFO ("Received MSG Interest " << interest.getName ());

  // ReplyMsgInterest
  auto data = std::make_shared<ndn::Data> (interest.getName ());
  data->setFreshnessPeriod (ndn::time::milliseconds (m_defaultFreshnessPeriod));

  // Set ALERT answer
  std::string msg_str = "<< TEST RSU MSG >>";
  data->setContent (reinterpret_cast<const uint8_t *> (msg_str.c_str ()), msg_str.size ());
  m_keyChain.sign (*data, m_signingInfo);
  m_face.put (*data);

  MYLOG_INFO ("MSG sent! Data content: " << msg_str);
}

void
ItsRsu::OnMsgContent (const ndn::Interest &interest, const ndn::Data &data)
{
  // Call the Validator upon receiving a data packet
  MYLOG_INFO ("Data received: " << data.getName ());

  /* Security validation */
  if (data.getSignature ().hasKeyLocator ())
    {
      NS_LOG_DEBUG ("Data signed with: " << data.getSignature ().getKeyLocator ().getName ()
                                         << " type=" << data.getSignature ().getType ());
    }

  // Validating data
  m_validator.validate (data, std::bind (&ItsRsu::OnMsgValidated, this, _1),
                        std::bind (&ItsRsu::OnMsgValidationFailed, this, _1, _2));
}

void
ItsRsu::OnMsgTimedOut (const ndn::Interest &interest)
{
  MYLOG_DEBUG ("Interest timed out for Name: " << interest.getName ());
}

void
ItsRsu::OnMsgNack (const ndn::Interest &interest, const ndn::lp::Nack &nack)
{
  MYLOG_DEBUG ("Received Nack with reason: " << nack.getReason ());
}

void
ItsRsu::OnMsgValidated (const ndn::Data &data)
{
  MYLOG_DEBUG ("Validated data: " << data.getName ());

  std::string neighName = data.getName ().getSubName (m_itsPrefix.size () + 1).toUri ();

  /* Get data content */
  size_t content_size = data.getContent ().value_size ();
  std::string content_value ((char *) data.getContent ().value (), content_size);
  MYLOG_INFO ("Received MSG from=" << neighName << " size=" << content_size
                                   << " msg=" << content_value);

  m_neighMap.emplace (neighName, content_value);
}

void
ItsRsu::OnMsgValidationFailed (const ndn::Data &data, const ndn::security::v2::ValidationError &ve)
{
  MYLOG_DEBUG ("Fail to validate data: " << data.getName () << " error: " << ve);
}

ndn::security::v2::Certificate
ItsRsu::GetCertificate ()
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
ItsRsu::InstallSignedCertificate (ndn::security::v2::Certificate signedCert)
{
  auto myIdentity = m_keyChain.getPib ().getIdentity (m_nodeName);
  auto myKey = myIdentity.getKey (signedCert.getKeyName ());
  m_keyChain.addCertificate (myKey, signedCert);
  m_keyChain.setDefaultCertificate (myKey, signedCert);
  m_signingInfo =
      ndn::security::SigningInfo (ndn::security::SigningInfo::SIGNER_TYPE_ID, m_nodeName);
}

void
ItsRsu::OnKeyInterest (const ndn::Interest &interest)
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
ItsRsu::OnItsInterest (const ndn::Interest &interest)
{
  MYLOG_INFO ("Received ITS Interest " << interest.getName ());
  int timeWindow = stoi (interest.getName ().at (-1).toUri ());
  std::string src_road = interest.getName ().at (3).toUri ();
  std::string dst_road = interest.getName ().at (4).toUri ();

  //Check - RSU will only provide 'real time' (a.k.a current time window) road-traffic data
  if (timeWindow != CalcTimeWindow (10)) //data is no longer available - it is outdated (past)
    return; // do nothing

  // *** calculating route (faster) using dijkstra algorithm ***
  int src = m_graphMap->getDstVertexIdByEdgeName (src_road), // source - vehicle current position
      dst = m_graphMap->getDstVertexIdByEdgeName (dst_road); // destination
  double pathWeight = 0; // time to travel between OD (origin destination)
  std::vector<std::string> road_path = {src_road}; // add current position first

  m_graphMap->dijkstra (src, dst, pathWeight, road_path);
  auto road_path_str = [road_path] () -> std::string {
    std::string str = "";
    for (auto it = road_path.begin (); it != road_path.end (); it++)
      {
        if (it != road_path.begin ())
          str += " ";
        str += *it;
      }
    return str;
  };

  /**
   * Application layer exchanges data in JSON format
   * JSON is lightweight data-interchange format
   */
  nlohmann::json jsonRes =
      nlohmann::json{{"vehiclePath", road_path_str ()}, {"pathTime", pathWeight}};

  // ReplyMsgInterest
  auto data = std::make_shared<ndn::Data> (interest.getName ());
  data->setFreshnessPeriod (ndn::time::milliseconds (m_defaultFreshnessPeriod));
  //data->setContent (reinterpret_cast<const uint8_t *> (road_path_str ().c_str ()), road_path_str ().size ());
  data->setContent (reinterpret_cast<const uint8_t *> (jsonRes.dump ().data ()),
                    jsonRes.dump ().size ());

  m_keyChain.sign (*data, m_signingInfo);
  m_face.put (*data);

  MYLOG_INFO ("ITS data message sent: " << interest.getName ());
}

void
ItsRsu::PrintFib ()
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
ItsRsu::RegisterPrefixes ()
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

      Name appHelloPrefix = Name (kRsuBeaconPrefix);
      appHelloPrefix.append (kRsuHelloType);
      MYLOG_DEBUG ("FibHelper::AddRoute prefix=" << appHelloPrefix
                                                 << " via faceId=" << face->getId ());
      // add Fib entry for properly faceId
      FibHelper::AddRoute (thisNode, appHelloPrefix, face, metric);
    }
}

int
ItsRsu::CalcTimeWindow (int window_size_in_seconds)
{
  if (window_size_in_seconds < 0)
    return -1;
  return (int) ns3::Simulator::Now ().GetSeconds () / window_size_in_seconds;
}

void
ItsRsu::UpdateRoadTrafficDatabase ()
{
  using namespace ns3;
  using namespace ns3::ndn;

  MYLOG_INFO ("Updating road-traffic database!");
  std::map<std::string, double> graph_edgeWeights;
  m_graphMap->getAllEdgeWeights (graph_edgeWeights);

  //updating graph
  for (auto const &g : graph_edgeWeights)
    graph_edgeWeights[g.first] = m_traci->TraCIAPI::edge.getTraveltime (g.first);
  m_graphMap->setAllEdgeWeights (graph_edgeWeights);

  ns3::Ptr<ns3::UniformRandomVariable> delay = ns3::CreateObject<ns3::UniformRandomVariable> ();
  m_scheduler.schedule (
      time::milliseconds (10 * m_defaultInterval + delay->GetInteger (50.0, 200.0)),
      [this] { UpdateRoadTrafficDatabase (); });
}

} // namespace its
} // namespace ndn