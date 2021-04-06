/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef NODE_NEIGHBOR_H
#define NODE_NEIGHBOR_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-phy.h"

#include <vector>

namespace ndn {

class NeighborInfo
{
private:
  uint32_t m_id;

  ns3::Vector m_position;

  ns3::Mac48Address m_mac;
  ns3::Time m_lastBeacon;

  double m_speed;

  /* future implementation*/
  char m_mainDirection;
  double m_distance;

public:
  NeighborInfo ();
  NeighborInfo (uint32_t id);
  ~NeighborInfo ();

  void SetId (const uint32_t id);
  uint32_t GetId ();

  void SetSpeed (const double speed);
  double GetSpeed ();

  void SetPosition (const ns3::Vector &position);
  ns3::Vector GetPosition ();

  void SetMac (const ns3::Mac48Address &mac);
  ns3::Mac48Address GetMac ();

  void SetLastBeacon (const ns3::Time &lastBeacon);
  ns3::Time GetLastBeacon ();
};

} // namespace ndn

#endif // NODE_NEIGHBOUR_H