/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023, 2024
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
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <fit.hpp>
#include <util.hpp>
#include <visual.hpp>

#define LENGTH(x) (sizeof x / sizeof *x)

extern GuiCollection g_gui;

Range::Range(double a_drop_old_s):
  m_mode(MODE_ALL),
  m_type(Input::kNone),
  m_drop_old_ms(a_drop_old_s < 0 ? 0 :
      (uint64_t)(1000 * a_drop_old_s / LENGTH(m_stat))),
  m_stat(),
  m_stat_i()
{
}

void Range::Add(Input::Type a_type, Input::Scalar const &a_v)
{
  if (Input::kNone == m_type) {
    m_type = a_type;
  } else if (m_type != a_type) {
    std::cerr << "Histogrammed signal cannot change type!\n";
    throw std::runtime_error(__func__);
  }

  auto &s = m_stat[m_stat_i];

  auto v = a_v.GetDouble(a_type);
  if (0 == s.num) {
    s.max = s.min = v;
  } else {
    s.min = std::min(s.min, v);
    s.max = std::max(s.max, v);
  }

  s.sum += v;
  s.sum2 += v * v;

  auto t_cur = Time_get_ms();
  if (0 == s.num || 0 == s.t_oldest) {
    s.t_oldest = t_cur;
  }

  ++s.num;
  if (m_drop_old_ms > 0 &&
      s.t_oldest + m_drop_old_ms < t_cur) {
    m_stat_i = (m_stat_i + 1) % LENGTH(m_stat);
    auto &s2 = m_stat[m_stat_i];
    s2.sum = 0.0;
    s2.sum2 = 0.0;
    s2.num = 0;
    s2.t_oldest = 0;
  }
}

void Range::Clear()
{
  for (size_t i = 0; i < LENGTH(m_stat); ++i) {
    auto &s = m_stat[i];
    s.min = 0.0;
    s.max = 0.0;
    s.sum = 0.0;
    s.sum2 = 0.0;
    s.num = 0;
    s.t_oldest = 0;
  }
  m_stat_i = 0;
}

Gui::Axis Range::GetExtents(uint32_t a_bins) const
{
  double l, r;
  switch (m_mode) {
    case MODE_ALL:
      {
        l = GetMin();
        r = GetMax();
        if (Input::kUint64 == m_type) {
          // For integers, 'r' is on the right side of max.
          ++r;
        }
        auto d = r - l;
        if (std::abs(d) < 1e-10) {
          // This can happen if:
          // -) We're looking at small numbers -> don't go too small.
          // -) We're looking at huge numbers and diffs cancel out ->
          //    pick some relative range based on 'l'.
          d = std::max(std::abs(l) * 1e-10, 1e-20);
        }
        // If we're looking at floats or huge integers, add some margin.
        if (Input::kUint64 != m_type || d > (1 << 16)) {
          l -= d * 0.1;
          r += d * 0.1;
        }
      }
      break;
    case MODE_STATS:
      {
        // Full range.
        l = GetMin();
        r = GetMax();
        auto d_ext = r - l;

        // Suggest extent from "peak".
        auto mean = GetMean();
        auto sigma = GetSigma();
        auto peak_l = mean - 3 * sigma;
        auto peak_r = mean + 3 * sigma;
        auto d_peak = peak_r - peak_l;

        if (d_peak < 0.1 * d_ext) {
          // We seem to have a narrow peak inside the range, take it.
          l = peak_l;
          r = peak_r;
        }

        // Snap to 0 if we're close.
        auto d = r - l;
        if (l > 0.0 && l < d) {
          l = 0.0;
        }
        if (r < 0.0 && r > -d) {
          r = 0.0;
        }
      }
      break;
    default:
      throw std::runtime_error(__func__);
  }

  assert(l != r);

  // Choose bins, which may fudge range.
  uint32_t bins;
  if (Input::kDouble == m_type) {
    bins = a_bins > 0 ? a_bins : 200;
  } else {
    if (a_bins > 0) {
      bins = a_bins;
      auto d = r - l;
      auto f = (uint32_t)ceil(d / bins);
      l /= f;
      r /= f;
      auto center = (l + r) / 2;
      l = center - bins / 2;
      r = l + bins;
      l *= f;
      r *= f;
    } else {
      for (bins = (uint32_t)ceil(r - l); bins > 128; bins /= 2);
    }
  }

  Gui::Axis a;
  a.bins = bins;
  a.min = l;
  a.max = r;
  return a;
}

double Range::GetMax() const
{
  double max = 0.0;
  bool is_touched = false;
  for (uint32_t i = 0; i < LENGTH(m_stat); ++i) {
    if (m_stat[i].num) {
      if (!is_touched) {
        max = m_stat[i].max;
        is_touched = true;
      } else {
        max = std::max(max, m_stat[i].max);
      }
    }
  }
  return max;
}

double Range::GetMean() const
{
  double sum = 0.0;
  uint32_t num = 0;
  for (uint32_t i = 0; i < LENGTH(m_stat); ++i) {
    if (m_stat[i].num) {
      auto &s = m_stat[i];
      sum += s.sum;
      num += s.num;
    }
  }
  return sum / num;
}

double Range::GetMin() const
{
  double min = 0.0;
  bool is_touched = false;
  for (uint32_t i = 0; i < LENGTH(m_stat); ++i) {
    if (m_stat[i].num) {
      if (!is_touched) {
        min = m_stat[i].min;
        is_touched = true;
      } else {
        min = std::min(min, m_stat[i].min);
      }
    }
  }
  return min;
}

double Range::GetSigma() const
{
  double sum = 0.0;
  double sum2 = 0.0;
  uint32_t num = 0;
  for (uint32_t i = 0; i < LENGTH(m_stat); ++i) {
    if (m_stat[i].num) {
      auto &s = m_stat[i];
      sum += s.sum;
      sum2 += s.sum2;
      num += s.num;
    }
  }
  return sqrt((sum2 - sum * sum / num) / num);
}

Visual::Visual(std::string const &a_name):
  m_name(a_name),
  m_gui_id(g_gui.AddPlot(m_name, this))
{
}

Visual::~Visual()
{
}

VisualHist::VisualHist(std::string const &a_title, uint32_t a_xb,
    LinearTransform const &a_transform, char const *a_fitter, bool a_is_log_y,
    double a_drop_old_s):
  Visual(a_title),
  m_xb(a_xb),
  m_transform(a_transform),
  m_fitter(),
  m_range(a_drop_old_s),
  m_axis(),
  m_hist_mutex(),
  m_hist(),
  m_axis_copy(),
  m_hist_copy(),
  m_is_log_y(a_is_log_y),
  m_peak_vec()
{
  if (!a_fitter) {
    m_fitter = FITTER_NONE;
  } else if (0 == strcmp(a_fitter, "gauss")) {
    m_fitter = FITTER_GAUSS;
  } else {
    std::cerr << a_fitter << ": Fitter not implemented.\n";
    throw std::runtime_error(__func__);
  }
}

void VisualHist::Draw(Gui *a_gui)
{
  if (m_hist_copy.empty()) {
    return;
  }

  // Fitting at 1 Hz should be fine?
  switch (m_fitter) {
    case FITTER_NONE:
      break;
    case FITTER_GAUSS:
      FitGauss(m_hist_copy, m_axis_copy);
      break;
    default:
      throw std::runtime_error(__func__);
  }

  g_gui.DrawHist1(a_gui, m_gui_id, m_axis_copy, m_transform, m_is_log_y,
      m_hist_copy, m_peak_vec);
}

void VisualHist::Fill(Input::Type a_type, Input::Scalar const &a_x)
{
  const std::lock_guard<std::mutex> lock(m_hist_mutex);

  // And the filling, at last.
  auto span = m_axis.max - m_axis.min;
  double dx;
  switch (a_type) {
    case Input::kUint64:
      dx = IntSubDouble(a_x.u64, m_axis.min);
      break;
    case Input::kInt64:
      dx = IntSubDouble(a_x.i64, m_axis.min);
      break;
    case Input::kDouble:
      dx = a_x.dbl - m_axis.min;
      break;
    case Input::kNone:
    default:
      throw std::runtime_error(__func__);
  }
  uint32_t i = (uint32_t)(m_axis.bins * dx / span);
  assert(i < m_axis.bins);
  ++m_hist.at(i);
}

void VisualHist::Fit()
{
  const std::lock_guard<std::mutex> lock(m_hist_mutex);

  if (m_range.GetMin() < m_axis.min || m_range.GetMax() >= m_axis.max) {
    auto axis = m_range.GetExtents(m_xb);
    if (m_axis.bins != axis.bins ||
        m_axis.min != axis.min ||
        m_axis.max != axis.max) {
      m_hist = Rebin1(m_hist,
          m_axis.bins, m_axis.min, m_axis.max,
          axis.bins, axis.min, axis.max);
      m_axis = axis;
    }
  }
}

void VisualHist::Latch()
{
  // The data thread will keep filling and modifying m_hist while the plotter
  // tries to figure out ranges, so a locked copy is important!
  const std::lock_guard<std::mutex> lock(m_hist_mutex);
  if (g_gui.DoClear(m_gui_id)) {
    // We should clear.
    // TODO: Clear all, or just histogram contents?
    m_range.Clear();
    m_axis.Clear();
    m_hist.clear();
  }
  m_axis_copy = m_axis;
  if (m_hist_copy.size() != m_hist.size()) {
    m_hist_copy.resize(m_hist.size());
  }
  memcpy(m_hist_copy.data(), m_hist.data(),
      m_hist.size() * sizeof m_hist[0]);
}

void VisualHist::Prefill(Input::Type a_type, Input::Scalar const &a_x)
{
  const std::lock_guard<std::mutex> lock(m_hist_mutex);

  m_range.Add(a_type, a_x);
}

// Fitters must work on given copy and not look at the ever-changing m_hist!
void VisualHist::FitGauss(std::vector<uint32_t> const &a_hist, Gui::Axis const
    &a_axis)
{
  m_peak_vec.clear();
  auto b = Snip(a_hist, 4);
  for (size_t i = 0; i < a_hist.size(); ++i) {
    b.at(i) = ((float)a_hist.at(i) - b.at(i)) / (float)sqrt(b.at(i) + 1);
  }
  // 2nd diffs make peaks look like:
  // ___/\  /\___
  //      \/
  std::vector<float> d_hist(a_hist.size());
  for (size_t i = 2; i < a_hist.size(); ++i) {
    auto f0 = (float)a_hist.at(i - 2);
    auto f1 = (float)a_hist.at(i - 1);
    auto f2 = (float)a_hist.at(i - 0);
    d_hist.at(i - 1) = (f2 - f1) - (f1 - f0);
  }
  // Mask for found peaks.
  std::vector<uint32_t> mask((a_hist.size() + 31) / 32);
  auto scale = (a_axis.max - a_axis.min) / (double)a_hist.size();
  for (unsigned n = 0; n < 30 && m_peak_vec.size() < 30; ++n) {
    // Find min 2nd diff.
    double max_y = 0;
    uint32_t max_i = 0;
    for (uint32_t i = 0; i < a_hist.size(); ++i) {
      auto u32_i = i / 32;
      auto bit_i = 1U << (i & 31);
      if (!(mask.at(u32_i) & bit_i)) {
        auto v = b.at(i);
        if (v > max_y) {
          max_y = v;
          max_i = i;
        }
      }
    }
    max_y = a_hist.at(max_i);
    // Step sideways in 2nd-diffs to find shoulders, ie the dots below:
    //    .    .
    // ___/\  /\___
    //      \/
    auto min_y = -1e9f;
    auto left_i = max_i;
    auto prev = min_y;
    for (;;) {
      if (left_i < 1) break;
      --left_i;
      auto d = d_hist.at(left_i);
      if (d < prev) break;
      prev = d;
    }
    auto right_i = max_i;
    prev = min_y;
    for (;;) {
      if (right_i + 1 >= d_hist.size()) break;
      ++right_i;
      auto d = d_hist.at(right_i);
      if (d < prev) break;
      prev = d;
    }
    try {
      // Fit left..right in histo.
      ::FitGauss fit(a_hist, max_y, left_i, right_i);
      // Shift by 0.5 because fit was done on left edges of bins.
      auto mean = fit.GetMean() + 0.5;
      // Mask range.
      auto width = std::max(fit.GetStd(), 1.0);
      auto left = floor(mean - 3 * width);
      left_i = (uint32_t)std::max(left, 0.0);
      auto right = ceil(mean + 3 * width);
      right_i = (uint32_t)std::min(right, (double)a_hist.size() - 1);
      if (fit.GetAmp() > 0) {
        // Reasonable fit, save peak.
        auto mean_x = a_axis.min + mean * scale;
        auto std_x = fit.GetStd() * scale;
        auto ofs = fit.GetOfs();
        m_peak_vec.push_back(Gui::Peak(mean_x, ofs, fit.GetAmp(), std_x));
      }
    } catch (...) {
    }
    for (uint32_t i = left_i; i <= right_i; ++i) {
      auto u32_i = i / 32;
      auto bit_i = 1U << (i & 31);
      mask.at(u32_i) |= bit_i;
    }
  }
}

VisualHist2::VisualHist2(std::string const &a_title, size_t a_colormap,
    uint32_t a_yb, uint32_t a_xb, LinearTransform const &a_ty, LinearTransform
    const &a_tx, char const *a_fitter, bool a_is_log_z, double a_drop_old_s):
  Visual(a_title),
  //m_colormap(a_colormap),
  m_xb(a_xb),
  m_yb(a_yb),
  m_transform_x(a_tx),
  m_transform_y(a_ty),
  m_range_x(a_drop_old_s),
  m_range_y(a_drop_old_s),
  m_axis_x(),
  m_axis_y(),
  m_hist_mutex(),
  m_hist(),
  m_axis_x_copy(),
  m_axis_y_copy(),
  m_hist_copy(),
  m_is_log_z(a_is_log_z),
  m_pixels()
{
}

void VisualHist2::Draw(Gui *a_gui)
{
  if (m_hist_copy.empty()) {
    return;
  }
  g_gui.DrawHist2(a_gui, m_gui_id, m_axis_x_copy, m_axis_y_copy,
      m_transform_x, m_transform_y, m_is_log_z, m_hist_copy);
}

void VisualHist2::Fill(Input::Type a_type_y, Input::Scalar const &a_y,
    Input::Type a_type_x, Input::Scalar const &a_x)
{
  const std::lock_guard<std::mutex> lock(m_hist_mutex);

  // Fill.
  auto span_x = m_axis_x.max - m_axis_x.min;
  auto span_y = m_axis_y.max - m_axis_y.min;
  double dx;
  switch (a_type_x) {
    case Input::kUint64:
      dx = IntSubDouble(a_x.u64, m_axis_x.min);
      break;
    case Input::kInt64:
      dx = IntSubDouble(a_x.i64, m_axis_x.min);
      break;
    case Input::kDouble:
      dx = a_x.dbl - m_axis_x.min;
      break;
    case Input::kNone:
    default:
      throw std::runtime_error(__func__);
  }
  double dy;
  switch (a_type_y) {
    case Input::kUint64:
      dy = IntSubDouble(a_y.u64, m_axis_y.min);
      break;
    case Input::kInt64:
      dy = IntSubDouble(a_y.i64, m_axis_y.min);
      break;
    case Input::kDouble:
      dy = a_y.dbl - m_axis_y.min;
      break;
    case Input::kNone:
    default:
      throw std::runtime_error(__func__);
  }
  uint32_t j = (uint32_t)(m_axis_x.bins * dx / span_x);
  uint32_t i = (uint32_t)(m_axis_y.bins * dy / span_y);
  assert(i < m_axis_y.bins);
  assert(j < m_axis_x.bins);
  ++m_hist.at(i * m_axis_x.bins + j);
}

void VisualHist2::Fit()
{
  const std::lock_guard<std::mutex> lock(m_hist_mutex);

  if (m_range_x.GetMin() < m_axis_x.min ||
      m_range_x.GetMax() >= m_axis_x.max ||
      m_range_y.GetMin() < m_axis_y.min ||
      m_range_y.GetMax() >= m_axis_y.max) {
    auto axis_x = m_range_x.GetExtents(m_xb);
    auto axis_y = m_range_y.GetExtents(m_yb);
    if (m_axis_x.bins != axis_x.bins ||
        m_axis_x.min != axis_x.min ||
        m_axis_x.max != axis_x.max ||
        m_axis_y.bins != axis_y.bins ||
        m_axis_y.min != axis_y.min ||
        m_axis_y.max != axis_y.max) {
      m_hist = Rebin2(m_hist,
          m_axis_x.bins, m_axis_x.min, m_axis_x.max,
          m_axis_y.bins, m_axis_y.min, m_axis_y.max,
          axis_x.bins, axis_x.min, axis_x.max,
          axis_y.bins, axis_y.min, axis_y.max);
      m_axis_x = axis_x;
      m_axis_y = axis_y;
    }
  }
}

void VisualHist2::Latch()
{
  const std::lock_guard<std::mutex> lock(m_hist_mutex);
  if (g_gui.DoClear(m_gui_id)) {
    m_range_x.Clear();
    m_range_y.Clear();
    m_axis_x.Clear();
    m_axis_y.Clear();
    m_hist.clear();
  }
  m_axis_x_copy = m_axis_x;
  m_axis_y_copy = m_axis_y;
  if (m_hist_copy.size() != m_hist.size()) {
    m_hist_copy.resize(m_hist.size());
  }
  memcpy(m_hist_copy.data(), m_hist.data(),
      m_hist.size() * sizeof m_hist[0]);
}

void VisualHist2::Prefill(Input::Type a_type_y, Input::Scalar const &a_y,
    Input::Type a_type_x, Input::Scalar const &a_x)
{
  const std::lock_guard<std::mutex> lock(m_hist_mutex);

  m_range_x.Add(a_type_x, a_x);
  m_range_y.Add(a_type_y, a_y);
}
