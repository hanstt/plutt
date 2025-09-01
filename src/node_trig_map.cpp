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
#include <node_trig_map.hpp>

NodeTrigMap::NodeTrigMap(std::string const &a_loc, TrigMap::Prefix const
    *a_prefix, NodeValue *a_sig, NodeValue *a_trig, double a_range):
  NodeValue(a_loc),
  m_prefix(a_prefix),
  m_sig(a_sig),
  m_trig(a_trig),
  m_range(a_range),
  m_value()
{
}

Value const &NodeTrigMap::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeTrigMap::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_sig, a_evid);
  NODE_PROCESS(m_trig, a_evid);

  m_value.Clear();
  m_value.SetType(Input::kDouble);

  // Build trigger lookup vector.
  std::vector<double> trig_vec;
  auto const &val_trig = m_trig->GetValue();
  auto const &vmi_trig = val_trig.GetID();
  uint32_t vi = 0;
  for (uint32_t i = 0; i < vmi_trig.size(); ++i) {
    uint32_t mi = vmi_trig.at(i);
    if (mi >= trig_vec.size()) {
      trig_vec.resize(mi + 1);
    }
    trig_vec.at(mi) = val_trig.GetV(vi, false);
    vi = val_trig.GetEnd().at(i);
  }

  auto const &val_sig = m_sig->GetValue();
  auto const &vmi_sig = val_sig.GetID();

  vi = 0;
  for (uint32_t i = 0; i < vmi_sig.size(); ++i) {
    auto mi = vmi_sig[i];
    auto me_sig = val_sig.GetEnd().at(i);
    for (; vi < me_sig; ++vi) {
      double sig = val_sig.GetV(vi, false);
      uint32_t trig_i;
      if (m_prefix->GetTrig(mi, &trig_i) &&
          trig_i < trig_vec.size()) {
        double trig = trig_vec.at(trig_i);
        Input::Scalar diff;
        diff.dbl = SubModDbl(sig, trig, m_range);
        m_value.Push(mi, diff);
      }
    }
  }
}
