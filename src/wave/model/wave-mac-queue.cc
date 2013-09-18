/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 *         Junling Bu <linlinjavaer@gmail.com>
 */
#include "ns3/simulator.h"
#include "wave-mac-queue.h"
#include "expire-time-tag.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WaveMacQueue);

TypeId
WaveMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WaveMacQueue")
    .SetParent<WifiMacQueue> ()
    .AddConstructor<WaveMacQueue> ()
  ;
  return tid;
}
WaveMacQueue::WaveMacQueue (void)
{
}
WaveMacQueue::~WaveMacQueue (void)
{
}
void
WaveMacQueue::Cleanup (void)
{
  if (m_queue.empty ())
    {
      return;
    }

  Time now = Simulator::Now ();
  for (PacketQueueI i = m_queue.begin (); i != m_queue.end (); )
    {
      ExpireTimeTag expireTimeTag;
      bool result;
      result = ConstCast<Packet> (i->packet)->RemovePacketTag (expireTimeTag);
      Time delay;
      if (result)
        {
          Time expire = MilliSeconds (expireTimeTag.GetExpireTime ());
          delay = expire < m_maxDelay ? expire : m_maxDelay;
        }
      else
        {
          delay = m_maxDelay;
        }

      if (i->tstamp + delay > now)
        {
          i++;
        }
      else
        {
          i = m_queue.erase (i);
        }
    }
}
} // namespace ns3
