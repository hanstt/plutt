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
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_mean_geom.hpp>

NodeMeanGeom::NodeMeanGeom(std::string const &a_loc, NodeValue *a_l, NodeValue
    *a_r):
  NodeValue(a_loc),
  m_l(a_l),
  m_r(a_r),
  m_value()
{
}

Value const &NodeMeanGeom::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeMeanGeom::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_l, a_evid);

  m_value.Clear();
  m_value.SetType(Input::kDouble);

  auto const &val_l = m_l->GetValue();

  if (!m_r) {
    // Arith-mean over all "I" of n:th entry in "v".
    for (uint32_t vi = 0;; ++vi) {
      double sum = 0.0;
      uint32_t num = 0;
      uint32_t me_0 = 0;
      for (uint32_t i = 0; i < val_l.GetID().size(); ++i) {
        auto me_1 = val_l.GetEnd()[i];
        if (me_0 + vi < me_1) {
          sum += val_l.GetV(me_0 + vi, false);
          ++num;
        }
        me_0 = me_1;
      }
      if (!num) {
        break;
      }
      Input::Scalar mean;
      mean.dbl = pow(sum, 1.0 / num);
      m_value.Push(0, mean);
    }
  } else {
    // Arith-mean between two signals for each "I".
    NODE_PROCESS(m_r, a_evid);
    auto const &val_r = m_r->GetValue();
    NODE_ASSERT(val_l.GetID().size(), ==, val_r.GetID().size());

    uint32_t me0_l = 0;
    uint32_t me0_r = 0;
    for (uint32_t i = 0; i < val_l.GetID().size(); ++i) {
      auto mi = val_l.GetID()[i];
      NODE_ASSERT(mi, ==, val_r.GetID()[i]);
      auto me1_l = val_l.GetEnd()[i];
      auto me1_r = val_r.GetEnd()[i];
      while (me0_l < me1_l && me0_r < me1_r) {
        Input::Scalar mean;
        double prod = 1.0;
        double num = 0;
        if (me0_l < me1_l) {
          prod *= val_l.GetV(me0_l++, true);
          ++num;
        }
        if (me0_r < me1_r) {
          prod *= val_r.GetV(me0_r++, true);
          ++num;
        }
        mean.dbl = pow(prod, 1 / num);
        m_value.Push(mi, mean);
      }
      me0_l = me1_l;
      me0_r = me1_r;
    }
  }
}
