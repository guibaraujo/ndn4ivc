#include "node-neighbour.h"

namespace ndn {

NodeNeighbour::NodeNeighbour ()
{
}

void
NodeNeighbour::SetId (uint32_t id)
{
  m_id = id;
}

uint32_t
NodeNeighbour::GetId ()
{
  return m_id;
}

} // namespace ndn