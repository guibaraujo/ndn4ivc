#include "ns3/wifi-setup-helper.h"

namespace ns3 {
namespace ndn {

WifiSetupHelper::WifiSetupHelper ()
{
}

WifiSetupHelper::~WifiSetupHelper ()
{
}

NetDeviceContainer
WifiSetupHelper::ConfigureDevices (NodeContainer &nodes, bool enablePcap)
{
  std::string phyMode ("OfdmRate6MbpsBW10MHz");
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();

  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);

  // 21dBm ~ 70m 
  // 24dBm ~ 100m
  // 30dBm ~ 150m
  wifiPhy.Set ("TxPowerStart", DoubleValue (21)); //Minimum available transmission level (dbm)
  wifiPhy.Set ("TxPowerEnd", DoubleValue (21)); //Maximum available transmission level (dbm)
  wifiPhy.Set (
      "TxPowerLevels",
      UintegerValue (
          8)); //Number of transmission power levels available between TxPowerStart and TxPowerEnd included

  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  wifi80211pMac.SetType (
      "ns3::OcbWifiMac"); //  in IEEE80211p MAC does not require any association between devices (similar to an adhoc WiFi MAC)...

  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue (phyMode), "ControlMode", StringValue (phyMode),
                                      "NonUnicastMode", StringValue (phyMode));
  NetDeviceContainer wifiNetDevices = wifi80211p.Install (wifiPhy, wifi80211pMac, nodes);

  if (enablePcap)
    wifiPhy.EnablePcap ("PCAP", wifiNetDevices);

  return wifiNetDevices;
}
} // namespace ndn
} // namespace ns3
