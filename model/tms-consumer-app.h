/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef TMS_CONSUMER_APP_H
#define TMS_CONSUMER_APP_H

#include "tms-consumer.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"

#include "ns3/traci-client.h"

namespace ns3 {

/**
 * @brief TMS (Transportation Management Service) application client
 *  
 */
class TmsConsumerApp : public Application
{
private:
  std::unique_ptr<::ndn::TmsConsumer> m_instance;
  uint32_t m_frequency;
  Ptr<TraciClient> m_traci;

public:
  static TypeId
  GetTypeId ()
  {
    static TypeId tid =
        TypeId ("TmsConsumerApp")
            .SetParent<Application> ()
            .AddConstructor<TmsConsumerApp> ()
            .AddAttribute ("Frequency", "Frequency of interest packets", StringValue ("1000"),
                           MakeUintegerAccessor (&TmsConsumerApp::m_frequency),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("Client", "TraCI client for SUMO", PointerValue (0),
                           MakePointerAccessor (&TmsConsumerApp::m_traci),
                           MakePointerChecker<TraciClient> ());

    return tid;
  }

  // inherited from Application base class
  virtual void
  StartApplication ()
  {
    // create an instance of the app
    m_instance.reset (new ::ndn::TmsConsumer (m_frequency, m_traci));
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

#endif // TMS_CONSUMER_APP_H
