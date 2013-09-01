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
#include "higher-tx-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (HigherDataTxVectorTag);

TypeId
HigherDataTxVectorTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HigherDataTxVectorTag")
    .SetParent<Tag> ()
    .AddConstructor<HigherDataTxVectorTag> ()
  ;
  return tid;
}
HigherDataTxVectorTag::HigherDataTxVectorTag (void)
  : m_adapter (false)
{
}
HigherDataTxVectorTag::HigherDataTxVectorTag (WifiTxVector dataTxVector, bool adapter)
  : m_dataTxVector (dataTxVector),
    m_adapter (adapter)
{
}
HigherDataTxVectorTag::~HigherDataTxVectorTag (void)
{
}
TypeId
HigherDataTxVectorTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

WifiTxVector
HigherDataTxVectorTag::GetDataTxVector (void) const
{
  return m_dataTxVector;
}
bool
HigherDataTxVectorTag::IsAdapter (void) const
{
  return m_adapter;
}

uint32_t
HigherDataTxVectorTag::GetSerializedSize (void) const
{
  return (sizeof (WifiTxVector) + 1);
}
void
HigherDataTxVectorTag::Serialize (TagBuffer i) const
{
  i.Write ((uint8_t *)&m_dataTxVector, sizeof (WifiTxVector));
  i.WriteU8 (static_cast<uint8_t> (m_adapter));
}
void
HigherDataTxVectorTag::Deserialize (TagBuffer i)
{
  i.Read ((uint8_t *)&m_dataTxVector, sizeof (WifiTxVector));
  m_adapter = i.ReadU8 ();
}
void
HigherDataTxVectorTag::Print (std::ostream &os) const
{
  os << " Data=" << m_dataTxVector << " Adapter=" << m_adapter;
}

} // namespace ns3
