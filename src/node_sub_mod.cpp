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
#include <node_sub_mod.hpp>

NodeSubMod::NodeSubMod(std::string const &a_loc, NodeValue *a_l, NodeValue
    *a_r, double a_range):
  NodeValue(a_loc),
  m_l(a_l),
  m_r(a_r),
  m_range(a_range),
  m_value()
{
}

Value const &NodeSubMod::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeSubMod::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_l, a_evid);
  NODE_PROCESS(m_r, a_evid);

  m_value.Clear();

  auto const &val_l = m_l->GetValue();
  auto const &val_r = m_r->GetValue();
  if (Input::kNone == val_l.GetType() ||
      Input::kNone == val_r.GetType()) {
    return;
  }
  NODE_ASSERT(val_l.GetType(), ==, val_r.GetType());
  m_value.SetType(Input::kDouble);

  uint32_t i_l = 0;
  uint32_t i_r = 0;
  uint32_t vi_l = 0;
  uint32_t vi_r = 0;
  while (i_l < val_l.GetID().size() && i_r < val_r.GetID().size()) {
    auto mi_l = val_l.GetID()[i_l];
    auto mi_r = val_r.GetID()[i_r];
    auto me_l = val_l.GetEnd()[i_l];
    auto me_r = val_r.GetEnd()[i_r];
    if (mi_l > 0 && mi_l < mi_r) {
      ++i_l;
      vi_l = me_l;
    } else if (mi_r > 0 && mi_l > mi_r) {
      ++i_r;
      vi_r = me_r;
    } else {
      auto id = std::max(mi_l, mi_r);
      while (vi_l < me_l && vi_r < me_r) {
        Input::Scalar diff;
        if (Input::kDouble == val_l.GetType()) {
          auto l = val_l.GetV(vi_l, false);
          auto r = val_r.GetV(vi_r, false);
          diff.dbl = SubModDbl(l, r, m_range);
        } else {
          auto l = val_l.GetV().at(vi_l).u64;
          auto r = val_r.GetV().at(vi_r).u64;
          diff.dbl = SubModU64(l, r, m_range);
        }
        m_value.Push(id, diff);
        ++vi_l;
        ++vi_r;
      }
      ++i_l;
      ++i_r;
      vi_l = me_l;
      vi_r = me_r;
    }
  }
}
