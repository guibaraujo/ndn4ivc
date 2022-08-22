#ifndef _CERT_HELPER_HPP_
#define _CERT_HELPER_HPP_

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/io.hpp>
#include <fstream>
#include <iostream>

inline void
setupRootCert(const ::ndn::Name& subjectName, std::string filename) {
  ::ndn::KeyChain keyChain;
  ::ndn::security::Identity rootIdentity;
  try {
    /* try to get an existing certificate with the same name */
    rootIdentity = keyChain.getPib().getIdentity(subjectName);
  }
  catch (const std::exception& e) {
    /* if it fails, create a new one */
    rootIdentity = keyChain.createIdentity(subjectName);
  }

  if (filename.empty())
    return;

  ::ndn::security::v2::Certificate cert = rootIdentity.getDefaultKey().getDefaultCertificate();
  std::ofstream os(filename);
  ::ndn::io::save(cert, filename);
}

inline void
setupRootCert(const ::ndn::Name& subjectName) {
  setupRootCert(subjectName, "");
}

inline ::ndn::security::v2::Certificate
signCert(::ndn::security::v2::Certificate cert, std::string issuerName) {
  ndn::KeyChain keyChain;
  ndn::SignatureInfo signatureInfo;
  signatureInfo.setValidityPeriod(ndn::security::ValidityPeriod(ndn::time::system_clock::TimePoint(),
                                                                ndn::time::system_clock::now() + ndn::time::days(365)));
  keyChain.sign(cert, ndn::security::SigningInfo(keyChain.getPib().getIdentity(issuerName))
                                                   .setSignatureInfo(signatureInfo));

  return cert;
}

#endif  // _CERT_HELPER_HPP_
