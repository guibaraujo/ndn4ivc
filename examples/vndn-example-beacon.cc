#include "ns3/beacon.h"
#include "ns3/beacon-app.h"

#include "ns3/wave-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-setup-helper.h"

#define YELLOW_CODE "\033[33m"
#define TEAL_CODE "\033[36m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("vndn-example-beacon");
NS_OBJECT_ENSURE_REGISTERED (BeaconApp);

namespace ns3 {

int
main (int argc, char *argv[])
{
  std::cout << TEAL_CODE << BOLD_CODE << "Starting simulation... " END_CODE << std::endl;

  // conf default values
  CommandLine cmd;
  uint32_t nNodes = 2;
  double simTime = 3;
  double interval = 0.5;
  bool enablePcap = false;
  bool enableLog = true;

  // command line attibutes
  cmd.AddValue ("i", "Beacon interval in seconds", interval);
  cmd.AddValue ("pcap", "Enable PCAP", enablePcap);
  cmd.AddValue ("log", "Enable Log", enableLog);

  cmd.Parse (argc, argv);

  if (enableLog)
    {
      LogComponentEnable ("vndn-example-beacon", LOG_LEVEL_INFO);
      LogComponentEnable ("ndn.Beacon", LOG_LEVEL_DEBUG);
    }

  NodeContainer nodes;
  nodes.Create (nNodes);

  // install mobility model
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      // set initial positions and velocities
      NS_LOG_INFO ("Setting up mobility for node " << i);
      Ptr<ConstantPositionMobilityModel> cvmm =
          DynamicCast<ConstantPositionMobilityModel> (nodes.Get (i)->GetObject<MobilityModel> ());
      cvmm->SetPosition (Vector (20 + i * 5, 20 + (i % 2) * 5, 0));
    }

  // install Wifi & set up
  ndn::WifiSetupHelper wifi;
  NetDeviceContainer devices = wifi.ConfigureDevices (nodes, enableLog);

  // install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll ();

  // choosing forwarding strategy
  ndn::StrategyChoiceHelper::Install (nodes, "/", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::Install (nodes, "/localhop/beacon",
                                      "/localhost/nfd/strategy/localhop");

  // installing beacon-app
  NS_LOG_INFO ("Installing Beacon Application");

  ApplicationContainer beaconContainer;
  ndn::AppHelper beaconHelper ("BeaconApp");
  beaconHelper.SetAttribute ("Frequency", StringValue ("1000")); // in milliseconds
  beaconContainer.Add (beaconHelper.Install (nodes.Get (0)));
  beaconContainer.Add (beaconHelper.Install (nodes.Get (1)));

  // producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/prefix/");
  producerHelper.SetAttribute ("PayloadSize", StringValue ("1024"));
  //producerHelper.Install (nodes.Get (0));
  //producerHelper.Install (nodes.Get (1));

  /* 
  SCH1 172 SCH2 174 SCH3 176
  CCH  178
  SCH4 180 SCH5 182 SCH6 184
  
  set IEEE 80211p Channel
 */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelNumber",
               ns3::UintegerValue (SCH3));

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  std::cout << YELLOW_CODE << BOLD_CODE << "Post simulation: " END_CODE << std::endl;

  Simulator::Destroy ();

  return 0;
};
} // namespace ns3

int
main (int argc, char *argv[])
{
  std::system ("clear");
  return ns3::main (argc, argv);
}