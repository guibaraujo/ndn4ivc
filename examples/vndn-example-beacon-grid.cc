#include "ns3/wave-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/internet-module.h"
#include "ns3/traci-module.h"
#include "ns3/netanim-module.h"

#include <functional>
#include <stdlib.h>

#include "ns3/wifi-setup-helper.h"

#define YELLOW_CODE "\033[33m"
#define TEAL_CODE "\033[36m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

/* NS_LOG=ndn.Consumer:ndn.Producer ./waf --run "vndn-sumo-circle" --vis */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ndn.vndn-sumo-grid");

namespace ns3 {
void
SomeEvent ()
{
}

// Note: this is a promiscuous trace for all packet reception
// This is also on physical layer, so packets still have WifiMacHeader
void
Rx (std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector,
    MpduInfo aMpdu, SignalNoiseDbm signalNoise)
{
  // context will include info about the source of this event
  // use string manipulation if you want to extract info
  std::cout << BOLD_CODE << context << END_CODE << std::endl;
  // print the info
  std::cout << "\tSize=" << packet->GetSize () << " Freq=" << channelFreqMhz
            << " Mode=" << txVector.GetMode () << " Signal=" << signalNoise.signal
            << " Noise=" << signalNoise.noise << std::endl;

  // now it's possible to examine the WifiMacHeader
  WifiMacHeader hdr;
  if (packet->PeekHeader (hdr))
    {
      //std::cout << "\tDestination MAC : " << hdr.GetAddr1 () << "\tSource MAC : " << hdr.GetAddr2 ()
      //          << std::endl;
    }
}

int
main (int argc, char *argv[])
{
  std::cout << TEAL_CODE << BOLD_CODE << "Starting main ... " END_CODE << std::endl;

  // conf default values
  CommandLine cmd;
  uint32_t nNodes = 10; // or automate waf ... --n=`cat /path/*.rou.xml|grep vehicle|wc -l`
  double simTime = 100;
  double interval = 0.5;
  bool enablePcap = false;
  bool enableLog = false;

  // command line attibutes
  cmd.AddValue ("i", "Broadcast interval in seconds", interval);
  cmd.AddValue ("n", "Number of nodes", nNodes);
  cmd.AddValue ("pcap", "Enable PCAP", enablePcap);
  cmd.AddValue ("log", "Enable Log", enableLog);

  cmd.Parse (argc, argv);

  if (enableLog)
    {
      LogComponentEnable ("TraciClient", LOG_LEVEL_INFO);
      LogComponentEnable ("TrafficControlApplication", LOG_LEVEL_INFO);
    }

  // create node pool and counter; large enough to cover all sumo vehicles
  ns3::Time simulationTime (ns3::Seconds (simTime));
  NodeContainer nodePool;
  nodePool.Create (nNodes);
  uint32_t nodeCounter (0);

  // install Wifi & set up
  ndn::WifiSetupHelper wifi;
  NetDeviceContainer devices = wifi.ConfigureDevices (nodePool, enablePcap);

  // install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll ();

  // choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll ("/prefix", "/localhost/nfd/strategy/multicast");

  NS_LOG_INFO ("Setting mobility");

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
                            StringValue ("contrib/vndn-samples/traces/mg5x5/sim.sumocfg"));
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

  // installing applications
  NS_LOG_INFO ("Installing Applications");

  // consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  // consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix ("/prefix");
  consumerHelper.SetAttribute ("Frequency", StringValue ("1")); // interests/second

  // producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/prefix");
  producerHelper.SetAttribute ("PayloadSize", StringValue ("1024"));
  producerHelper.Install (nodePool.Get (1));

  // callback function for node creation
  std::function<Ptr<Node> ()> setupNewWifiNode = [&] () -> Ptr<Node> {
    if (nodeCounter >= nodePool.GetN ())
      NS_FATAL_ERROR ("Node Pool empty!: " << nodeCounter << " nodes created.");

    // don't create and install the protocol stack of the node at simulation time -> take from "node pool"
    Ptr<Node> includedNode = nodePool.Get (nodeCounter);
    ++nodeCounter; // increment counter for next node
    std::cout << TEAL_CODE << BOLD_CODE << includedNode->GetId() << END_CODE << std::endl;

    if (includedNode->GetId() == 0)
      {
        auto apps = consumerHelper.Install (nodePool.Get (0)); // first node
        apps.Stop (Seconds (simTime)); // stop the consumer app at 10 seconds mark
      }

    return includedNode;
  };

  // callback function for node shutdown
  std::function<void (Ptr<Node>)> shutdownWifiNode = [] (Ptr<Node> exNode) {
    // set position outside communication range
    Ptr<ConstantPositionMobilityModel> mob = exNode->GetObject<ConstantPositionMobilityModel> ();
    mob->SetPosition (Vector (-100.0 + (rand () % 25), 320.0 + (rand () % 25),
                              250.0)); // rand() for visualization purposes
    std::cout << TEAL_CODE << BOLD_CODE << exNode->GetId() << END_CODE << std::endl;


    // NOTE: further actions could be required for a save shut down!
  };

  // start traci client with given function pointers
  sumoClient->SumoSetup (setupNewWifiNode, shutdownWifiNode);

  /*** Setup and Start Simulation + Animation ***/
  //AnimationInterface anim ("contrib/vndn-samples/traces/circle-simple/circle.sumo.xml"); // Mandatory

  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
                   MakeCallback (&Rx));

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
  return ns3::main (argc, argv);
}