/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// ███╗░░██╗██████╗░███╗░░██╗░░██╗██╗██╗██╗░░░██╗░█████╗░
// ████╗░██║██╔══██╗████╗░██║░██╔╝██║██║██║░░░██║██╔══██╗
// ██╔██╗██║██║░░██║██╔██╗██║██╔╝░██║██║╚██╗░██╔╝██║░░╚═╝
// ██║╚████║██║░░██║██║╚████║███████║██║░╚████╔╝░██║░░██╗
// ██║░╚███║██████╔╝██║░╚███║╚════██║██║░░╚██╔╝░░╚█████╔╝
// ╚═╝░░╚══╝╚═════╝░╚═╝░░╚══╝░░░░░╚═╝╚═╝░░░╚═╝░░░░╚════╝░
// https://github.com/guibaraujo/NDN4IVC

#ifndef BEACON_APP_H
#define BEACON_APP_H

#include "beacon.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"

namespace ns3 {

/**
 * @brief Beacon application over NDN for intervehicle communication
 * 
 */
class BeaconApp : public Application
{
private:
  std::unique_ptr<::ndn::Beacon> m_instance;
  uint32_t m_frequency;

public:
  static TypeId
  GetTypeId ()
  {
    static TypeId tid =
        TypeId ("BeaconApp")
            .SetParent<Application> ()
            .AddConstructor<BeaconApp> ()
            .AddAttribute ("Frequency", "Frequency of interest packets", StringValue ("1000"),
                           MakeUintegerAccessor (&BeaconApp::m_frequency),
                           MakeUintegerChecker<uint32_t> ());

    return tid;
  }

  // inherited from Application base class
  virtual void
  StartApplication ()
  {
    // create an instance of the app
    m_instance.reset (new ::ndn::Beacon (m_frequency));
    m_instance->run (); // can be omitted
    m_instance->start ();
  }

  virtual void
  StopApplication ()
  {
    // Stop and destroy the instance of the app
    m_instance->stop ();
    m_instance.reset ();
  }
};

} // namespace ns3

#endif // BEACON_APP_H
