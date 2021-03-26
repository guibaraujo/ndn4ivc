/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef BEACON_APP_H
#define BEACON_APP_H

#include "beacon.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"

namespace ns3 {

/**
 * @brief A simple beacon application for intervehicle communication and intelligent transportation system.
 * 
 */
class BeaconApp : public Application
{
public:
  static TypeId
  GetTypeId ()
  {
    static TypeId tid = TypeId ("BeaconApp").SetParent<Application> ().AddConstructor<BeaconApp> ()
          .AddAttribute ("Frequency", "Frequency of interest packets", StringValue ("1000"),
                         MakeDoubleAccessor (&BeaconApp::m_frequency),
                         MakeDoubleChecker<int> ());

    return tid;
  }

protected:
  // inherited from Application base class.
  virtual void
  StartApplication ()
  {
    // create an instance of the app
    m_instance.reset (new ::ndn::Beacon (m_frequency));
    m_instance->run (); // can be omitted
  }

  virtual void
  StopApplication ()
  {
    // Stop and destroy the instance of the app
    m_instance.reset ();
  }

private:
  std::unique_ptr<::ndn::Beacon> m_instance;
  int m_frequency;
};

} // namespace ns3

#endif // BEACON_APP_H
