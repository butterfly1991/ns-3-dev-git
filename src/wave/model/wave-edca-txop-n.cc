/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 * Copyright (c) 2013 Dalian University of Technology
 *
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 * Author: Junling Bu <linlinjavaer@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/wifi-mac-trailer.h"
#include <map>
#include "wave-mac-queue.h"
#include "expire-time-tag.h"
#include "channel-manager.h"
#include "wave-edca-txop-n.h"

NS_LOG_COMPONENT_DEFINE ("WaveEdcaTxopN");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WaveEdcaTxopN);

TypeId
WaveEdcaTxopN::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WaveEdcaTxopN")
    .SetParent<EdcaTxopN> ()
    .AddConstructor<WaveEdcaTxopN> ()
  ;
  return tid;
}

WaveEdcaTxopN::WaveEdcaTxopN (void)
{
}

WaveEdcaTxopN::~WaveEdcaTxopN (void)
{

}
void
WaveEdcaTxopN::DoDispose (void)
{
  EdcaTxopN::DoDispose ();
  m_queues.clear ();
  m_scheduler = 0;
}
void
WaveEdcaTxopN::DoInitialize (void)
{
  EdcaTxopN::DoInitialize ();
  m_queues.insert (std::make_pair (CCH, CreateObject<WaveMacQueue> ()));
  m_queue = m_queues.find (CCH)->second;
  m_baManager->SetQueue (m_queue);
  m_baManager->SetMaxPacketDelay (m_queue->GetMaxDelay ());
}

void
WaveEdcaTxopN::SetChannelScheduler (Ptr<ChannelScheduler> scheduler)
{
  m_scheduler = scheduler;
}

void
WaveEdcaTxopN::StartTransmission (Ptr<const Packet> packet,
                                  const WifiMacHeader* hdr,
                                  MacLowTransmissionParameters params,
                                  MacLowTransmissionListener *listener)
{
  // if current channel access is not AlternatingAccess, just do as MacLow.
  if (m_scheduler->GetAccess () != ChannelScheduler::AlternatingAccess)
    {
      EdcaTxopN::StartTransmission (packet, hdr, params, listener);
      return;
    }

  Time transmissionTime = Low ()->CalculateTransmissionTime (packet, hdr, params);
  Time remainingTime = m_scheduler->GetChannelCoodinator ()->NeedTimeToGuardInterval ();
  Time t = Now ();

  if (transmissionTime > remainingTime)
    {
      NS_LOG_DEBUG ("transmission time = " << transmissionTime << " ,"
                                           << "remainingTime = " << remainingTime << " ,"
                                           << "this packet will be queued again." << t);
      EdcaTxopN::PushFront (packet, *hdr);
    }
  else
    {
      EdcaTxopN::StartTransmission (packet, hdr, params, listener);
    }
}

void
WaveEdcaTxopN::NotifyChannelSwitching (void)
{
  switch (m_scheduler->GetAccess ())
    {
    case ChannelScheduler::ContinuousAccess:
    case ChannelScheduler::ExtendedAccess:
      uint32_t cn;
      cn = m_scheduler->GetChannel ();
      NS_ASSERT (m_queues.find (cn) != m_queues.end ());
      // we will flush mac queue
      m_queues.find (cn)->second->Flush ();
      break;
    case ChannelScheduler::AlternatingAccess:
      // we not flush mac queue
      break;
    case ChannelScheduler::NoAccess:
      break;
    default:
      NS_FATAL_ERROR ("");
      break;
    }

  // the current packet will be released.
  m_currentPacket = 0;
}
void
WaveEdcaTxopN::Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  ChannelTag tag;
  bool result;
  uint32_t channelNumber;
  result = ConstCast<Packet> (packet)->RemovePacketTag (tag);
  if (result)
    {
      channelNumber = tag.GetChannelNumber ();
    }
  else
    {
      NS_FATAL_ERROR ("In WAVE, we should queue packet by qos and channel");
    }

  std::map<uint32_t,Ptr<WifiMacQueue> >::iterator i = m_queues.find (channelNumber);
  if (i == m_queues.end ())
    {
      m_queues.insert (std::make_pair (channelNumber, CreateObject<WaveMacQueue> ()));
      i = m_queues.find (channelNumber);
    }

  WifiMacTrailer fcs;
  uint32_t fullPacketSize = hdr.GetSerializedSize () + packet->GetSize () + fcs.GetSerializedSize ();
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr, packet, fullPacketSize);
  i->second->Enqueue (packet, hdr);

  // if the channel is active, then start access operation; otherwise just queue packet.
  if (m_scheduler->GetChannelManager ()->IsChannelActive (channelNumber))
    {
      StartAccessIfNeeded ();
    }
}

void
WaveEdcaTxopN::FlushAlternatingAccess (void)
{
  NS_ASSERT (m_scheduler->GetAccess () == ChannelScheduler::AlternatingAccess);
  // first flush CCH queue
  Ptr<WifiMacQueue> queue = m_queues.find (CCH)->second;
  queue->Flush ();
  // second flush SCH queue
  queue = m_queues.find (m_scheduler->GetChannel ())->second;
  queue->Flush ();
  m_currentPacket = 0;
}

void
WaveEdcaTxopN::SwitchToChannel (uint32_t channelNumber)
{
  std::map<uint32_t,Ptr<WifiMacQueue> >::iterator i = m_queues.find (channelNumber);
  if (i == m_queues.end ())
    {
      m_queues.insert (std::make_pair (channelNumber, CreateObject<WaveMacQueue> ()));
      i = m_queues.find (channelNumber);
    }
  NS_ASSERT (i != m_queues.end ());
  m_queue = i->second;
  m_baManager->SetQueue (m_queue);
  m_baManager->SetMaxPacketDelay (m_queue->GetMaxDelay ());

  NS_ASSERT (m_currentPacket == 0);
}

} // namespace ns3
