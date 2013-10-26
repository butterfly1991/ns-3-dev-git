/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Dalian University of Technology
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Junling Bu <linlinjavaer@gmail.com>
 */
#include "channel-coordinator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE ("ChannelCoordinator");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ChannelCoordinator);

const static uint8_t DEFAULT_CCH_INTERVAL = 50;
const static uint8_t DEFAULT_SCH_INTERVAL = 50;
const static uint8_t DEFAULT_SYNC_TOLERANCE = 2;
const static uint8_t DEFAULT_MAC_SWITCH_TIME = 2;

TypeId
ChannelCoordinator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ChannelCoordinator")
    .SetParent<Object> ()
    .AddConstructor<ChannelCoordinator> ()
    .AddAttribute ("CchInterval", "CCH Interval, default value is 50ms.",
                   TimeValue (GetDefaultCchInterval ()),
                   MakeTimeAccessor (&ChannelCoordinator::m_cchInterval),
                   MakeTimeChecker ())
    .AddAttribute ("SchInterval", "SCH Interval, default value is 50ms.",
                   TimeValue (GetDefaultSchInterval ()),
                   MakeTimeAccessor (&ChannelCoordinator::m_schInterval),
                   MakeTimeChecker ())
    .AddAttribute ("SyncTolerance", "SyncTolerance, default value is 2ms.",
                   TimeValue (GetDefaultSyncTolerance ()),
                   MakeTimeAccessor (&ChannelCoordinator::m_syncTolerance),
                   MakeTimeChecker ())
    .AddAttribute ("MaxChSwitchTime", "MaxChannelSwitchTime, default value is 2ms.",
                   TimeValue (GetDefaultMaxSwitchTime ()),
                   MakeTimeAccessor (&ChannelCoordinator::m_maxSwitchTime),
                   MakeTimeChecker ())
  ;
  return tid;
}
ChannelCoordinator::ChannelCoordinator ()
  : m_guardCount (0)
{
  NS_LOG_FUNCTION (this);
}
ChannelCoordinator::~ChannelCoordinator ()
{
  NS_LOG_FUNCTION (this);
  m_listeners.clear ();
}
void
ChannelCoordinator::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  Time sync = GetSyncInterval ();
  // 1000 is 1000ms, modeling one UTC second
  if ((1000 % sync.GetMilliSeconds ()) != 0)
    {
      NS_FATAL_ERROR ("every UTC second shall be an integer number of SyncInterval");
    }
  if ( m_cchInterval < GetGuardInterval ())
    {
      NS_FATAL_ERROR ("CCH Interval should be large than GuardInterval");
    }
  if ( m_schInterval < GetGuardInterval ())
    {
      NS_FATAL_ERROR ("SCH Interval should be large than GuardInterval");
    }
}
void
ChannelCoordinator::SetCchInterval (Time cchInterval)
{
  NS_LOG_FUNCTION (this << cchInterval);
  m_cchInterval = cchInterval;
}
Time
ChannelCoordinator::GetCchInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_cchInterval;
}
void
ChannelCoordinator::SetSchInterval (Time schInterval)
{
  NS_LOG_FUNCTION (this << schInterval);
  m_schInterval = schInterval;
}
Time
ChannelCoordinator::GetSchInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_schInterval;
}
void
ChannelCoordinator::SetSyncTolerance (Time syncTolerance)
{
  NS_LOG_FUNCTION (this << syncTolerance);
  m_syncTolerance = syncTolerance;
}
Time
ChannelCoordinator::GetSyncTolerance (void) const
{
  NS_LOG_FUNCTION (this);
  return m_syncTolerance;
}
void
ChannelCoordinator::SetMaxSwitchTime (Time maxSwitchTime)
{
  NS_LOG_FUNCTION (this << maxSwitchTime);
  m_maxSwitchTime = maxSwitchTime;
}
Time
ChannelCoordinator::GetMaxSwitchTime (void) const
{
  NS_LOG_FUNCTION (this);
  return m_maxSwitchTime;
}
Time
ChannelCoordinator::GetSyncInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return GetCchInterval () + GetSchInterval ();
}
Time
ChannelCoordinator::GetGuardInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return GetSyncTolerance () + GetMaxSwitchTime ();
}
Time
ChannelCoordinator::GetDefaultCchInterval (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return MilliSeconds (DEFAULT_CCH_INTERVAL);
}
Time
ChannelCoordinator::GetDefaultSchInterval (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return MilliSeconds (DEFAULT_SCH_INTERVAL);
}
Time
ChannelCoordinator::GetDefaultSyncTolerance (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return MilliSeconds (DEFAULT_SYNC_TOLERANCE);
}
Time
ChannelCoordinator::GetDefaultMaxSwitchTime (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return MilliSeconds (DEFAULT_MAC_SWITCH_TIME);
}
 Time
ChannelCoordinator::GetSchSlot (void) const
{
  NS_LOG_FUNCTION (this);
  return m_schInterval - (m_syncTolerance + m_maxSwitchTime);
}
 Time
ChannelCoordinator::GetCchSlot (void) const
{
  NS_LOG_FUNCTION (this);
  return m_cchInterval - (m_syncTolerance + m_maxSwitchTime);
}
bool
ChannelCoordinator::IsCchInterval (Time duration) const
{
  NS_LOG_FUNCTION (this << duration);
  Time future = GetIntervalTime (duration);
  return (future < m_cchInterval);
}
bool
ChannelCoordinator::IsSchInterval (Time duration) const
{
  NS_LOG_FUNCTION (this << duration);
  return !IsCchInterval (duration);
}
Time
ChannelCoordinator::NeedTimeToSchInterval (Time duration) const
{
  NS_LOG_FUNCTION (this << duration);
  if (IsSchInterval (duration))
    {
      return Time (0);
    }
  return GetCchInterval () - GetIntervalTime (duration);
}
Time
ChannelCoordinator::NeedTimeToCchInterval (Time duration) const
{
  NS_LOG_FUNCTION (this << duration);
  if (IsCchInterval (duration))
    {
      return Time (0);
    }
  return GetSyncInterval () - GetIntervalTime (duration);
}
Time
ChannelCoordinator::NeedTimeToGuardInterval (Time duration) const
{
  NS_LOG_FUNCTION (this << duration);
  if (IsCchInterval (duration))
    {
      return (GetCchInterval () - GetIntervalTime (duration));
    }

  return (GetSyncInterval () - GetIntervalTime (duration));
}
bool
ChannelCoordinator::IsInSyncTolerance (Time duration) const
{
  NS_LOG_FUNCTION (this << duration);
  Time future = GetIntervalTime (duration);
  // interval is either in CchInterval or SchInterval
  Time interval = future < m_cchInterval ? future : future - m_cchInterval;
  Time halfSyncTolerance = MilliSeconds (m_syncTolerance.GetMilliSeconds () / 2);
  if (interval < halfSyncTolerance)
    {
      return true;
    }
  if (interval >= (halfSyncTolerance + m_maxSwitchTime)
      && interval < GetGuardInterval ())
    {
      return true;
    }
  return false;
}
bool
ChannelCoordinator::IsInMaxSwitchTime (Time duration) const
{
  NS_LOG_FUNCTION (this << duration);
  Time future = GetIntervalTime (duration);
  // interval is either in CchInterval or SchInterval
  Time interval = future < m_cchInterval ? future : future - m_cchInterval;
  Time halfSyncTolerance = MilliSeconds (m_syncTolerance.GetMilliSeconds () / 2);
  if (interval >= halfSyncTolerance
      && interval < (halfSyncTolerance + m_maxSwitchTime))
    {
      return true;
    }
  return false;
}
bool
ChannelCoordinator::IsGuardInterval (Time duration) const
{
  NS_LOG_FUNCTION (this << duration);
  Time future = GetIntervalTime (duration);
  // interval is either in CchInterval or SchInterval
  Time interval = future < m_cchInterval ? future : future - m_cchInterval;
  return interval < GetGuardInterval ();
}
Time
ChannelCoordinator::GetIntervalTime (Time duration) const
{
  NS_LOG_FUNCTION (this << duration);
  Time future = Now () + duration;
  Time sync = GetSyncInterval ();
  uint32_t n = future.GetMilliSeconds () / sync.GetMilliSeconds ();
  return future - MilliSeconds (n * sync.GetMilliSeconds ());
}

void
ChannelCoordinator::RegisterListener (ChannelCoordinationListener *listener)
{
  NS_LOG_FUNCTION (this << listener);
  m_listeners.push_back (listener);
}

void
ChannelCoordinator::Start (void)
{
  NS_LOG_FUNCTION (this);
  Time now = GetIntervalTime ();
  if (now == Time (0))
    {
      m_guardCount = 1;
      NotifyGuard ();
    }
  else if (now < m_cchInterval)
    {
      m_guardCount = 2;
      m_channelCoordinate = Simulator::Schedule ((m_cchInterval - now), &ChannelCoordinator::NotifyGuard, this);
    }
  else if (now == m_cchInterval)
    {
      m_guardCount = 2;
      NotifyGuard ();
    }
  else
    {
      m_guardCount = 1;
      m_channelCoordinate = Simulator::Schedule ((m_cchInterval + m_schInterval - now), &ChannelCoordinator::NotifyGuard, this);
    }
}
void
ChannelCoordinator::Stop (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_channelCoordinate.IsExpired ())
    {
      m_channelCoordinate.Cancel ();
    }
  m_guardCount = 0;
}
bool
ChannelCoordinator::IsStopped (void) const
{
  NS_LOG_FUNCTION (this);
  return m_guardCount == 0;
}

void
ChannelCoordinator::NotifySch (void)
{
  NS_LOG_FUNCTION (this);
  m_channelCoordinate = Simulator::Schedule (GetSchSlot (), &ChannelCoordinator::NotifyGuard, this);
  for (ListenersI i = m_listeners.begin (); i != m_listeners.end (); ++i)
    {
      (*i)->NotifySchStart (GetSchSlot ());
    }
}
void
ChannelCoordinator::NotifyCch (void)
{
  NS_LOG_FUNCTION (this);
  m_channelCoordinate = Simulator::Schedule (GetCchSlot (), &ChannelCoordinator::NotifyGuard, this);
  for (ListenersI i = m_listeners.begin (); i != m_listeners.end (); ++i)
    {
      (*i)->NotifyCchStart (GetCchSlot ());
    }
}
void
ChannelCoordinator::NotifyGuard (void)
{
  NS_LOG_FUNCTION (this);
  Time guardSlot = GetGuardInterval ();
  bool inCchi = ((++m_guardCount % 2) == 0);
  if (inCchi)
    {
      m_channelCoordinate = Simulator::Schedule (guardSlot, &ChannelCoordinator::NotifyCch, this);
    }
  else
    {
      m_channelCoordinate = Simulator::Schedule (guardSlot, &ChannelCoordinator::NotifySch, this);
    }
  for (ListenersI i = m_listeners.begin (); i != m_listeners.end (); ++i)
    {
      (*i)->NotifyGuardStart (guardSlot, inCchi);
    }
}

} // namespace ns3
