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
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/qos-tag.h"
#include "ns3/mac-low.h"
#include "ns3/dcf-manager.h"
#include "ns3/mac-rx-middle.h"
#include "ns3/mgt-headers.h"
#include "ocb-wifi-mac.h"
#include "vendor-specific-action.h"
#include "higher-tx-tag.h"

NS_LOG_COMPONENT_DEFINE ("OcbWifiMac");

namespace ns3 {

/********************** WaveMacLow *******************************/
/**
 * This class allow higher layer control data rate and tx power level.
 * If higher layer do not select, it will select by WifiRemoteStationManager
 * of MAC layer;
 * If higher layer selects tx arguments without adapter set, the data rate
 * and tx power level will be used to send the packet.
 * If higher layer selects tx arguments with adapter set, the data rate
 * will be lower bound for the actual data rate, and the power level
 * will be upper bound for the actual transmit power.
 */
class WaveMacLow : public MacLow
{
public:
  static TypeId GetTypeId (void);
  WaveMacLow ();
  ~WaveMacLow ();
private:
  virtual WifiTxVector GetDataTxVector (Ptr<const Packet> packet, const WifiMacHeader *hdr) const;
};
NS_OBJECT_ENSURE_REGISTERED (WaveMacLow);

TypeId
WaveMacLow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WaveMacLow")
    .SetParent<MacLow> ()
    .AddConstructor<WaveMacLow> ()
  ;
  return tid;
}
WaveMacLow::WaveMacLow (void)
{
}
WaveMacLow::~WaveMacLow (void)
{
}

WifiTxVector
WaveMacLow::GetDataTxVector (Ptr<const Packet> packet, const WifiMacHeader *hdr) const
{
  HigherDataTxVectorTag datatag;
  bool found;
  found = ConstCast<Packet> (packet)->PeekPacketTag (datatag);
  if (!found)
    {
      return MacLow::GetDataTxVector (packet, hdr);
    }

  if (!datatag.IsAdapter ())
    {
      return datatag.GetDataTxVector ();
    }

  WifiTxVector txHigher = datatag.GetDataTxVector ();
  WifiTxVector txMac = MacLow::GetDataTxVector (packet, hdr);
  WifiTxVector txAdaper;
  // if adapter is true, DataRate set by higher layer is the minimum data rate
  // that sets the lower bound for the actual data rate.
  if (txHigher.GetMode ().GetDataRate () > txMac.GetMode ().GetDataRate ())
    {
      txAdaper.SetMode (txHigher.GetMode ());
    }
  else
    {
      txAdaper.SetMode (txMac.GetMode ());
    }

  // if adapter is true, TxPwr_Level set by higher layer is the maximum
  // transmit power that sets the upper bound for the actual transmit power;
  txAdaper.SetTxPowerLevel (std::min (txHigher.GetTxPowerLevel (), txMac.GetTxPowerLevel ()));
  return txAdaper;
}

/********************** OcbWifiMac *******************************/
NS_OBJECT_ENSURE_REGISTERED (OcbWifiMac);

const static Mac48Address WILDCARD_BSSID = Mac48Address::GetBroadcast ();

TypeId
OcbWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OcbWifiMac")
    .SetParent<RegularWifiMac> ()
    .AddConstructor<OcbWifiMac> ()
  ;
  return tid;
}

OcbWifiMac::OcbWifiMac ()
{
  NS_LOG_FUNCTION (this);

  // use WaveMacLow instead of MacLow
  m_low = CreateObject<WaveMacLow> ();
  m_low->SetRxCallback (MakeCallback (&MacRxMiddle::Receive, m_rxMiddle));
  m_dcfManager->SetupLowListener (m_low);
  m_dca->SetLow (m_low);
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetLow (m_low);
    }

  // Let the lower layers know that we are acting as an OCB node
  SetTypeOfStation (OCB);
  // BSSID is still needed in the low part of MAC
  RegularWifiMac::SetBssid (WILDCARD_BSSID);
}

OcbWifiMac::~OcbWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
OcbWifiMac::SendVsc (Ptr<Packet> vsc, Mac48Address peer, OrganizationIdentifier oi)
{
  NS_LOG_FUNCTION (this << vsc << peer << oi);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (peer);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (WILDCARD_BSSID);
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  VendorSpecificActionHeader vsa;
  vsa.SetOrganizationIdentifier (oi);
  vsc->AddHeader (vsa);

  if (m_qosSupported)
    {
      uint8_t tid = QosUtilsGetTidForPacket (vsc);
      tid = tid > 7 ? 0 : tid;
      m_edca[QosUtilsMapTidToAc (tid)]->Queue (vsc, hdr);
    }
  else
    {
      m_dca->Queue (vsc, hdr);
    }
}

void
OcbWifiMac::AddReceiveVscCallback (OrganizationIdentifier oi, VscCallback cb)
{
  NS_LOG_FUNCTION (this << oi << &cb);
  m_vscManager.RegisterVscCallback (oi, cb);
}

void
OcbWifiMac::RemoveReceiveVscCallback (OrganizationIdentifier oi)
{
  NS_LOG_FUNCTION (this << oi);
  m_vscManager.DeregisterVscCallback (oi);
}

void
OcbWifiMac::SetSsid (Ssid ssid)
{
  NS_LOG_WARN ("in OCB mode we should not call SetSsid");
}

Ssid
OcbWifiMac::GetSsid (void) const
{
  NS_LOG_WARN ("in OCB mode we should not call GetSsid");
  // we really do not want to return ssid, however we have to provide
  return RegularWifiMac::GetSsid ();
}


void
OcbWifiMac::SetBssid (Mac48Address bssid)
{
  NS_LOG_WARN ("in OCB mode we should not call SetSsid");
}

Mac48Address
OcbWifiMac::GetBssid (void) const
{
  NS_LOG_WARN ("in OCB mode we should not call GetBssid");
  return WILDCARD_BSSID;
}

void
OcbWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  // The approach taken here is that, from the point of view of a STA
  // in OCB mode, the link is always up, so we immediately invoke the
  // callback if one is set
  linkUp ();
}

void
OcbWifiMac::SetLinkDownCallback (Callback<void> linkDown)
{
  NS_LOG_FUNCTION (this << &linkDown);
  RegularWifiMac::SetLinkDownCallback (linkDown);
  NS_LOG_DEBUG ("in OCB mode the like will never down, so linkDown will never be called");
}


void
OcbWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  if (m_stationManager->IsBrandNew (to))
    {
      // In ocb mode, we assume that every destination supports all
      // the rates we support.
      for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
        {
          m_stationManager->AddSupportedMode (to, m_phy->GetMode (i));
        }
      m_stationManager->RecordDisassociated (to);
    }

  WifiMacHeader hdr;

  // If we are not a QoS STA then we definitely want to use AC_BE to
  // transmit the packet. A TID of zero will map to AC_BE (through \c
  // QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  if (m_qosSupported)
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoEosp ();
      hdr.SetQosNoAmsdu ();
      // About transmission of multiple frames,
      // in Ad-hoc mode TXOP is not supported for now, so TxopLimit=0;
      // however in OCB mode, 802.11p do not allow transmit multiple frames
      // so TxopLimit must equal 0
      hdr.SetQosTxopLimit (0);

      // Fill in the QoS control field in the MAC header
      tid = QosUtilsGetTidForPacket (packet);
      // Any value greater than 7 is invalid and likely indicates that
      // the packet had no QoS tag, so we revert to zero, which'll
      // mean that AC_BE is used.
      if (tid >= 7)
        {
          tid = 0;
        }
      hdr.SetQosTid (tid);
    }
  else
    {
      hdr.SetTypeData ();
    }

  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (WILDCARD_BSSID);
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  if (m_qosSupported)
    {
      // Sanity check that the TID is valid
      NS_ASSERT (tid < 8);
      m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
    }
  else
    {
      m_dca->Queue (packet, hdr);
    }
}

/*
 * see 802.11p-2010 chapter 11.19
 * here we only care about data packet and vsa management frame
 */
void
OcbWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  NS_ASSERT (!hdr->IsCtl ());
  NS_ASSERT (hdr->GetAddr3 () == WILDCARD_BSSID);

  Mac48Address from = hdr->GetAddr2 ();
  Mac48Address to = hdr->GetAddr1 ();

  if (hdr->IsData ())
    {
      if (hdr->IsQosData () && hdr->IsQosAmsdu ())
        {
          NS_LOG_DEBUG ("Received A-MSDU from" << from);
          DeaggregateAmsduAndForward (packet, hdr);
        }
      else
        {
          ForwardUp (packet, from, to);
        }
      return;
    }

  // why put check here, not before "if (hdr->IsData ())" ?
  // because WifiNetDevice::ForwardUp needs to m_promiscRx data packet
  // and will filter data packet for itself
  // so we need to filter management frame
  if (to != GetAddress () && !to.IsGroup ())
    {
      NS_LOG_LOGIC ("the management frame is not for us");
      NotifyRxDrop (packet);
      return;
    }

  if (hdr->IsMgt () && hdr->IsAction ())
    {
      // yes, we only care about VendorSpecificAction frame in OCB mode
      // other management frames will be handled by RegularWifiMac::Receive
      VendorSpecificActionHeader vsaHdr;
      packet->PeekHeader (vsaHdr);
      if (vsaHdr.GetCategory () == CATEGORY_OF_VSA)
        {
          VendorSpecificActionHeader vsa;
          packet->RemoveHeader (vsa);
          OrganizationIdentifier oi = vsa.GetOrganizationIdentifier ();
          VscCallback cb = m_vscManager.FindVscCallback (oi);

          if (cb.IsNull ())
            {
              NS_LOG_DEBUG ("cannot find VscCallback for OrganizationIdentifier=" << oi);
              return;
            }
          bool succeed = cb (oi,packet, from);

          if (!succeed)
            {
              NS_LOG_DEBUG ("vsc callback could not handle the packet successfully");
            }

          return;
        }
    }
  // Invoke the receive handler of our parent class to deal with any
  // other frames. Specifically, this will handle Block Ack-related
  // Management Action frames.
  RegularWifiMac::Receive (packet, hdr);
}

void
OcbWifiMac::FinishConfigureStandard (enum WifiPhyStandard standard)
{
  NS_ASSERT ((standard == WIFI_PHY_STANDARD_80211_10MHZ)
             || (standard == WIFI_PHY_STANDARD_80211a));

  uint32_t cwmin = 15;
  uint32_t cwmax = 1023;

  // The special value of AC_BE_NQOS which exists in the Access
  // Category enumeration allows us to configure plain old DCF.
  ConfigureOcbDcf (m_dca, cwmin, cwmax, AC_BE_NQOS);

  // Now we configure the EDCA functions
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      ConfigureOcbDcf (i->second, cwmin, cwmax, i->first);
    }
}


void
OcbWifiMac::ConfigureOcbDcf (Ptr<Dcf> dcf, uint32_t cwmin, uint32_t cwmax, enum AcIndex ac)
{
  /* see IEEE802.11p-2010 section 7.3.2.29 */
  switch (ac)
    {
    case AC_VO:
      dcf->SetMinCw ((cwmin + 1) / 4 - 1);
      dcf->SetMaxCw ((cwmin + 1) / 2 - 1);
      dcf->SetAifsn (2);
      break;
    case AC_VI:
      dcf->SetMinCw ((cwmin + 1) / 2 - 1);
      dcf->SetMaxCw (cwmin);
      dcf->SetAifsn (3);
      break;
    case AC_BE:
      dcf->SetMinCw (cwmin);
      dcf->SetMaxCw (cwmax);
      dcf->SetAifsn (6);
      break;
    case AC_BK:
      dcf->SetMinCw (cwmin);
      dcf->SetMaxCw (cwmax);
      dcf->SetAifsn (9);
      break;
    case AC_BE_NQOS:
      dcf->SetMinCw (cwmin);
      dcf->SetMaxCw (cwmax);
      dcf->SetAifsn (2);
      break;
    case AC_UNDEF:
      NS_FATAL_ERROR ("I don't know what to do with this");
      break;
    }
}

void
OcbWifiMac::NotifyBusy (Time duration)
{
  m_dcfManager->NotifyMaybeCcaBusyStartNow (duration);
}

void
OcbWifiMac::SetWaveEdcaQueue (AcIndex ac, Ptr<EdcaTxopN> edca)
{
  EdcaQueues::iterator i = m_edca.find (ac);
  if (i != m_edca.end ())
    {
      m_edca.erase (i);
    }

  edca->SetLow (m_low);
  edca->SetManager (m_dcfManager);
  edca->SetTxMiddle (m_txMiddle);
  edca->SetTxOkCallback (MakeCallback (&OcbWifiMac::TxOk, this));
  edca->SetTxFailedCallback (MakeCallback (&OcbWifiMac::TxFailed, this));
  edca->SetAccessCategory (ac);
  edca->SetWifiRemoteStationManager (m_stationManager);
  m_edca.insert (std::make_pair (ac, edca));
}

} // namespace ns3
