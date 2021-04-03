#include "neighbor-info.h"

namespace ndn {

NeighborInfo::NeighborInfo (uint32_t id) : m_id (id)
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
NeighborInfo::SetMac (const ns3::Mac48Address &mac)
{
  m_mac = mac;
}

ns3::Mac48Address
NeighborInfo::GetMac ()
{
  return m_mac;
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