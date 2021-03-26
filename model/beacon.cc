/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <ns3/beacon.h>

#include <ns3/ptr.h>
#include <limits>
#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/node-list.h>
#include <ns3/ndnSIM/helper/ndn-stack-helper.hpp>
#include <ndn-cxx/lp/tags.hpp>

#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"

NS_LOG_COMPONENT_DEFINE ("ndn.Beacon");

namespace ndn {

Beacon::Beacon (uint32_t frequency)
    // these should appear in the same order as they appear in the class definition
    : m_scheduler (m_face.getIoService ()),
      m_rand (ns3::CreateObject<ns3::UniformRandomVariable> ()),
      m_seq (0),
      m_frequency (frequency)
{
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());

  RegisterPrefixes ();
  m_scheduler.schedule (ndn::time::seconds (1), [this] { PrintFib (); });

  m_face.setInterestFilter (BEACONPREFIX, std::bind (&Beacon::ProcessInterest, this, _2),
                            std::bind ([] {}), std::bind ([] {}));

  // if (thisNode->GetId () == 0)
  SendBeaconInterest ();
}

uint64_t
Beacon::ExtractIncomingFace (const ndn::Interest &interest)
{
  /** Incoming Face Indication
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

void
Beacon::PrintFib ()
{
  using namespace ns3;
  using namespace ns3::ndn;

  Ptr<Node> thisNode = NodeList::GetNode (Simulator::GetContext ());
  const ::nfd::Fib &fib = thisNode->GetObject<L3Protocol> ()->getForwarder ()->getFib ();
  NS_LOG_DEBUG ("FIB Size: " << fib.size ());
  for (const ::nfd::fib::Entry &fibEntry : fib)
    {
      std::string s;
      for (const auto &nh : fibEntry.getNextHops ())
        s += std::to_string (nh.getFace ().getId ()) + ",";
      NS_LOG_DEBUG ("MyFIB: prefix=" << fibEntry.getPrefix () << " via faceIdList=" << s);
    }
}

void
Beacon::ProcessInterest (const ndn::Interest &interest)
{
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
  std::cout << "\t___OI___ respondToAnyInterest meu id:" << thisNode->GetId () << " "
            << interest.getName () << std::endl;

  uint64_t inFaceId = ExtractIncomingFace (interest);
  std::cout << "\t___inFaceId___" << inFaceId << std::endl;
  if (!inFaceId)
    {
      NS_LOG_DEBUG ("Discarding Interest from internal face: " << interest);
      return;
    }
  NS_LOG_INFO ("Interest: " << interest << " inFaceId=" << inFaceId);

  const ndn::Name interestName (interest.getName ());
  if (BEACONPREFIX.isPrefixOf (interestName))
    return OnBeaconInterest (interest, inFaceId);
}

void
Beacon::OnBeaconInterest (const ndn::Interest &interest, uint64_t inFaceId)
{
  const ndn::Name interestName (interest.getName ());
  NS_LOG_INFO ("Received HELLO Interest " << interestName);
}

void
Beacon::RegisterPrefixes ()
{
  using namespace ns3;
  using namespace ns3::ndn;

  int32_t metric = 0; // should it be 0 or std::numeric_limits<int32_t>::max() ??
  Ptr<Node> thisNode = NodeList::GetNode (Simulator::GetContext ());
  NS_LOG_DEBUG ("THIS node is: " << thisNode->GetId ());

  for (uint32_t deviceId = 0; deviceId < thisNode->GetNDevices (); deviceId++)
    {
      Ptr<NetDevice> device = thisNode->GetDevice (deviceId);
      Ptr<L3Protocol> ndn = thisNode->GetObject<L3Protocol> ();
      NS_ASSERT_MSG (ndn != nullptr, "Ndn stack should be installed on the node");

      auto face = ndn->getFaceByNetDevice (device);
      NS_ASSERT_MSG (face != nullptr, "There is no face associated with the net-device");

      NS_LOG_DEBUG ("FibHelper::AddRoute prefix=" << BEACONPREFIX
                                                  << " via faceId=" << face->getId ());
      FibHelper::AddRoute (thisNode, BEACONPREFIX, face, metric);

      auto addr = device->GetAddress ();
      if (Mac48Address::IsMatchingType (addr))
        {
          m_macaddr = boost::lexical_cast<std::string> (Mac48Address::ConvertFrom (addr));
          NS_LOG_DEBUG ("Node " << thisNode->GetId () << " >> MAC=" << m_macaddr << " saved!");
        }
    }
}

void
Beacon::run ()
{
  m_face.processEvents ();
}

void
Beacon::SendBeaconInterest ()
{
  /* First of all, cancel any previously scheduled events */
  send_beacon_event.cancel ();

  Name name = Name (BEACONPREFIX);
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
  
  // name schema /localhop/beacon/<sender-id>/<pos-x>/<pos-y>/<pos-z>/
  name.append (std::to_string (thisNode->GetId ())); // sender-id
  name.append (std::to_string (thisNode->GetId ())); // pos x
  name.append (std::to_string (thisNode->GetId ())); // pos y
  name.append (std::to_string (thisNode->GetId ())); // pos z
  
  NS_LOG_INFO ("Sending Interest " << name);

  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (name);
  interest.setCanBePrefix (false);
  interest.setInterestLifetime (time::milliseconds (0));

  interest.setApplicationParameters (reinterpret_cast<const uint8_t *> (m_macaddr.data ()),
                                     m_macaddr.size ());

  m_face.expressInterest (
      interest, [] (const Interest &, const Data &) {}, [] (const Interest &, const lp::Nack &) {},
      [] (const Interest &) {});

  send_beacon_event =
      m_scheduler.schedule (time::milliseconds (m_frequency), [this] { SendBeaconInterest (); });
}

void
Beacon::setBeaconInterval (uint32_t frequencyInMillisecond)
{
  m_frequency = frequencyInMillisecond;
}

} // namespace ndn
