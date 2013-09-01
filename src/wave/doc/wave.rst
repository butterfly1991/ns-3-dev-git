WAVE models
-----------
WAVE is a system architecture for wireless-based vehicular communications, 
specified by the IEEE.  This chapter documents available models for WAVE
within |ns3|.  The focus is on the MAC layer and MAC extension layer
defined by [ieee80211p]_, and on the multichannel operation defined in
[ieee1609.4]_.

.. include:: replace.txt

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Model Description
*****************

WAVE is an overall system architecture for vehicular communications.
The standards for specifying WAVE include a set of extensions to the IEEE
802.11 standard, found in IEEE Std 802.11p-2010 [ieee80211p]_, and
the IEEE 1609 standard set, consisting of four documents:  
resource manager:  IEEE 1609.1 [ieee1609.1]_,
security services:  IEEE 1609.2 [ieee1609.2]_,
network and transport layer services:  IEEE 1609.3 [ieee1609.3]_,
and multi-channel coordination:  IEEE 1609.4 [ieee1609.4]_.

In |ns3|, the focus of the ``wave`` module is on the MAC layer.
The key design aspect of WAVE-compilant MACs is that they allow
communications outside the context of a basic service set (BSS).
The literature uses the acronym OCB to denote "outside the context
of a BSS", and the class ``ns3::OcbWifiMac`` models this in |ns3|. 
Many management frames will not be used; the BSSID field needs to set 
a wildcard BSSID value. 
Management information is transmitted by what is called **vendor specific 
action frame**. 
With these changes, the packet transmissions (for a moving vehicle) can
be fast with small delay.  At the physical layer, the biggest difference is 
to use the 5.9 GHz band with a channel bandwidth of 10 MHz.  These physical
layer changes can make the wireless signal relatively more stable,
without degrading throughput too much (ranging from 3 Mbps to 27 Mbps), 
although 20 MHz channel bandwidth is still supported.
The class ``ns3::WaveNetDevice`` models a set of extensions,found in 
IEEE Std 1609.4-2010 [ieee1609.4]_.
The source code for the WAVE MAC models lives in the directory
``src/wave``.

About 802.11p
=============
When ``dot11OCBEnabled`` is true, a data frame can be sent to either an 
individual or a group destination MAC address.
This type of communication is only possible between STAs that are able to 
communicate directly over the wireless medium. 
It allows immediate communication, avoiding the latency associated with 
establishing a BSS.
When ``dot11OCBEnabled`` is true, a STA is not a member of a BSS 
and it does not utilize the IEEE 802.11 authentication, association, or 
data confidentiality services. 
This capability is particularly well-suited for use in rapidly varying 
communication environments 
such as those involving mobile STAs where the interval over 
which the communication exchanges take place may be of very short-duration 
(e.g., on the order of tens or hundreds of milliseconds).
Communication of data frames when ``dot11OCBEnabled`` is true might take 
place in a frequency band
that is dedicated for its use, and such a band might require licensing 
depending on the regulatory domain.
A STA for which ``dot11OCBEnabled`` is true initially transmits and 
receives on a channel known in advance, 
either through regulatory designation or some other out-of-band communication. 
A STA’s SME determines PHY layer parameters, as well as any changes in 
the operating channel. The Vendor Specific Action frame provides one 
means for STAs 
to exchange management information prior to communicating data frames 
outside the context of a BSS. 
When ``dot11OCBEnabled`` is true, a sending STA sets the BSSID field to 
the wildcard BSSID value

About 1609.4
============

WAVE devices must support a control channel with multiple service
channels.
To operate over multiple wireless channels while in operation with
OCBEnabled, there is a need to perform channel coordination.
Both data plane and management plane features are specified.

Data plane
##########

* Channel coordination
  The MAC sublayer coordinates channel intervals so that data packets 
  are transmitted on the proper RF channel at the right time.

* Channel routing
  The MAC sublayer handles inbound and outbound higher layer data.
  This specification includes routing of data packets from the LLC to 
  the designated channel, and setting parameters (e.g.,
  transmit power) for WAVE transmissions.

* User priority
  WAVE supports a variety of safety and nonsafety applications with 
  up to eight levels of priority as defined in IEEE Std 802.11. The
  use of user priority (UP) and related access category (AC)
  supports quality of service using enhanced distributed channel 
  access (EDCA) functionality specified in IEEE Std 802.11.

Management plane
################

* Multi-channel synchronization
  The MLME uses information derived locally and received over the air
  to provide a synchronization function with the objective of aligning
  channel intervals among communicating WAVE devices. The MLME provides
  the capability to generate Timing Advertisement (TA) frames to
  distribute system timing information and monitor received TA frames.

* Channel access
  The MLME controls the access to specific radio channels in support
  of communication requests received from the WME.

* Vendor Specific Action frames
  The MLME will accept incoming VSA frames and pass them to the WAVE Manage

* Readdressing
  The MLME allows device address changes to be triggered in 
  support of pseudonymity.

Design
======

In |ns3|, support for 802.11p involves the MAC and PHY layers.  

a) MAC layer: The interesting thing is that |ns3| AdhocWifiMac class is 
implemented 
very close to 802.11p OCB mode rather than real 802.11 ad-hoc mode.
The AdhocWifiMac has no BSS context that is defined in 802.11 standard, 
so it will not take time to send beacon and authenticate, which 
I think is not implemented perfectly. 
So OcbWifiMac is very similar to AdhocWifiMac, with some modifications. 
1. SetBssid, GetBssid, SetSsid, GetSsid
these methods are related to 802.11 BSS context which is unused in OCB context.
2. SetLinkUpCallback, SetLinkDownCallback
WAVE device can send packets directly, so wifi link is never down.
3. SendVsc, AddReceiveVscCallback
WAVE management information shall be sent by vendor specific action frame, 
and it will be called by upper layer 1609.4 standard to send WSA
(WAVE Service Advertisement) packets or other vendor specific information.
4. SendTimingAdvertisement (not implemented)
although Timing Advertisement is very important and specific defined in 
802.11p standard, it is useless in simulation environment. Every node in |ns3| is 
assumed already time synchronized,
which we can imagine that every node has a GPS device installed. 
5. ConfigureOcbDcf
This method will set default EDCA parameters not only in WAVE channeles including 
CCH ans SCHs. Further, the deprecated 802.11p code in wifi module has some 
mistakes in 802.11p DCF configuration.
The reference is old 1609.4-2006, but the newest shall be 1609.4-2010 
and 802.11p-2010 which set EDCA parameters different from the old standard.
6. WILDCARD BSSID
the WILDCARD BSSID is set to "ff:ff:ff:ff:ff:ff".
As defined in 802.11-2007, a wildcard BSSID shall not be used in the
BSSID field except for management frames of subtype probe request. But Adhoc 
mode of ns3 simplify this mechanism, when stations receive packets, packets 
regardless of BSSID will forward up to higher layer. This process is very close 
to OCB mode as defined in 802.11p-2010 which stations use the wildcard BSSID 
to allow higher layer of other stations can hear directly.
7. Enqueue, Receive
The most important methods are send and receive methods. According to the 
standard, we should filter frames that are not permitted. However here we 
just idetify the frames we care about, the other frames will be disgard.

b) phy layer: the original WifiPhyStandard contains 
WIFI_PHY_STANDARD_80211p_SCH and WIFI_PHY_STANDARD_80211p_CCH.
In the standard, the PHY 
layer wireless technology is still 80211a OFDM with 10MHz, so we just 
need WIFI_PHY_STANDARD_80211_10MHZ
(WIFI_PHY_STANDARD_80211a with 20MHz is supported, but not recommended.)
Another difference is transmit power. Normally the maximum station transmit power 
and maximum permited EIRP defined in 802.11p is larger than wifi, so transmit
range can get longer than usual wifi. But power will not be set automatically. Users 
who want to get longer range should configure atrributes "TxPowerStart", 
"TxPowerEnd" and "TxPowerLevels" of YansWifiPhy class by themself which means
tx power control mechanism will not be implemented.

In |ns3|, support for 1609.4 involves the MAC extension layers.  
The implemention approach follows that found in the |ns3| mesh module, which
will define a new WaveNetDevice that controls multiple MAC entities and one 
PHY entity instead of doing modification in source code of wifi module.
Note: we try best to not add code directly into wifi code, but this cannot 
avoid. WaveNetDevice is composed of ChannelScheduler, ChannelManager,  
ChannelCoordinator and VsaRepeater classess to assign the channel access resource 
of multiple channels.
1. SetChannelManager, SetChannelScheduler, SetChannelCoordinator
These methods are used when users want to use customized channel operations or 
want to debug state information of internal classes.
2. AddMac, SetPhy, SetRemoteStationManager
Different from WifiNetDevice, WaveNetDevice need more than one MAC entity to 
deal with channel switch. Although the 1609.4 standard also supports multiple 
PHY entities, the WaveNetDevice do not support that which now only condsiders the 
context of single-PHY. And RemoteStationManager maybe could not work well as 
in single channel environment. So we suggest users should consider whether the 
selected RemoteStationManager can suffer from multiple channel environment.
3. StartSch and StopSch
Different from 802.11p device can sent packets after constructing, a wave device 
should request to assign channel access for sending packets. The methods will use 
ChannelScheduler to assign and free channel resource.
4. SendX
After access is assigned, we can send WSMP packets or other packets by SendX method. 
We should specify the parameters of packets, e.g. which channel the packets will be 
sent. Beside that, we can also control data rate and tx power level, if we not set, 
then the data rate and tx power level will be set by RemoteStationManager automatically.
5. RegisterTxProfile and UnregisterTxProfile
If access is assigned and we want to send IP-based packets by Send method, we must 
register a tx profile to specify tx parameter then all packets sent by Send method 
will use the same channel number. If the adapate parameter is false, data rate and 
tx power level will be same for sending, otherwise data rate and tx power level will 
be set automatically by RemoteStationManager.
6. Send
Only access is assigned and tx profile is registered, we can send IP-based packets 
or other packets. And no matter data packets is sent by Send method or SendX method, 
the received data packets will forward up to higher layer by setted ReceiveCallback.
7. StartVsa and StopVsa
VSA frames is useful for 1609.3 to send WSA managemnt information. By StartVsa method 
we can specify transmit parameters.

Scope and Limitations
=====================

1. Is this model going to involve vehicular mobility of some sort?

Vehicular networks involve not only communication protocols, but also 
communication environment 
including vehicular mobility and propagation models. Because of specific 
features of the latter, the protocols need to change. The MAC layer model 
in this 
project just adopts MAC changes to vehicular environment. However this model 
does not involves any
vehicular mobility with time limit. Users can use any mobility model in |ns3|, 
but should know these models are not real vehicular mobility. 

2. Is this model going to use different propagation models?

Referring to the first issue, some more realistic propagation models 
for vehicualr environment
are suggested and welcome. And some existed propagation models in |ns3| are 
also suitable. 
Normally users can use Friis,Two-Ray Ground and Nakagami model. This 
model will not care about special propagation models.

3. are there any vehicular application models to drive the code?

About vehicular application models, SAE J2375 depends on WAVE architecture and
is an application message set in US; CAM and DENM in Europe between network 
and application layer, but is very close to application model. The BSM in 
J2375 and CAM send alert messages that every 
vehicle node will sent periodicity about its status information to 
cooperate with others. 
Fow now, there is no plan to develop a vehicular application model. 
But to drive WAVE MAC layer 
code, a simple application model will be tried to develop. 

4. any previous code you think you can leverage ?

This project will provide two NetDevices. 

First is normal 802.11p netdevice which uses new implemented OcbWifiMac. 
This is very useful 
for those people who only want to simulate routing protocols or upper layer
protocols for vehicular 
environment and do not need the whole WAVE architecture.
For any previous code, my suggestion is to use Wifi80211phelper with 
little modification 
on previous code or just use ad-hoc mode with 802.11a 10MHz (this is enough 
to simulate the main characteristic of 802.11p). 

The second NetDevices is a 1609.4 netdevice which provides some methods 
to deal with multi-channel operation. This is part of the whole WAVE 
architecture and provides service for upper 1609.3 standards, which could 
leverage no previous codes. 

5 Why here are two kinds of NetDevice?

In wave module, actually here are two device, one is 802.11p device which 
is the object of WifiNetDevice class, another is wave device which is 
the object of WaveNetDevice class. An "802.11p Net Device" is one that 
just runs the 802.11p extensions (channel frequency = 802.11a 5.9GHz, 
channel width = 10MHz, single instance of OcbWifiMac, and specific EDCA 
parameters). A "WAVE Net Device" is one that implements also 1609.1-4 
standards based on 802.11p, and now we are focusing only on 1609.4 modeling 
aspects (multi channel). 

The reason to provide a 802.11p device is that considering the fact many 
researchers are interested in route protocol or other aspects on vehicular 
environment of single channel, so they need neither multi-channel operation 
nor WAVE architectures. Besides that, the European standard could use 802.11p 
device in an modified ITS-G5 implementation (maybe could named ITSG5NetDevice).

References
==========

.. [ieee80211p] IEEE Std 802.11p-2010 *IEEE Standard for Information 
technology-- Local and metropolitan area networks-- Specific requirements-- 
Part 11: Wireless LAN Medium Access Control (MAC) and Physical Layer (PHY) 
Specifications Amendment 6: Wireless Access in Vehicular Environments*
.. [ieee1609.1] IEEE Std 1609.1-2010 *IEEE Standard for Wireless Access in 
Vehicular Environments (WAVE) - Resource Manager, 2010*
.. [ieee1609.2] IEEE Std 1609.2-2010 *IEEE Standard for Wireless Access in 
Vehicular Environments (WAVE) - Security Services for Applications and Management Messages, 2010*
.. [ieee1609.3] IEEE Std 1609.3-2010 *IEEE Standard for Wireless Access in 
Vehicular Environments (WAVE) - Networking Services, 2010*
.. [ieee1609.4] IEEE Std 1609.4-2010 *IEEE Standard for Wireless Access in 
Vehicular Environments (WAVE) - Multi-Channel Operation, 2010*

Usage
*****

Helpers
=======
The helpers include WaveMacHelper, NqosWaveMacHelper, QosWaveMacHelper, 
Wifi80211pHelper and WaveHelper. Wifi80211pHelper is used to create 
802.11p devices that follow the 802.11p-2010 standard. WaveHelper is 
used to create WAVE devices that follow the 802.11p-2010 and 1609.4-2010 
standards which are the MAC and PHY layers of the WAVE architecture. 
The relation of them is described as below:

::
    WifiHelper ----------use---------->  WifiMacHelper
     ^      ^                             ^       ^
     |      |                             |       |
     |      |                          inherit  inherit
     |      |                             |       |
     |      |                 QosWifiMacHelper  NqosWifiMacHelper
     |      |                             ^       ^
     |      |                             |       |
 inherit    inherit                     inherit  inherit
     |      |                             |       |
WaveHelper Wifi80211pHelper     QosWaveMacHelper  NqosWaveHelper

Although Wifi80211Helper can use any subclasses inheriting from 
WifiMacHelper, we force user shall use subclasses inheriting from 
QosWaveMacHelper or NqosWaveHelper.
Although WaveHelper can use any subclasses inheriting from WifiMacHelper, 
we force user shall use subclasses inheriting from QosWaveMacHelper. 
NqosWaveHelper is also not permitted, because 1609.4 standard requires the 
support for priority. 

Users can use Wifi80211pHelper to create "real" wifi 802.11p device.
Although the functions of wifi 802.11p device can be achieved by WaveNetDevice's 
ContinuousAccess assignment, we suggest to use Wifi80211pHelper.
Usage as follows:

::
    NodeContainer nodes;
    NetDeviceContainer devices;
    nodes.Create (2);
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    NqosWave80211pMacHelper wifi80211pMac = NqosWaveMacHelper::Default();
    Wifi80211pHelper 80211pHelper = Wifi80211pHelper::Default ();
    devices = 80211pHelper.Install (wifiPhy, wifi80211pMac, nodes);

Users can use wave-helper to create wave devices to deal with multiple
channel operations. The devices are WaveNetDevice objects. It's useful 
when users want to research on multi-channel environment or use WAVE 
architecture.usage as follows:

::
    NodeContainer nodes;
    NetDeviceContainer devices;
    nodes.Create (2);
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    QosWaveMacHelper waveMac = QosWaveMacHelper::Default ();
    WaveHelper waveHelper = WaveHelper::Default ();
    devices = waveHelper.Install (wifiPhy, waveMac, nodes);
APIs
====
The 802.11p device can allow the upper layer to send different information
over Vendor Specific Action management frames by using different
OrganizationIdentifier fields to identify differences.

1. already define a Node object and WifiNetdevice object
2. define an OrganizationIdentifier

::
   uint8_t oi_bytes[5] = {0x00, 0x50, 0xC2, 0x4A, 0x40};
   OrganizationIdentifier oi(oi_bytes,5);

3. define a Callback for the defined OrganizationIdentifier

::
   VscCallback vsccall = MakeCallback (&VsaExample::GetWsaAndOi, this);

4. OcbWifiMac of 802.11p device regists this identifier and function

::
      Ptr<WifiNetDevice> device1 = DynamicCast<WifiNetDevice>(nodes.Get (i)->GetDevice (0));
      Ptr<OcbWifiMac> ocb1 = DynamicCast<OcbWifiMac>(device->GetMac ());
      ocb1->AddReceiveVscCallback (oi, vsccall);

5. now one can send management packets over VSA frames

::
      Ptr<Packet> vsc = Create<Packet> ();
      ocb2->SendVsc (vsc, Mac48Address::GetBroadcast (), m_16093oi);

6. then registered callbacks in other devices will be called.

The wave device 

Attributes
==========

**What classes hold attributes, and what are the key ones worth mentioning?**

Output
======

**What kind of data does the model generate?  What are the key trace
sources?   What kind of logging output can be enabled?**

Advanced Usage
==============

**Go into further details (such as using the API outside of the helpers)
in additional sections, as needed.**

Examples
========

**What examples using this new code are available?  Describe them here.**

Troubleshooting
===============

**Add any tips for avoiding pitfalls, etc.**

Validation
**********

**Describe how the model has been tested/validated.  What tests run in the
test suite?  How much API and code is covered by the tests?  Again,
references to outside published work may help here.**

