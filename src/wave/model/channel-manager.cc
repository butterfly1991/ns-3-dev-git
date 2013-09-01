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
#include "channel-manager.h"
#include "ns3/log.h"
#include "ns3/assert.h"

NS_LOG_COMPONENT_DEFINE ("ChannelManager");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ChannelManager);

TypeId
ChannelManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ChannelManager")
    .SetParent<Object> ()
    .AddConstructor<ChannelManager> ()
  ;
  return tid;
}

ChannelManager::ChannelManager ()
{
  WaveChannel c = {0, DEFAULT_CCH_OPERATING_CLASS, true, Ofdm6Mbps, 4, ChannelDead};
  for (uint32_t channel_index = 0; channel_index != CHANNELS_OF_WAVE; ++channel_index)
    {
      WaveChannel *channel = new WaveChannel;
      *channel = c;
      channel->channelNumber = SCH1 + 2 * channel_index;
      m_channels.push_back (channel);
    }
}
ChannelManager::~ChannelManager ()
{
  std::vector<WaveChannel *>::iterator i;
  for (i = m_channels.begin (); i != m_channels.end (); ++i)
    {
      delete (*i);
    }
  m_channels.clear ();
}
bool
ChannelManager::IsCch (uint32_t channelNumber)
{
  return channelNumber == CCH;
}
bool
ChannelManager::IsSch (uint32_t channelNumber)
{
  if (channelNumber < SCH1 || channelNumber > SCH6)
    {
      return false;
    }
  if (channelNumber % 2 == 1)
    {
      return false;
    }
  return (channelNumber != CCH);
}
bool
ChannelManager::IsWaveChannel (uint32_t channelNumber)
{
  return IsCch (channelNumber) || IsSch (channelNumber);
}

uint32_t
ChannelManager::GetIndex (uint32_t channelNumber)
{
  if (channelNumber < SCH1 || channelNumber > SCH6)
    {
      return CHANNELS_OF_WAVE;
    }
  if (channelNumber % 2 == 1)
    {
      return CHANNELS_OF_WAVE;
    }
  return (channelNumber - SCH1) / 2;
}

enum ChannelManager::ChannelState
ChannelManager::GetState (uint32_t channelNumber)
{
  uint32_t index = GetIndex (channelNumber);
  return m_channels[index]->state;
}
void
ChannelManager::SetState (uint32_t channelNumber, enum ChannelManager::ChannelState state)
{
  uint32_t index = GetIndex (channelNumber);
  m_channels[index]->state = state;
}
bool
ChannelManager::IsChannelActive (uint32_t channelNumber)
{
  return (GetState (channelNumber)) == ChannelActive;
}
bool
ChannelManager::IsChannelInactive (uint32_t channelNumber)
{
  return (GetState (channelNumber)) == ChannelInactive;
}
bool
ChannelManager::IsChannelDead (uint32_t channelNumber)
{
  return (GetState (channelNumber)) == ChannelDead;
}
uint32_t
ChannelManager::GetOperatingClass (uint32_t channelNumber)
{
  uint32_t index = GetIndex (channelNumber);
  return m_channels[index]->operatingClass;
}
bool
ChannelManager::IsAdapter (uint32_t channelNumber)
{
  uint32_t index = GetIndex (channelNumber);
  return m_channels[index]->adapter;
}
enum DataRate
ChannelManager::GetDataRate (uint32_t channelNumber)
{
  uint32_t index = GetIndex (channelNumber);
  return m_channels[index]->dataRate;
}
uint32_t
ChannelManager::GetTxPowerLevel (uint32_t channelNumber)
{
  uint32_t index = GetIndex (channelNumber);
  return m_channels[index]->txPowerLevel;
}

} // namespace ns3
