/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2025
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
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
#include <node_array.hpp>

NodeArray::NodeArray(std::string const &a_loc, NodeValue *a_child, uint64_t
    a_i, uint64_t a_mhit_i):
  NodeValue(a_loc),
  m_value(),
  m_child(a_child),
  m_i(a_i),
  m_mhit_i(a_mhit_i)
{
}

Value const &NodeArray::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeArray::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_child, a_evid);

  m_value.Clear();

  auto const &val = m_child->GetValue();
  auto const &vid = val.GetID();
  if (m_i < vid.size()) {
    auto const &v = val.GetV();
    auto const &vend = val.GetEnd();
    auto end = vend[m_i];
    auto vi = 0 == m_i ? 0 : vend[m_i - 1];
    if (vi + m_mhit_i < end) {
      m_value.SetType(val.GetType());
      auto id = vid[m_i];
      auto s = v[vi + m_mhit_i];
      m_value.Push(id, s);
    }
  }
}
