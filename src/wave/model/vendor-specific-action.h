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
#ifndef VendorSpecificAction_H
#define VendorSpecificAction_H

#include <ostream>
#include <map>
#include "ns3/header.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/attribute.h"
#include "ns3/attribute-helper.h"
#include "ns3/address.h"

namespace ns3 {
class VendorSpecificContentManager;

/**
 * \brief the organization identifier is a public organizationally
 * unique identifier assigned by the IEEE.
 *
 * Similar to protocol field of data packets that indicates which
 * entity of higher layer should handle received packets, Organization
 * Identifier field is used in Vendor Specific Action frames to
 * indicate which entity or higher layer should handle vendor specific
 * content.
 * Normally the value is assigned by IEEE and the length of field is
 * either 24 bits or 36 bits.
 * For more, see IEEE802.11p-2010 section 7.3.1.31 and 7.4.5
 */
class OrganizationIdentifier
{
public:
  OrganizationIdentifier (void);
  OrganizationIdentifier (const uint8_t *str, uint32_t length);
  OrganizationIdentifier& operator= (const OrganizationIdentifier& oi);
  virtual ~OrganizationIdentifier (void);

  const uint8_t * PeekData (void) const;

  bool IsNull (void) const;

  enum OrganizationIdentifierType
  {
    OUI24 = 3,  // 3 bytes
    OUI36 = 5,   // 5 bytes
    Unknown = 0,
  };

  void SetType (enum OrganizationIdentifierType type);

  enum OrganizationIdentifierType GetType () const;

  // below methods will be called by VendorSpecificActionHeader
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

private:
  friend bool operator == (const OrganizationIdentifier& a, const OrganizationIdentifier& b);
  friend bool operator != (const OrganizationIdentifier& a, const OrganizationIdentifier& b);
  friend bool operator < (const OrganizationIdentifier& a, const OrganizationIdentifier& b);
  friend std::ostream& operator << (std::ostream& os, const OrganizationIdentifier& oi);
  friend std::istream& operator >> (std::istream& is, const OrganizationIdentifier& oi);

  enum OrganizationIdentifierType m_type;
  uint8_t m_oi[5];
};

ATTRIBUTE_HELPER_HEADER (OrganizationIdentifier);

bool operator == (const OrganizationIdentifier& a, const OrganizationIdentifier& b);
bool operator != (const OrganizationIdentifier& a, const OrganizationIdentifier& b);
bool operator < (const OrganizationIdentifier& a, const OrganizationIdentifier& b);

std::ostream& operator << (std::ostream& os, const OrganizationIdentifier& oi);
std::istream& operator >> (std::istream& is, const OrganizationIdentifier& oi);

/**
 * see IEEE 802.11-2007 chapter 7.3.1.11
 * Table 7-24—Category values
 */
const static uint8_t CATEGORY_OF_VSA = 127;

/**
 * See IEEE 802.11-2007 chapter 7.3.1.11 and 7.4.5
 * also IEEE 802.11p-2010 chapter 7.4.5
 * Although WifiActionHeader has defined in wifi mgt-header.h/.cc,
 * it is not a good way to inherit from it or add vendor specific action support.
 * The reason is Action field. Other Action frames contains Action field, and
 * the VSA frame contains OUI or variable OrganizationIdentifier instead of one byte Action field.
 * Header format: | Category | Organization IdentifierType | Vendor Specific content |
 *        Octets:       1                j                        Variable
 *
 * In ns-3, VSA frame will be divided into  VendorSpecificActionHeader vsaHeader
 * and Packet vsc that indicates vendor specific content.
 *
 * This frame is used by IEEE 1609.4 to send WSA packet of IEEE 1609.3,
 * and if users want to send management information without customized management
 * frames, he can use VSA frames.
 */
class VendorSpecificActionHeader : public Header
{
public:
  VendorSpecificActionHeader (void);
  virtual ~VendorSpecificActionHeader (void);

  void SetOrganizationIdentifier (OrganizationIdentifier oi);
  OrganizationIdentifier GetOrganizationIdentifier (void) const;
  /**
   * the category field shall be CATEGORY_OF_VSA
   */
  uint8_t GetCategory () const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  OrganizationIdentifier m_oi;
  uint8_t m_category;
};

/**
 * \param oi the organization identifier of vendor specific action frame
 * \param packet the vendor specifc content packet received
 * \param sender the address of the sender
 * \returns true if the callback could handle the packet successfully;
 *       false otherwise.
 */
typedef Callback<bool, const OrganizationIdentifier &, Ptr<const Packet>,const Address &> VscCallback;

class VendorSpecificContentManager
{
public:
  VendorSpecificContentManager (void);
  virtual ~VendorSpecificContentManager (void);
  /**
   * \param oi the specific OrganizationIdentifier when receive management information
   * by VendorSpecificAction management frame.
   * \param cb the receive callback when oi related management packets are received
   */
  void RegisterVscCallback (OrganizationIdentifier oi, VscCallback cb);
  void DeregisterVscCallback (OrganizationIdentifier &oi);
  VscCallback FindVscCallback (OrganizationIdentifier &oi);

private:
  typedef std::map<OrganizationIdentifier,VscCallback> VscCallbacks;
  typedef std::map<OrganizationIdentifier,VscCallback>::iterator VscCallbacksI;

  VscCallbacks m_callbacks;
  friend class OrganizationIdentifier;
};

static std::vector<OrganizationIdentifier> OrganizationIdentifiers;
}

#endif /* VendorSpecificActionHeader_H */
