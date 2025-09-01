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
  m_counter(),
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

  // Pick span, swap if we've reached ~10%.
  Span *span = &m_span_vec[m_span_i];
  if (span->sum > 1000 &&
      span->sum > 100 * span->max) {
    Calib();
    m_span_i ^= 1;
    span = &m_span_vec[m_span_i];
    span->sum = 0;
    span->min = 1;
    span->max = 0;
    // Keep vector buffer, just zero it.
    std::fill(span->hist.begin(), span->hist.end(), 0);
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
  ++m_counter;
  if (m_counter >= 100000) {
    // Revert to a calib state.
    m_counter = 10000;
  }

  if (10 == m_counter ||
      100 == m_counter ||
      1000 == m_counter ||
      10000 == m_counter) {
    Calib();
  }

  // Convert.
  if (m_acc.empty()) {
    return 0.0;
  }
  a_i = std::min(a_i, m_acc_max);
  return (double)m_acc.at(a_i) / m_acc_sum;
}
