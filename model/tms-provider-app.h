/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef TMS_PROVIDER_APP_H
#define TMS_PROVIDER_APP_H

#include "tms-provider.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"

#include "ns3/traci-client.h"

namespace ns3 {

/**
 * @brief TMS (Transportation Management Service) application client
 *  
 */
class TmsProviderApp : public Application
{
private:
  std::unique_ptr<::ndn::TmsProvider> m_instance;
  uint32_t m_frequency;
  Ptr<TraciClient> m_traci;

public:
  static TypeId
  GetTypeId ()
  {
    static TypeId tid =
        TypeId ("TmsProviderApp")
            .SetParent<Application> ()
            .AddConstructor<TmsProviderApp> ()
            .AddAttribute ("Frequency", "Frequency of interest packets", StringValue ("1000"),
                           MakeUintegerAccessor (&TmsProviderApp::m_frequency),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("Client", "TraCI client for SUMO", PointerValue (0),
                           MakePointerAccessor (&TmsProviderApp::m_traci),
                           MakePointerChecker<TraciClient> ());

    return tid;
  }

  // inherited from Application base class
  virtual void
  StartApplication ()
  {
    // create an instance of the app
    m_instance.reset (new ::ndn::TmsProvider (m_frequency, m_traci));
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

#endif // TMS_PROVIDER_APP_H
