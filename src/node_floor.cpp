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
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_floor.hpp>

NodeFloor::NodeFloor(std::string const &a_loc, NodeValue *a_child):
  NodeValue(a_loc),
  m_child(a_child),
  m_value()
{
}

Value const &NodeFloor::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeFloor::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_child, a_evid);

  m_value.Clear();

  auto const &val = m_child->GetValue();
  m_value.SetType(val.GetType());

  uint32_t v_i = 0;
  for (uint32_t i = 0; i < val.GetID().size(); ++i) {
    auto mi = val.GetID()[i];
    auto me = val.GetEnd()[i];
    for (; v_i < me; ++v_i) {
      auto f = floor(val.GetV(v_i, true));
      Input::Scalar s;
      s.dbl = f;
      m_value.Push(mi, s);
    }
  }
}
