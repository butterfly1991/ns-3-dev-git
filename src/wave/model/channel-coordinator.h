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
  ChannelCoordinator (void);
  virtual ~ChannelCoordinator (void);

  static Time GetDefaultCchInterval (void);
  static Time GetDefaultSchInterval (void);
  static Time GetDefaultSyncTolerance (void);
  static Time GetDefaultMaxSwitchTime (void);

  void SetCchInterval (Time cchInterval);
  Time GetCchInterval (void) const;
  void SetSchInterval (Time schInterval);
  Time GetSchInterval (void) const;
  Time GetSyncInterval (void) const;
  void SetSyncTolerance (Time syncTolerance);
  Time GetSyncTolerance (void) const;
  void SetMaxSwitchTime (Time maxSwitchTime);
  Time GetMaxSwitchTime (void) const;
  Time GetGuardInterval (void) const;

  /**
   * \returns whether current time is in SCH interval
   */
  bool IsSchInterval (void) const;
  /**
   * \param duration the future time after duration
   * \returns whether future time is in SCH interval
   */
  bool IsSchInterval (Time duration) const;
  /**
   * \returns whether current time is in CCH interval
   */
  bool IsCchInterval (void) const;
  /**
   * \param duration the future time after duration
   * \returns whether future time is in CCH interval
   */
  bool IsCchInterval (Time duration) const;
  /**
   * \returns whether current time is in Guard interval
   * If user want to get whether current guard interval
   * is in CCH guard interval or SCH guard interval, then
   * call IsSchInterval or IsCchInterval
   */
  bool IsGuardInterval (void) const;
  /**
   * \param duration the future time after duration
   * \returns whether future time is in Guard interval
   */
  bool IsGuardInterval (Time duration) const;
  /**
   * \returns whether current time is in SyncToleranc time
   */
  bool IsInSyncTolerance (void) const;
  /**
   * \param duration the future time after duration
   * \returns whether future time is in SyncToleranc time
   */
  bool IsInSyncTolerance (Time duration) const;
  /**
   * \returns whether current time is in MaxSwitchTime time
   *
   * although real channel switch time of PHY is less than MaxSwitchTime;
   * this method will return true if the time is in MaxSwitchTime.
   */
  bool IsInMaxSwitchTime (void) const;
  /**
   * \param duration the future time after duration
   * \returns whether future time is in MaxSwitchTime time
   */
  bool IsInMaxSwitchTime (Time duration) const;

  /**
   * \returns the duration time to next SCH interval
   * If current time is already in SCH interval, return 0;
   */
  Time NeedTimeToSchInterval (void) const;
  Time NeedTimeToSchInterval (Time duration) const;
  /**
   * \returns the duration time to next SCH interval
   * If current time is already in CCH interval, return 0;
   */
  Time NeedTimeToCchInterval (void) const;
  Time NeedTimeToCchInterval (Time duration) const;
  /**
   * \returns the duration time to next Guard interval
   * If current time is already in Guard interval, return 0;
   */
  Time NeedTimeToGuardInterval (void) const;
  Time NeedTimeToGuardInterval (Time duration) const;

  /**
   *  \return the time in a Sync Interval of current time
   *  for example:
   *  SyncInterval = 100ms;
   *  Now = 5s20ms;
   *  then GetIntervalTime () = 20ms.
   */
  Time GetIntervalTime (void) const;
  /**
   * \param duration the future time after duration
   * \return the time in a Sync Interval of future time
   *  for example:
   *  SyncInterval = 100ms;
   *  Now = 5s20ms;
   *  duration = 50ms;
   *  then GetIntervalTime (duration) = 70ms.
   */
  Time GetIntervalTime (Time duration) const;

  /**
   * Why need start and stop?
   * Actually we can Simulator::Schedule channel coordination events from Time(0).
   * However we should also support continuous access and extended access, and
   * if users do not need alternating access, these 50ms events are needless.
   * Normally the methods are called by ChannelScheduler when need.
   */
  void Start (void);
  void Stop (void);
  bool IsStopped (void) const;
  /**
   * \param listener the new listener pointer
   *
   * Add the input listener to the list of objects to be notified of
   * channel coordination events.
   * The listener object will be deleted and cleaned in ChannelCoordinator
   * when ChannelCoordinator destruct, so the caller should not delete
   * listener by itself.
   */
  void RegisterListener (ChannelCoordinationListener *listener);

protected:
  virtual void DoInitialize (void);
private:
  void NotifySch ();
  void NotifyCch ();
  void NotifyGuard ();
  /**
   * get SCH channel access time, default 46ms
   */
  Time GetSchSlot (void) const;
  /**
   * get CCH channel access time, default 46ms
   */
  Time GetCchSlot (void) const;

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
