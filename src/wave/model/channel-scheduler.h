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
#ifndef CHANNE_SCHEDULER_H
#define CHANNE_SCHEDULER_H

#include <map>
#include "ns3/qos-utils.h"
#include "channel-manager.h"
#include "channel-coordinator.h"
#include "wave-net-device.h"
#include "wave-edca-txop-n.h"
namespace ns3 {
class WaveNetDevice;
class WaveEdcaTxopN;
/**
 * \brief
 * \ingroup wave
 * This class assigns channel access for users' requirements.
 * The channel access options include continuous access, alternating
 * service channel and CCH access, immediate SCH access, and extended
 * access.
 */
class ChannelScheduler : public Object
{
public:
  static TypeId GetTypeId (void);
  ChannelScheduler (void);
  virtual ~ChannelScheduler (void);
  void SetWaveNetDevice (Ptr<WaveNetDevice> device);
  void SetChannelManager (Ptr<ChannelManager> manager);
  void SetChannelCoodinator (Ptr<ChannelCoordinator> coordinator);
  Ptr<ChannelCoordinator> GetChannelCoodinator (void);
  Ptr<ChannelManager> GetChannelManager (void);

  enum ChannelAccess
  {
    ContinuousAccess,
    AlternatingAccess,
    ExtendedAccess,
    NoAccess,
  };
  bool IsAccessAssigned (uint32_t channelNumber) const;
  bool IsAccessAssigned () const;
  enum ChannelScheduler::ChannelAccess GetAccess (uint32_t channelNumber) const;
  enum ChannelScheduler::ChannelAccess GetAccess () const;
  uint32_t GetChannel () const;

  /**
   * \param channelNumber the specific channel
   * \param immediate indicate whether channel switch to channel
   * \return whether the channel access is assigned successfully
   *
   * This method will assign alternating access.
   * Unimplemented now
   */
  bool AssignAlternatingAccess (uint32_t channelNumber, bool immediate);
  /**
   * \param channelNumber the specific channel
   * \param immediate indicate whether channel switch to channel
   * \return whether the channel access is assigned successfully
   *
   * This method will assign continuous access.
   */
  bool AssignContinuousAccess (uint32_t channelNumber, bool immediate);
  /**
   * \param channelNumber the specific channel
   * \param immediate indicate whether channel switch to channel
   * \return whether the channel access is assigned successfully
   *
   * This method will assign extended access.
   */
  bool AssignExtendedAccess (uint32_t channelNumber, uint32_t extends, bool immediate);
  /**
   * \param channelNumber release channel source of the channel
   * when release, we will drop all queued packets in the MAC layer.
   * We use channel switch event of phy to notify mac to do this work.
   * After release, no channel access is assigned for mac although the phy is in
   * CCH channel and ready to send packets.
   */
  void Release (uint32_t channelNumber);

  /**
   * The three methods are used when channel access is AlternatingAccess.
   * The main work is to change the state of CCH channel and SCH mac channel.
   * and do channel switch operation.
   * Different from channel switch operation in other access which will cause
   * mac entity reset and flush mac queue, the channel switch operation
   * happened during the guard interval will make previous channel mac queue
   * suspended, and next channel mac queue started or resumed.
   */
  void NotifyCchStartNow (Time duration);
  void NotifySchStartNow (Time duration);
  void NotifyGuardStartNow (Time duration, bool cchi);
private:
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  void SwitchQueueToChannel (uint32_t channelNumber);

  void QueueStartAccess (void);

  Ptr<ChannelManager> m_manager;
  Ptr<ChannelCoordinator> m_coordinator;
  Ptr<WaveNetDevice> m_device;

  Ptr<WifiMac> m_mac;
  Ptr<WifiPhy> m_phy;

  typedef std::map<AcIndex, Ptr<WaveEdcaTxopN> > EdcaWaveQueues;
  typedef std::map<AcIndex, Ptr<WaveEdcaTxopN> >::iterator EdcaWaveQueuesI;
  EdcaWaveQueues m_edcaQueues;

  /**
   *  when m_channelAccess is ContinuousAccess, m_channelNumber
   *   is continuous channel number;
   *  when m_channelAccess is AlternatingAccess, m_channelNumber
   *   is SCH channel number, another alternating channel is CCH;
   *  when m_channelAccess is ExtendedAccess, m_channelNumber
   *   is extended access, extends is the number of extends access.
   */
  uint32_t m_channelNumber;
  uint32_t m_extend;
  enum ChannelAccess m_channelAccess;

  EventId m_waitEvent;
  EventId m_extendEvent;

  ChannelCoordinationListener *m_coordinationListener;
};

}
#endif /* CHANNE_SCHEDULER_H */
