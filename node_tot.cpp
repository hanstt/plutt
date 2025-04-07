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
#include <util.hpp>
#include <node_tot.hpp>

NodeTot::NodeTot(std::string const &a_loc, NodeValue *a_l, NodeValue *a_t,
    double a_range):
  NodeValue(a_loc),
  m_l(a_l),
  m_t(a_t),
  m_range(a_range),
  m_value()
{
}

Value const &NodeTot::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeTot::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_l, a_evid);
  NODE_PROCESS(m_t, a_evid);

  m_value.Clear();

  auto const &val_l = m_l->GetValue();
  auto const &val_t = m_t->GetValue();
  NODE_ASSERT(val_l.GetType(), ==, val_t.GetType());
  m_value.SetType(Input::kDouble);

  uint32_t i_l = 0;
  uint32_t i_t = 0;
  uint32_t vi_l = 0;
  uint32_t vi_t = 0;
  while (i_l < val_l.GetID().size() && i_t < val_t.GetID().size()) {
    auto mi_l = val_l.GetID()[i_l];
    auto mi_t = val_t.GetID()[i_t];
    auto me_l = val_l.GetEnd()[i_l];
    auto me_t = val_t.GetEnd()[i_t];
    if (mi_l < mi_t) {
      ++i_l;
      vi_l = me_l;
    } else if (mi_l > mi_t) {
      ++i_t;
      vi_t = me_t;
    } else {
      while (vi_l < me_l && vi_t < me_t) {
        double l = val_l.GetV(vi_l, false);
        double t = val_t.GetV(vi_t, false);
        double d = SubModDbl(t, l, m_range);
        if (d > 0) {
          Input::Scalar diff;
          diff.dbl = d;
          m_value.Push(mi_l, diff);
          ++vi_l;
        }
        ++vi_t;
      }
      ++i_l;
      ++i_t;
      vi_l = me_l;
      vi_t = me_t;
    }
  }
}
