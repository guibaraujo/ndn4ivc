/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef MYLOG_HELPER_H
#define MYLOG_HELPER_H

#ifdef NS3_LOG_ENABLE
#include <ns3/log.h>
NS_LOG_COMPONENT_DEFINE (MYLOG_COMPONENT);
#define MYLOG_DEBUG(msg) NS_LOG_DEBUG (msg)
#define MYLOG_INFO(msg) NS_LOG_INFO (msg)
#endif
#ifndef NS3_LOG_ENABLE
#include <chrono>
#include <ctime>

char curtime[30];
void
update_curtime ()
{
  std::time_t now = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now ());
  std::strftime (curtime, sizeof (curtime), "%Y-%m-%d,%H:%M:%S", std::gmtime (&now));
}

#define MYLOG(level, msg) \
  update_curtime ();      \
  std::cout << curtime << " [" << level << "] " << __func__ << "() " << msg << std::endl;
#define MYLOG_DEBUG(msg) MYLOG ("DEBUG", msg)
#define MYLOG_INFO(msg) MYLOG ("INFO", msg)

#endif

#endif // MYLOG_HELPER_H
