/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// ███╗░░██╗██████╗░███╗░░██╗░░██╗██╗██╗██╗░░░██╗░█████╗░
// ████╗░██║██╔══██╗████╗░██║░██╔╝██║██║██║░░░██║██╔══██╗
// ██╔██╗██║██║░░██║██╔██╗██║██╔╝░██║██║╚██╗░██╔╝██║░░╚═╝
// ██║╚████║██║░░██║██║╚████║███████║██║░╚████╔╝░██║░░██╗
// ██║░╚███║██████╔╝██║░╚███║╚════██║██║░░╚██╔╝░░╚█████╔╝
// ╚═╝░░╚══╝╚═════╝░╚═╝░░╚══╝░░░░░╚═╝╚═╝░░░╚═╝░░░░╚════╝░
// https://github.com/guibaraujo/NDN4IVC

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

// specify the SUMO scenario put in 'ndn4ivc/traces' directory
#define SUMO_SCENARIO_NAME "intersection"

#define SHELLSCRIPT_NUM_VEHICLES                      \
  "\
#/bin/bash \n\
echo $1 \n\
echo `cat contrib/ndn4ivc/traces/" SUMO_SCENARIO_NAME \
  "/routes.rou.xml |grep \"vehicle id\"|wc -l` \n\
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
    throw std::runtime_error ("popen() failed!");
  while (fgets (buffer.data (), buffer.size (), pipe.get ()) != nullptr)
    result += buffer.data ();
  return result;
}

int
main (int argc, char *argv[])
{
  std::cout << CYAN_CODE << BOLD_CODE << "Starting simulation... " END_CODE << std::endl;
  std::cout << "Selected SUMO scenario: " << SUMO_SCENARIO_NAME << std::endl;

  // conf default values
  uint32_t nNodes = std::stoi (exec (SHELLSCRIPT_NUM_VEHICLES));
  uint32_t beaconInterval = 1000;
  uint32_t simTime = 600;
  bool enablePcap = false;
  bool enableLog = true;
  bool enableSumoGui = false;

  std::cout << "Number of nodes (vehicles) detected in SUMO scenario: " << nNodes << std::endl;

  // command line attibutes
  CommandLine cmd;
  cmd.AddValue ("i", "Beacon interval (milliseconds)", beaconInterval);
  cmd.AddValue ("s", "Simulation time (seconds)", simTime);
  cmd.AddValue ("pcap", "Enable PCAP", enablePcap);
  cmd.AddValue ("log", "Enable Log", enableLog);
  cmd.AddValue ("sumo-gui", "Enable SUMO with graphical user interface", enableSumoGui);
  cmd.Parse (argc, argv);

  if (enableLog)
    {
      LogComponentEnable ("vndn-example-beacon", LOG_LEVEL_INFO);
      LogComponentEnable ("ndn.Beacon", LOG_LEVEL_INFO);
      LogComponentEnable ("TraciClient", LOG_LEVEL_INFO);
    }

  /* create node pool and counter; large enough to cover all sumo vehicles */
  NodeContainer nodePool;
  nodePool.Create (nNodes);
  uint32_t nodeCounter (0);

  // install wifi & set up
  // selecting IEEE 80211p channel for vehicular application
  /** 
   * SCH1 172 SCH2 174 SCH3 176
   * CCH  178
   * SCH4 180 SCH5 182 SCH6 184
   * 
   * Ref.: doi: 10.1109/VETECF.2007.461
   */
  std::cout << "Installing network devices in vehicles... " << std::endl;
  ndn::WifiSetupHelper wifi;
  NetDeviceContainer devices = wifi.ConfigureDevices (nodePool, enableLog);
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelNumber",
               ns3::UintegerValue (SCH3));

  // install mobility model
  std::cout << "Setting up mobility & TraCI... " << std::endl;

  /*** setup mobility and position to node pool ***/
  MobilityHelper mobility;
  Ptr<UniformDiscPositionAllocator> positionAlloc = CreateObject<UniformDiscPositionAllocator> ();
  positionAlloc->SetX (400.0);
  positionAlloc->SetY (1500.0);
  positionAlloc->SetRho (25.0);
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodePool);

  /*** setup Traci and start SUMO ***/
  Ptr<TraciClient> sumoClient = CreateObject<TraciClient> ();
  sumoClient->SetAttribute (
      "SumoConfigPath", StringValue ("contrib/ndn4ivc/traces/" SUMO_SCENARIO_NAME "/sim.sumocfg"));
  sumoClient->SetAttribute ("SumoBinaryPath", StringValue ("")); // use system installation of sumo
  sumoClient->SetAttribute ("SynchInterval", TimeValue (Seconds (0.1)));
  sumoClient->SetAttribute ("StartTime", TimeValue (Seconds (0.0)));
  sumoClient->SetAttribute ("SumoGUI", BooleanValue (enableSumoGui));
  sumoClient->SetAttribute ("SumoPort", UintegerValue (3400));
  sumoClient->SetAttribute ("PenetrationRate",
                            DoubleValue (1.0)); // portion of vehicles equipped with wifi
  sumoClient->SetAttribute ("SumoLogFile", BooleanValue (true));
  sumoClient->SetAttribute ("SumoStepLog", BooleanValue (false));
  sumoClient->SetAttribute ("SumoSeed", IntegerValue (10));
  sumoClient->SetAttribute ("SumoAdditionalCmdOptions", StringValue ("--verbose true"));
  sumoClient->SetAttribute ("SumoWaitForSocket", TimeValue (Seconds (1.0)));

  /** Define the callback function for dynamic node creation from 
   *  Simulation of Urban MObility - SUMO simulator (https://sumo.dlr.de)
   *  
   *  NOTE: ns-3 and SUMO have different behaviors: 
   *  - ns-3 -> static node creation (at the beginning of the simulation t = 0)
   *  - SUMO -> dynamic process
   * 
   *  Install the ns-3 app only when the vehicle has been created by SUMO
   */
  std::function<Ptr<Node> ()> setupNewSumoVehicle = [&] () -> Ptr<Node> {
    if (nodeCounter >= nodePool.GetN ())
      NS_FATAL_ERROR ("Node Pool empty: " << nodeCounter << " nodes created.");

    NS_LOG_INFO ("Node/vehicle " << nodeCounter << " is starting now at "
                                 << ns3::Simulator::Now ());
    Ptr<Node> includedNode = nodePool.Get (nodeCounter);
    ++nodeCounter;
    Ptr<BeaconApp> beaconApp = CreateObject<BeaconApp> ();
    beaconApp->SetAttribute ("Frequency", UintegerValue (beaconInterval)); // in milliseconds
    includedNode->AddApplication (beaconApp);

    return includedNode;
  };

  /** Define callback function for node shutdown
   * 
   *  ns-3 app must be terminated and ns-3 node (vehicle) will be 
   *  put away ('removed') from the simulation scenario
   */
  std::function<void (Ptr<Node>)> shutdownSumoVehicle = [] (Ptr<Node> exNode) {
    Ptr<BeaconApp> c_app = DynamicCast<BeaconApp> (exNode->GetApplication (0));
    c_app->StopApplication ();

    // put the node in new position, outside the simulation communication range
    Ptr<ConstantPositionMobilityModel> mob = exNode->GetObject<ConstantPositionMobilityModel> ();
    mob->SetPosition (Vector (-100.0 + (rand () % 25), 320.0 + (rand () % 25),
                              250.0)); // rand() for visualization purposes

    // NOTE: further actions could be required for a save shutdown!
  };

  sumoClient->SumoSetup (setupNewSumoVehicle, shutdownSumoVehicle);

  // install Ndn stack
  std::cout << "Installing Ndn stack on all nodes in the simulation... " << std::endl;
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll ();
  // forwarding strategy
  ndn::StrategyChoiceHelper::Install (nodePool, "/", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::Install (nodePool, "/localhop/beacon",
                                      "/localhost/nfd/strategy/localhop");

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