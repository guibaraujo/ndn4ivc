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

#include "ns3/traci-module.h"
#include "ns3/netanim-module.h"

#include <functional>
#include <stdlib.h>
#include <stdio.h>

#define YELLOW_CODE "\033[33m"
#define RED_CODE "\033[31m"
#define BLUE_CODE "\033[34m"
#define BOLD_CODE "\033[1m"
#define CYAN_CODE "\033[36m"
#define END_CODE "\033[0m"

#define SUMOSCENARIO "intersection"

#define SHELLSCRIPT \
  "\
#/bin/bash \n\
echo $1 \n\
echo `cat contrib/ndn4ivc/traces/" SUMOSCENARIO "/routes.rou.xml |grep \"vehicle id\"|wc -l` \n\
"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("vndn-example-beacon");
NS_OBJECT_ENSURE_REGISTERED (BeaconApp);

namespace ns3 {

std::string
exec (const char *cmd)
{
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype (&pclose)> pipe (popen (cmd, "r"), pclose);
  if (!pipe)
    {
      throw std::runtime_error ("popen() failed!");
    }
  while (fgets (buffer.data (), buffer.size (), pipe.get ()) != nullptr)
    {
      result += buffer.data ();
    }
  return result;
}

int
main (int argc, char *argv[])
{
  std::cout << CYAN_CODE << BOLD_CODE << "Starting simulation... " END_CODE << std::endl;

  // conf default values
  uint32_t nNodes = std::stoi (exec (SHELLSCRIPT));
  uint32_t beaconInterval = 1000;
  uint32_t simTime = 600;
  bool enablePcap = false;
  bool enableLog = true;

  std::cout << "# nodes: " << nNodes << std::endl;

  // command line attibutes
  CommandLine cmd;
  cmd.AddValue ("i", "Beacon interval (milliseconds)", beaconInterval);
  cmd.AddValue ("s", "Simulation time (seconds)", simTime);
  cmd.AddValue ("pcap", "Enable PCAP", enablePcap);
  cmd.AddValue ("log", "Enable Log", enableLog);
  cmd.Parse (argc, argv);

  if (enableLog)
    {
      LogComponentEnable ("vndn-example-beacon", LOG_LEVEL_INFO);
      LogComponentEnable ("ndn.Beacon", LOG_LEVEL_DEBUG);
      LogComponentEnable ("TraciClient", LOG_LEVEL_INFO);
    }

  // create node pool and counter; large enough to cover all sumo vehicles
  NodeContainer nodePool;
  nodePool.Create (nNodes);
  uint32_t nodeCounter (0);

  // install Wifi & set up
  NS_LOG_INFO ("Installing devices");
  ndn::WifiSetupHelper wifi;
  NetDeviceContainer devices = wifi.ConfigureDevices (nodePool, enableLog);

  /* 
  SCH1 172 SCH2 174 SCH3 176
  CCH  178
  SCH4 180 SCH5 182 SCH6 184
  
  set IEEE 80211p Channel
 */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelNumber",
               ns3::UintegerValue (SCH3));

  // NDN stack
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll ();
  // Forwarding strategy
  ndn::StrategyChoiceHelper::Install (nodePool, "/", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::Install (nodePool, "/localhop/beacon",
                                      "/localhost/nfd/strategy/localhop");

  // install mobility model
  NS_LOG_INFO ("Setting up mobility");

  /*** setup mobility and position to node pool ***/
  MobilityHelper mobility;
  Ptr<UniformDiscPositionAllocator> positionAlloc = CreateObject<UniformDiscPositionAllocator> ();
  positionAlloc->SetX (0.0);
  positionAlloc->SetY (1500.0);
  positionAlloc->SetRho (25.0);
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodePool);

  /*** setup Traci and start SUMO ***/
  Ptr<TraciClient> sumoClient = CreateObject<TraciClient> ();
  sumoClient->SetAttribute ("SumoConfigPath",
                            StringValue ("contrib/ndn4ivc/traces/" SUMOSCENARIO "/sim.sumocfg"));
  sumoClient->SetAttribute ("SumoBinaryPath", StringValue ("")); // use system installation of sumo
  sumoClient->SetAttribute ("SynchInterval", TimeValue (Seconds (0.1)));
  sumoClient->SetAttribute ("StartTime", TimeValue (Seconds (0.0)));
  sumoClient->SetAttribute ("SumoGUI", BooleanValue (true));
  sumoClient->SetAttribute ("SumoPort", UintegerValue (3400));
  sumoClient->SetAttribute ("PenetrationRate",
                            DoubleValue (1.0)); // portion of vehicles equipped with wifi
  sumoClient->SetAttribute ("SumoLogFile", BooleanValue (true));
  sumoClient->SetAttribute ("SumoStepLog", BooleanValue (false));
  sumoClient->SetAttribute ("SumoSeed", IntegerValue (10));
  sumoClient->SetAttribute ("SumoAdditionalCmdOptions", StringValue ("--verbose true"));
  sumoClient->SetAttribute ("SumoWaitForSocket", TimeValue (Seconds (1.0)));

  // installing beacon-app
  NS_LOG_INFO ("Installing Beacon application");

  ApplicationContainer beaconContainer;
  ndn::AppHelper beaconHelper ("BeaconApp");
  beaconHelper.SetAttribute ("Frequency", UintegerValue (beaconInterval)); // in milliseconds
  beaconContainer.Add (beaconHelper.Install (nodePool.Get (0)));
  beaconContainer.Add (beaconHelper.Install (nodePool.Get (1)));

  /** Define callback function for node creation */
  std::function<Ptr<Node> ()> setupNewWifiNode = [&] () -> Ptr<Node> {
    if (nodeCounter >= nodePool.GetN ())
      NS_FATAL_ERROR ("Node Pool empty!: " << nodeCounter << " nodes created.");

    // don't create and install the protocol stack of the node at simulation time -> take from "node pool"
    Ptr<Node> includedNode = nodePool.Get (nodeCounter);
    ++nodeCounter; // increment counter for next node

    return includedNode;
  };

  /** Define callback function for node shutdown */
  std::function<void (Ptr<Node>)> shutdownWifiNode = [] (Ptr<Node> exNode) {
    // set position outside communication range
    Ptr<ConstantPositionMobilityModel> mob = exNode->GetObject<ConstantPositionMobilityModel> ();
    mob->SetPosition (Vector (-100.0 + (rand () % 25), 320.0 + (rand () % 25),
                              250.0)); // rand() for visualization purposes

    // NOTE: further actions could be required for a save shut down!
  };

  // start traci client with given function pointers
  sumoClient->SumoSetup (setupNewWifiNode, shutdownWifiNode);

  /*** Setup and Start Simulation + Animation ***/
  //AnimationInterface anim ("contrib/vndn-samples/traces/circle-simple/circle.sumo.xml"); // Mandatory

  std::cout << YELLOW_CODE << BOLD_CODE << "Simulation is running: " END_CODE << std::endl;
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  std::cout << RED_CODE << BOLD_CODE << "Post simulation: " END_CODE << std::endl;

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