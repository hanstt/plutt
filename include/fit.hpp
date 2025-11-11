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

#ifndef FIT_HPP
#define FIT_HPP

struct PeakFitEntry {
  PeakFitEntry(std::string const &a_name, double a_l, double a_r):
    name(a_name),
    l(a_l),
    r(a_r)
  {}
  std::string name;
  double l;
  double r;
};
typedef std::vector<PeakFitEntry> PeakFitVec;

/*
 * Fitting y-offset functions against vector range [left_i, right_i].
 * Since this online tool ranks reasonable visualization higher than correct
 * decimals, the curve peak is constrained with max_y. This value should be
 * the max in the histogram, and will after fitting be close to GetOfs() +
 * GetAmp().
 */

class FitExpGauss {
  public:
    FitExpGauss(std::vector<uint32_t> const &, double, uint32_t, uint32_t);
    ~FitExpGauss();
    double GetY() const;
    double GetExpPhase() const;
    double GetExpTau() const;
    double GetGaussAmp() const;
    double GetGaussMean() const;
    double GetGaussStd() const;

  private:
    double m_y;
    double m_phase;
    double m_tau;
    double m_amp;
    double m_mean;
    double m_std;
};

class FitGauss {
  public:
    FitGauss(std::vector<uint32_t> const &, double, uint32_t, uint32_t);
    ~FitGauss();
    double GetY() const;
    double GetAmp() const;
    double GetMean() const;
    double GetStd() const;

  private:
    double m_y;
    double m_amp;
    double m_mean;
    double m_std;
};

#endif
