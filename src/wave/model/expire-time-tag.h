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
#ifndef EXPIRE_TIME_TAG_H
#define EXPIRE_TIME_TAG_H

#include "ns3/packet.h"

namespace ns3 {

class Tag;

/**
 * Now WifiMacQueue already contains a parameter named MaxDelay to
 * support lifetime, but that is queue-based which means the all packets
 * in the queue are the same.
 * This tag will be used in WaveMacQueue to support lifetime of packets.
 * Every packet can add this tag to control different lifetime.
 * If a packet has not added this tag, then the lifetime will be default
 * MaxDelay of WifiMacQueue.
 */
class ExpireTimeTag : public Tag
{
public:
  ExpireTimeTag (void);
  ExpireTimeTag (uint32_t expire);
  virtual ~ExpireTimeTag (void);

  void SetExpireTime (uint32_t expire);
  uint32_t GetExpireTime (void) const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  uint32_t m_expire;
};

} // namespace ns3

#endif /* EXPIRE_TIME_TAG_H */
