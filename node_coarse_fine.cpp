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
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_coarse_fine.hpp>

NodeCoarseFine::NodeCoarseFine(std::string const &a_loc, NodeValue *a_coarse,
    NodeValue *a_fine, double a_fine_range):
  NodeValue(a_loc),
  m_coarse(a_coarse),
  m_fine(a_fine),
  m_fine_range(a_fine_range),
  m_cal_fine(),
  m_value()
{
}

Value const &NodeCoarseFine::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeCoarseFine::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);

  NODE_PROCESS(m_coarse, a_evid);
  NODE_PROCESS(m_fine, a_evid);

  m_value.Clear();

  auto const &val_c = m_coarse->GetValue();
  auto const &val_f = m_fine->GetValue();
  NODE_ASSERT(Input::kUint64, ==, val_c.GetType());
  NODE_ASSERT(Input::kUint64, ==, val_f.GetType());
  NODE_ASSERT(val_c.GetID().size(), ==, val_f.GetID().size());
  NODE_ASSERT(val_c.GetV().size(), ==, val_f.GetV().size());

  m_value.SetType(Input::kDouble);

  uint32_t vi = 0;
  for (uint32_t i = 0; i < val_c.GetID().size(); ++i) {
    auto mi = val_c.GetID()[i];
    NODE_ASSERT(mi, ==, val_f.GetID()[i]);
    auto me = val_c.GetEnd()[i];
    NODE_ASSERT(me, ==, val_f.GetEnd()[i]);
    for (; vi < me; ++vi) {
      auto c = (uint32_t)val_c.GetV().at(vi).u64;
      auto f = (uint32_t)val_f.GetV().at(vi).u64;
      if (mi >= m_cal_fine.size()) {
        m_cal_fine.resize(mi + 1);
      }
      double ft = m_cal_fine.at(mi).Get(f);
      Input::Scalar time;
      time.dbl = m_fine_range * ((c + 1) - ft);
      m_value.Push(mi, time);
    }
  }
}
