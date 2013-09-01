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
#include "expire-time-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ExpireTimeTag);

TypeId
ExpireTimeTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ExpireTimeTag")
    .SetParent<Tag> ()
    .AddConstructor<ExpireTimeTag> ()
  ;
  return tid;
}
TypeId
ExpireTimeTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

ExpireTimeTag::ExpireTimeTag (void)
  : m_expire (500)
{
}
ExpireTimeTag::ExpireTimeTag (uint32_t expire)
  : m_expire (expire)
{
}
ExpireTimeTag::~ExpireTimeTag (void)
{
}
uint32_t
ExpireTimeTag::GetSerializedSize (void) const
{
  return sizeof (uint32_t);
}
void
ExpireTimeTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_expire);
}
void
ExpireTimeTag::Deserialize (TagBuffer i)
{
  m_expire = i.ReadU32 ();
}
void
ExpireTimeTag::Print (std::ostream &os) const
{
  os << "expire = " << m_expire << "ms";
}
void
ExpireTimeTag::SetExpireTime (uint32_t expire)
{
  m_expire = expire;
}
uint32_t
ExpireTimeTag::GetExpireTime (void) const
{
  return m_expire;
}

} // namespace ns3
