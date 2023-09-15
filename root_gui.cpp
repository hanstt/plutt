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

#include <root_gui.hpp>
#include <sstream>
#include <TCanvas.h>
#include <TH1I.h>
#include <TH2I.h>
#include <THttpServer.h>
#include <TSystem.h>

RootGui::RootGui(uint16_t a_port):
  m_server(),
  m_page_vec()
{
  std::ostringstream oss;
  oss << "http:" << a_port;
  m_server = new THttpServer(oss.str().c_str());
}

RootGui::~RootGui()
{
  for (auto it = m_page_vec.begin(); m_page_vec.end() != it; ++it) {
    auto page = *it;
    auto &vec = page->plot_wrap_vec;
    for (auto it2 = vec.begin(); vec.end() != it2; ++it2) {
      auto plot = *it2;
      delete plot->h1;
      delete plot->h2;
      delete plot;
    }
    m_server->Unregister(page->canvas);
    delete page->canvas;
    delete page;
  }
  delete m_server;
}

void RootGui::AddPage(std::string const &a_name)
{
  m_page_vec.push_back(new Page);
  auto page = m_page_vec.back();
  page->name = a_name;
}

uint32_t RootGui::AddPlot(std::string const &a_name, Plot *a_plot)
{
  if (m_page_vec.empty()) {
    AddPage("Default");
  }
  auto page = m_page_vec.back();
  auto &vec = page->plot_wrap_vec;
  vec.push_back(new PlotWrap);
  auto plot_wrap = vec.back();
  plot_wrap->name = a_name;
  plot_wrap->plot = a_plot;
  return ((uint32_t)m_page_vec.size() - 1) << 16 | ((uint32_t)vec.size() - 1);
}

bool RootGui::Draw()
{
  for (auto it = m_page_vec.begin(); m_page_vec.end() != it; ++it) {
    auto page = *it;
    auto &vec = page->plot_wrap_vec;
    if (!page->canvas) {
      auto rows = (int)sqrt(vec.size());
      auto cols = ((int)vec.size() + rows - 1) / rows;
      page->canvas = new TCanvas(page->name.c_str(), page->name.c_str());
      page->canvas->Divide(cols, rows);
      m_server->Register(page->name.c_str(), page->canvas);
    }
    int i = 1;
    for (auto it2 = vec.begin(); vec.end() != it2; ++it2) {
      auto plot_wrap = *it2;
      page->canvas->cd(i++);
      plot_wrap->plot->Draw(this);
    }
  }
  return gSystem->ProcessEvents();
}

void RootGui::SetHist1(uint32_t a_id, Axis const &a_axis,
    std::vector<uint32_t> const &a_v)
{
  auto page_i = a_id >> 16;
  auto plot_i = a_id & 0xffff;
  auto page = m_page_vec.at(page_i);
  auto plot = page->plot_wrap_vec.at(plot_i);
  if (plot->h2) {
    throw std::runtime_error(__func__);
  }
  if (plot->h1) {
    auto axis = plot->h1->GetXaxis();
    if (axis->GetNbins() != a_axis.bins ||
        axis->GetXmin()  != a_axis.min ||
        axis->GetXmax()  != a_axis.max) {
      m_server->Unregister(plot->h1);
      delete plot->h1;
      plot->h1 = nullptr;
    }
  }
  if (!plot->h1) {
    plot->h1 = new TH1I(plot->name.c_str(), plot->name.c_str(),
        (int)a_axis.bins, a_axis.min, a_axis.max);
    page->canvas->cd(1 + (int)plot_i);
    plot->h1->Draw();
  }
  for (size_t i = 0; i < a_axis.bins; ++i) {
    plot->h1->SetBinContent(1 + (int)i, a_v.at(i));
  }
}

void RootGui::SetHist2(uint32_t a_id, Axis const &a_axis_x, Axis const
    &a_axis_y, std::vector<uint32_t> const &a_v)
{
  auto page_i = a_id >> 16;
  auto plot_i = a_id & 0xffff;
  auto page = m_page_vec.at(page_i);
  auto plot = page->plot_wrap_vec.at(plot_i);
  if (plot->h1) {
    throw std::runtime_error(__func__);
  }
  if (plot->h2) {
    auto axis_x = plot->h2->GetXaxis();
    auto axis_y = plot->h2->GetYaxis();
    if (axis_x->GetNbins() != a_axis_x.bins ||
        axis_x->GetXmin()  != a_axis_x.min ||
        axis_x->GetXmax()  != a_axis_x.max ||
        axis_y->GetNbins() != a_axis_y.bins ||
        axis_y->GetXmin()  != a_axis_y.min ||
        axis_y->GetXmax()  != a_axis_y.max) {
      m_server->Unregister(plot->h2);
      delete plot->h2;
      plot->h2 = nullptr;
    }
  }
  if (!plot->h2) {
    plot->h2 = new TH2I(plot->name.c_str(), plot->name.c_str(),
        (int)a_axis_x.bins, a_axis_x.min, a_axis_x.max,
        (int)a_axis_y.bins, a_axis_y.min, a_axis_y.max);
    page->canvas->cd(1 + (int)plot_i);
    plot->h2->Draw("colz");
  }
  size_t k = 0;
  for (size_t i = 0; i < a_axis_y.bins; ++i) {
    for (size_t j = 0; j < a_axis_x.bins; ++j) {
      plot->h2->SetBinContent(1 + (int)j, 1 + (int)i, a_v.at(k++));
    }
  }
}
