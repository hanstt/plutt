/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2025
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
#include <string>
#include <vector>
#include <util.hpp>
#include <node_hist2.hpp>

extern Output *g_output;

NodeHist2::NodeHist2(std::string const &a_loc, char const *a_title, NodeValue
    *a_y, NodeValue *a_x, uint32_t a_yb, uint32_t a_xb, LinearTransform const
    &a_transformy, LinearTransform const &a_transformx, char const *a_fit,
    bool a_log_z, double a_drop_counts_s, unsigned a_drop_counts_num, double
    a_drop_stats_s):
  NodeCuttable(a_loc, a_title),
  m_x(a_x),
  m_y(a_y),
  m_xb(a_xb),
  m_yb(a_yb),
  m_visual_hist2(a_title, m_yb, m_xb, a_transformy, a_transformx, a_fit,
      a_log_z, a_drop_counts_s, a_drop_counts_num, a_drop_stats_s),
  m_out_x(),
  m_out_y()
{
  if (g_output) {
    g_output->Add(&m_out_x, std::string(a_title) + "_x");
    g_output->Add(&m_out_y, std::string(a_title) + "_y");
  }
}

void NodeHist2::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  m_cut_consumer.Process(a_evid);
  if (!m_cut_consumer.IsOk()) {
    return;
  }
  NODE_PROCESS(m_y, a_evid);

  auto const &val_y = m_y->GetValue();
  auto const &vec_y = val_y.GetV();

  if (!m_x) {
    // Plot y.v vs y.I.
    auto const &vmi = val_y.GetID();
    auto const &vme = val_y.GetEnd();
    // Pre-fill.
    uint32_t vi = 0;
    for (uint32_t i = 0; i < vmi.size(); ++i) {
      Input::Scalar x;
      x.u64 = vmi[i];
      auto me = vme[i];
      for (; vi < me; ++vi) {
        Input::Scalar const &y = vec_y.at(vi);
        m_cut_producer.Test(Input::kUint64, x, val_y.GetType(), y);
        m_visual_hist2.Prefill(val_y.GetType(), y, Input::kUint64, x);
      }
    }
    m_visual_hist2.Fit();
    // Fill.
    vi = 0;
    for (uint32_t i = 0; i < vmi.size(); ++i) {
      Input::Scalar x;
      x.u64 = vmi[i];
      auto me = vme[i];
      for (; vi < me; ++vi) {
        Input::Scalar const &y = vec_y.at(vi);
        m_visual_hist2.Fill(val_y.GetType(), y, Input::kUint64, x);
      }
    }
  } else {
    // Plot y.v vs x.v until either is exhausted.
    NODE_PROCESS(m_x, a_evid);
    auto const &val_x = m_x->GetValue();
    auto const &vec_x = val_x.GetV();

    auto size = std::min(vec_x.size(), vec_y.size());

    // Pre-fill.
    for (uint32_t i = 0; i < size; ++i) {
      auto const &x = vec_x.at(i);
      auto const &y = vec_y.at(i);
      m_cut_producer.Test(val_x.GetType(), x, val_y.GetType(), y);
      m_visual_hist2.Prefill(val_y.GetType(), y, val_x.GetType(), x);
    }
    m_visual_hist2.Fit();
    // Fill.
    for (uint32_t i = 0; i < size; ++i) {
      auto const &x = vec_x.at(i);
      auto const &y = vec_y.at(i);
      if (g_output) {
        g_output->Fill(m_out_x, val_x.GetV(i, true));
        g_output->Fill(m_out_y, val_y.GetV(i, true));
      }
      m_visual_hist2.Fill(val_y.GetType(), y, val_x.GetType(), x);
    }
  }
}
