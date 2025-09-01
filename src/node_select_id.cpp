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
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_select_id.hpp>

NodeSelectId::NodeSelectId(std::string const &a_loc, NodeValue *a_child,
    uint32_t a_first, uint32_t a_last):
  NodeValue(a_loc),
  m_child(a_child),
  m_first(a_first),
  m_last(a_last),
  m_value()
{
  assert(m_first <= m_last);
}

Value const &NodeSelectId::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeSelectId::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_child, a_evid);

  m_value.Clear();

  auto const &val = m_child->GetValue();
  m_value.SetType(val.GetType());

  auto const &vmi = val.GetID();
  auto const &vme = val.GetEnd();
  auto const &vv = val.GetV();

  for (uint32_t i = 0; i < vmi.size(); ++i) {
    auto mi = vmi[i];
    if (m_first <= mi && mi <= m_last) {
      auto me_0 = 0 == i ? 0 : vme[i - 1];
      auto me_1 = vme[i];
      for (; me_0 < me_1; ++me_0) {
        m_value.Push(mi, vv[me_0]);
      }
    }
  }
}
