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
#include <ndn-cxx/util/time.hpp>

#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"

#include "ns3/mobility-model.h"

#include <vector>
#include <stdlib.h>
#include <stdio.h>

NS_LOG_COMPONENT_DEFINE ("ndn.Beacon");

namespace ndn {

Beacon::Beacon (uint32_t frequency, ns3::Ptr<ns3::TraciClient> &traci)
    : m_scheduler (m_face.getIoService ()),
      m_rand (ns3::CreateObject<ns3::UniformRandomVariable> ()),
      m_seq (0),
      m_frequency (frequency),
      m_lastSpeedChange (0),
      m_traci (traci)
{
  RegisterPrefixes ();
  m_scheduler.schedule (ndn::time::seconds (1), [this] { PrintFib (); });
  m_face.setInterestFilter (BEACONPREFIX, std::bind (&Beacon::ProcessInterest, this, _2),
                            std::bind ([] {}), std::bind ([] {}));
}

uint64_t
Beacon::ExtractIncomingFace (const ndn::Interest &interest)
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
Beacon::isValidBeacon (const Name &name, NeighborInfo &neighbor)
{
  try
    {
      /*(std::string)???*/
      neighbor.SetId (std::stoi (name.get (0).toUri ())); //<node-type>
      neighbor.SetType (name.get (1).toUri ()); //<node-id>
      neighbor.SetRoad (name.get (2).toUri ()); //<road-id>

      double x, y, z;
      x = std::stod (name.get (3).toUri ());
      y = std::stod (name.get (4).toUri ());
      z = std::stod (name.get (5).toUri ());
      neighbor.SetPosition (ns3::Vector (x, y, z)); //<pos-x>/<pos-y>/<pos-z>
  } catch (const std::exception &e)
    {
      return false;
  }
  return true;
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
      std::string str = "";
      for (const auto &nh : fibEntry.getNextHops ())
        str += std::to_string (nh.getFace ().getId ()) + ",";
      NS_LOG_DEBUG ("MyFIB: prefix=" << fibEntry.getPrefix () << " via faceIdList=" << str);
    }
}

void
Beacon::PrintNeighbors ()
{
  NS_LOG_DEBUG ("Number (#) of known neighbors: " << m_neighbors.size ());
  for (auto it = m_neighbors.begin (); it != m_neighbors.end (); ++it)
    {
      NS_LOG_DEBUG ("Neighbor=" << it->second.GetId () << ", type=" << it->second.GetType ()
                                << ", last seen at simtime=" << it->second.GetLastBeacon ());
    }
}

void
Beacon::setBeaconInterval (uint32_t frequencyInMillisecond)
{
  m_frequency = frequencyInMillisecond;
}

void
Beacon::ProcessInterest (const ndn::Interest &interest)
{
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
  uint64_t inFaceId = ExtractIncomingFace (interest);
  NS_LOG_DEBUG ("Receiving a interest from face " << inFaceId << ": " << interest.getName ());
  if (!inFaceId)
    {
      NS_LOG_DEBUG ("Discarding interest from internal face " << inFaceId);
      return;
    }
  if (BEACONPREFIX.isPrefixOf (interest.getName ()))
    return OnBeaconInterest (interest, inFaceId);
}

void
Beacon::OnBeaconInterest (const ndn::Interest &interest, uint64_t inFaceId)
{
  NS_LOG_DEBUG ("Processing a beacon from incomming face " << inFaceId);
  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());
  NeighborInfo neighbor = NeighborInfo ();

  // working with specific name prefix Uri
  if (!isValidBeacon (interest.getName ().getSubName (BEACONPREFIX.size (), 6).toUri (), neighbor))
    {
      NS_LOG_INFO ("Beacon invalid, ignoring...");
      return;
    }

  // Beacon is ok, continue...
  std::string neighborSpeed;
  neighborSpeed.assign ((char *) interest.getApplicationParameters ().value (),
                        interest.getApplicationParameters ().value_size ());
  neighbor.SetSpeed (stof (neighborSpeed));
  neighbor.SetLastBeacon ((ns3::Time) ns3::Simulator::Now ().GetMilliSeconds ());

  if (m_traci->TraCIAPI::vehicle.getVehicleClass (m_traci->GetVehicleId (thisNode))
              .compare ("emergency") != 0 &&
      neighbor.GetType ().compare ("emergency") == 0)
    {
      NS_LOG_DEBUG (
          "Type=" << m_traci->TraCIAPI::vehicle.getVehicleClass (m_traci->GetVehicleId (thisNode)));

      std::string myRoad = m_traci->TraCIAPI::vehicle.getRoadID (m_traci->GetVehicleId (thisNode));
      if (myRoad.compare (neighbor.GetRoad ()) == 0) // if emergency vehicle in the same road
        {
          int numLanes = m_traci->TraCIAPI::edge.getLaneNumber (myRoad);
          if (numLanes > 1)
            {
              int laneIndex = 0;
              int durationInSeconds = 5;
              m_traci->TraCIAPI::vehicle.changeLane (m_traci->GetVehicleId (thisNode), laneIndex,
                                                     durationInSeconds);



              m_lastSpeedChange = (ns3::Time) ns3::Simulator::Now ().GetMilliSeconds ();
            }
        }
    }
  else
    {
      std::string strLaneId =
          m_traci->TraCIAPI::vehicle.getLaneID (m_traci->GetVehicleId (thisNode));
      //m_traci->TraCIAPI::vehicle.setSpeed (m_traci->GetVehicleId (thisNode),
      //m_traci->TraCIAPI::lane.getMaxSpeed (strLaneId));
    }

  if (m_neighbors.find (neighbor.GetId ()) == m_neighbors.end ())
    {
      NS_LOG_DEBUG ("New neighbor (NodeId=" << neighbor.GetId () << ") detected!");
      m_neighbors.emplace (neighbor.GetId (), neighbor);
    }
  else
    {
      NS_LOG_DEBUG ("Neighbor (NodeId=" << neighbor.GetId ()
                                        << ") is already known!! Updating info...");
      m_neighbors.at (neighbor.GetId ()) = neighbor;
      //m_neighbors.erase (neighbor.GetId ());
      //m_neighbors.emplace (neighbor.GetId (), NeighborInfo (neighbor.GetId ()));
    }
}

void
Beacon::SendBeaconInterest ()
{
  /* cancel any previously scheduled events */
  m_sendBeaconEvent.cancel ();
  ++m_seq; // just for control

  ns3::Ptr<ns3::Node> thisNode = ns3::NodeList::GetNode (ns3::Simulator::GetContext ());

  // check speed change
  if (m_lastSpeedChange != 0 &&
      (ns3::Time) ns3::Simulator::Now ().GetMilliSeconds () - m_lastSpeedChange > 5 * m_frequency)
    m_traci->TraCIAPI::vehicle.setSpeed (
        m_traci->GetVehicleId (thisNode),
        m_traci->TraCIAPI::vehicle.getMaxSpeed (m_traci->GetVehicleId (thisNode)));

  // name schema Beacon App >> /localhop/beacon/<node-id>/<node-type>/<road-id>/<pos-x>/<pos-y>/<pos-z>/<speed>
  Name name = Name (BEACONPREFIX);
  name.append (std::to_string (thisNode->GetId ())); //<node-id>
  name.append (
      m_traci->TraCIAPI::vehicle.getVehicleClass (m_traci->GetVehicleId (thisNode))); //<node-type>
  //name.append (m_traci->TraCIAPI::vehicle.getRoadID (
  //    m_traci->GetVehicleId (ns3::NodeList::GetNode (thisNode->GetId ())))); //changed
  name.append (m_traci->TraCIAPI::vehicle.getRoadID (m_traci->GetVehicleId (thisNode))); //<road-id>

  name.append (
      std::to_string (thisNode->GetObject<ns3::MobilityModel> ()->GetPosition ().x)); ///<pos-x>
  name.append (
      std::to_string (thisNode->GetObject<ns3::MobilityModel> ()->GetPosition ().y)); //<pos-y>
  name.append (
      std::to_string (thisNode->GetObject<ns3::MobilityModel> ()->GetPosition ().z)); //<pos-z>

  Interest interest = Interest ();
  interest.setNonce (m_rand->GetValue (0, std::numeric_limits<uint32_t>::max ()));
  interest.setName (name);
  interest.setCanBePrefix (false);
  interest.setInterestLifetime (time::milliseconds (0));

  std::string strSpeed =
      std::to_string (m_traci->TraCIAPI::vehicle.getSpeed (m_traci->GetVehicleId (thisNode)));
  NS_LOG_DEBUG ("Encoding vehicle speed (" << strSpeed
                                           << " m/s) to send via ApplicationParameters");
  interest.setApplicationParameters (reinterpret_cast<const uint8_t *> (strSpeed.data ()),
                                     strSpeed.size ());

  NS_LOG_INFO ("Sending a interest name: " << name);

  m_face.expressInterest (
      interest, [] (const Interest &, const Data &) {}, [] (const Interest &, const lp::Nack &) {},
      [] (const Interest &) {});

  ns3::Ptr<ns3::UniformRandomVariable> delay =
      ns3::CreateObject<ns3::UniformRandomVariable> (); ///< @brief reduce collision

  m_sendBeaconEvent =
      m_scheduler.schedule (time::milliseconds (m_frequency + delay->GetInteger (0.0, 500.0)),
                            [this] { SendBeaconInterest (); });
}

void
Beacon::RegisterPrefixes ()
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

      NS_LOG_DEBUG ("FibHelper::AddRoute prefix=" << BEACONPREFIX
                                                  << " via faceId=" << face->getId ());
      // add Fib entry for BEACONPREFIX with properly faceId
      FibHelper::AddRoute (thisNode, BEACONPREFIX, face, metric);
    }
}

void
Beacon::run ()
{
  m_face.processEvents ();
}

void
Beacon::start ()
{
  SendBeaconInterest ();
}

void
Beacon::stop ()
{
  NS_LOG_DEBUG ("Stopping Beacon application...");
  PrintNeighbors ();
}

} // namespace ndn
