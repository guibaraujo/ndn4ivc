/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// ███╗░░██╗██████╗░███╗░░██╗░░██╗██╗██╗██╗░░░██╗░█████╗░
// ████╗░██║██╔══██╗████╗░██║░██╔╝██║██║██║░░░██║██╔══██╗
// ██╔██╗██║██║░░██║██╔██╗██║██╔╝░██║██║╚██╗░██╔╝██║░░╚═╝
// ██║╚████║██║░░██║██║╚████║███████║██║░╚████╔╝░██║░░██╗
// ██║░╚███║██████╔╝██║░╚███║╚════██║██║░░╚██╔╝░░╚█████╔╝
// ╚═╝░░╚══╝╚═════╝░╚═╝░░╚══╝░░░░░╚═╝╚═╝░░░╚═╝░░░░╚════╝░
// https://github.com/guibaraujo/NDN4IVC

#include "ns3/tms-consumer.h"
#include "ns3/tms-consumer-app.h"
#include "ns3/tms-provider.h"
#include "ns3/tms-provider-app.h"

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
#include <exception>
#include <vector>

#define YELLOW_CODE "\033[33m"
#define RED_CODE "\033[31m"
#define BLUE_CODE "\033[34m"
#define BOLD_CODE "\033[1m"
#define CYAN_CODE "\033[36m"
#define END_CODE "\033[0m"

// specify the SUMO scenario put in 'ndn4ivc/traces' directory
//#define SUMO_SCENARIO_NAME "intersection"
//#define SUMO_SCENARIO_NAME "highway"
#define SUMO_SCENARIO_NAME "grid"
//#define SUMO_SCENARIO_NAME "osm-openstreetmap"
//#define SUMO_SCENARIO_NAME "multi-lane"

#define SHELLSCRIPT_NUM_VEHICLES \
  "\
#/bin/bash \n\
#echo $1 \n\
echo `cat contrib/ndn4ivc/traces/" SUMO_SCENARIO_NAME "/*.rou.xml |grep 'vehicle id'|wc -l` \n\
"

#define SHELLSCRIPT_SUMOMAP_BOUNDARIES                \
  "\
#/bin/bash \n\
#echo $1 \n\
echo `cat contrib/ndn4ivc/traces/" SUMO_SCENARIO_NAME \
  "/*.net.xml |grep '<location'|cut -d '=' -f3|cut -d '\"' -f2` \n\
"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("vndn-example-tms");

NS_OBJECT_ENSURE_REGISTERED (TmsConsumerApp);
NS_OBJECT_ENSURE_REGISTERED (TmsProviderApp);

namespace ns3 {

std::string
exec (const char *cmd)
{
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype (&pclose)> pipe (popen (cmd, "r"), pclose);
  if (!pipe)
    throw std::runtime_error ("exec failed!");
  while (fgets (buffer.data (), buffer.size (), pipe.get ()) != nullptr)
    result += buffer.data ();
  return result;
}

vector<double>
splitSumoMapBoundaries (std::string s, std::string delimiter)
{
  size_t posStart = 0, posEnd, delimLen = delimiter.length ();
  string token;
  vector<double> res;

  while ((posEnd = s.find (delimiter, posStart)) != string::npos)
    {
      token = s.substr (posStart, posEnd - posStart);
      posStart = posEnd + delimLen;
      res.push_back (std::stof (token));
    }

  res.push_back (std::stof (s.substr (posStart)));
  return res;
}

int
main (int argc, char *argv[])
{
  std::cout << CYAN_CODE << BOLD_CODE << "Starting simulation... " END_CODE << std::endl;

  uint32_t nVehicles = std::stoi (exec (SHELLSCRIPT_NUM_VEHICLES));
  // Getting the network boundary for 2D SUMO map >> coordinate C1(x1,y1) and coordinate C2(x2,y2)
  vector<double> sumoMapBoundaries =
      splitSumoMapBoundaries (exec (SHELLSCRIPT_SUMOMAP_BOUNDARIES), ",");

  std::cout << "Selected SUMO scenario: " << SUMO_SCENARIO_NAME << std::endl;
  std::cout << "SUMO map boundaries: C1(" << sumoMapBoundaries.at (0) << ","
            << sumoMapBoundaries.at (1) << ") C2(" << sumoMapBoundaries.at (2) << ","
            << sumoMapBoundaries.at (3) << ")" << std::endl;

  uint32_t nRSUs = 1;

  uint32_t interestInterval = 1000;
  uint32_t simTime = 600;
  bool enablePcap = false;
  bool enableLog = true;
  bool enableSumoGui = false;

  std::cout << "Number of nodes (vehicles) detected in SUMO scenario: " << nVehicles << std::endl;
  std::cout << "Number of Road Side Units (RSUs): " << nRSUs << std::endl;

  if (!nVehicles || sumoMapBoundaries.size () < 4)
    throw std::runtime_error ("SUMO failed!");

  // command line attibutes
  CommandLine cmd;
  cmd.AddValue ("i", "InterestInterval interval (milliseconds)", interestInterval);
  cmd.AddValue ("s", "Simulation time (seconds)", simTime);
  cmd.AddValue ("pcap", "Enable PCAP", enablePcap);
  cmd.AddValue ("log", "Enable Log", enableLog);
  cmd.AddValue ("sumo-gui", "Enable SUMO with graphical user interface", enableSumoGui);
  cmd.Parse (argc, argv);

  // alternative for NS_LOG="class|token" ./waf
  if (enableLog) // see more in https://www.nsnam.org/docs/manual/html/logging.html
    {
      // The severity class and level options can be given in the NS_LOG environment variable by these tokens:
      // Class	Level
      // error	level_error
      // warn	level_warn
      // debug	level_debug
      // info	level_info
      // function	level_function
      // logic	level_logic
      // level_all
      // all
      // *

      // The options can be given in the NS_LOG environment variable by these tokens:
      // Token	Alternate
      // prefix_func	func
      // prefix_time	time
      // prefix_node	node
      // prefix_level	level
      // prefix_all
      // all
      // *

      std::vector<std::string> componentsLogLevelAll;
      componentsLogLevelAll.push_back ("vndn-example-tms");
      componentsLogLevelAll.push_back ("ndn.TmsConsumer");
      componentsLogLevelAll.push_back ("ndn.TmsProvider");

      std::vector<std::string> componentsLogLevelError;
      componentsLogLevelError.push_back ("TraciClient");

      for (auto const &c : componentsLogLevelAll)
        {
          LogComponentEnable (c.c_str (), LOG_LEVEL_ALL);
          LogComponentEnable (c.c_str (), LOG_PREFIX_ALL);
        }

      for (auto const &c : componentsLogLevelError)
        {
          LogComponentEnable (c.c_str (), LOG_LEVEL_ERROR);
          LogComponentEnable (c.c_str (), LOG_PREFIX_ALL);
        }
    }

  /* create node pool and counter; large enough to cover all sumo vehicles */
  NodeContainer nodePool;
  nodePool.Create (nVehicles + nRSUs);
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
  std::cout << "Installing networking devices for every node..." << std::endl;
  ndn::WifiSetupHelper wifi;
  NetDeviceContainer devices = wifi.ConfigureDevices (nodePool, enablePcap);

  // install Ndn stack
  std::cout << "Installing Ndn stack in " << nVehicles + nRSUs << " nodes... " << std::endl;
  ndn::StackHelper ndnHelper;
  //ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.setCsSize(1000);
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll ();
  // forwarding strategy
  ndn::StrategyChoiceHelper::Install (nodePool, "/", "/localhost/nfd/strategy/multicast");

  // install mobility model
  std::cout << "Setting up mobility... " << std::endl;

  /*** setup mobility and position to node pool ***/
  MobilityHelper mobility;
  Ptr<UniformDiscPositionAllocator> positionAlloc = CreateObject<UniformDiscPositionAllocator> ();
  positionAlloc->SetX (0);
  positionAlloc->SetY (sumoMapBoundaries.at (1) - 5000);
  positionAlloc->SetZ (-5000.0);
  positionAlloc->SetRho (20.0);
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodePool);

  std::cout << "Config SUMO/TraCI..." << std::endl;
  /*** setup Traci and start SUMO ***/
  Ptr<TraciClient> sumoClient = CreateObject<TraciClient> ();
  sumoClient->SetAttribute (
      "SumoConfigPath", StringValue ("contrib/ndn4ivc/traces/" SUMO_SCENARIO_NAME "/sim.sumocfg"));
  sumoClient->SetAttribute ("SumoBinaryPath",
                            StringValue ("")); // use system installation of sumo
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

    NS_LOG_INFO ("Ns3SumoSetup: node " << nodeCounter << " has initialized and the app installed!");
    Ptr<Node> includedNode = nodePool.Get (nodeCounter);
    nodeCounter++;
    Ptr<TmsConsumerApp> tmsConsumerApp = CreateObject<TmsConsumerApp> ();
    tmsConsumerApp->SetAttribute ("Frequency", UintegerValue (interestInterval)); // in milliseconds
    tmsConsumerApp->SetAttribute ("Client", (PointerValue) (sumoClient)); // pass TraCI object

    includedNode->AddApplication (tmsConsumerApp);

    return includedNode;
  };

  /** Define the callback function for node shutdown
   * 
   *  ns-3 app must be terminated and ns-3 node (vehicle) will be 
   *  put away ('removed') from the simulation scenario
   */
  std::function<void (Ptr<Node>)> shutdownSumoVehicle = [] (Ptr<Node> exNode) {
    Ptr<TmsConsumerApp> c_app = DynamicCast<TmsConsumerApp> (exNode->GetApplication (0));
    NS_LOG_INFO ("Ns3SumoSetup: node " << exNode->GetId ()
                                       << " has finalized and the app removed!");
    if (c_app)
      c_app->StopApplication ();
    c_app->SetStopTime(Seconds(0.1));
    Ptr<NetDevice> dev = exNode->GetDevice (0);

    //dev->DoDelete();

    // put the node in new position, outside the simulation communication range
    Ptr<ConstantPositionMobilityModel> mob = exNode->GetObject<ConstantPositionMobilityModel> ();
    mob->SetPosition (Vector ((double) exNode->GetId (), -4800 - (rand () % 25), -4500.0));

    // NOTE: further actions could be required for a save shutdown!
  };

  Ptr<MobilityModel> mobilityRsuNode = nodePool.Get (0)->GetObject<MobilityModel> ();
  nodeCounter++;
  //mobilityRsuNode->SetPosition (Vector (70, 70, 3.0)); // set RSU to fixed position
  //mobilityRsuNode->SetPosition (Vector (((sumoMapBoundaries.at (1) + sumoMapBoundaries.at (3)) / 2),
  //                                      70, 3.0)); // set RSU to fixed position
  mobilityRsuNode->SetPosition (Vector (50, 25, 3));

  std::cout << "Installing RSU application... " << std::endl;
  /* RSU apps */
  ApplicationContainer tmsProviderContainer;
  ndn::AppHelper tmsProviderHelper ("TmsProviderApp");
  tmsProviderHelper.SetAttribute ("Client", (PointerValue) (sumoClient)); // pass TraCI object
  tmsProviderContainer.Add (tmsProviderHelper.Install (nodePool.Get (0)));

  // config
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelNumber",
               ns3::UintegerValue (SCH3));

  sumoClient->SumoSetup (setupNewSumoVehicle, shutdownSumoVehicle);
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