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

#ifndef GUI_HPP
#define GUI_HPP

#include <string>
#include <vector>

/*
 * GUI interface for plots and such.
 */

class Gui {
  public:
    struct Axis {
      void Clear();
      uint32_t bins;
      double min;
      double max;
    };
    class Plot {
      public:
        virtual ~Plot();
        virtual void Draw(Gui *) = 0;
    };

    virtual ~Gui();

    virtual void AddPage(std::string const &) = 0;
    virtual uint32_t AddPlot(std::string const &, Plot *) = 0;

    virtual bool Draw(double) = 0;

    virtual void SetHist1(uint32_t, Axis const &, bool,
        std::vector<uint32_t> const &) = 0;
    virtual void SetHist2(uint32_t, Axis const &, Axis const &, bool,
        std::vector<uint32_t> const &) = 0;
};

#endif
