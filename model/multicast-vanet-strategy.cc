#include "multicast-vanet-strategy.h"
#include "algorithm.hpp"
#include "common/logger.hpp"
namespace nfd {
namespace fw {

NFD_REGISTER_STRATEGY (MulticastVanetStrategy);

NFD_LOG_INIT (MulticastVanetStrategy);

const time::milliseconds MulticastVanetStrategy::RETX_SUPPRESSION_INITIAL (10);
const time::milliseconds MulticastVanetStrategy::RETX_SUPPRESSION_MAX (250);

MulticastVanetStrategy::MulticastVanetStrategy (Forwarder &forwarder, const Name &name)
    : Strategy (forwarder),
      m_retxSuppression (RETX_SUPPRESSION_INITIAL, RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                         RETX_SUPPRESSION_MAX)
{
  this->setInstanceName (makeInstanceName (name, getStrategyName ()));
}

const Name &
MulticastVanetStrategy::getStrategyName ()
{
  static Name strategyName ("/localhost/nfd/strategy/multicast-vanet/%FD%01");

  return strategyName;
}

void
MulticastVanetStrategy::afterReceiveInterest (const FaceEndpoint &ingress, const Interest &interest,
                                              const shared_ptr<pit::Entry> &pitEntry)
{
  const fib::Entry &fibEntry = this->lookupFib (*pitEntry);
  const fib::NextHopList &nexthops = fibEntry.getNextHops ();
  NFD_LOG_DEBUG ("Interest=" << interest << " inFaceId=" << ingress.face.getId ());

  int nEligibleNextHops = 0;

  bool isSuppressed = false;

  for (const auto &nexthop : nexthops)
    {
      Face &outFace = nexthop.getFace ();

      RetxSuppressionResult suppressResult =
          m_retxSuppression.decidePerUpstream (*pitEntry, outFace);

      if (suppressResult == RetxSuppressionResult::SUPPRESS)
        {
          NFD_LOG_DEBUG (interest << " from=" << ingress << " to=" << outFace.getId ()
                                  << " suppressed");
          isSuppressed = true;
          continue;
        }

      if ((outFace.getId () == ingress.face.getId () &&
           outFace.getLinkType () != ndn::nfd::LINK_TYPE_AD_HOC) ||
          wouldViolateScope (ingress.face, interest, outFace))
        {
          continue;
        }

      this->sendInterest (pitEntry, FaceEndpoint (outFace, 0), interest);
      NFD_LOG_DEBUG (interest << " from=" << ingress << " pitEntry-to=" << outFace.getId ());

      if (suppressResult == RetxSuppressionResult::FORWARD)
        {
          m_retxSuppression.incrementIntervalForOutRecord (*pitEntry->getOutRecord (outFace));
        }
      ++nEligibleNextHops;
    }

  if (nEligibleNextHops == 0 && !isSuppressed)
    {
      NFD_LOG_DEBUG (interest << " from=" << ingress << " noNextHop: rejectPendingInterest");
      this->rejectPendingInterest (pitEntry);
    }
}

} // namespace fw
} // namespace nfd