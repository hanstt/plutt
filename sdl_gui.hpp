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

#ifndef SDL_GUI_HPP
#define SDL_GUI_HPP

#if PLUTT_SDL2

#include <gui.hpp>

class SdlGui: public Gui {
  public:
    SdlGui(char const *, unsigned, unsigned);
    ~SdlGui();

    void AddPage(std::string const &);
    uint32_t AddPlot(std::string const &, Plot *);

    bool DoClear(uint32_t);

    bool Draw(double);

    void DrawHist1(uint32_t, Axis const &, LinearTransform const &, bool,
        std::vector<uint32_t> const &, std::vector<Gui::Peak> const &);
    void DrawHist2(uint32_t, Axis const &, Axis const &,
        LinearTransform const &, LinearTransform const &,
        bool, std::vector<uint32_t> const &);

    bool DoClose();

  private:
    SdlGui(SdlGui const &);
    SdlGui &operator=(SdlGui const &);
    struct PlotWrap {
      PlotWrap();
      std::string name;
      Plot *plot;
      bool is_log_set;
      ImPlutt::PlotState plot_state;
      std::vector<uint8_t> pixels;
      private:
        PlotWrap(PlotWrap const &);
        PlotWrap &operator=(PlotWrap const &);
    };
    struct Page {
      Page();
      std::string name;
      std::vector<PlotWrap *> plot_wrap_vec;
    };
    ImPlutt::Window *m_window;
    std::vector<Page *> m_page_vec;
    Page *m_page_sel;
};

#endif

#endif
