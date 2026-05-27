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

#include <cstddef>
#include <cstdint>
#include <vector>
#include <cal.hpp>

CalFineTime::Span::Span():
  sum(),
  min(1),
  max(),
  hist()
{
}

CalFineTime::CalFineTime():
  m_recal_at(10),
  m_span_vec(2),
  m_span_i(),
  m_acc_max(),
  m_acc_sum(),
  m_acc()
{
}

void CalFineTime::Calib()
{
  auto &span = m_span_vec[m_span_i];
  m_acc_max = span.max;
  if (m_acc_max >= m_acc.size()) {
    m_acc.resize(m_acc_max + 1);
  }
  m_acc_sum = 0;
  for (size_t i = 0; i <= m_acc_max; ++i) {
    m_acc[i] = m_acc_sum;
    m_acc_sum += span.hist[i];
  }
}

double CalFineTime::Get(uint32_t a_i)
{
  if (0x3ff == a_i) {
    // Magical Tamex fine time...
    return 0.0;
  }

  // Pick span
  Span *span = &m_span_vec[m_span_i];
  if (span->sum >= m_recal_at) {
    // Initially calibration happens after accumulating 10, 20, 40,
    // 80, ... counts.  After that, calibration happens based on
    // actual span width.
    Calib();
    // Estimate required statistics from span width.  Considering the
    // calibration as a counting problem from the fixed edges to the
    // adjustable middle, uncertainty is proportional to sqrt(counts).
    // Here ignoring reducing factors of two that come from only
    // having to count half the way, and counting both ways.
    uint32_t span_width = span->max - span->min + 1;
    uint32_t want_stats = span_width * span_width;
    // Swap (i.e. restart collection) if when at least 7/8 of the
    // wanted statistics has been reached.  7/8 in condition such that
    // we most likely swap span, since we end up here when full
    // requirement (m_recal_at) based on last estimate is fulfilled
    // (which may have seen a slightly smaller total span).
    // First swap will happen at 7/8 of estimated need.
    if (span->sum > 1000 &&
	span->sum > (7 * want_stats) / 8) {
      // Next recalibration estimated from current span width.
      m_recal_at = want_stats;
      // Swap collection.
      m_span_i ^= 1;
      span = &m_span_vec[m_span_i];
      span->sum = 0;
      span->min = 1;
      span->max = 0;
      // Keep vector buffer, just zero it.
      std::fill(span->hist.begin(), span->hist.end(), 0);
    } else {
      m_recal_at *= 2;
    }
  }

  // Add value to hist.
  if (span->min > span->max) {
    span->max = span->min = a_i;
  } else {
    span->min = std::min(span->min, a_i);
    span->max = std::max(span->max, a_i);
  }
  if (a_i >= span->hist.size()) {
    span->hist.resize(a_i + 1);
  }
  ++span->hist[a_i];
  ++span->sum;

  // Convert.
  if (m_acc.empty()) {
    return 0.0;
  }
  a_i = std::min(a_i, m_acc_max);
  return (double)m_acc.at(a_i) / m_acc_sum;
}
