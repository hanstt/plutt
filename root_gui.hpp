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

#ifndef ROOT_GUI_HPP
#define ROOT_GUI_HPP

#if PLUTT_ROOT

#include <gui.hpp>

class TCanvas;
class TGraph;
class TH1I;
class TH2I;
class THttpServer;
class TText;

class RootGui: public Gui {
  public:
    RootGui(uint16_t);
    ~RootGui();

    void AddPage(std::string const &);
    uint32_t AddPlot(std::string const &, Plot *);

    bool DoClear(uint32_t);

    bool Draw(double);

    void DrawHist1(uint32_t, Axis const &, LinearTransform const &, bool,
        bool, std::vector<uint32_t> const &, std::vector<Peak> const &);
    void DrawHist2(uint32_t, Axis const &, Axis const &,
        LinearTransform const &, LinearTransform const &,
        bool, std::vector<uint32_t> const &);

  private:
    RootGui(RootGui const &);
    RootGui &operator=(RootGui const &);

    std::string CleanName(std::string const &);

    class Bind;
    class ClearMany;

    struct PlotWrap {
      PlotWrap();
      std::string name;
      Plot *plot;
      TH1I *h1;
      TH2I *h2;
      std::vector<TGraph> gr_vec;
      std::vector<TText> tx_vec;
      bool do_clear;
      bool is_log_set;
      bool is_log;
      private:
        PlotWrap(PlotWrap const &);
        PlotWrap &operator=(PlotWrap const &);
    };
    struct Page {
      Page();
      std::string name;
      TCanvas *canvas;
      std::vector<PlotWrap *> plot_wrap_vec;
      std::list<Bind *> m_bind_list;
      private:
        Page(Page const &);
        Page &operator=(Page const &);
    };
    THttpServer *m_server;
    std::vector<Page *> m_page_vec;
    std::list<Bind *> m_bind_list;
};

#endif

#endif
