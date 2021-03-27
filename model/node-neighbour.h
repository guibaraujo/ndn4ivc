/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef NODE_NEIGHBOUR_H
#define NODE_NEIGHBOUR_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-phy.h"

#include <vector>

namespace ndn {

class NodeNeighbour
{
private:
  uint32_t m_id;
  ns3::Mac48Address m_mac;
  ns3::Time m_lastBeacon;
  uint64_t m_faceId;
  ns3::SignalNoiseDbm m_signalNoise;

public:
  NodeNeighbour ();
  void SetId(uint32_t id);
  uint32_t GetId();
};

} // namespace ndn

#endif // NODE_NEIGHBOUR_H