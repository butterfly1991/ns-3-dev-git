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
#ifndef CHANNEL_COORDINATOR_H
#define CHANNEL_COORDINATOR_H
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"

namespace ns3 {

/**
 * \brief receive notifications about channel coordination events.
 */
class ChannelCoordinationListener
{
public:
  virtual ~ChannelCoordinationListener ()
  {
  }

  /**
   * \param duration the time CCHI continues, normally is 46ms
   * Although CCHI is 50ms and contains GuardI 4ms,
   * here CchStart event means after 4ms at the end of GuardI,
   * a real CCHI comes to start.
   */
  virtual void NotifyCchStart (Time duration) = 0;
  /**
   * \param duration the time SCHI continues, normally is 46ms
   * Although SCHI is 50ms and contains GuardI 4ms too,
   * here SchStart event means after 4ms at the end of GuardI,
   * a real SCHI comes to start.
   */
  virtual void NotifySchStart (Time duration) = 0;
  /**
   * \param duration the time GuardI continues, normally is 4ms
   * \param cchi indicate whether the guard is in CCHI
   * Although GuardI contains SyncTolerance interval and switch interval,
   * here we do not care, normally in GuardI the work is channel switch
   * and can neither send packets nor receive packets.
   */
  virtual void NotifyGuardStart (Time duration, bool cchi) = 0;
};
/**
 *  ChannelCoordinator deals with channel coordination in data
 *  plane (see 1609.4 chapter 5.2) and multi-channel synchronization
 *  in management plane (see 1609.4 chapter 6.2).
 *  The ChannelCoordinator will only be used when channel access
 *  is alternating class.
 *
 *      <          SyncI            > <            SyncI          >
 *          CchI          SchI             CchI           SchI
 * CCH |..************|              |..************|              |
 * SCH |              |..************|              |..************|
 *      .. is GuardI
 *
 *  The relation among CchI, SchI, GuardI, SyncI:
 *  1. they are all time durations, normally default CCH interval is 50ms,
 *  SCH interval is 50ms, Guard Interval is 4ms, SyncI is 100ms.
 *  2. Every UTC second shall be an integer number of SyncI, and the
 *  beginning of a sync interval shall align with beginning of UTC second;
 *  SyncI is sum of CchI and SchI. Channel will be in either CchI
 *  state or SchI state. If channel access is alternating, the channel
 *  shall do channel switch operation. At the beginning of each CCH interval
 *  or SCH interval is a guard interval. GuardI is sum of SyncTolerance
 *  and MaxSwitchTime.
 *  3. because some papers need to use dynamic CchI and SchI interval,
 *  so in this class, every UTC second may not be an integer number of SyncI.
 *
 *
 *   < receive only  > < No transmit and Receive > < receive only  >
 *  | SyncTolerance/2 |      MaxChSwitchTime      | SyncTolerance/2 |
 *  <---------------      Guard Interval     ---------------------->
 *
 * The relation among GuargInterval, SyncTolerance and MaxSwitchTime
 * 1.  GuardInterval = SyncTolerance/2 + MaxChSwitchTime + SyncTolerance/2
 * they are all time durations, normally default SyncTolerance is 2ms,
 * MaxChSwitchTime is 2ms, GuardInterval is 4ms.
 * 2. SyncTolerance/2 is used for synchronization that WAVE devices
 * may not be precisely aligned in time. MaxChSwitchTime represents that
 * WAVE device is in channel switch state. Although the channel switch time
 * ChannelSwitchDelay of YansWifiPhy is 250 microseconds, the channel switch
 * time of real wifi device is more than 1 milliseconds.
 */
class ChannelCoordinator : public Object
{
public:
  static TypeId GetTypeId (void);
  ChannelCoordinator ();
  virtual ~ChannelCoordinator ();

  virtual void SetCchInterval (Time cchInterval);
  virtual Time GetCchInterval ();
  virtual void SetSchInterval (Time schInterval);
  virtual Time GetSchInterval ();


  inline bool IsSchiNow ()
  {
    return IsSchiAfter (Seconds (0));
  }
  inline bool IsCchiNow ()
  {
    return IsCchiAfter (Seconds (0));
  }
  virtual bool IsSchiAfter (Time duration);
  virtual bool IsCchiAfter (Time duration);

  inline Time NeedTimeToSchiNow (void)
  {
    return NeedTimeToSchiAfter (Seconds (0));
  }
  virtual Time NeedTimeToSchiAfter (Time duration);
  inline Time NeedTimeToCchiNow (void)
  {
    return NeedTimeToCchiAfter (Seconds (0));
  }
  virtual Time NeedTimeToCchiAfter (Time duration);
  inline Time NeedTimeToGuardiNow (void)
  {
    return NeedTimeToGuardiAfter (Seconds (0));
  }
  virtual Time NeedTimeToGuardiAfter (Time duration);

  inline bool IsSyncToleranceNow (void)
  {
    return IsSyncToleranceAfter (Seconds (0));
  }
  virtual bool IsSyncToleranceAfter (Time duration);

  // although channel switch time of PHY is less than MaxSwitchTime;
  // this method will return true if the time is in MaxSwitchTime.
  inline bool IsMaxSwitchTimeNow (void)
  {
    return IsMaxSwitchTimeAfter (Seconds (0));
  }
  virtual bool IsMaxSwitchTimeAfter (Time duration);

  inline bool IsGuardiNow (void)
  {
    return IsGuardiAfter (Seconds (0));
  }
  virtual bool IsGuardiAfter (Time duration);
  /**
   *  return the time in a Sync Interval
   *  for example:
   *  SyncInterval = 100ms;
   *  Now = 5s20ms;
   *  then GetIntervalTimeNow () = 20ms.
   */
  inline Time GetIntervalTimeNow ()
  {
    return GetIntervalTimeAfter (Seconds (0));
  }
  /**
   * \param duration the future time after duration
   *  return the time in a Sync Interval
   *  for example:
   *  SyncInterval = 100ms;
   *  Now = 5s20ms;
   *  duration = 50ms;
   *  then GetIntervalTimeAfter (duration) = 70ms.
   */
  Time GetIntervalTimeAfter (Time duration);
  /**
   * Why need start and stop?
   * Actually we can Simulator::Schedule channel coordination events from Time(0).
   * However we should also support continuous access and extended access, and
   * if users do not need alternating access, these 50ms events are needless.
   * Normally the methods are called by ChannelScheduler when need.
   */
  void Start ();
  void Stop ();
  bool IsStopped ();
  /**
   * \param listener the new listener pointer
   *
   * Add the input listener to the list of objects to be notified of
   * channel coordination events.
   * The listener object will be deleted and cleaned in ChannelCoordinator
   * when ChannelCoordinator destruct, so the caller should not delete
   * listener by itself.
   */
  virtual void RegisterListener (ChannelCoordinationListener *listener);

  virtual void SetSyncTolerance (Time syncTolerance);
  virtual Time GetSyncTolerance ();
  virtual void SetMaxSwitchTime (Time maxSwitchTime);
  virtual Time GetMaxSwitchTime ();

  virtual Time GetSyncInterval ();
  virtual Time GetGuardInterval ();

  static Time GetDefaultCchInterval ();
  static Time GetDefaultSchInterval ();
  static Time GetDefaultSyncTolerance ();
  static Time GetDefaultMaxSwitchTime ();
protected:
  virtual void DoInitialize (void);
private:
  void NotifySch ();
  void NotifyCch ();
  void NotifyGuard ();
  /**
   * means SCH channel access time, default 46ms
   */
  inline Time GetSchSlot ()
  {
    return m_schInterval - (m_syncTolerance + m_maxSwitchTime);
  }
  /**
   * means CCH channel access time, default 46ms
   */
  inline Time GetCchSlot ()
  {
    return m_cchInterval - (m_syncTolerance + m_maxSwitchTime);
  }
  Time m_cchInterval;
  Time m_schInterval;
  Time m_syncTolerance;
  Time m_maxSwitchTime;

  typedef std::vector<ChannelCoordinationListener *> Listeners;
  typedef std::vector<ChannelCoordinationListener *>::iterator ListenersI;

  Listeners m_listeners;

  EventId m_channelCoordinate;
  uint32_t m_guardCount;
};


}

#endif /* CHANNEL_COORDINATOR_H */
