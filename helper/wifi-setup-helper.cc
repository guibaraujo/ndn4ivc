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
  // default parameters for propagation delay and propagation loss
  std::string phyMode ("OfdmRate6MbpsBW10MHz");
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  /* 
  If you create applications that control TxPower, define the low & high end of TxPower
	This is done by using 'TxInfo' to set the transmission parameters at the higher layers
	33 dBm is the highest allowed for non-government use and 44.8 dBm is for government use
  Setting them to the same value is the easy way to go - or use by default
  I can instead set TxPowerStart to a value lower than 33, but it's necessary set the 
  number of levels for each PHY - 
 */
  wifiPhy.Set ("TxPowerStart", DoubleValue (33));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (33));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (8));

  /*  
  --- Radio Transmit Power ---
  The signal is measured in decibels. 
  In WiFi, decibels or dBm (deciBels por miliwatt) are measured in negatives.

	This is the trickiest of the 3 parts mainly due to the fact that it can be 
  expressed in Watts, Milliwatts  or dBm. Ideally you want to convert this to a 
  dBm value if not already expressed this way. At this point it is worth noting:
	1mW = 0.001W
	1W = 1000mW
	 
	0dBm = 1mW
	 
	--- Rule 1: The Rule of 3 ---
	 
	If you raise the dBm value by “3″ you double the mW value:
	0dBm = 1mW
	3dBm = 2mW
	6dBm = 4mW
	9dBm = 8mW
	12dBm = 16mW
	15dBm = 32mW
	18dBm = 64mW
	21dBm = 128mW
	24dBm = 256mW
	27dBm = 512mW
	30dBm = 1024mW (5GHz Band B legal limit)
	33dBm = 2048mW
	36dBm = 4096mW (5GHz Band C legal limit)
	 
	This also mean that if you lower the dBm value by “3″ you halve the mW value
	 
	--- Rule 2: The Rule of 10 ---
	 
	If you raise the dBm value by “10″ you multiply the mW value by 10:
	0dBm = 1mW
	10dBm = 10mW
	20dBm = 100mW
	30dBm = 1000mW (5GHz Band B legal limit)
	40dBm = 10000mW (Over the 5GHz Band C legal limit)
	 
	Yup you guessed it, if you lower the dBm value by 10 you divide the mW value by 10
	The above calculations aren’t exact, but they are typically accurate enough to get by on

  -- SNR (Signal to Noise Ratio) --

  So to calculate your SNR value you sub the Signal Value to the Noise Value and it generates 
  a positive number that is expressed in decibels (db)

  EXAMPLE: lets say your Signal value is -55db and your Noise value is -95db
  -55db - -95db = 40db 
  this means that you have an SNR of 40 and any SNR above 20 is good

  [5 dB to 10 dB] is below the minimum level to establish a connection, due to the noise level 
                  being nearly indistinguishable from the desired signal (useful information)
  [10 dB to 15 dB] is the accepted minimum to establish an unreliable connection.
  [15 dB to 25 dB] is typically considered the minimally acceptable level to establish poor connectivity.
  [25 dB to 40 dB] is deemed to be good. 
  [41 dB or higher] is considered to be excellent.
 */

  // use default values or specify
  //wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel", "m0", DoubleValue (1.0), "m1", DoubleValue (1.0), "m2", DoubleValue (1.0));
  //wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (19.0));

  /* sett up MAC LAYER (VANET) */
  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();

  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue (phyMode), "ControlMode", StringValue (phyMode),
                                      "NonUnicastMode", StringValue (phyMode));

  // Set it to adhoc mode if WifiMacHelper
  // wifi80211pMac.SetType ("ns3::AdhocWifiMac");
  // install wifi80211p
  NetDeviceContainer wifiNetDevices = wifi80211p.Install (wifiPhy, wifi80211pMac, nodes);

  if (enablePcap)
    {
      // generates PCAP trace files (*.pcap)
      wifiPhy.EnablePcap ("PCAP", wifiNetDevices);
    }

  return wifiNetDevices;
}
} // namespace ndn
} // namespace ns3
