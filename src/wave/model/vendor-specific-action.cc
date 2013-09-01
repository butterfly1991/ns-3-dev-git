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
#include <iomanip>
#include <iostream>
#include <cstring>
#include "ns3/log.h"
#include "ns3/assert.h"
#include "vendor-specific-action.h"

NS_LOG_COMPONENT_DEFINE ("VendorSpecificAction");

namespace ns3 {

/*********** OrganizationIdentifier *******/

ATTRIBUTE_HELPER_CPP (OrganizationIdentifier);

OrganizationIdentifier::OrganizationIdentifier (void)
  :     m_type (Unknown)
{
  NS_LOG_FUNCTION (this);
  m_type = Unknown;
  std::memset (m_oi, 0, 5);
}

OrganizationIdentifier::OrganizationIdentifier (const uint8_t *str, uint32_t length)
{
  NS_LOG_FUNCTION (this << str << length);
  if (length == 3)
    {
      m_type = OUI24;
      std::memcpy (m_oi, str, length);
    }
  else if (length == 5)
    {
      m_type = OUI36;
      std::memcpy (m_oi, str, length);
    }
  else
    {
      m_type = Unknown;
      NS_FATAL_ERROR ("cannot support organization identifier with length=" << length);
    }
}

OrganizationIdentifier&
OrganizationIdentifier::operator= (const OrganizationIdentifier& oi)
{
  this->m_type = oi.m_type;
  std::memcpy (this->m_oi, oi.m_oi, 5);
  return (*this);
}

OrganizationIdentifier::~OrganizationIdentifier (void)
{

}

const uint8_t *
OrganizationIdentifier::PeekData (void) const
{
  return (const uint8_t *)this->m_oi;
}

bool
OrganizationIdentifier::IsNull (void) const
{
  return m_type == Unknown;
}

uint32_t
OrganizationIdentifier::GetSerializedSize (void) const
{
  switch (m_type)
    {
    case OUI24:
      return 3;
    case OUI36:
      return 5;
    case Unknown:
    default:
      NS_FATAL_ERROR_NO_MSG ();
      return 0;
    }
}

void
OrganizationIdentifier::SetType (enum OrganizationIdentifierType type)
{
  m_type = type;
}

enum OrganizationIdentifier::OrganizationIdentifierType
OrganizationIdentifier::GetType (void) const
{
  return m_type;
}

void
OrganizationIdentifier::Serialize (Buffer::Iterator start) const
{
  start.Write (m_oi, GetSerializedSize ());
}

/*  because OrganizationIdentifier field is not standard
 *  and the length of OrganizationIdentifier is variable
 *  so data parse here is troublesome
 */
uint32_t
OrganizationIdentifier::Deserialize (Buffer::Iterator start)
{
  // first try to parse OUI23 with 3 bytes
  start.Read (m_oi,  3);
  std::vector<OrganizationIdentifier>& c = OrganizationIdentifiers;
  for (std::vector<OrganizationIdentifier>::iterator  i = c.begin (); i != c.end (); ++i)
    {
      const OrganizationIdentifier & m = *i;
      const uint8_t * data = m.PeekData ();
      if ((m.GetType () == OUI24)
          && (std::memcmp (data, m_oi, 3) == 0 ))
        {
          m_type = OUI24;
          return 3;
        }
    }

  // then try to parse OUI36 with 5 bytes
  start.Read (m_oi + 3,  2);
  for (std::vector<OrganizationIdentifier>::iterator  i = c.begin (); i != c.end (); ++i)
    {
      const OrganizationIdentifier & m = *i;
      const uint8_t * data = m.PeekData ();
      if ((m.GetType () == OUI36)
          && (std::memcmp (data, m_oi, 4) == 0 ))
        {
          // OUI36 first check 4 bytes, then check half of the 5th byte
          if ((data[4] & 0xf0) == (m_oi[4] & 0xf0))
            {
              m_type = OUI36;
              return 5;
            }
        }
    }

  // if we cannot deserialize the organization identifier field,
  // we will fail
  NS_FATAL_ERROR ("cannot deserialize the organization identifier field successfully");
  return 0;
}

bool operator == (const OrganizationIdentifier& a, const OrganizationIdentifier& b)
{
  if (a.m_type != b.m_type)
    {
      return false;
    }

  if (a.m_type == OrganizationIdentifier::OUI24)
    {
      return memcmp (a.m_oi, b.m_oi, 3) == 0;
    }

  if (a.m_type == OrganizationIdentifier::OUI36)
    {
      return (memcmp (a.m_oi, b.m_oi, 4) == 0)
             && ((a.m_oi[4] & 0xf0) == (b.m_oi[4] & 0xf0));
    }

  return false;
}

bool operator != (const OrganizationIdentifier& a, const OrganizationIdentifier& b)
{
  return !(a == b);
}

bool operator < (const OrganizationIdentifier& a, const OrganizationIdentifier& b)
{
  return memcmp (a.m_oi, b.m_oi, std::min (a.m_type, b.m_type)) < 0;
}

std::ostream& operator << (std::ostream& os, const OrganizationIdentifier& oi)
{
  for (uint32_t i = 0; i < oi.m_type; i++)
    {
      os << "0x" << std::hex << static_cast<int> (oi.m_oi[i]) << " ";
    }
  os << std::endl;
  return os;
}

std::istream& operator >> (std::istream& is, const OrganizationIdentifier& oi)
{
  return is;
}

/*********** VendorSpecificActionHeader *******/
NS_OBJECT_ENSURE_REGISTERED (VendorSpecificActionHeader);

VendorSpecificActionHeader::VendorSpecificActionHeader (void)
  :     m_oi (),
    m_category (CATEGORY_OF_VSA)
{
}

VendorSpecificActionHeader::~VendorSpecificActionHeader (void)
{

}

void
VendorSpecificActionHeader::SetOrganizationIdentifier (OrganizationIdentifier oi)
{
  m_oi = oi;
}

OrganizationIdentifier
VendorSpecificActionHeader::GetOrganizationIdentifier (void) const
{
  return m_oi;
}

TypeId
VendorSpecificActionHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VendorSpecificActionHeader")
    .SetParent<Header> ()
    .AddConstructor<VendorSpecificActionHeader> ()
  ;

  return tid;
}

uint8_t
VendorSpecificActionHeader::GetCategory (void) const
{
  return m_category;
}

TypeId
VendorSpecificActionHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
VendorSpecificActionHeader::Print (std::ostream &os) const
{
  os << "VendorSpecificActionHeader[ "
     << "category = 0x" << std::hex << (int)m_category
     << "organization identifier = " << m_oi
     << std::endl;
}

uint32_t
VendorSpecificActionHeader::GetSerializedSize (void) const
{
  return sizeof(m_category) + m_oi.GetSerializedSize ();
}

void
VendorSpecificActionHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU8 (m_category);
  m_oi.Serialize (start);
}

uint32_t
VendorSpecificActionHeader::Deserialize (Buffer::Iterator start)
{
  m_category = start.ReadU8 ();
  if (m_category != CATEGORY_OF_VSA)
    {
      return 0;
    }
  m_oi.Deserialize (start);

  return GetSerializedSize ();
}

/********* VendorSpecificContentManager ***********/
VendorSpecificContentManager::VendorSpecificContentManager (void)
{

}

VendorSpecificContentManager::~VendorSpecificContentManager (void)
{

}

void
VendorSpecificContentManager::RegisterVscCallback (OrganizationIdentifier oi, VscCallback cb)
{
  m_callbacks.insert (std::make_pair (oi, cb));
  OrganizationIdentifiers.push_back (oi);
}

void
VendorSpecificContentManager::DeregisterVscCallback (OrganizationIdentifier &oi)
{
  m_callbacks.erase (oi);
}

static VscCallback null_callback = MakeNullCallback<bool,const OrganizationIdentifier &,Ptr<const Packet>,const Address &> ();

VscCallback
VendorSpecificContentManager::FindVscCallback (OrganizationIdentifier &oi)
{
  VscCallbacksI i;
  i = m_callbacks.find (oi);
  return (i == m_callbacks.end ()) ? null_callback : i->second;
}

} // namespace ns3
