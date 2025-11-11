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

#ifndef GUI_HPP
#define GUI_HPP

class LinearTransform;

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
    struct Peak {
      Peak(double,
          bool, double, double,
          bool, double, double, double);
      double y;
      bool has_exp;
      double exp_phase;
      double exp_tau;
      bool has_gauss;
      double gauss_amp;
      double gauss_x;
      double gauss_std;
    };
    class Plot {
      public:
        virtual ~Plot();
        virtual void Draw(Gui *) = 0;
        virtual void Latch() = 0;
    };

    virtual ~Gui();

  protected:
    virtual void AddPage(std::string const &) = 0;
    virtual uint32_t AddPlot(std::string const &, Plot *) = 0;

    virtual bool DoClear(uint32_t) = 0;

    virtual bool Draw(double) = 0;

    virtual void DrawAnnular(uint32_t, Axis const &, double, double, Axis
        const &, double, bool, std::vector<uint32_t> const &) = 0;
    virtual void DrawHist1(uint32_t, Axis const &, LinearTransform const &,
        bool, bool, std::vector<uint32_t> const &, std::vector<Peak> const &)
        = 0;
    virtual void DrawHist2(uint32_t, Axis const &, Axis const &,
        LinearTransform const &, LinearTransform const &,
        bool, std::vector<uint32_t> const &) = 0;

    friend class GuiCollection;
};

class GuiCollection {
  public:
    void AddGui(Gui *);

    void AddPage(std::string const &);
    uint32_t AddPlot(std::string const &, Gui::Plot *);

    bool DoClear(uint32_t);

    bool Draw(double);

    void DrawAnnular(Gui *, uint32_t, Gui::Axis const &, double, double,
        Gui::Axis const &, double, bool, std::vector<uint32_t> const &);
    void DrawHist1(Gui *, uint32_t, Gui::Axis const &,
        LinearTransform const &, bool, bool, std::vector<uint32_t> const &,
        std::vector<Gui::Peak> const &);
    void DrawHist2(Gui *, uint32_t, Gui::Axis const &, Gui::Axis const &,
        LinearTransform const &, LinearTransform const &,
        bool, std::vector<uint32_t> const &);

  private:
    std::map<Gui *, uint32_t> m_gui_map;
    struct Entry {
      Entry();
      Entry(Entry const &);
      Gui::Plot *plot;
      std::vector<uint32_t> id_vec;
      private:
        Entry &operator=(Entry const &);
    };
    std::vector<Entry> m_plot_vec;
};

#endif
