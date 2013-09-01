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
#ifndef VSA_REPEATER_H
#define VSA_REPEATER_H
#include <vector>
#include <stdint.h>
#include "ns3/wave-net-device.h"
#include "vendor-specific-action.h"

namespace ns3 {

struct VsaInfo;
class WaveNetDevice;

/**
 * when need, the class will sent VSA frame repeatedly and periodically.
 */
class VsaRepeater : public Object
{
public:
  static TypeId GetTypeId (void);
  VsaRepeater (void);
  VsaRepeater (WaveNetDevice *device);
  virtual ~VsaRepeater (void);

  void SetDevice (WaveNetDevice *device);
  WaveNetDevice * GetDevice (void);
  void SendVsa (const VsaInfo &vsaInfo);
  void RemoveByChannel (uint32_t channelNumber);
private:
  void DoDispose (void);


  // during the period of 5s, the number of Vendor Specific Action
  // frames to be transmitted.
  const static uint32_t VSA_REPEAT_PERIOD = 5;

  struct VsaWork
  {
    Mac48Address peer;
    OrganizationIdentifier oi;
    Ptr<Packet> vsc;
    uint32_t channelNumber;
    uint8_t repeatRate;
    uint32_t sentInterval;
    EventId repeat;
  };
  std::vector<VsaWork *> m_vsas;

  void DoRepeat (VsaWork *vsa);
  void DoSendVsaByInterval (uint32_t interval, uint32_t channel,
                            Ptr<Packet> p, OrganizationIdentifier oi, Mac48Address peer);

  WaveNetDevice *m_device;
};

}
#endif /* VSA_REPEATER_H */
