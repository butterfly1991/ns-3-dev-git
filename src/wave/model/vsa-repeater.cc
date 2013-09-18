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
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/simulator.h"
#include "ns3/qos-tag.h"

#include "vsa-repeater.h"
#include "ocb-wifi-mac.h"
#include "higher-tx-tag.h"

NS_LOG_COMPONENT_DEFINE ("VsaRepeater");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (VsaRepeater);

TypeId
VsaRepeater::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VsaRepeater")
    .SetParent<Object> ()
    .AddConstructor<VsaRepeater> ()
  ;
  return tid;
}

VsaRepeater::VsaRepeater (void)
  : m_device (0)
{
}
VsaRepeater::VsaRepeater (WaveNetDevice *device)
{
  m_device = device;
}

VsaRepeater::~VsaRepeater (void)
{

}
void
VsaRepeater::DoDispose (void)
{
  for (std::vector<VsaWork *>::iterator i = m_vsas.begin ();
       i != m_vsas.end (); ++i)
    {
      if (!(*i)->repeat.IsExpired ())
        {
          (*i)->repeat.Cancel ();
        }
      (*i)->vsc = 0;
      delete (*i);
    }
  m_vsas.clear ();
}
void
VsaRepeater::SetDevice (WaveNetDevice *device)
{
  m_device = device;
}
WaveNetDevice *
VsaRepeater::GetDevice (void)
{
  return m_device;
}
void
VsaRepeater::SendVsa (const VsaInfo &vsaInfo)
{
  OrganizationIdentifier oi;
  if (vsaInfo.oi.IsNull ())
    {
      uint8_t oibytes[5] = {0x00, 0x50, 0xC2, 0x4A, 0x40};
      oibytes[4] |= (vsaInfo.managementId & 0x0f);
      oi = OrganizationIdentifier (oibytes, 5);
    }
  else
    {
      oi = vsaInfo.oi;
    }

  Ptr<Packet> p = vsaInfo.vsc;

  // refer to 1609.4-2010 chapter 5.4.1
  // Management frames are assigned the highest AC (AC_VO).
  QosTag qosTag (7);
  p->AddPacketTag (qosTag);
  ChannelTag channelTag (vsaInfo.channelNumber);
  p->AddPacketTag (channelTag);

  // if destination MAC address indicates a unicast address,
  // we can only sent one VSA frame, and repeat rate is ignored;
  // or repeat rate is 0, we also send one VSA frame.
  if (vsaInfo.peer.IsGroup () && (vsaInfo.repeatRate != 0))
    {
      VsaWork *vsa = new VsaWork ();
      // XXX REDO, it is a bad implementation to change enum to uint32_t,
      // however the compiler that not supports c++11 cannot forward declare enum
      // I need to find a better way.
      vsa->sentInterval = vsaInfo.sendInterval;
      vsa->channelNumber = vsaInfo.channelNumber;
      vsa->peer = vsaInfo.peer;
      vsa->repeatRate = vsaInfo.repeatRate;
      vsa->vsc = vsaInfo.vsc->Copy ();
      vsa->oi = oi;
      Time delay = MilliSeconds (VSA_REPEAT_PERIOD * 1000 / vsa->repeatRate);
      vsa->repeat =  Simulator::Schedule (delay, &VsaRepeater::DoRepeat, this, vsa);
      m_vsas.push_back (vsa);
    }
  DoSendVsaByInterval (vsaInfo.sendInterval, vsaInfo.channelNumber, p, oi, vsaInfo.peer);
}

void
VsaRepeater::DoRepeat (VsaWork *vsa)
{
  NS_ASSERT (vsa->repeatRate != 0);
  // next time the vendor specific content packet will be sent repeatedly.
  Time delay = MilliSeconds (VSA_REPEAT_PERIOD * 1000 / vsa->repeatRate);
  vsa->repeat =  Simulator::Schedule (delay, &VsaRepeater::DoRepeat, this, vsa);
  DoSendVsaByInterval (vsa->sentInterval, vsa->channelNumber, vsa->vsc->Copy (), vsa->oi, vsa->peer);
}

/**
 * XXX REDO, to ensure VSAs can be sent on the channel and in the channel interval,
 * we should consider current channel access assigned and current channel interval.
 * Now we just consider current channel interval to sent immediately or wait.
 * But if channel access is alternating access and we want to sent VSAs on CCH and
 * in SCHI, this could be never succeed. And if channel access is continuous or
 * extended access and we want to sent only in CCHI, current implementation will exist
 * the bug: although VSAs is sent to MAC queue in CCHI, but it could be queued to
 * send in next SCHI,
 */
void
VsaRepeater::DoSendVsaByInterval (uint32_t interval, uint32_t channel,
                                  Ptr<Packet> packet, OrganizationIdentifier oi, Mac48Address peer)
{
  NS_ASSERT (interval < 4);
  Ptr<ChannelCoordinator> coordinator = m_device->GetChannelCoordinator ();
  Ptr<ChannelManager> manager = m_device->GetChannelManager ();
  Ptr<ChannelScheduler> scheduler = m_device->GetChannelScheduler ();
  bool immediate = false;
  // if request is for transmitting in SCH Interval (or CCH Interval),
  // but now is not in SCH Interval (or CCH Interval) and , then we
  // will wait some time to send this vendor specific content packet.
  // if request is for transmitting in any channel interval, then we
  // send packets immediately.
  if (interval == 1)
    {
      Time wait = coordinator->NeedTimeToSchiNow ();
      if (wait.GetMilliSeconds () == 0)
        {
          immediate = true;
        }
      else
        {
          Simulator::Schedule (wait, &VsaRepeater::DoSendVsaByInterval, this,
                               interval, channel, packet, oi, peer);
        }
    }
  else if (interval == 2)
    {
      Time wait = coordinator->NeedTimeToCchiNow ();
      if (wait.GetMilliSeconds () == 0)
        {
          immediate = true;
        }
      else
        {
          Simulator::Schedule (wait, &VsaRepeater::DoSendVsaByInterval, this,
                               interval, channel, packet, oi, peer);
        }
    }
  else if (interval == 3)
    {
      immediate = true;
    }

  if (!immediate)
    {
      return;
    }

  if (!scheduler->IsAccessAssigned (channel))
    {
      return;
    }

  WifiMode datarate =  m_device->GetPhy ()->GetMode (manager->GetDataRate (channel));
  WifiTxVector txVector;
  txVector.SetTxPowerLevel (manager->GetTxPowerLevel (channel));
  txVector.SetMode (datarate);
  HigherDataTxVectorTag tag = HigherDataTxVectorTag (txVector, manager->IsAdapter (channel));
  packet->AddPacketTag (tag);

  Ptr<WifiMac> mac = m_device->GetMac ();
  Ptr<OcbWifiMac> ocbmac = DynamicCast<OcbWifiMac> (mac);
  NS_ASSERT (ocbmac != 0);
  ocbmac->SendVsc (packet, peer, oi);
}

void
VsaRepeater::RemoveByChannel (uint32_t channelNumber)
{

  for (std::vector<VsaWork *>::iterator i = m_vsas.begin ();
       i != m_vsas.end (); )
    {
      if ((*i)->channelNumber == channelNumber)
        {
          if (!(*i)->repeat.IsExpired ())
            {
              (*i)->repeat.Cancel ();
            }
          (*i)->vsc = 0;
          delete (*i);
          i = m_vsas.erase (i);
        }
      else
        {
          ++i;
        }
    }
}

} // namespace ns3
