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

#ifndef VISUAL_HPP
#define VISUAL_HPP

#include <gui.hpp>
#include <input.hpp>

/*
 * Visual representations of data, eg histograms.
 * Not really visual until they're drawn, just a bunch of numbers, but
 * 'visual' is a nice short word...
 */

/*
 * Tries to guess the kind of data and its range, eg:
 *  -) Only integers? Limit # bins to integer multiples.
 *  -) Is there a huge peak? Forget about the tails.
 */
class Range {
  public:
    enum Mode {
      MODE_ALL,
      MODE_STATS
    };
    Range(double);
    void Add(Input::Type, Input::Scalar const &);
    void Clear();
    Gui::Axis GetExtents(uint32_t) const;
    double GetMax() const;
    double GetMean() const;
    double GetMin() const;
    double GetSigma() const;
    bool IsAdded() const;
    void SetMode(Mode);
  private:
    Mode m_mode;
    Input::Type m_type;
    uint64_t m_drop_old_ms;
    struct {
      double min;
      double max;
      double sum;
      double sum2;
      uint32_t num;
      uint64_t t_oldest;
    } m_stat[10];
    size_t m_stat_i;
};

class Visual: public Gui::Plot {
  public:
    Visual(std::string const &);
    virtual ~Visual();
    virtual void Draw(Gui *) = 0;
    virtual void Latch() = 0;

    std::string m_name;
    uint32_t m_gui_id;
};

class VisualHist: public Visual {
  public:
    enum Fitter {
      FITTER_NONE,
      FITTER_GAUSS
    };

    VisualHist(std::string const &, uint32_t, LinearTransform const &, char
        const *, bool, double);
    void Draw(Gui *);
    void Fill(Input::Type, Input::Scalar const &);
    void Fit();
    void Latch();
    void Prefill(Input::Type, Input::Scalar const &);

  private:
    void FitGauss(std::vector<uint32_t> const &, Gui::Axis const &);

    uint32_t m_xb;
    LinearTransform m_transform;
    Fitter m_fitter;
    Range m_range;
    Gui::Axis m_axis;
    std::mutex m_hist_mutex;
    std::vector<uint32_t> m_hist;
    Gui::Axis m_axis_copy;
    std::vector<uint32_t> m_hist_copy;
    bool m_is_log_y;
    std::vector<Gui::Peak> m_peak_vec;
};

class VisualHist2: public Visual {
  public:
    VisualHist2(std::string const &, size_t, uint32_t, uint32_t,
        LinearTransform const &, LinearTransform const &, char const *, bool,
        double);
    void Draw(Gui *);
    void Fill(
        Input::Type, Input::Scalar const &,
        Input::Type, Input::Scalar const &);
    void Fit();
    void Latch();
    void Prefill(
        Input::Type, Input::Scalar const &,
        Input::Type, Input::Scalar const &);

  private:
    //size_t m_colormap;
    uint32_t m_xb;
    uint32_t m_yb;
    LinearTransform m_transform_x;
    LinearTransform m_transform_y;
    Range m_range_x;
    Range m_range_y;
    Gui::Axis m_axis_x;
    Gui::Axis m_axis_y;
    std::mutex m_hist_mutex;
    std::vector<uint32_t> m_hist;
    Gui::Axis m_axis_x_copy;
    Gui::Axis m_axis_y_copy;
    std::vector<uint32_t> m_hist_copy;
    bool m_is_log_z;
    std::vector<uint8_t> m_pixels;
};

#endif
