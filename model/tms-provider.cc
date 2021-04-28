/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <ns3/tms-provider.h>

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

NS_LOG_COMPONENT_DEFINE ("ndn.TmsProvider");

namespace ndn {

TmsProvider::TmsProvider (uint32_t frequency, ns3::Ptr<ns3::TraciClient> &traci)
    : m_scheduler (m_face.getIoService ()),
      m_rand (ns3::CreateObject<ns3::UniformRandomVariable> ()),
      m_timeWindow (-1),
      m_windowSize (30),
      m_lastInterestSent (0),
      m_traci (traci)
{
  RegisterPrefixes ();
  m_scheduler.schedule (ndn::time::seconds (1), [this] { PrintFib (); });
  m_face.setInterestFilter (TMSPREFIX, std::bind (&TmsProvider::ProcessInterest, this, _2),
                            std::bind ([] {}), std::bind ([] {}));
}

uint64_t
TmsProvider::ExtractIncomingFace (const ndn::Interest &interest)
{
  /**Incoming Face Indication
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

bool
TmsProvider::isValidInterest (const Name &name)
{
  try
    {

  } catch (const std::exception &e)
    {
      return false;
  }
  return true;
}

void
TmsProvider::PrintFib ()
{
  using namespace ns3;
  using namespace ns3::ndn;

  Ptr<Node> thisNode = NodeList::GetNode (Simulator::GetContext ());
  const ::nfd::Fib &fib = thisNode->GetObject<L3Protocol> ()->getForwarder ()->getFib ();
  NS_LOG_DEBUG ("FIB Size: " << fib.size ());
  for (const ::nfd::fib::Entry &fibEntry : fib)
    {
      std::string str = "";
      for (const auto &nh : fibEntry.getNextHops ())
        str += std::to_string (nh.getFace ().getId ()) + ",";
      NS_LOG_DEBUG ("MyFIB: prefix=" << fibEntry.getPrefix () << " via faceIdList=" << str);
    }
}

void
TmsProvider::ProcessInterest (const ndn::Interest &interest)
{
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
  uint64_t inFaceId = ExtractIncomingFace (interest);
  NS_LOG_DEBUG ("Receiving a interest from face " << inFaceId << ": " << interest.getName ());
  if (!inFaceId)
    {
      NS_LOG_DEBUG ("Discarding interest from internal face " << inFaceId);
      return;
    }
  if (TMSPREFIX.isPrefixOf (interest.getName ()))
    return OnInterest (interest, inFaceId);
}

void
TmsProvider::OnInterest (const ndn::Interest &interest, uint64_t inFaceId)
{
  NS_LOG_DEBUG ("Processing an interest from incomming face " << inFaceId);
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());

  m_timeWindow = (int) ns3::Simulator::Now ().GetSeconds () / m_windowSize;
  if (stoi (interest.getName ().at (-1).toUri ()) == m_timeWindow)
    {
      std::string roadId = interest.getName ().at (-2).toUri ();

      double edgeCurrAvgSpeed = m_traci->TraCIAPI::edge.getLastStepMeanSpeed (roadId) /
                                m_traci->TraCIAPI::lane.getMaxSpeed (roadId + "_0");
      double edgeCurrOccupancy = m_traci->TraCIAPI::edge.getLastStepOccupancy (roadId);

      // JSON (JavaScript Object Notation) is an open standard (language-independent data)
      // file format, and a lightweight data-interchange format
      nlohmann::json jDataRes = nlohmann::json{
          {"roadId", roadId},
          {"speedLevel", edgeCurrAvgSpeed},
          {"occupancyLevel", edgeCurrOccupancy},
      };
      // preparing data response
      auto data = make_shared<Data> (interest.getName ());
      // Freshness specifies time in seconds (since Timestamp) for which the content is considered valid
      //data->setFreshnessPeriod (ndn::time::milliseconds (100000));
      //data->setContent (make_shared<::ndn::Buffer> (8800));  // limit 8800 octets (ndn-cxx/encoding/tlv.hpp)
      data->setContent (reinterpret_cast<const uint8_t *> (jDataRes.dump ().data ()),
                        jDataRes.dump ().size ());

      Signature signature;
      SignatureInfo signatureInfo (static_cast<::ndn::tlv::SignatureTypeValue> (255));

      signature.setInfo (signatureInfo);
      signature.setValue (::ndn::makeNonNegativeIntegerBlock (::ndn::tlv::SignatureValue, 0));

      data->setSignature (signature);
      data->wireEncode ();

      NS_LOG_INFO ("RSU(" << thisNode->GetId () << ") responding with Data: " << data->getName ());
      m_face.put (*data);

      /* working with specific name prefix Uri
  if (!isValidInterest (interest.getName ().getSubName (TMSPREFIX.size (), 6).toUri ()))
    {
      NS_LOG_INFO ("Interest invalid, ignoring...");
      return;
    }*/
    }
}

void
TmsProvider::RegisterPrefixes ()
{
  using namespace ns3;
  using namespace ns3::ndn;

  NS_LOG_DEBUG ("Registering prefixes...");

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

      NS_LOG_DEBUG ("FibHelper::AddRoute prefix=" << TMSPREFIX << " via faceId=" << face->getId ());
      // add Fib entry for TMSPREFIX with properly faceId
      FibHelper::AddRoute (thisNode, TMSPREFIX, face, metric);
    }
}

void
TmsProvider::run ()
{
  m_face.processEvents ();
}

void
TmsProvider::start ()
{
  NS_LOG_INFO ("TMS Provider application started!");
}

void
TmsProvider::stop ()
{
  NS_LOG_INFO ("Stopping TMS Provider application...");
}

} // namespace ndn