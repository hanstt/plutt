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

#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <gui.hpp>

void Gui::Axis::Clear()
{
  bins = 0;
  min = 0.0;
  max = 0.0;
}

Gui::Peak::Peak(double a_y,
    bool a_has_exp, double a_exp_phase, double a_exp_tau,
    bool a_has_gauss, double a_gauss_amp, double a_gauss_x, double
    a_gauss_std):
  y(a_y),
  has_exp(a_has_exp),
  exp_phase(a_exp_phase),
  exp_tau(a_exp_tau),
  has_gauss(a_has_gauss),
  gauss_amp(a_gauss_amp),
  gauss_x(a_gauss_x),
  gauss_std(a_gauss_std)
{
}

Gui::Plot::~Plot()
{
}

Gui::~Gui()
{
}

GuiCollection::Entry::Entry():
  plot(),
  id_vec()
{
}

GuiCollection::Entry::Entry(Entry const &a_entry):
  plot(a_entry.plot),
  id_vec(a_entry.id_vec)
{
}

void GuiCollection::AddGui(Gui *a_gui)
{
  m_gui_map.insert(std::make_pair(a_gui, m_gui_map.size()));
}

#define FOR_GUI \
  for (auto it = m_gui_map.begin(); m_gui_map.end() != it; ++it)

void GuiCollection::AddPage(std::string const &a_name)
{
  FOR_GUI {
    auto gui = it->first;
    gui->AddPage(a_name);
  }
}

uint32_t GuiCollection::AddPlot(std::string const &a_name, Gui::Plot *a_plot)
{
  m_plot_vec.push_back(Entry());
  auto &pe = m_plot_vec.back();
  pe.plot = a_plot;
  FOR_GUI {
    auto gui = it->first;
    auto id = gui->AddPlot(a_name, a_plot);
    pe.id_vec.push_back(id);
  }
  return (uint32_t)m_plot_vec.size() - 1;
}

bool GuiCollection::DoClear(uint32_t a_id)
{
  bool yes = false;
  FOR_GUI {
    auto gui = it->first;
    auto gui_i = it->second;
    auto const &pe = m_plot_vec.at(a_id);
    yes |= gui->DoClear(pe.id_vec.at(gui_i));
  }
  return yes;
}

bool GuiCollection::Draw(double a_event_rate)
{
  for (auto it = m_plot_vec.begin(); m_plot_vec.end() != it; ++it) {
    auto plot = it->plot;
    plot->Latch();
  }
  bool ok = true;
  FOR_GUI {
    auto gui = it->first;
    ok &= gui->Draw(a_event_rate);
  }
  return ok;
}

void GuiCollection::DrawAnnular(Gui *a_gui, uint32_t a_id, Gui::Axis const
    &a_axis_r, double a_r_min, double a_r_max, Gui::Axis const &a_axis_p,
    double a_phi0, bool a_is_log_z, std::vector<uint32_t> const &a_v)
{
  auto it = m_gui_map.find(a_gui);
  assert(m_gui_map.end() != it);
  auto gui_i = it->second;
  auto const &pe = m_plot_vec.at(a_id);
  a_gui->DrawAnnular(pe.id_vec.at(gui_i), a_axis_r, a_r_min, a_r_max,
      a_axis_p, a_phi0, a_is_log_z, a_v);
}

void GuiCollection::DrawHist1(Gui *a_gui, uint32_t a_id, Gui::Axis const
    &a_axis, LinearTransform const &a_transform, bool a_is_log_y, bool
    a_is_contour, std::vector<uint32_t> const &a_v, std::vector<Gui::Peak>
    const &a_peak_vec)
{
  auto it = m_gui_map.find(a_gui);
  assert(m_gui_map.end() != it);
  auto gui_i = it->second;
  auto const &pe = m_plot_vec.at(a_id);
  a_gui->DrawHist1(pe.id_vec.at(gui_i), a_axis, a_transform, a_is_log_y,
      a_is_contour, a_v, a_peak_vec);
}

void GuiCollection::DrawHist2(Gui *a_gui, uint32_t a_id, Gui::Axis const
    &a_axis_x, Gui::Axis const &a_axis_y, LinearTransform const
    &a_transform_x, LinearTransform const &a_transform_y, bool a_is_log_z,
    std::vector<uint32_t> const &a_v)
{
  auto it = m_gui_map.find(a_gui);
  assert(m_gui_map.end() != it);
  auto gui_i = it->second;
  auto const &pe = m_plot_vec.at(a_id);
  a_gui->DrawHist2(pe.id_vec.at(gui_i), a_axis_x, a_axis_y, a_transform_x,
      a_transform_y, a_is_log_z, a_v);
}
