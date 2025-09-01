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
#include <cmath>
#include <set>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_cluster.hpp>

NodeCluster::NodeCluster(std::string const &a_loc, NodeValue *a_child):
  NodeValue(a_loc),
  m_child(a_child),
  m_clu(),
  m_eta()
{
  m_clu.SetType(Input::kDouble);
  m_eta.SetType(Input::kDouble);
}

Value const &NodeCluster::GetValue(uint32_t a_ret_i)
{
  switch (a_ret_i) {
    case 0:
      return m_clu;
    case 1:
      return m_eta;
    default:
      throw std::runtime_error(__func__);
  }
}

void NodeCluster::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_child, a_evid);

  m_clu.Clear();
  m_eta.Clear();

  auto const &val = m_child->GetValue();
  auto const &miv = val.GetID();
  auto const &mev = val.GetEnd();
  auto const &v = val.GetV();
  if (miv.empty()) {
    return;
  }

  // Keep temp set so the output gets sorted.
  struct Entry {
    double x;
    double e;
  };
  auto cmp = [](Entry const &a_l, Entry const &a_r) {
    return a_l.e > a_r.e;
  };
  std::set<Entry, decltype(cmp)> sorted_set(cmp);
  uint32_t mi_prev = UINT32_MAX / 2;
  // The next few will be inited, but not all compilers figure that out.
  double sum_x = 0.0;
  double sum_e = 0.0;
  uint32_t v_i = 0;
  for (uint32_t i = 0; i < miv.size(); ++i) {
    auto mi = miv.at(i);
    if (mi_prev + 1 != mi) {
      if (sum_e > 0) {
        // Create previous cluster.
        Entry entry;
        entry.x = sum_x / sum_e;
        entry.e = sum_e;
        sorted_set.insert(entry);
      }
      sum_x = 0.0;
      sum_e = 0.0;
    }
    // Keep clusterizing.
    auto vv = v.at(v_i).GetDouble(val.GetType());
    sum_x += mi * vv;
    sum_e += vv;
    v_i = mev.at(i);
    mi_prev = mi;
  }
  if (sum_e > 0) {
    // Create remaining cluster.
    Entry entry;
    entry.x = sum_x / sum_e;
    entry.e = sum_e;
    sorted_set.insert(entry);
  }

  // Move sorted set to value.
  for (auto it = sorted_set.begin(); sorted_set.end() != it; ++it) {
    auto mi = (uint32_t)it->x;
    Input::Scalar s;
    s.dbl = it->e;
    m_clu.Push(mi, s);
    s.dbl = it->x - floor(it->x);
    m_eta.Push(mi, s);
  }
}
