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
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/command-line.h"
#include "ns3/mobility-model.h"
#include <iostream>

#include "ns3/wave-net-device.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/wave-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WaveMultipleChannel");

namespace ns3 {
class StatsTag : public Tag
{
public:
  StatsTag (void)
    : m_packetId (0),
      m_sendTime (Seconds (0.0))
  {
  }
  StatsTag (uint32_t packetId, Time sendTime)
    : m_packetId (packetId),
      m_sendTime (sendTime)
  {
  }
  virtual ~StatsTag (void)
  {
  }

  uint32_t GetPacketId (void)
  {
    return m_packetId;
  }
  Time GetSendTime (void)
  {
    return m_sendTime;
  }

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  uint32_t m_packetId;
  Time m_sendTime;
};
TypeId
StatsTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StatsTag")
    .SetParent<Tag> ()
    .AddConstructor<StatsTag> ()
  ;
  return tid;
}
TypeId
StatsTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
StatsTag::GetSerializedSize (void) const
{
  return sizeof (uint32_t) + sizeof (uint64_t);
}
void
StatsTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_packetId);
  i.WriteU64 (m_sendTime.GetMicroSeconds ());
}
void
StatsTag::Deserialize (TagBuffer i)
{
  m_packetId = i.ReadU32 ();
  m_sendTime = MicroSeconds (i.ReadU64 ());
}
void
StatsTag::Print (std::ostream &os) const
{
  os << "packet=" << m_packetId << " sendTime=" << m_sendTime;
}

} // namespace ns3
/**
 * (1) some nodes are in constant speed mobility, every node sends
 * two types of packets. the first type is a small size with 200 bytes
 * which models beacon and safety message, this packet is broadcasted
 * to neighbor nodes;the second type is a big size with 1500 bytes which
 * models information or entertainment message, this packet is sent to
 * individual destination node (this may cause ACK and retransmission).
 *
 * (2) Here are four configurations:
 * - a the first is every node sends packets randomly in CCH channel with continuous access.
 * - b the second is every node sends packets randomly with alternating access.
 *   Safety packets are sent in CCH channel, non-safety packets are sent in SCH channel.
 * - c the third is similar to the second. But Safety packets will be sent randomly in CCH interval,
 *   non-safety packets will be sent randomly in SCH interval.
 *   This is the best situation of configuration 2 which models higher layer be aware of lower layer.
 * - d the fourth is also similar to the second. But Safety packets will be sent randomly in SCH interval,
 *   non-safety packets will be sent randomly in CCH interval.
 *   This is the worst situation of configuration 2 which makes packets get high queue delay.
 *
 * (3) Besides (2), users can also configures send frequency and nodes number.
 *
 * (4) The output is the delay of safety packets, the delay of non-safety packets, the throughput of
 * safety packets, and the throughput of safety packets.
 *
 * (5) I think the result can show the advantage and disadvantage of 1609.4.
 * Normally to divide safety and non-safety to different channels can help improve success probability of
 * safety message than send them on only one channel. Suppose that if send frequency of safety message and
 * non-safety message are same, but because of non-safety-message has bigger size, so will cost more channel
 * time, so I think the delay and throughput of safety message in (b) should be better than in (a).
 * The best performance could be find is in (c) that every safety message is sent in CCHI and non-safety message
 * is sent in SCHI, so the queue delay of safety message is min than in (b) and (d),  and the contending at the
 * end of guard interval will also not serious than others and cause throughput better.
 * The worst performance could be find in (d) that every safety message is sent in SCHI and non-safety message
 * is sent in CCHI. And this performance may be worse than (a).
 */
const static uint16_t IPv4_PROT_NUMBER = 0x0800;
const static uint16_t WSMP_PROT_NUMBER = 0x88DC;

class MultipleChannelExperiment
{
public:
  MultipleChannelExperiment ();

  bool Configure (int argc, char **argv);
  void Usage (void);
  void Run (void);
  void Stats (void);

private:
  void CreateWaveNodes (void);

  // we treat WSMP packets as safety message
  void SendWsmpPackets (Ptr<WaveNetDevice> sender, uint32_t channelNumber);
  // we treat IP packets as non-safety message
  void SendIpPackets (Ptr<WaveNetDevice> sender);

  void ConfigurationA ();
  void ConfigurationB ();
  void ConfigurationC ();
  void ConfiguartionD ();

  bool Receive (Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender);

  NodeContainer nodes;
  NetDeviceContainer devices;
  uint32_t nodesNumber;
  uint32_t frequencySafety;
  uint32_t frequencyNonSafety;
  uint32_t simulationTime;
  uint32_t sizeSafety;
  uint32_t sizeNonSafety;

  Ptr<UniformRandomVariable> rngSafety;
  Ptr<UniformRandomVariable> rngNonSafety;
  Ptr<UniformRandomVariable> rngOther;

  uint32_t safetyPacketID;
  // we should filter the safety packets because the packet is broadcasted,
  // we only need only one packet from multiple packets with same "unique id".
  // and nonsafety packets is unicasted, so it do not need.
  std::set<uint32_t> broadcastPackets;
  uint32_t nonSafetyPacketID;

  struct SendIntervalStat
  {
    uint32_t        sendInCchi;
    uint32_t        sendInCguardi;
    uint32_t    sendInSchi;
    uint32_t    sendInSguardi;
  };

  SendIntervalStat sendSafety;
  SendIntervalStat sendNonSafety;
  uint32_t receiveSafety;     // the number of packets
  uint32_t receiveNonSafety;  // the number of packets
  uint64_t timeSafety;        // us
  uint64_t timeNonSafety;     // us

  std::ofstream outfile;
};

MultipleChannelExperiment::MultipleChannelExperiment ()
  : nodesNumber (20),
    frequencySafety (10),
    // 10Hz, 100ms send one safety packet
    frequencyNonSafety (10),
    // 10Hz, 100ms send one non-safety packet
    simulationTime (100),
    // make it run 100s
    sizeSafety (200),
    // 100 bytes small size
    sizeNonSafety (1500),
    // 1500 bytes big size
    safetyPacketID (0),
    nonSafetyPacketID (0),
    receiveSafety (0),
    receiveNonSafety (0),
    timeSafety (0),
    timeNonSafety (0)
{
}

bool
MultipleChannelExperiment::Configure (int argc, char **argv)
{
  CommandLine cmd;
  cmd.AddValue ("numNodes", "Number of nodes.", nodesNumber);
  cmd.AddValue ("time", "Simulation time, s.", simulationTime);
  cmd.AddValue ("sizeSafety", "Size of safety packet, bytes.", sizeSafety);
  cmd.AddValue ("sizeNonSafety", "Size of non-safety packet, bytes.", sizeNonSafety);
  cmd.AddValue ("frequencySafety", "Frequency of sending safety packets, Hz.", frequencySafety);
  cmd.AddValue ("frequencyNonSafety", "Frequency of sending non-safety packets, Hz.", frequencyNonSafety);

  cmd.Parse (argc, argv);
  return true;
}
void
MultipleChannelExperiment::Usage (void)
{
  std::cout << "usage:"
		    << "./waf --run=\"wave-multiple-channel \""
            << std::endl;
}

void
MultipleChannelExperiment::CreateWaveNodes (void)
{
  nodes = NodeContainer ();
  nodes.Create (nodesNumber);

  // Create static grid
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator");
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
    {
      Ptr<Node> node = (*i);
      Ptr<MobilityModel> model = node->GetObject<MobilityModel> ();
      Vector pos = model->GetPosition ();
      NS_LOG_DEBUG ( "position: " << pos);
    }

  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  QosWaveMacHelper waveMac = QosWaveMacHelper::Default ();
  WaveHelper waveHelper = WaveHelper::Default ();
  devices = waveHelper.Install (wifiPhy, waveMac, nodes);

  // Enable WAVE logs.
  // WaveHelper::LogComponentEnable();
  // or waveHelper.LogComponentEnable();

  for (uint32_t i = 0; i != devices.GetN (); ++i)
    {
      devices.Get (i)->SetReceiveCallback (MakeCallback (&MultipleChannelExperiment::Receive,this));
    }

  rngSafety = CreateObject<UniformRandomVariable> ();
  rngSafety->SetStream (1);
  rngNonSafety = CreateObject<UniformRandomVariable> ();
  rngNonSafety->SetStream (2);
  rngOther = CreateObject<UniformRandomVariable> ();
  rngOther->SetStream (3);

  broadcastPackets.clear ();
  safetyPacketID = 0;
  nonSafetyPacketID = 0;
  receiveSafety = 0;
  receiveNonSafety = 0;
  timeSafety = 0;
  timeNonSafety = 0;
  sendSafety.sendInCchi = 0;
  sendSafety.sendInCguardi = 0;
  sendSafety.sendInSchi = 0;
  sendSafety.sendInSguardi = 0;
  sendNonSafety.sendInCchi = 0;
  sendNonSafety.sendInCguardi = 0;
  sendNonSafety.sendInSchi = 0;
  sendNonSafety.sendInSguardi = 0;
}
bool
MultipleChannelExperiment::Receive (Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender)
{
  StatsTag tag;
  bool result;
  result = pkt->FindFirstMatchingByteTag (tag);
  if (!result)
    {
      NS_FATAL_ERROR ("the packet here should has a stats tag");
    }
  Time now = Now ();
  Time sendTime = tag.GetSendTime ();
  uint32_t packetId = tag.GetPacketId ();


  if (mode == WSMP_PROT_NUMBER)
    {
      if (broadcastPackets.count (packetId))
        {
          return true;                // this packet will not used for stats
        }
      broadcastPackets.insert (packetId);
      receiveSafety++;
      timeSafety += (now - sendTime).GetMicroSeconds ();
    }
  else
    {
      receiveNonSafety++;
      timeNonSafety += (now - sendTime).GetMicroSeconds ();
    }

  outfile << "Time = " << std::dec << now.GetMicroSeconds () << "us, receive packet: "
          << " protocol = 0x" << std::hex << mode << std::dec
          << " id = " << packetId << " sendTime = " << sendTime.GetMicroSeconds ()
          << " type = " << (mode == WSMP_PROT_NUMBER ? "SafetyPacket" : "NonSafetyPacket")
          << std::endl;

  return true;
}
// although WAVE devices support send IP-based packets, here we
// simplify ip routing protocol and application. Actually they will
// make delay and through of safety message more serious.
void
MultipleChannelExperiment::SendIpPackets (Ptr<WaveNetDevice> sender)
{
  NS_LOG_FUNCTION (this << sender);

  Time now = Now ();
  Ptr<Packet> packet = Create<Packet> (sizeNonSafety);
  StatsTag tag = StatsTag (nonSafetyPacketID++, now);
  packet->AddByteTag (tag);

  uint32_t i = rngOther->GetInteger (0, nodesNumber - 1);
  Address dest = devices.Get (i)->GetAddress ();
  // we randomly select a node  as destination node
  // however if the destination is unluckily sender itself, we select again
  if (dest == sender->GetAddress ())
    {
      if (i != 0)
        {
          i--;
        }
      else
        {
          i++;
        }
      dest = devices.Get (i)->GetAddress ();
    }

  bool result = false;
  result = sender->Send (packet, dest, IPv4_PROT_NUMBER);
  if (result)
    {
      outfile << "Time = " << now.GetMicroSeconds () << "us, unicast IP packet:  ID = " << tag.GetPacketId ()
              << ", dest = " << dest << std::endl;
    }
  else
    {
      outfile << "unicast IP packet fail" << std::endl;
    }

  Ptr<ChannelCoordinator> coordinator = sender->GetChannelCoordinator ();
  if (coordinator->IsCchiNow ())
    {
      if (coordinator->IsGuardiNow ())
        {
          sendNonSafety.sendInCguardi++;
        }
      else
        {
          sendNonSafety.sendInCchi++;
        }

    }
  else
    {
      if (coordinator->IsGuardiNow ())
        {
          sendNonSafety.sendInSguardi++;
        }
      else
        {
          sendNonSafety.sendInSchi++;
        }
    }
}

void
MultipleChannelExperiment::SendWsmpPackets (Ptr<WaveNetDevice> sender, uint32_t channelNumber)
{
  NS_LOG_FUNCTION (this << sender << channelNumber);
  Time now = Now ();
  Ptr<Packet> packet = Create<Packet> (sizeSafety);
  StatsTag tag = StatsTag (safetyPacketID++, now);
  packet->AddByteTag (tag);
  const Address dest = Mac48Address::GetBroadcast ();
  TxInfo info = TxInfo (channelNumber);
  bool result = false;
  result = sender->SendX (packet, dest, WSMP_PROT_NUMBER, info);
  if (result)
    {
      outfile << "Time = " << now.GetMicroSeconds () << "us, broadcast WSMP packet: ID = " << tag.GetPacketId () << std::endl;
    }
  else
    {
      outfile << "broadcast WSMP packet fail" << std::endl;
    }

  Ptr<ChannelCoordinator> coordinator = sender->GetChannelCoordinator ();
  if (coordinator->IsCchiNow ())
    {
      if (coordinator->IsGuardiNow ())
        {
          sendSafety.sendInCguardi++;
        }
      else
        {
          sendSafety.sendInCchi++;
        }

    }
  else
    {
      if (coordinator->IsGuardiNow ())
        {
          sendSafety.sendInSguardi++;
        }
      else
        {
          sendSafety.sendInSchi++;
        }
    }
}
void
MultipleChannelExperiment::ConfigurationA ()
{
  NetDeviceContainer::Iterator i;
  for (i = devices.Begin (); i != devices.End (); ++i)
    {
      Ptr<WaveNetDevice> sender = DynamicCast<WaveNetDevice> (*i);
      SchInfo schInfo = SchInfo (SCH1, false, 0xff);
      Simulator::Schedule (Seconds (0.0), &WaveNetDevice::StartSch, sender, schInfo);
      TxProfile profile = TxProfile (SCH1);
      Simulator::Schedule (Seconds (0.0), &WaveNetDevice::RegisterTxProfile, sender, profile);
      for (uint32_t time = 0; time != simulationTime; ++time)
        {
          for (uint32_t sends = 0; sends != frequencySafety; ++sends)
            {
              Simulator::Schedule (Seconds (rngSafety->GetValue (time, time + 1)), &MultipleChannelExperiment::SendWsmpPackets, this, sender, SCH1);
            }
          for (uint32_t sends = 0; sends != frequencyNonSafety; ++sends)
            {
              Simulator::Schedule (Seconds (rngNonSafety->GetValue (time, time + 1)), &MultipleChannelExperiment::SendIpPackets, this, sender);
            }
        }
    }
}
void
MultipleChannelExperiment::ConfigurationB ()
{
  NetDeviceContainer::Iterator i;
  for (i = devices.Begin (); i != devices.End (); ++i)
    {
      Ptr<WaveNetDevice> sender = DynamicCast<WaveNetDevice> (*i);
      SchInfo schInfo = SchInfo (SCH1, false, 0x0);
      Simulator::Schedule (Seconds (0.0),&WaveNetDevice::StartSch,sender,schInfo);
      TxProfile profile = TxProfile (SCH1);
      Simulator::Schedule (Seconds (0.0), &WaveNetDevice::RegisterTxProfile, sender, profile);

      for (uint32_t time = 0; time != simulationTime; ++time)
        {
          for (uint32_t sends = 0; sends != frequencySafety; ++sends)
            {
              Simulator::Schedule (Seconds (rngSafety->GetValue (time, time + 1)), &MultipleChannelExperiment::SendWsmpPackets, this, sender, CCH);
            }
          for (uint32_t sends = 0; sends != frequencyNonSafety; ++sends)
            {
              Simulator::Schedule (Seconds (rngNonSafety->GetValue (time, time + 1)), &MultipleChannelExperiment::SendIpPackets, this, sender);
            }
        }
    }
}
void
MultipleChannelExperiment::ConfigurationC ()
{
  NetDeviceContainer::Iterator i;
  for (i = devices.Begin (); i != devices.End (); ++i)
    {
      Ptr<WaveNetDevice> sender = DynamicCast<WaveNetDevice> (*i);
      SchInfo schInfo = SchInfo (SCH1, false, 0x0);
      Simulator::Schedule (Seconds (0.0),&WaveNetDevice::StartSch,sender,schInfo);
      TxProfile profile = TxProfile (SCH1);
      Simulator::Schedule (Seconds (0.0), &WaveNetDevice::RegisterTxProfile, sender, profile);

      Ptr<ChannelCoordinator> coordinator = sender->GetChannelCoordinator ();
      for (uint32_t time = 0; time != simulationTime; ++time)
        {
          for (uint32_t sends = 0; sends != frequencySafety; ++sends)
            {
              Time t = Seconds (rngSafety->GetValue (time, time + 1));
              // if the send time is not at CCHI, we will calculate a new time
              if (!coordinator->IsCchiAfter (t))
                {
                  t = t + coordinator->NeedTimeToCchiAfter (t)
                    +  MicroSeconds (rngOther->GetInteger (0, coordinator->GetCchInterval ().GetMicroSeconds () - 1));
                }
              Simulator::Schedule (t, &MultipleChannelExperiment::SendWsmpPackets, this, sender, CCH);
            }
          for (uint32_t sends = 0; sends != frequencyNonSafety; ++sends)
            {

              Time t = Seconds (rngNonSafety->GetValue (time, time + 1));
              // if the send time is not at CCHI, we will calculate a new time
              if (!coordinator->IsSchiAfter (t))
                {
                  t =  t
                    + coordinator->NeedTimeToSchiAfter (t)
                    +  MicroSeconds (rngOther->GetInteger (0, coordinator->GetSchInterval ().GetMicroSeconds () - 1));
                }
              Simulator::Schedule (t, &MultipleChannelExperiment::SendIpPackets, this, sender);
            }
        }
    }
}
void
MultipleChannelExperiment::ConfiguartionD ()
{
  NetDeviceContainer::Iterator i;
  for (i = devices.Begin (); i != devices.End (); ++i)
    {
      Ptr<WaveNetDevice> sender = DynamicCast<WaveNetDevice> (*i);
      SchInfo schInfo = SchInfo (SCH1, false, 0x0);
      Simulator::Schedule (Seconds (0.0),&WaveNetDevice::StartSch,sender,schInfo);
      TxProfile profile = TxProfile (SCH1);
      Simulator::Schedule (Seconds (0.0), &WaveNetDevice::RegisterTxProfile, sender, profile);

      Ptr<ChannelCoordinator> coordinator = sender->GetChannelCoordinator ();
      for (uint32_t time = 0; time != simulationTime; ++time)
        {
          for (uint32_t sends = 0; sends != frequencySafety; ++sends)
            {
              Time t = Seconds (rngSafety->GetValue (time, time + 1));
              if (!coordinator->IsSchiAfter (t))
                {
                  t =  t + coordinator->NeedTimeToSchiAfter (t)
                    +  MicroSeconds (rngOther->GetInteger (0, coordinator->GetSchInterval ().GetMicroSeconds () - 1));

                }
              Simulator::Schedule (t, &MultipleChannelExperiment::SendWsmpPackets, this, sender, CCH);
            }
          for (uint32_t sends = 0; sends != frequencyNonSafety; ++sends)
            {

              Time t = Seconds (rngNonSafety->GetValue (time, time + 1));
              // if the send time is not at CCHI, we will calculate a new time
              if (!coordinator->IsCchiAfter (t))
                {
                  t =  t + coordinator->NeedTimeToCchiAfter (t)
                    +  MicroSeconds (rngOther->GetInteger (0, coordinator->GetCchInterval ().GetMicroSeconds () - 1));

                }
              Simulator::Schedule (t, &MultipleChannelExperiment::SendIpPackets, this, sender);
            }
        }
    }
}
void
MultipleChannelExperiment::Run ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("simulation configuration arguments: ");

  {
    NS_LOG_UNCOND ("configuration A:");
    SeedManager::SetSeed (7);
    outfile.open ("config-a");
    CreateWaveNodes ();
    ConfigurationA ();
    Simulator::Stop (Seconds (simulationTime));
    Simulator::Run ();
    Simulator::Destroy ();
    Stats ();
    outfile.close ();
  }
  {
    NS_LOG_UNCOND ("configuration B:");
    SeedManager::SetSeed (7);
    outfile.open ("config-b");
    CreateWaveNodes ();
    ConfigurationB ();
    Simulator::Stop (Seconds (simulationTime));
    Simulator::Run ();
    Simulator::Destroy ();
    Stats ();
    outfile.close ();
  }
  {
    NS_LOG_UNCOND ("configuration C:");
    SeedManager::SetSeed (7);
    outfile.open ("config-c");
    CreateWaveNodes ();
    ConfigurationC ();
    Simulator::Stop (Seconds (simulationTime));
    Simulator::Run ();
    Simulator::Destroy ();
    Stats ();
    outfile.close ();
  }
  {
    NS_LOG_UNCOND ("configuration D:");
    SeedManager::SetSeed (7);
    outfile.open ("config-d");
    CreateWaveNodes ();
    ConfiguartionD ();
    Simulator::Stop (Seconds (simulationTime));
    Simulator::Run ();
    Simulator::Destroy ();
    Stats ();
    outfile.close ();
  }
}
void
MultipleChannelExperiment::Stats (void)
{
  // first show stats information
  NS_LOG_UNCOND (" safety packet: ");
  NS_LOG_UNCOND ("  sends = " << safetyPacketID);
  NS_LOG_UNCOND ( "  CGuardI CCHI SGuardI SCHI " );
  NS_LOG_UNCOND ("  " << sendSafety.sendInCguardi  << " "
                      << sendSafety.sendInCchi  << " "
                      << sendSafety.sendInSguardi  << " "
                      << sendSafety.sendInSchi);
  NS_LOG_UNCOND ( "  receives = " << receiveSafety);
  NS_LOG_UNCOND (" non-safety packet: ");
  NS_LOG_UNCOND ("  sends = " << nonSafetyPacketID);
  NS_LOG_UNCOND ("  CGuardI CCHI SGuardI SCHI ");
  NS_LOG_UNCOND ("  " << sendNonSafety.sendInCguardi  << " "
                      << sendNonSafety.sendInCchi  << " "
                      << sendNonSafety.sendInSguardi  << " "
                      << sendNonSafety.sendInSchi);
  NS_LOG_UNCOND ("  receives = " << receiveNonSafety);

  // second show performance result
  NS_LOG_UNCOND (" performance result:");
  // stats PDR (packet delivery ratio)
  double safetyPDR = receiveSafety / (double)safetyPacketID;
  double nonSafetyPDR = receiveNonSafety / (double)nonSafetyPacketID;
  NS_LOG_UNCOND ("  safetyPDR = " << safetyPDR
                                  << " , nonSafetyPDR = " << nonSafetyPDR);
  // stats average delay
  double delaySafety = timeSafety / receiveSafety / 1000.0;
  double delayNonSafety = timeNonSafety / receiveNonSafety / 1000.0;
  NS_LOG_UNCOND ("  delaySafety = " << delaySafety << "ms"
                                    << " , delayNonSafety = " << delayNonSafety << "ms");
  // stats average throughout
  double throughoutSafety = receiveSafety * sizeSafety * 8 / simulationTime / 1000.0;
  double throughoutNonSafety = receiveNonSafety * sizeNonSafety * 8 / simulationTime / 1000.0;
  NS_LOG_UNCOND ("  throughoutSafety = " << throughoutSafety << "kbps"
                                         << " , throughoutNonSafety = " << throughoutNonSafety << "kbps");
}

int
main (int argc, char *argv[])
{
  //LogComponentEnable ("WaveMultipleChannel", LOG_LEVEL_DEBUG);

  MultipleChannelExperiment experiment;
  if (experiment.Configure (argc, argv))
    {
      experiment.Run ();
    }
  else
    {
      experiment.Usage ();
    }

  return 0;
}
