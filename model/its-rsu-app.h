/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef ITS_RSU_APP_H
#define ITS_RSU_APP_H

#include "its-rsu.h"

#include "ns3/traci-client.h"
#include "ns3/sumoMap-graph.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/application.h"
#include <ns3/core-module.h>

namespace ns3 {

/**
 * @brief  Intelligent Transport Systems RSU Application
 *  
 */
class ItsRsuApp : public Application // Class inheriting from ns3::Application
{
public:
  static TypeId
  GetTypeId ()
  {
    static TypeId tid =
        TypeId ("ItsRsuApp")
            .SetParent<Application> ()
            .AddConstructor<ItsRsuApp> ()
            .AddAttribute ("AppPrefix", "NDN Application Prefix", StringValue ("/localhop/ndn-app"),
                           MakeStringAccessor (&ItsRsuApp::m_appPrefix), MakeStringChecker ())
            .AddAttribute ("NodeName", "Node name", StringValue ("/ndn/NodeA"),
                           MakeStringAccessor (&ItsRsuApp::m_nodeName), MakeStringChecker ())
            .AddAttribute ("SumoGraph", "SUMO map as a graph (vertex, edges)", PointerValue (0),
                           MakePointerAccessor (&ItsRsuApp::m_graphMap),
                           MakePointerChecker<ns3::GraphSumoMap> ())
            .AddAttribute ("SumoClient", "TraCI client for SUMO", PointerValue (0),
                           MakePointerAccessor (&ItsRsuApp::m_traci),
                           MakePointerChecker<TraciClient> ());

    return tid;
  }

  void
  SetSignCertCb (std::string rootName,
                 ::ndn::security::v2::Certificate (*cb) (::ndn::security::v2::Certificate,
                                                         std::string))
  {
    signCertCb = cb;
    m_rootName = rootName;
  }

  virtual void
  StartApplication ()
  {
    m_instance.reset (new ::ndn::its::ItsRsu (m_appPrefix, m_nodeName, m_graphMap, m_traci));
    auto cert = m_instance->GetCertificate ();

    m_instance->InstallSignedCertificate (signCertCb (cert, m_rootName));
    m_instance->Start ();
  }

  virtual void
  StopApplication ()
  {
    // Stop and destroy the instance of the app
    m_instance->Stop ();
    m_instance.reset ();
  }

private:
  std::unique_ptr<::ndn::its::ItsRsu> m_instance;
  std::string m_appPrefix;
  std::string m_nodeName;

  ::ndn::security::v2::Certificate (*signCertCb) (::ndn::security::v2::Certificate, std::string);
  std::string m_rootName;

  ns3::Ptr<ns3::GraphSumoMap> m_graphMap;
  ns3::Ptr<ns3::TraciClient> m_traci;
};

} // namespace ns3

#endif
