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
#include "channel-scheduler.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mac-trailer.h"
#include "ns3/mac-low.h"
#include "ocb-wifi-mac.h"

NS_LOG_COMPONENT_DEFINE ("ChannelScheduler");

namespace ns3 {

class CoordinationListener : public ChannelCoordinationListener
{
public:
  CoordinationListener (ChannelScheduler * scheduler)
    : m_scheduler (scheduler)
  {
  }
  virtual ~CoordinationListener ()
  {
  }
  virtual void NotifyCchStart (Time duration)
  {
    m_scheduler->NotifyCchStartNow (duration);
  }
  virtual void NotifySchStart (Time duration)
  {
    m_scheduler->NotifySchStartNow (duration);
  }
  virtual void NotifyGuardStart (Time duration, bool cchi)
  {
    m_scheduler->NotifyGuardStartNow (duration, cchi);
  }
private:
  ChannelScheduler * m_scheduler;
};

NS_OBJECT_ENSURE_REGISTERED (ChannelScheduler);

TypeId
ChannelScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ChannelScheduler")
    .SetParent<Object> ()
    .AddConstructor<ChannelScheduler> ()
  ;
  return tid;
}

ChannelScheduler::ChannelScheduler (void)
  : m_device (0),
    m_channelNumber (0),
    m_extend (0),
    m_channelAccess (NoAccess),
    m_coordinationListener (0)
{
  NS_LOG_FUNCTION (this);
}
ChannelScheduler::~ChannelScheduler (void)
{
  NS_LOG_FUNCTION (this);
}

void
ChannelScheduler::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_mac = m_device->GetMac ();
  m_phy = m_device->GetPhy ();
  Ptr<OcbWifiMac> mac = DynamicCast<OcbWifiMac> (m_mac);
  NS_ASSERT (mac != 0);

  Ptr<WaveEdcaTxopN> edcaQueue = CreateObject<WaveEdcaTxopN> ();
  edcaQueue->SetChannelScheduler (this);
  mac->SetWaveEdcaQueue (AC_VO, edcaQueue);
  m_edcaQueues.insert (std::make_pair (AC_VO, edcaQueue));

  edcaQueue = CreateObject<WaveEdcaTxopN> ();
  edcaQueue->SetChannelScheduler (this);
  mac->SetWaveEdcaQueue (AC_VI, edcaQueue);
  m_edcaQueues.insert (std::make_pair (AC_VI, edcaQueue));

  edcaQueue = CreateObject<WaveEdcaTxopN> ();
  edcaQueue->SetChannelScheduler (this);
  mac->SetWaveEdcaQueue (AC_BE, edcaQueue);
  m_edcaQueues.insert (std::make_pair (AC_BE, edcaQueue));

  edcaQueue = CreateObject<WaveEdcaTxopN> ();
  edcaQueue->SetChannelScheduler (this);
  mac->SetWaveEdcaQueue (AC_BK, edcaQueue);
  m_edcaQueues.insert (std::make_pair (AC_BK, edcaQueue));

  m_coordinationListener = new CoordinationListener (this);
  m_coordinator->RegisterListener (m_coordinationListener);
}

void
ChannelScheduler::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_edcaQueues.clear ();

  if (!m_coordinator->IsStopped ())
    {
      m_coordinator->Stop ();
    }
  // we just need set to 0,
  // WaveNetDevice will call DoDispose of m_manager and m_coordinator
  m_manager = 0;
  m_coordinator = 0;
  m_device = 0;
  delete m_coordinationListener;
  m_coordinationListener = 0;
}

void
ChannelScheduler::SetWaveNetDevice (Ptr<WaveNetDevice> device)
{
  m_device = device;
}

void
ChannelScheduler::SetChannelManager (Ptr<ChannelManager> manager)
{
  m_manager = manager;
}
void
ChannelScheduler::SetChannelCoodinator (Ptr<ChannelCoordinator> coordinator)
{
  m_coordinator = coordinator;
}
Ptr<ChannelCoordinator>
ChannelScheduler::GetChannelCoodinator (void)
{
  return m_coordinator;
}
Ptr<ChannelManager>
ChannelScheduler::GetChannelManager (void)
{
  return m_manager;
}

bool
ChannelScheduler::IsAccessAssigned (uint32_t channelNumber) const
{
  switch (m_channelAccess)
    {
    // continuous access is similar to extended access except extend operation
    case ContinuousAccess:
    case ExtendedAccess:
      return m_channelNumber == channelNumber;
    case AlternatingAccess:
      return (channelNumber == CCH) || (m_channelNumber == channelNumber);
    case NoAccess:
      NS_ASSERT (m_channelNumber == 0);
      return false;
    default:
      NS_FATAL_ERROR ("we could not get here");
      return false;
    }
}
bool
ChannelScheduler::IsAccessAssigned (void) const
{
  return (GetAccess () != NoAccess);
}
enum ChannelScheduler::ChannelAccess
ChannelScheduler::GetAccess (uint32_t channelNumber) const
{
  if (channelNumber == CCH && m_channelAccess == AlternatingAccess)
    {
      return AlternatingAccess;
    }
  if (m_channelNumber == channelNumber)
    {
      return m_channelAccess;
    }
  return NoAccess;
}
enum ChannelScheduler::ChannelAccess
ChannelScheduler::GetAccess (void) const
{
  return m_channelAccess;
}
uint32_t
ChannelScheduler::GetChannel () const
{
  return m_channelNumber;
}

bool
ChannelScheduler::AssignAlternatingAccess (uint32_t channelNumber, bool immediate)
{
  NS_LOG_FUNCTION (this << channelNumber << immediate);
  uint32_t cn = channelNumber;
  if (cn == CCH)
    {
      return false;
    }

  // if channel access is already assigned for the same channel ,
  // we just need to return successfully
  if (m_channelAccess == AlternatingAccess && channelNumber == m_channelNumber)
    {
      return true;
    }

  // channel access is already assigned for other channel
  if (m_channelNumber != 0)
    {
      return false;
    }

  // if we need immediately switch SCH in CCHI, or we are in SCHI now,
  // we switch to CCH channel.
  // another issue we need consider but is not declared in the standard
  // is remaining time, suppose we are now at 49.9999ms, and we begin
  // SCH switch operation, however at 50ms there will come another switch
  // operation, this could cause assert by the phy class.
  if ((immediate || m_coordinator->IsSchIntervalNow ())
      && (m_coordinator->NeedTimeToGuardiNow () <= m_coordinator->GetMaxSwitchTime ()))
    {
      m_manager->SetState (cn, ChannelManager::CHANNEL_ACTIVE);
      m_manager->SetState (CCH, ChannelManager::CHANNEL_INACTIVE);
      m_phy->SetChannelNumber (cn);
      SwitchQueueToChannel (cn);
    }
  else
    {
      m_manager->SetState (CCH, ChannelManager::CHANNEL_ACTIVE);
      m_manager->SetState (cn, ChannelManager::CHANNEL_INACTIVE);
    }
  m_channelNumber = cn;
  m_channelAccess = AlternatingAccess;
  // start channel coordination operation which will send event
  // about CCH interval, SCH interval and guard interval repeatedly.
  m_coordinator->Start ();

  return true;
}
bool
ChannelScheduler::AssignContinuousAccess (uint32_t channelNumber, bool immediate)
{
  NS_LOG_FUNCTION (this << channelNumber << immediate);
  uint32_t cn = channelNumber;
  // if channel access is already assigned for the same channel ,
  // we just need to return successfully
  if (m_channelAccess == ContinuousAccess && channelNumber == m_channelNumber)
    {
      return true;
    }

  // channel access is already assigned for other channel
  if (m_channelNumber != 0)
    {
      return false;
    }

  bool switchNow = (ChannelManager::IsCch (cn) && m_coordinator->IsCchIntervalNow ())
    || (ChannelManager::IsSch (cn) && m_coordinator->IsSchIntervalNow ());
  if (immediate || switchNow)
    {
      m_phy->SetChannelNumber (cn);
      SwitchQueueToChannel (cn);
      m_manager->SetState (cn, ChannelManager::CHANNEL_ACTIVE);
      m_channelNumber = cn;
      m_channelAccess = ContinuousAccess;
    }
  else
    {
      Time wait = ChannelManager::IsCch (cn) ? m_coordinator->NeedTimeToCchiNow () : m_coordinator->NeedTimeToSchiNow ();
      m_waitEvent = Simulator::Schedule (wait, &ChannelScheduler::AssignContinuousAccess, this, cn, false);
    }

  return true;
}
bool
ChannelScheduler::AssignExtendedAccess (uint32_t channelNumber, uint32_t extends, bool immediate)
{
  NS_LOG_FUNCTION (this << channelNumber << extends << immediate);
  uint32_t cn = channelNumber;

  // channel access is already for the same channel
  if ((m_channelAccess == ExtendedAccess) && (m_channelNumber == channelNumber) && (extends <= m_extend))
    {
      return true;
    }

  // channel access is already assigned for other channel
  if (m_channelNumber != 0)
    {
      return false;
    }

  bool switchNow = (ChannelManager::IsCch (cn) && m_coordinator->IsCchIntervalNow ())
    || (ChannelManager::IsSch (cn) && m_coordinator->IsSchIntervalNow ());
  Time wait = ChannelManager::IsCch (cn) ? m_coordinator->NeedTimeToCchiNow () : m_coordinator->NeedTimeToSchiNow ();

  if (immediate || switchNow)
    {
      m_extend = extends;
      m_phy->SetChannelNumber (cn);
      SwitchQueueToChannel (cn);
      m_manager->SetState (cn, ChannelManager::CHANNEL_ACTIVE);
      m_channelNumber = cn;
      m_channelAccess = ExtendedAccess;

      Time sync = m_coordinator->GetSyncInterval ();
      NS_ASSERT (extends != 0 && extends < 0xff);
      // the wait time to proper interval will not be calculated as extended time.
      Time extendedDuration = wait + MilliSeconds (extends * sync.GetMilliSeconds ());
      // after end_duration time, ChannelScheduler will release channel access automatically
      m_extendEvent = Simulator::Schedule (extendedDuration, &ChannelScheduler::Release, this, cn);
    }
  else
    {
      m_waitEvent = Simulator::Schedule (wait, &ChannelScheduler::AssignExtendedAccess, this, cn, extends, false);
    }
  return true;
}

void
ChannelScheduler::Release (uint32_t channelNumber)
{
  NS_LOG_FUNCTION (this << channelNumber);
  uint32_t cn = channelNumber;

  if (!IsAccessAssigned (cn))
    {
      NS_LOG_DEBUG ("the channel access of CH =" << cn << " "
                    " has already been released.");
      return;
    }

  switch (m_channelAccess)
    {
    // continuous access is similar to extended access except extend operation
    // here we can release continuous access as releasing extended access
    case ContinuousAccess:
    case ExtendedAccess:
      m_manager->SetState (m_channelNumber, ChannelManager::CHANNEL_DEAD);
      m_phy->SetChannelNumber (CCH);
      m_extend = 0;
      m_waitEvent.Cancel ();
      m_extendEvent.Cancel ();
      break;
    case AlternatingAccess:
      m_manager->SetState (CCH, ChannelManager::CHANNEL_DEAD);
      m_manager->SetState (m_channelNumber, ChannelManager::CHANNEL_DEAD);
      m_phy->SetChannelNumber (CCH);
      // since channel switch will not flush queue automatically now,
      // so we need flush queues by hand
      for (EdcaWaveQueuesI i = m_edcaQueues.begin (); i != m_edcaQueues.end (); ++i)
        {
          i->second->FlushAlternatingAccess ();
        }
      NS_ASSERT (!m_coordinator->IsStopped ());
      // stop channel coordinator operation
      m_coordinator->Stop ();
      break;
    case NoAccess:
      break;
    default:
      NS_FATAL_ERROR ("cannot get here");
      break;
    }

  m_channelNumber = 0;
  m_channelAccess = NoAccess;
}
void
ChannelScheduler::SwitchQueueToChannel (uint32_t channelNumber)
{
  for (EdcaWaveQueuesI i = m_edcaQueues.begin (); i != m_edcaQueues.end (); ++i)
    {
      i->second->SwitchToChannel (channelNumber);
    }
}
void
ChannelScheduler::QueueStartAccess (void)
{
  for (EdcaWaveQueuesI i = m_edcaQueues.begin (); i != m_edcaQueues.end (); ++i)
    {
      i->second->StartAccessIfNeeded ();
    }
}


void
ChannelScheduler::NotifyCchStartNow (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  NS_ASSERT (m_channelAccess == AlternatingAccess);
  m_manager->SetState (m_channelNumber, ChannelManager::CHANNEL_INACTIVE);
  m_manager->SetState (CCH, ChannelManager::CHANNEL_ACTIVE);
  QueueStartAccess ();
}
void
ChannelScheduler::NotifySchStartNow (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  NS_ASSERT (m_channelAccess == AlternatingAccess);
  m_manager->SetState (m_channelNumber, ChannelManager::CHANNEL_ACTIVE);
  m_manager->SetState (CCH, ChannelManager::CHANNEL_INACTIVE);
  QueueStartAccess ();
}

void
ChannelScheduler::NotifyGuardStartNow (Time duration, bool cchi)
{
  NS_LOG_FUNCTION (this << duration << cchi);
  NS_ASSERT (m_channelAccess == AlternatingAccess);
  // this could happen as section 6.3.3 immediate SCH access describes.
  // In CCHI, we switch to SCH channel immediately, and when next SCHI starts,
  // the guard interval will not be used. Or initial channel is CCH, when we
  // assign channel access, we want to switch to CCH again.
  // this check will be true only once when alternating access is just assigned.
//  uint16_t cn = m_phy->GetChannelNumber ();
//  if ((cchi && ChannelManager::IsCch (cn)) || (!cchi && ChannelManager::IsSch (cn)))
//	  return;

  if (cchi)
    {
      NS_ASSERT (m_coordinator->IsCchIntervalNow () && m_coordinator->IsGuardIntervalNow ());
      m_phy->SetChannelNumber (CCH);
      SwitchQueueToChannel (CCH);
    }
  else
    {
      m_phy->SetChannelNumber (m_channelNumber);
      SwitchQueueToChannel (m_channelNumber);
    }
  Ptr<OcbWifiMac> ocbmac = DynamicCast<OcbWifiMac> (m_mac);
  NS_ASSERT (ocbmac);
  // see chapter 6.2.5 Sync tolerance
  // a medium busy shall be declared during the guard interval.
  ocbmac->NotifyBusy (duration);
}
} // namespace ns3
