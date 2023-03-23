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
  /** Propagationa = TwoRayGroundPropagationLossModel && ConstantSpeedPropagationDelayModel***
  * 21dBm ~ 75m (radio coverage)
  * 33.8dbm ~ 200m  (radio coverage)
  * 45.6dbm~ 500m  (radio coverage)
  */
  double txPower_dBm = 45.6; //dBm
  double MinDistance = 500;

  /* Propagation loss models >> implemented:
  - Cost231PropagationLossModel
  - FixedRssLossModel
  - FriisPropagationLossModel
  - ItuR1411LosPropagationLossModel
  - ItuR1411NlosOverRooftopPropagationLossModel
  - JakesPropagationLossModel
  - Kun2600MhzPropagationLossModel
  - LogDistancePropagationLossModel
  - MatrixPropagationLossModel
  - NakagamiPropagationLossModel
  - OkumuraHataPropagationLossModel
  - RandomPropagationLossModel
  - RangePropagationLossModel
  - ThreeLogDistancePropagationLossModel
  - TwoRayGroundPropagationLossModel

  More info: https://coe.northeastern.edu/Research/krclab/crens3-doc/group___attribute_list.html
  
  ns3::TwoRayGroundPropagationLossModel
    Frequency: The carrier frequency (in Hz) at which propagation occurs (default is 5.15 GHz).
    SystemLoss: The system loss
    MinDistance: The distance under which the propagation model refuses to give results (m)
    HeightAboveZ: The height of the antenna (m) above the node's Z coordinate
    .
    .
    .
  ns3::FriisPropagationLossModel
    Frequency: The carrier frequency (in Hz) at which propagation occurs (default is 5.15 GHz).
    SystemLoss: The system loss
    MinDistance: The distance under which the propagation model refuses to give results (m)
  */

  std::string phyMode ("OfdmRate6MbpsBW10MHz");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel", "HeightAboveZ",
                                  DoubleValue (1.5), "SystemLoss", DoubleValue (1), "MinDistance",
                                  DoubleValue (MinDistance));

  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);

  wifiPhy.Set ("TxPowerStart",
               DoubleValue (txPower_dBm)); //Minimum available transmission level (dbm)
  wifiPhy.Set ("TxPowerEnd",
               DoubleValue (txPower_dBm)); //Maximum available transmission level (dbm)
  //Number of transmission power levels available between TxPowerStart and TxPowerEnd included (default 8)
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));

  //wifiPhy.Set ("RxGain", DoubleValue (1));
  //wifiPhy.Set ("ShortPlcpPreambleSupported", BooleanValue(true) );
  //wifiPhy.Set ("TxPowerEnd", DoubleValue (16) );
  //wifiPhy.Set ("TxPowerStart", DoubleValue(16) );
  //wifiPhy.Set ("TxPowerLevels", UintegerValue(1) );
  //wifiPhy.Set ("TxGain", DoubleValue(1));
  //wifiPhy.Set ("Frequency", UintegerValue(5880)); //CH176
  //wifiPhy.Set ("ChannelWidth", UintegerValue(20));
  //wifiPhy.Set ("EnergyDetectionThreshold",DoubleValue(-96));
  //wifiPhy.Set ("RxNoiseFigure", DoubleValue(7));
  //wifiPhy.Set ("TxAntennas", UintegerValue(1));
  //wifiPhy.Set ("RxAntennas", UintegerValue(5));
  //wifiPhy.Set ("Antennas", UintegerValue(1));
  //wifiPhy.Set ("ShortGuardEnabled", BooleanValue(true));

  wifiPhy.SetChannel (wifiChannel.Create ());

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
