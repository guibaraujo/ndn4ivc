/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// ███╗░░██╗██████╗░███╗░░██╗░░██╗██╗██╗██╗░░░██╗░█████╗░
// ████╗░██║██╔══██╗████╗░██║░██╔╝██║██║██║░░░██║██╔══██╗
// ██╔██╗██║██║░░██║██╔██╗██║██╔╝░██║██║╚██╗░██╔╝██║░░╚═╝
// ██║╚████║██║░░██║██║╚████║███████║██║░╚████╔╝░██║░░██╗
// ██║░╚███║██████╔╝██║░╚███║╚════██║██║░░╚██╔╝░░╚█████╔╝
// ╚═╝░░╚══╝╚═════╝░╚═╝░░╚══╝░░░░░╚═╝╚═╝░░░╚═╝░░░░╚════╝░

#include "ns3/its-car-app.h"
#include "ns3/its-car.h"
#include "ns3/its-rsu-app.h"
#include "ns3/its-rsu.h"

#include "ns3/sumoMap-graph.h"
#include "ns3/traci-client.h"

#include "../helper/rapidxml-1.13/rapidxml.hpp"
#include "../helper/cert-helper.h"

#include "ns3/wave-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-setup-helper.h"
#include "ns3/wifi-adhoc-helper.h"
#include "ns3/traci-module.h"
#include "ns3/netanim-module.h"

#include <functional>
#include <stdlib.h>
#include <stdio.h>
#include <exception>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>

#define YELLOW_CODE "\033[33m"
#define RED_CODE "\033[31m"
#define BLUE_CODE "\033[34m"
#define BOLD_CODE "\033[1m"
#define CYAN_CODE "\033[36m"
#define END_CODE "\033[0m"

#define MYLOG_COMPONENT "vndn-example-its"
#include "../helper/mylog-helper.h"

// ********** config **********
#define NDN4IVC_FOLDER "contrib/ndn4ivc/"
#define NDN4IVC_SUMO_SCENARIO_NAME "spider-map"
//  "grid-map-test" //see more in 'ndn4ivc/traces' (e.g., grid-map)
// ********** config **********

#define NDN4IVC_SUMO_MAP_NET_FILE \
  NDN4IVC_FOLDER "traces/" NDN4IVC_SUMO_SCENARIO_NAME "/map.net.xml" //define net-file SUMO
#define NDN4IVC_SUMO_NUM_VEHICLES                               \
  "\
#/bin/bash \n\
#echo $1 \n\
echo `cat " NDN4IVC_FOLDER "traces/" NDN4IVC_SUMO_SCENARIO_NAME \
  "/*.rou.xml |grep 'vehicle id'|wc -l` \n\
"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ItsCarApp);
NS_OBJECT_ENSURE_REGISTERED (ItsRsuApp);
std::map<uint32_t, ns3::Time> nodesDisable2Move;

/**
 * Control Functions 
*/
void
checkDisableNodes ()
{
  for (auto it = nodesDisable2Move.begin (), it_next = it; it != nodesDisable2Move.end ();
       it = it_next)
    {
      ++it_next;
      Ptr<Node> exNode = ns3::NodeList::GetNode (it->first);
      // NOTE: we'll put the node in a new position, outside the simulation
      // communication range, but this is just for better visualization mode (--vis)
      if ((ns3::Time) ns3::Simulator::Now ().GetSeconds () - it->second > 1)
        {
          Ptr<ConstantPositionMobilityModel> mob =
              exNode->GetObject<ConstantPositionMobilityModel> ();
          mob->SetPosition (Vector ((double) exNode->GetId (), -4000 - (rand () % 25), -5000.0));
          nodesDisable2Move.erase (it);
        }
    }
  Simulator::Schedule (Seconds (1), &checkDisableNodes);
}

std::string
execInTerminal (const char *cmd)
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

/**
 * Import SUMO Map  
 * Note: make sure that no additional code is required when the map template changes
 * 
 * [Online] ref.: https://sumo.dlr.de/docs/Networks/SUMO_Road_Networks.html
*/
void
importSumoNetMapFromXmlFile (std::vector<VertexSumoMap *> &vertices,
                             std::vector<EdgeSumoMap *> &edges)
{
  std::cout << "Selected SUMO map: " << NDN4IVC_SUMO_MAP_NET_FILE << std::endl;
  std::cout << "Importing SUMO XML net file: " << NDN4IVC_SUMO_MAP_NET_FILE << std::endl;

  //handling SUMO XML file
  rapidxml::xml_document<> doc;
  rapidxml::xml_node<> *root_node;
  ifstream theFile (NDN4IVC_SUMO_MAP_NET_FILE);
  vector<char> buffer ((istreambuf_iterator<char> (theFile)), istreambuf_iterator<char> ());
  buffer.push_back ('\0');
  doc.parse<0> (&buffer[0]); //parse the buffer using the xml file parsing library into doc
  root_node = doc.first_node ("net"); //find root
  int idx = 0;

  //iterate over vertex/junction
  for (rapidxml::xml_node<> *brewery_node = root_node->first_node ("junction"); brewery_node;
       brewery_node = brewery_node->next_sibling ("junction"))
    {
      std::string type = brewery_node->first_attribute ("type")->value ();
      if (type.compare ("internal"))
        {
          std::string nameId = brewery_node->first_attribute ("id")->value ();
          VertexSumoMap *v = new VertexSumoMap (idx, nameId);
          vertices.push_back (v);
          idx++;
        }
    }
  //iterate over edge
  for (rapidxml::xml_node<> *brewery_node = root_node->first_node ("edge"); brewery_node;
       brewery_node = brewery_node->next_sibling ("edge"))
    {
      std::string type = brewery_node->first_attribute ("id")->value ();
      //if (!type.compare ("priority"))
      if (brewery_node->first_attribute ("from"))
        {
          EdgeSumoMap *e = new EdgeSumoMap ();
          e->nameId = std::string (brewery_node->first_attribute ("id")->value ());
          e->m_from = std::string (brewery_node->first_attribute ("from")->value ());
          e->m_to = std::string (brewery_node->first_attribute ("to")->value ());
          edges.push_back (e);
          /*
          printf ("%s from: %s to: %s\n", brewery_node->first_attribute ("id")->value (),
                  brewery_node->first_attribute ("from")->value (),
                  brewery_node->first_attribute ("to")->value ());
          */
        }
    }
  std::cout << "Done! XML imported with #v=" << vertices.size () << " #e=" << edges.size ()
            << std::endl;
  std::cout << "If no errors/alerts, all SUMO net map has been imported!" << std::endl;
  //vertices == 0 || edges == 0 //alert here, problems
}

/**
 * This scenario simulates an ITS app with NDN architecture
 */
int
main (int argc, char *argv[])
{
  std::cout << CYAN_CODE << BOLD_CODE << "Preparing simulation environment: " END_CODE << std::endl;

  uint32_t nVehicles = std::stoi (execInTerminal (NDN4IVC_SUMO_NUM_VEHICLES));
  uint32_t nRSUs = 1;
  uint32_t interestInterval = 1000;
  uint32_t simTime = 600;
  bool enablePcap = false;
  bool enableLog = true;
  bool enableSumoGui = false;

  std::cout << "# Road Side Units (RSUs): " << nRSUs << std::endl;
  std::cout << "# nodes (vehicles) detected in SUMO scenario: " << nVehicles << std::endl;

  /** rename + info */
  std::vector<VertexSumoMap *> vertices;
  std::vector<EdgeSumoMap *> edges;
  importSumoNetMapFromXmlFile (vertices, edges);
  Ptr<GraphSumoMap> sumoGraph = CreateObject<GraphSumoMap> ();
  sumoGraph->init (vertices, edges);

  if (!nVehicles || !vertices.size () || !edges.size ())
    throw std::runtime_error ("SUMO failed! Files not imported.");

  /** command line attributes */
  CommandLine cmd;
  cmd.AddValue ("i", "Interest interval (milliseconds)", interestInterval);
  cmd.AddValue ("s", "Simulation time (seconds)", simTime);
  cmd.AddValue ("pcap", "Enable PCAP", enablePcap);
  cmd.AddValue ("log", "Enable Log", enableLog);
  cmd.AddValue ("sumo-gui", "Enable SUMO with graphical user interface", enableSumoGui);
  cmd.Parse (argc, argv);

  /** Logging -> simple alternative for NS_LOG="class|token" ./waf */
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

      std::cout
          << "NDN4IVC log setting as: <simulation-time> <node-id> <class> <method> [log-type] <msg>"
          << std::endl;

      std::vector<std::string> myComponentsLogLevel;
      myComponentsLogLevel.push_back ("vndn-example-its");
      myComponentsLogLevel.push_back ("its.car");
      myComponentsLogLevel.push_back ("its.rsu");
      //myComponentsLogLevel.push_back ("ndn-cxx.nfd.MulticastStrategy");
      //myComponentsLogLevel.push_back ("ndn-cxx.nfd.Forwarder");
      //myComponentsLogLevel.push_back ("WifiPhy");
      std::vector<std::string> componentsLogLevelError;
      componentsLogLevelError.push_back ("TraciClient");

      for (auto const &c : myComponentsLogLevel)
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

  /** create node pool and counter; large enough to cover all sumo vehicles */
  NodeContainer nodePool;
  nodePool.Create (nVehicles + nRSUs);
  uint32_t nodeCounter (0);

  /** install NetDevice - wifi & set up */
  std::cout << "Installing networking devices for every node..." << std::endl;
  /* 
   * IEEE 80211p channels for vehicular application
   *
   * SCH1 172 SCH2 174 SCH3 176
   * CCH  178
   * SCH4 180 SCH5 182 SCH6 184
   * 
   * [Online] ref.: doi: 10.1109/VETECF.2007.461
   */
  ndn::WifiSetupHelper wifi;
  NetDeviceContainer devices = wifi.ConfigureDevices (nodePool, enablePcap);

  /** install Ndn Stack */
  std::cout << "Installing Ndn stack in " << nVehicles + nRSUs << " nodes ... " << std::endl;
  ndn::StackHelper ndnHelper;
  ndnHelper.AddFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback (FixLinkTypeAdhocCb));
  //ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.setCsSize (1000); //content store size: see also data Freshness ('expiration') time
  ndnHelper.InstallAll ();

  /** forwarding strategy */
  ndn::StrategyChoiceHelper::Install (nodePool, "/", "/localhost/nfd/strategy/multicast-vanet");
  //ndn::StrategyChoiceHelper::Install (nodePool, "/", "/localhost/nfd/strategy/multicast");

  /** setup root cert to be used as trusted anchor based on SCMS
   * 
   * The Security Credential Management System (SCMS) is a proof-of-concept (POC) 
   * message security solution for vehicle-to-vehicle (V2V) and vehicle-to-infrastructure (V2I) communication 
   * 
   * It uses a Public Key Infrastructure (PKI)-based approach that employs highly innovative methods of 
   * encryption and certificate management to facilitate trusted communication
   * 
   * [Online] ref.: https://www.its.dot.gov/resources/scms.htm
  */
  std::string rootCertName = "/DoT"; // Department of Transportation
  std::string certDir = "./contrib/ndn4ivc/config/rootCert.crt";
  setupRootCert (ndn::Name (rootCertName), certDir);
  std::cout << "Adding trusted root certificate in: " << certDir << std::endl;

  /** install mobility & config SUMO */
  std::cout << "Config SUMO/TraCI..." << std::endl;
  MobilityHelper mobility;
  Ptr<UniformDiscPositionAllocator> positionAlloc = CreateObject<UniformDiscPositionAllocator> ();
  positionAlloc->SetX (0);
  positionAlloc->SetY (-5000 - (rand () % 25));
  positionAlloc->SetZ (-5000.0);
  positionAlloc->SetRho (25.0);
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodePool);

  /** setup Traci and start SUMO 
   * Ónline red.: https://sumo.dlr.de/docs/TraCI.html
  */
  Ptr<TraciClient> sumoClient = CreateObject<TraciClient> ();
  sumoClient->SetAttribute (
      "SumoConfigPath",
      StringValue ("contrib/ndn4ivc/traces/" NDN4IVC_SUMO_SCENARIO_NAME "/sim.sumocfg"));
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

  /**
   *  Define the callback function for dynamic node creation from 
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

    MYLOG_INFO ("Ns3SumoSetup: node [" << nodeCounter
                                       << "] has initialized and the app installed!");
    Ptr<Node> includedNode = nodePool.Get (nodeCounter);
    nodeCounter++;

    std::string nodeName = "/ufba/CAR/idCar_" + std::to_string (nodeCounter - 1);
    std::string myAppPrefix = "/service/its/navigo";

    Ptr<ItsCarApp> itsCarApp = CreateObject<ItsCarApp> ();
    itsCarApp->SetAttribute ("AppPrefix", StringValue (myAppPrefix));
    itsCarApp->SetAttribute ("NodeName", StringValue (nodeName));
    itsCarApp->SetAttribute ("SumoClient", (PointerValue) (sumoClient));

    includedNode->AddApplication (itsCarApp);

    auto app = DynamicCast<ItsCarApp> (includedNode->GetApplication (0));
    app->SetSignCertCb (rootCertName, signCert);

    return includedNode;
  };

  /** 
   *  Define the callback function for node shutdown
   * 
   *  ns-3 app must be terminated and ns-3 node (vehicle) will be 
   *  put away ('removed') from the simulation scenario
  */
  std::function<void (Ptr<Node>)> shutdownSumoVehicle = [&] (Ptr<Node> exNode) {
    MYLOG_INFO ("Ns3SumoSetup: node [" << exNode->GetId ()
                                       << "] will be finished and disconnected!");

    Ptr<ItsCarApp> itsCarApp = DynamicCast<ItsCarApp> (exNode->GetApplication (0));
    if (itsCarApp)
      {
        itsCarApp->StopApplication ();
        //tmsConsumerApp->SetStopTime (NanoSeconds (1));
      }

    for (uint32_t i = 0; i < exNode->GetNDevices (); ++i)
      if (exNode->GetDevice (i)->GetObject<WifiNetDevice> ()) // it is a WifiNetDevice
        exNode->GetDevice (i)->GetObject<WifiNetDevice> ()->GetPhy ()->SetOffMode ();
    //GetPhy ()->SetSleepMode ();
    // only to avoid animation error (link drag) in PyViz
    nodesDisable2Move.emplace (exNode->GetId (), (ns3::Time) ns3::Simulator::Now ().GetSeconds ());

    // the SUMO node has been finished and the ns3 node has also fully 'deactivated' accordingly
    // further actions could be required for a save shutdown!
  };

  /** Road Side Unit(s) parameters */
  ApplicationContainer itsProviderContainer;
  ndn::AppHelper itsRsuAppHelper ("ItsRsuApp");
  std::string nodeName;
  std::string myAppPrefix;

  /** RSU0 **/
  Ptr<MobilityModel> mobilityRsuNode0 = nodePool.Get (0)->GetObject<MobilityModel> (); // RSU0
  mobilityRsuNode0->SetPosition (Vector (200, 172, 3)); //RSU fixed position: x,y,z 
  nodeCounter++;
  nodeName = "/ufba/RSU/idRsu_" + std::to_string (nodeCounter - 1);
  myAppPrefix = "/service/its/navigo";
  // installing apps
  itsRsuAppHelper.SetAttribute ("AppPrefix", StringValue (myAppPrefix));
  itsRsuAppHelper.SetAttribute ("NodeName", StringValue (nodeName));
  itsRsuAppHelper.SetAttribute ("SumoGraph", (PointerValue) (sumoGraph));
  itsRsuAppHelper.SetAttribute ("SumoClient", (PointerValue) (sumoClient));
  itsProviderContainer.Add (itsRsuAppHelper.Install (nodePool.Get (0)));
  // installing certs
  auto app = DynamicCast<ItsRsuApp> (nodePool.Get (0)->GetApplication (0));
  app->SetSignCertCb (rootCertName, signCert);

  /** Others ns3 parameters */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelNumber",
               ns3::UintegerValue (SCH3));

  Simulator::Schedule (Seconds (1), &checkDisableNodes);
  std::cout << YELLOW_CODE << BOLD_CODE << "Simulation is running: " END_CODE << std::endl;
  sumoClient->SumoSetup (setupNewSumoVehicle, shutdownSumoVehicle);

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  std::cout << RED_CODE << BOLD_CODE << "Post simulation: " END_CODE << std::endl;
  return 0;
};
} // namespace ns3

int
main (int argc, char *argv[])
{
  //std::system ("clear");
  return ns3::main (argc, argv);
}