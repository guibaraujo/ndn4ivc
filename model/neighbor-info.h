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

  std::string m_type; // @brief bus, delivery, passenger, trailer, truck
  std::string m_road;

  ns3::Vector m_position;

  double m_speed;

  ns3::Time m_lastBeacon;

public:
  NeighborInfo ();
  NeighborInfo (uint32_t id);
  ~NeighborInfo ();

  void SetId (const uint32_t id);
  uint32_t GetId ();

  void SetType (const std::string type);
  std::string GetType ();

  void SetRoad (const std::string road);
  std::string GetRoad ();

  void SetSpeed (const double speed);
  double GetSpeed ();

  void SetPosition (const ns3::Vector &position);
  ns3::Vector GetPosition ();

  void SetLastBeacon (const ns3::Time &lastBeacon);
  ns3::Time GetLastBeacon ();
};

} // namespace ndn

#endif // NODE_NEIGHBOUR_H