/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023, 2025
 * Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <node.hpp>

Node::ProcessGuard::ProcessGuard(Node *a_node, uint64_t a_evid):
  m_node(a_node)
{
  m_node->m_evid = a_evid;
  m_node->m_is_active = true;
}

Node::ProcessGuard::~ProcessGuard()
{
  m_node->m_is_active = false;
}

Node::Node(std::string const &a_loc):
  m_loc(a_loc),
  m_evid(),
  m_is_active()
{
}

std::string Node::GetLocStr() const
{
  return m_loc;
}

bool Node::IsActive() const
{
  return m_is_active;
}

bool Node::IsEvent(uint64_t a_evid) const
{
  return m_evid == a_evid;
}

NodeValue::NodeValue(std::string const &a_loc):
  Node(a_loc)
{
}

NodeCuttable::NodeCuttable(std::string const &a_loc, std::string const
    &a_title):
  Node(a_loc),
  m_title(a_title),
  m_cut_consumer(),
  m_cut_producer()
{
}

NodeCutValue *NodeCuttable::CutDataAdd(CutPolygon const *a_poly)
{
  return m_cut_producer.AddData(a_poly);
}

void NodeCuttable::CutEventAdd(NodeCuttable *a_node, CutPolygon const *a_poly)
{
  auto is_ok = a_node->m_cut_producer.AddEvent(a_poly);
  m_cut_consumer.Add(a_node, is_ok);
}

void NodeCuttable::CutReset()
{
  m_cut_producer.Reset();
}

std::string const &NodeCuttable::GetTitle() const
{
  return m_title;
}
