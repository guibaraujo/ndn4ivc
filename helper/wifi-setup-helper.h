#ifndef WIFISETUPHELPER_SETUP_H
#define WIFISETUPHELPER_SETUP_H
#include "ns3/core-module.h"
#include "ns3/wave-module.h"
#include "ns3/network-module.h"

namespace ns3 {
namespace ndn {
/** \brief This is a "utility class".
 */
class WifiSetupHelper
{
public:
  WifiSetupHelper ();
  virtual ~WifiSetupHelper ();

  NetDeviceContainer ConfigureDevices (NodeContainer &n, bool enableLog);
};
} // namespace ivts
} // namespace ns3
#endif