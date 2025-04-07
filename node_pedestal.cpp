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
#include <util.hpp>
#include <node_tpat.hpp>
#include <node_pedestal.hpp>

#define STATS_MAX 10000

void NodePedestal::StatPage::Add(double a_v)
{
  // Welford's online algorithm.
  ++num;
  auto delta = a_v - mean;
  mean += delta / num;
  auto delta2 = a_v - mean;
  M2 += delta * delta2;
  // We can spend some time in here, because we should be collecting
  // pedestals off-spill or on special triggers where less happens in
  // other detectors.
  if (num > 1) {
    var = M2 / (num - 1);
  }
}

void NodePedestal::Stats::Get(double *a_mean, double *a_var)
{
  uint32_t num = 0;
  double mean_sum = 0.0;
  double var_sum = 0.0;
  for (size_t j = 0; j < LENGTH(page); ++j) {
    auto const &p = page[j];
    num += p.num > 0;
    mean_sum += p.mean;
    var_sum += p.var;
  }
  num += 0 == num;
  *a_mean = mean_sum / num;
  *a_var = var_sum / num;
}

NodePedestal::NodePedestal(std::string const &a_loc, NodeValue *a_child,
    double a_cutoff, NodeValue *a_tpat):
  NodeValue(a_loc),
  m_child(a_child),
  m_cutoff(a_cutoff),
  m_tpat(a_tpat),
  m_value(),
  m_sigma(),
  m_stats()
{
  m_value.SetType(Input::kDouble);
  m_sigma.SetType(Input::kDouble);
}

Value const &NodePedestal::GetValue(uint32_t a_ret_i)
{
  switch (a_ret_i) {
    case 0:
      return m_value;
    case 1:
      return m_sigma;
    default:
      throw std::runtime_error(__func__);
  }
}

void NodePedestal::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_child, a_evid);

  bool do_accounting = true;
  if (m_tpat) {
    NODE_PROCESS(m_tpat, a_evid);
    if (!NodeTpat::Test(this, m_tpat)) {
      do_accounting = false;
    }
  }

  m_value.Clear();
  m_sigma.Clear();

  auto const &val = m_child->GetValue();
  auto const &vmi = val.GetID();
  auto const &vme = val.GetEnd();

  if (!vmi.empty()) {
    // Try to size stats by looking at the last index.
    StatsFit(vmi.back());
  }
  uint32_t v_i = 0;
  for (uint32_t i = 0; i < vmi.size(); ++i) {
    auto const mi = vmi[i];
    auto const me = vme[i];
    StatsFit(mi);
    auto &s = m_stats.at(mi);
    auto &s_write = s.page[s.write_i];
    for (; v_i < me; ++v_i) {
      auto v = val.GetV(v_i, true);
      if (do_accounting) {
        s_write.Add(v);
      }
      double mean, var;
      s.Get(&mean, &var);
      if (var > 0) {
        Input::Scalar scl;
        auto std = sqrt(var);
        auto e = v - mean;
        if (e > m_cutoff * std) {
          scl.dbl = e;
          m_value.Push(mi, scl);
        }
        scl.dbl = std;
        m_sigma.Push(mi, scl);
      }
    }
    if (STATS_MAX <= s_write.num) {
      s.write_i ^= 1;
      auto &s_new = s.page[s.write_i];
      s_new.num = 0;
      s_new.mean = 0.0;
      s_new.M2 = 0.0;
      s_new.var = 0.0;
    }
  }
}

void NodePedestal::StatsFit(uint32_t a_mi)
{
  if (a_mi >= m_stats.size()) {
    m_stats.resize(a_mi + 1);
  }
}
