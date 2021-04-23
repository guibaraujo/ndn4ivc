#ifndef MULTICAST_VANET_STRATEGY_H
#define MULTICAST_VANET_STRATEGY_H

#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/algorithm.hpp"
#include "fw/retx-suppression-exponential.hpp"

namespace nfd {
namespace fw {

/** @brief a forwarding strategy similar strategy=Multicast, but customized to 
 * VANET/MANET scenarios, which implies that nodes should not generate NACKs, 
 * otherwise creating unexpected results
 * 
 * https://redmine.named-data.net/issues/4040?tab=history
 */
class MulticastVanetStrategy : public Strategy
{
public:
  explicit MulticastVanetStrategy (Forwarder &forwarder, const Name &name = getStrategyName ());

  static const Name &getStrategyName ();

  void afterReceiveInterest (const FaceEndpoint &ingress, const Interest &interest,
                             const shared_ptr<pit::Entry> &pitEntry) override;

private:
  RetxSuppressionExponential m_retxSuppression;
  static const time::milliseconds RETX_SUPPRESSION_INITIAL;
  static const time::milliseconds RETX_SUPPRESSION_MAX;
};

} // namespace fw
} // namespace nfd

#endif // MULTICAST_VANET_STRATEGY_H