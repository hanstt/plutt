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
#include <node_match_value.hpp>

NodeMatchValue::NodeMatchValue(std::string const &a_loc, NodeValue *a_l,
    NodeValue *a_r, double a_cutoff):
  NodeValue(a_loc),
  m_node_l(a_l),
  m_node_r(a_r),
  m_cutoff(a_cutoff),
  m_val_l(),
  m_val_r()
{
}

Value const &NodeMatchValue::GetValue(uint32_t a_ret_i)
{
  switch (a_ret_i) {
    case 0:
      return m_val_l;
    case 1:
      return m_val_r;
    default:
      throw std::runtime_error(__func__);
  }
}

void NodeMatchValue::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_node_l, a_evid);
  NODE_PROCESS(m_node_r, a_evid);

  m_val_l.Clear();
  m_val_r.Clear();

  auto const &val_l = m_node_l->GetValue();
  auto const &val_r = m_node_r->GetValue();
  m_val_l.SetType(val_l.GetType());
  m_val_r.SetType(val_r.GetType());

  uint32_t i_l = 0;
  uint32_t i_r = 0;
  while (i_l < val_l.GetID().size() && i_r < val_r.GetID().size()) {
    auto mi_l = val_l.GetID()[i_l];
    auto mi_r = val_r.GetID()[i_r];
    if (mi_l < mi_r) {
      ++i_l;
    } else if (mi_l > mi_r) {
      ++i_r;
    } else {

      auto me_l0 = 0 == i_l ? 0 : val_l.GetEnd().at(i_l - 1);
      auto me_l1 = val_l.GetEnd().at(i_l);

      auto me_r0 = 0 == i_r ? 0 : val_r.GetEnd().at(i_r - 1);
      auto me_r1 = val_r.GetEnd().at(i_r);

      while (me_l0 < me_l1 && me_r0 < me_r1) {
        auto v_l = val_l.GetV()[me_l0];
        auto v_r = val_r.GetV()[me_r0];

        auto d = std::abs(v_r.dbl - v_l.dbl);
        if (d < m_cutoff) {
          // Match!
          m_val_l.Push(mi_l, v_l);
          m_val_r.Push(mi_r, v_r);
          ++me_l0;
          ++me_r0;
        } else {
          if (me_l0 + 1 == me_l1) {
            ++me_r0;
          } else if (me_r0 + 1 == me_r1) {
            ++me_l0;
          } else {
            // Let's assume values come in sorted order, but we don't know the
            // direction! (Eg. vftx2 and tamex3 are opposite.) So, peek at the
            // next value and see which gets closer.
            auto vn_l = val_l.GetV()[me_l0 + 1];
            auto vn_r = val_r.GetV()[me_r0 + 1];
            auto dl = std::abs(vn_l.dbl - v_r.dbl);
            auto dr = std::abs(vn_r.dbl - v_l.dbl);
            if (dl < dr) {
              ++me_l0;
            } else {
              ++me_r0;
            }
          }
        }
      }

      ++i_l;
      ++i_r;
    }
  }
}
