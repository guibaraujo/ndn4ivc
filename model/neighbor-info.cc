#include "neighbor-info.h"

namespace ndn {

NeighborInfo::NeighborInfo (uint32_t id) : m_id (id), m_speed (0)
{
}

NeighborInfo::NeighborInfo ()
{
}

NeighborInfo::~NeighborInfo ()
{
}

void
NeighborInfo::SetId (const uint32_t id)
{
  m_id = id;
}

uint32_t
NeighborInfo::GetId ()
{
  return m_id;
}

void
NeighborInfo::SetType (const std::string type)
{
  m_type = type;
}

std::string
NeighborInfo::GetType ()
{
  return m_type;
}

void
NeighborInfo::SetRoad (const std::string road)
{
  m_road = road;
}

std::string
NeighborInfo::GetRoad ()
{
  return m_road;
}

void
NeighborInfo::SetSpeed (const double speed)
{
  m_speed = speed;
}

double
NeighborInfo::GetSpeed ()
{
  return m_speed;
}

void
NeighborInfo::SetPosition (const ns3::Vector &position)
{
  m_position = position;
}

ns3::Vector
NeighborInfo::GetPosition ()
{
  return m_position;
}

void
NeighborInfo::SetLastBeacon (const ns3::Time &lastBeacon)
{
  m_lastBeacon = lastBeacon;
}

ns3::Time
NeighborInfo::GetLastBeacon ()
{
  return m_lastBeacon;
}

} // namespace ndn