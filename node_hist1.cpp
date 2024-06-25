/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023  Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
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
#include <string>
#include <vector>
#include <util.hpp>
#include <node_hist1.hpp>

NodeHist1::NodeHist1(std::string const &a_loc, char const *a_title, NodeValue
    *a_x, uint32_t a_xb, LinearTransform const &a_transform, char const
    *a_fit, bool a_log_y, double a_drop_stats_s):
  NodeCuttable(a_loc, a_title),
  m_x(a_x),
  m_xb(a_xb),
  m_visual_hist(a_title, m_xb, a_transform, a_fit, a_log_y, a_drop_stats_s)
{
}

void NodeHist1::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  m_cut_consumer.Process(a_evid);
  if (!m_cut_consumer.IsOk()) {
    return;
  }
  NODE_PROCESS(m_x, a_evid);

  auto const &val_x = m_x->GetValue();
  auto const &v = val_x.GetV();

  // Pre-fill.
  for (uint32_t i = 0; i < v.size(); ++i) {
    auto const &x = v.at(i);
    m_cut_producer.Test(val_x.GetType(), x);
    m_visual_hist.Prefill(val_x.GetType(), x);
  }
  m_visual_hist.Fit();
  // Fill.
  for (uint32_t i = 0; i < v.size(); ++i) {
    auto const &x = v.at(i);
    m_visual_hist.Fill(val_x.GetType(), x);
  }
}
