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

#ifndef CAL_HPP
#define CAL_HPP

/*
 * Rolling on-the-fly fine-time calibrator, gives times in [0,1).
 *
 * Calibrations are double-buffered, so in the beginning the fine-time is
 * slowly building, and eventually old calibrations are discarded.
 */
class CalFineTime {
  public:
    CalFineTime();
    double Get(uint32_t);

  private:
    void Calib();
    uint32_t m_counter;

    struct Span {
      Span();
      uint32_t sum;
      uint32_t min, max;
      std::vector<uint32_t> hist;
    };
    std::vector<Span> m_span_vec;
    size_t m_span_i;
    uint32_t m_acc_max, m_acc_sum;
    std::vector<uint32_t> m_acc;
};

#endif
