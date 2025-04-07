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

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_member.hpp>

NodeMember::NodeMember(std::string const &a_loc, NodeValue *a_child, char
    const *a_suffix):
  NodeValue(a_loc),
  m_value(),
  m_child(a_child),
  m_is_id()
{
  if (0 == strcmp(a_suffix, "id")) {
    m_is_id = true;
  } else if (0 == strcmp(a_suffix, "v")) {
    m_is_id = false;
  } else {
    std::cerr << GetLocStr() << ": Invalid suffix '" << a_suffix << "'.\n";
    throw std::runtime_error(__func__);
  }
}

Value const &NodeMember::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeMember::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_child, a_evid);

  m_value.Clear();

  auto const &val = m_child->GetValue();
  if (m_is_id) {
    m_value.SetType(Input::kUint64);
    for (auto it = val.GetID().begin(); val.GetID().end() != it; ++it) {
      Input::Scalar s;
      s.u64 = *it;
      m_value.Push(0, s);
    }
  } else {
    m_value.SetType(val.GetType());
    for (auto it = val.GetV().begin(); val.GetV().end() != it; ++it) {
      m_value.Push(0, *it);
    }
  }
}
