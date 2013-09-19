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
#ifndef WAVE_MAC_QUEUE_H
#define WAVE_MAC_QUEUE_H

#include "ns3/wifi-mac-queue.h"

namespace ns3 {

/**
 * this class allow higher layer control the lifetime of each packet.
 * If higher layer do not select, each packet will still has a lifetime
 * which is same for the mac queue.
 * If higher layer selects one, the lifetime will be the minimum value
 * between set by higher layer and set by mac queue.
 */
class WaveMacQueue : public WifiMacQueue
{
public:
  static TypeId GetTypeId (void);
  WaveMacQueue (void);
  virtual ~WaveMacQueue (void);
protected:
  virtual void Cleanup (void);
};

} // namespace ns3

#endif /* WAVE_MAC_QUEUE_H */
