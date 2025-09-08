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
#include <node_mean_arith.hpp>

NodeMeanArith::NodeMeanArith(std::string const &a_loc, NodeValue *a_l,
    NodeValue *a_r):
  NodeValue(a_loc),
  m_l(a_l),
  m_r(a_r),
  m_value()
{
}

Value const &NodeMeanArith::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeMeanArith::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_l, a_evid);

  m_value.Clear();
  m_value.SetType(Input::kDouble);

  auto const &val_l = m_l->GetValue();
  auto const &iv_l = val_l.GetID();
  auto const &ev_l = val_l.GetEnd();

  if (!m_r) {
    // Arith-mean over all "I" of n:th entry in "v".
    for (uint32_t dvi = 0;; ++dvi) {
      double sum = 0.0;
      uint32_t num = 0;
      uint32_t me_0 = 0;
      for (uint32_t i = 0; i < iv_l.size(); ++i) {
        auto me_1 = ev_l[i];
        auto vi = me_0 + dvi;
        if (vi < me_1) {
          sum += val_l.GetV(vi, false);
          ++num;
        }
        me_0 = me_1;
      }
      if (!num) {
        break;
      }
      Input::Scalar mean;
      mean.dbl = sum / num;
      m_value.Push(0, mean);
    }
  } else {
    // Arith-mean between two signals for each "I".
    NODE_PROCESS(m_r, a_evid);

    auto const &val_r = m_r->GetValue();
    auto const &iv_r = val_r.GetID();
    auto const &ev_r = val_r.GetEnd();

    uint32_t i_l = 0;
    uint32_t i_r = 0;
    uint32_t me0_l = 0;
    uint32_t me0_r = 0;
    for (;;) {
      uint32_t mi_l = UINT32_MAX;
      uint32_t mi_r = UINT32_MAX;
      uint32_t me1_l = 0;
      uint32_t me1_r = 0;
      if (i_l < iv_l.size()) {
        mi_l = iv_l[i_l];
        me1_l = ev_l[i_l];
      }
      if (i_r < iv_r.size()) {
        mi_r = iv_r[i_r];
        me1_r = ev_r[i_r];
      }
      if (UINT32_MAX == mi_l && UINT32_MAX == mi_r) {
        break;
      }
      auto const mi = std::min(mi_l, mi_r);
      for (;;) {
        Input::Scalar mean;
        double sum = 0.0;
        double num = 0;
        if (mi == mi_l && me0_l < me1_l) {
          sum += val_l.GetV(me0_l++, true);
          ++num;
        }
        if (mi == mi_r && me0_r < me1_r) {
          sum += val_r.GetV(me0_r++, true);
          ++num;
        }
        if (0 == num) {
          break;
        }
        mean.dbl = sum / num;
        m_value.Push(mi, mean);
      }
      if (mi == mi_l) {
        ++i_l;
      }
      if (mi == mi_r) {
        ++i_r;
      }
    }
  }
}
