/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *         Junling Bu <linlinjavaer@gmail.com>
 */
#include "ns3/wifi-mac.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-channel.h"
#include "ns3/llc-snap-header.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/node.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/log.h"
#include "ns3/qos-tag.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"

#include "ocb-wifi-mac.h"
#include "wave-net-device.h"
#include "expire-time-tag.h"
#include "higher-tx-tag.h"

NS_LOG_COMPONENT_DEFINE ("WaveNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WaveNetDevice);

TypeId
WaveNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WaveNetDevice")
    .SetParent<WifiNetDevice> ()
    .AddConstructor<WaveNetDevice> ()
    .AddAttribute ("IpOnCCH",
    		       "This Boolean attribute is set to enable IPv4 and Ipv6 packets sent on CCH",
    		       BooleanValue (false),
    		       MakeBooleanAccessor (&WaveNetDevice::SetIpOnCchSupported,
    		    		                &WaveNetDevice::GetIpOnCchSupported),
    		       MakeBooleanChecker ())
  ;
  return tid;
}

WaveNetDevice::WaveNetDevice ()
  : m_txProfile (0)
{
  NS_LOG_FUNCTION (this);
  m_waveVscReceived = MakeNullCallback<bool, Ptr<const Packet>,const Address &, uint32_t, uint32_t> ();
  m_channelCoordinator = CreateObject<ChannelCoordinator> ();
  m_channelManager =  CreateObject<ChannelManager> ();
  m_channelScheduler = CreateObject<ChannelScheduler> ();
  m_channelScheduler->SetWaveNetDevice (this);
  m_channelScheduler->SetChannelManager (m_channelManager);
  m_channelScheduler->SetChannelCoodinator (m_channelCoordinator);
  m_vsaRepeater = CreateObject<VsaRepeater> (this);
}
WaveNetDevice::~WaveNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
WaveNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  if (m_txProfile != 0)
    {
      delete m_txProfile;
      m_txProfile = 0;
    }
  m_channelCoordinator->Dispose ();
  m_channelManager->Dispose ();
  m_channelScheduler->Dispose ();
  m_vsaRepeater->Dispose ();
  m_channelCoordinator = 0;
  m_channelManager = 0;
  m_channelScheduler = 0;
  m_vsaRepeater = 0;
  m_waveVscReceived = MakeNullCallback<bool, Ptr<const Packet>,const Address &, uint32_t, uint32_t> ();
  // chain up.
  WifiNetDevice::DoDispose ();
}

void
WaveNetDevice::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_channelCoordinator->Initialize ();
  m_channelManager->Initialize ();
  m_channelScheduler->Initialize ();
  m_vsaRepeater->Initialize ();
  WifiNetDevice::DoInitialize ();
  GetMac ()->SetForwardUpCallback (MakeCallback (&WaveNetDevice::WaveForwardUp, this));
}

bool
WaveNetDevice::StartVsa (const VsaInfo & vsaInfo)
{
  NS_LOG_FUNCTION (this << &vsaInfo);

  if (vsaInfo.vsc == 0)
    {
      NS_LOG_DEBUG ("vendor specific information shall not be null");
      return false;
    }

  if (!ChannelManager::IsWaveChannel (vsaInfo.channelNumber))
    {
      NS_LOG_DEBUG ("the specific channel is not valid WAVE channel");
      return false;
    }
  if (vsaInfo.oi.IsNull () && vsaInfo.managementId >= 16)
    {
      NS_LOG_DEBUG ("when organization identifier is not set, management ID "
                    "shall be in range from 0 to 15");
      return false;
    }

  uint32_t cn = vsaInfo.channelNumber;
  if (!m_channelScheduler->IsAccessAssigned (cn))
    {
      NS_LOG_DEBUG ("channel = " << cn << "has no access assigned");
      return false;
    }

  if ((m_channelScheduler->GetAccess (cn) == ChannelScheduler::AlternatingAccess)
      && (vsaInfo.channelInterval == VsaSentInBothi))
    {
      NS_LOG_DEBUG ("AlternatingAccess cannot fulfill both channel interval");
      return false;
    }

  m_vsaRepeater->SendVsa (vsaInfo);
  return true;
}

bool
WaveNetDevice::DoReceiveVsc (const OrganizationIdentifier &oi, Ptr<const Packet> vsc, const Address &src)
{
  NS_ASSERT (oi == oi_1609);
  uint32_t channelNumber = WifiNetDevice::GetPhy ()->GetChannelNumber ();
  // we should ensure that channel access resource is assigned.
  // even mac entity already receives vendor specific frames successfully,
  // here we still treat packets as not received by MAC.
  if (!m_channelScheduler->IsAccessAssigned (channelNumber))
    {
      return true; // return ok but not forward up to higher layer
    }
  uint32_t managementId = oi.PeekData ()[4] | 0x0f;
  if (!m_waveVscReceived.IsNull ())
    {
      bool succeed = false;
      succeed = m_waveVscReceived (vsc, src, managementId, channelNumber);
      return succeed;
    }
  return true;
}

void
WaveNetDevice::StopVsa (uint32_t channelNumber)
{
  NS_LOG_FUNCTION (this << channelNumber);
  m_vsaRepeater->RemoveByChannel (channelNumber);
}
void
WaveNetDevice::SetVsaReceiveCallback (WaveCallback waveCallback)
{
  m_waveVscReceived = waveCallback;
}
bool
WaveNetDevice::StartSch (const SchInfo & schInfo)
{
  NS_LOG_FUNCTION (this << &schInfo);
  if (!ChannelManager::IsWaveChannel (schInfo.channelNumber))
    {
      return false;
    }

  for (EdcaParameterSetI i = schInfo.edcaParameterSet.begin ();
		  i != schInfo.edcaParameterSet.end (); ++i)
    {
	  Ptr<WifiMac> mac = WifiNetDevice::GetMac ();
      Ptr<OcbWifiMac> ocbMac = DynamicCast<OcbWifiMac>(mac);
      NS_ASSERT (ocbMac != 0);
      EdcaParameter edca = i->second;
      ocbMac->ConfigureEdca (edca.cwmin, edca.cwmax, edca.aifsn, i->first);
    }

  uint32_t cn = schInfo.channelNumber;
  uint32_t extends = schInfo.extendedAccess;
  // now only support channel continuous access and extended access
  if (extends == 0xff)
    {
      return m_channelScheduler->AssignContinuousAccess (cn, schInfo.immediateAccess);
    }
  else if (extends == 0)
    {
      return m_channelScheduler->AssignAlternatingAccess (cn, schInfo.immediateAccess);
    }
  else
    {
      return m_channelScheduler->AssignExtendedAccess (cn, schInfo.extendedAccess, schInfo.immediateAccess);
    }
  return true;
}
void
WaveNetDevice::StopSch (uint32_t channelNumber)
{
  NS_LOG_FUNCTION (this << channelNumber);
  if (!ChannelManager::IsWaveChannel (channelNumber))
    {
      return;
    }
  m_channelScheduler->Release (channelNumber);
}
bool
WaveNetDevice::RegisterTxProfile (const TxProfile &txprofile)
{
  NS_LOG_FUNCTION (this << &txprofile);
  uint32_t cn = txprofile.channelNumber;
  if (!ChannelManager::IsWaveChannel (cn))
    {
      return false;
    }

  // then check whether here is already a TxProfile registered
  if (m_txProfile != 0)
    {
      return false;
    }

  // then check txprofile parameters
  // IP-based packets is not allowed to send in the CCH.
  if (txprofile.channelNumber == CCH)
    {
      return false;
    }

  if (txprofile.txPowerLevel > 8)
    {
      return false;
    }
  m_txProfile = new TxProfile;
  *m_txProfile = txprofile;
  return true;
}
void
WaveNetDevice::UnregisterTxProfile (uint32_t channelNumber)
{
  NS_LOG_FUNCTION (this << channelNumber);
  if ((m_txProfile != 0) && (m_txProfile->channelNumber == channelNumber))
    {
      delete m_txProfile;
      m_txProfile = 0;
    }
}
bool
WaveNetDevice::SendX (Ptr<Packet> packet, const Address & dest, uint32_t protocol, const TxInfo & txInfo)
{
  NS_LOG_FUNCTION (this << packet << dest << protocol << &txInfo);
  uint32_t cn = txInfo.channelNumber;
  if (!ChannelManager::IsWaveChannel (cn))
    {
      return false;
    }
  // channel access shall be assigned
  if (m_channelManager->IsChannelDead (cn))
    {
      return false;
    }

  if (txInfo.priority > 7)
    {
      return false;
    }

  if (txInfo.expiryTime > 500)
    {
      return false;
    }
  // according to channel number and priority,
  // route the packet to a proper queue.
  Ptr<WifiMac> mac = WifiNetDevice::GetMac ();
  QosTag qos (txInfo.priority);
  packet->AddPacketTag (qos);
  ChannelTag channel (txInfo.channelNumber);
  packet->AddPacketTag (channel);

  // if expireTime == 0, channel mac queue will has
  // its default expire time (500ms).
  if (txInfo.expiryTime != 0)
    {
      ExpireTimeTag expire (txInfo.expiryTime);
      packet->AddPacketTag (expire);
    }

  if (txInfo.txPowerLevel < 8 && txInfo.dataRate != UnknownDataRate)
    {
      WifiMode datarate =  GetPhy ()->GetMode (txInfo.dataRate);
      WifiTxVector txVector;
      txVector.SetTxPowerLevel (txInfo.txPowerLevel);
      txVector.SetMode (datarate);
      HigherDataTxVectorTag tag = HigherDataTxVectorTag (txVector, false);
      packet->AddPacketTag (tag);
    }

  LlcSnapHeader llc;
  llc.SetType (protocol);
  packet->AddHeader (llc);

  Mac48Address realTo = Mac48Address::ConvertFrom (dest);
  mac->NotifyTx (packet);
  mac->Enqueue (packet, realTo);
  return true;
}
void
WaveNetDevice::ChangeAddress (Address newAddress)
{
  NS_LOG_FUNCTION (this << newAddress);
  Address oldAddress = WifiNetDevice::GetAddress ();
  if (newAddress == oldAddress)
    {
      return;
    }
  WifiNetDevice::SetAddress (newAddress);
  m_addressChange (oldAddress, newAddress);
}

void
WaveNetDevice::SetIpOnCchSupported (bool enable)
{
  m_ipOnCch = enable;
}
bool
WaveNetDevice::GetIpOnCchSupported (void) const
{
  return m_ipOnCch;
}

void
WaveNetDevice::SetChannelManager (Ptr<ChannelManager> channelManager)
{
  m_channelManager = channelManager;
}
Ptr<ChannelManager>
WaveNetDevice::GetChannelManager (void) const
{
  return m_channelManager;
}
void
WaveNetDevice::SetChannelScheduler (Ptr<ChannelScheduler> channelScheduler)
{
  m_channelScheduler = channelScheduler;
}
Ptr<ChannelScheduler>
WaveNetDevice::GetChannelScheduler (void) const
{
  return m_channelScheduler;
}
void
WaveNetDevice::SetChannelCoordinator (Ptr<ChannelCoordinator> channelCoordinator)
{
  m_channelCoordinator = channelCoordinator;
}
Ptr<ChannelCoordinator>
WaveNetDevice::GetChannelCoordinator (void) const
{
  return m_channelCoordinator;
}
bool
WaveNetDevice::IsLinkUp (void) const
{
  return true;
}
void
WaveNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_WARN ("WaveNetDevice is linkup forever, so this callback will be never called");
}
bool
WaveNetDevice::NeedsArp (void) const
{
  return true;
}

bool
WaveNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  // now I do know what difference is between Send and SendFrom
  // but codes seem the same
  return WifiNetDevice::SendFrom (packet, GetAddress (), dest, protocolNumber);
}

bool
WaveNetDevice::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);

  // here shall be a txprofile. otherwise discard packet
  if (m_txProfile == 0)
    {
      return false;
    }
  // channel access shall be assigned
  if (m_channelManager->IsChannelDead (m_txProfile->channelNumber))
    {
      return false;
    }

  if (protocolNumber == 0x0800 || protocolNumber == 0x86DD)
    {
      if (!m_ipOnCch)
      {
    	NS_LOG_DEBUG ("IP-based packets shall not be transmitted on the CCH");
        return false;
      }
    }

  // qos tag is added by higher layer or not add to use default qos level.
  ChannelTag channel (m_txProfile->channelNumber);
  packet->AddPacketTag (channel);

  if (m_txProfile->txPowerLevel < 8 && m_txProfile->dataRate != UnknownDataRate)
    {
      WifiMode datarate =  GetPhy ()->GetMode (m_txProfile->dataRate);
      WifiTxVector txVector;
      txVector.SetTxPowerLevel (m_txProfile->txPowerLevel);
      txVector.SetMode (datarate);
      HigherDataTxVectorTag tag = HigherDataTxVectorTag (txVector, false);
      packet->AddPacketTag (tag);
    }
  return WifiNetDevice::SendFrom (packet, source, dest, protocolNumber);
}
void
WaveNetDevice::WaveForwardUp (Ptr<Packet> packet, Mac48Address from, Mac48Address to)
{
  // if channel access has not been assigned, we not allow to receive
  if (m_channelScheduler->GetAccess () == ChannelScheduler::NoAccess)
    {
      return;
    }

  /**
   * when channel access is AlternatingAccess, sometimes devices will receive packets in guard interval.
   * Here are two situations
   * 1. According to the standard, when transmission of an MPDU is not completed by the
   * time the channel interval ends, the transmission may be canceled by the MAC through
   * the primitive PHY-TXEND.request.
   * This means if channel begin to switch and there is a packet which is in sending state,
   * channel switch will cause sending state to switching state immediately, however
   * YansWifiPhy has not implemented this, instead channel switch operation will wait some
   * until sent operation is over and will cause packets sent in guard interval.
   * Actually this situation may be a bug.
   * 2. If devices are all using AlternatingAccess, send event will not happen in guard interval
   * because of busy notification by MAC. However if a device is using AlternatingAccess and
   * another device is using ContinusAccess, the second device can sent packets in guard interval
   * and then will cause the first device receive packets in guard interval.
   * So I add a check here, when devices receive packets at guard interval, the packets will
   * be dropped as they haven't been received correctly.
   */
  if (m_channelScheduler->GetAccess () == ChannelScheduler::AlternatingAccess && m_channelCoordinator->IsGuardiNow ())
    {
      return;
    }

  WifiNetDevice::ForwardUp (packet, from, to);
}

} // namespace ns3
