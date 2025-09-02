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
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <util.hpp>
#include <node_annular.hpp>

extern Output *g_output;

NodeAnnular::NodeAnnular(std::string const &a_loc, char const *a_title,
    NodeValue *a_r, double a_r_min, double a_r_max, NodeValue *a_phi, double
    a_phi0, bool a_log_z, double a_drop_counts_s, unsigned a_drop_counts_num,
    double a_drop_stats_s):
  NodeCuttable(a_loc, a_title),
  m_r(a_r),
  m_phi(a_phi),
  m_visual_annular(a_title, a_r_min, a_r_max, a_phi0, a_log_z,
      a_drop_counts_s, a_drop_counts_num, a_drop_stats_s),
  m_out_r(),
  m_out_p()
{
  if (g_output) {
    g_output->Add(&m_out_r, std::string(a_title) + "_r");
    g_output->Add(&m_out_p, std::string(a_title) + "_p");
  }
}

void NodeAnnular::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  m_cut_consumer.Process(a_evid);
  if (!m_cut_consumer.IsOk()) {
    return;
  }
  NODE_PROCESS(m_r, a_evid);
  NODE_PROCESS(m_phi, a_evid);

  auto const &val_r = m_r->GetValue();
  auto const &vec_r = val_r.GetV();

  auto const &val_phi = m_phi->GetValue();
  auto const &vec_phi = val_phi.GetV();

  // Plot phi.v vs r.v until either is exhausted.

  auto size = std::min(vec_r.size(), vec_phi.size());

  // Pre-fill.
  for (uint32_t i = 0; i < size; ++i) {
    auto const &r = vec_r.at(i);
    auto const &phi = vec_phi.at(i);
    m_visual_annular.Prefill(val_r.GetType(), r, val_phi.GetType(), phi);
  }
  m_visual_annular.Fit();
  // Fill.
  for (uint32_t i = 0; i < size; ++i) {
    auto const &r = vec_r.at(i);
    auto const &phi = vec_phi.at(i);
    if (g_output) {
      g_output->Fill(m_out_r, val_r.GetV(i, true));
      g_output->Fill(m_out_p, val_phi.GetV(i, true));
    }
    m_visual_annular.Fill(val_r.GetType(), r, val_phi.GetType(), phi);
  }
}
