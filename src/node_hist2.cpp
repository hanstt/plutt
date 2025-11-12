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
    *a_x, NodeValue *a_y, uint32_t a_xb, uint32_t a_yb, LinearTransform const
    &a_transformx, LinearTransform const &a_transformy, bool a_log_z, double
    a_drop_counts_s, unsigned a_drop_counts_num, double a_drop_stats_s, double
    a_single, bool a_permutate):
  NodeCuttable(a_loc, a_title),
  m_x(a_x),
  m_y(a_y),
  m_xb(a_xb),
  m_yb(a_yb),
  m_visual_hist2(a_title, m_xb, m_yb, a_transformx, a_transformy, a_log_z,
      a_drop_counts_s, a_drop_counts_num, a_drop_stats_s, a_single),
  m_out_x(),
  m_out_y(),
  m_permutate(a_permutate)
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
  NODE_PROCESS(m_x, a_evid);

  if (!m_visual_hist2.IsWritable()) {
    return;
  }

  auto const &val_x = m_x->GetValue();
  auto const &vec_x = val_x.GetV();

  if (!m_y) {
    // Plot x.v vs x.I.
    auto const &vmi = val_x.GetID();
    auto const &vme = val_x.GetEnd();
    // Pre-fill.
    uint32_t vi = 0;
    for (uint32_t i = 0; i < vmi.size(); ++i) {
      Input::Scalar x;
      x.u64 = vmi[i];
      auto me = vme[i];
      for (; vi < me; ++vi) {
        Input::Scalar const &y = vec_x.at(vi);
        m_cut_producer.Test(Input::kUint64, x, val_x.GetType(), y);
        m_visual_hist2.Prefill(Input::kUint64, x, val_x.GetType(), y);
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
        Input::Scalar const &y = vec_x.at(vi);
        m_visual_hist2.Fill(Input::kUint64, x, val_x.GetType(), y);
      }
    }
  } else {
    // Plot y.v vs x.v until either is exhausted, or plot plot all
    // combinations if m_permutate is set.
    NODE_PROCESS(m_y, a_evid);
    auto const &val_y = m_y->GetValue();
    auto const &vec_y = val_y.GetV();

    auto size_min = std::min(vec_x.size(), vec_y.size());

    // Pre-fill.
    if (m_permutate) {
      for (auto ity = vec_y.begin(); vec_y.end() != ity; ++ity) {
        auto const &y = *ity;
        for (auto itx = vec_x.begin(); vec_x.end() != itx; ++itx) {
          auto const &x = *itx;
          m_cut_producer.Test(val_x.GetType(), x, val_y.GetType(), y);
          m_visual_hist2.Prefill(val_x.GetType(), x, val_y.GetType(), y);
        }
      }
    } else {
      for (uint32_t i = 0; i < size_min; ++i) {
        auto const &x = vec_x.at(i);
        auto const &y = vec_y.at(i);
        m_cut_producer.Test(val_x.GetType(), x, val_y.GetType(), y);
        m_visual_hist2.Prefill(val_x.GetType(), x, val_y.GetType(), y);
      }
    }

    m_visual_hist2.Fit();

    // Fill.
    if (m_permutate) {
      for (uint32_t i = 0; i < vec_y.size(); ++i) {
        auto const &y = vec_y.at(i);
        for (uint32_t j = 0; j < vec_x.size(); ++j) {
          auto const &x = vec_x.at(j);
          if (g_output) {
            g_output->Fill(m_out_x, val_x.GetV(i, true));
            g_output->Fill(m_out_y, val_y.GetV(i, true));
          }
          m_visual_hist2.Fill(val_x.GetType(), x, val_y.GetType(), y);
        }
      }
    } else {
      for (uint32_t i = 0; i < size_min; ++i) {
        auto const &x = vec_x.at(i);
        auto const &y = vec_y.at(i);
        if (g_output) {
          g_output->Fill(m_out_x, val_x.GetV(i, true));
          g_output->Fill(m_out_y, val_y.GetV(i, true));
        }
        m_visual_hist2.Fill(val_x.GetType(), x, val_y.GetType(), y);
      }
    }
  }
}
