/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2024
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

#if PLUTT_ROOT

#include <iostream>
#include <list>
#include <sstream>

#include <TCanvas.h>
#include <TGraph.h>
#include <TH1I.h>
#include <TH2I.h>
#include <THttpServer.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TText.h>

#include <root_gui.hpp>

class RootGui::Bind: public TNamed
{
  public:
    Bind(std::string const &a_name, PlotWrap *a_plot_wrap):
      TNamed(a_name.c_str(), a_name.c_str()),
      m_plot_wrap(a_plot_wrap)
    {
    }
    void Clear(Option_t *) {
      // This is abuse, this should clear name and title, but whatever.
      m_plot_wrap->do_clear = true;
    }
    void SetDrawOption(Option_t *) {
      m_plot_wrap->is_log ^= true;
    }

  private:
    Bind(Bind const &);
    Bind &operator=(Bind const &);

    PlotWrap *m_plot_wrap;
};

RootGui::PlotWrap::PlotWrap():
  name(),
  plot(),
  h1(),
  h2(),
  gr_vec(),
  tx_vec(),
  do_clear(),
  is_log_set(),
  is_log()
{
}

RootGui::Page::Page():
  name(),
  canvas(),
  plot_wrap_vec()
{
}

RootGui::RootGui(uint16_t a_port):
  m_server(),
  m_page_vec(),
  m_bind_list()
{
  std::ostringstream oss;
  oss << "http:" << a_port << ";noglobal";
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
  for (auto it = m_bind_list.begin(); m_bind_list.end() != it; ++it) {
    delete *it;
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
  {
    auto bind_name = std::string("plutt_Bind_") +
        CleanName(page->name + "_" + a_name);
    auto clearer = new Bind(bind_name, plot_wrap);
    gROOT->Append(clearer);
    m_bind_list.push_back(clearer);

    {
      auto cmdname = std::string("/Clear/") +
          CleanName(page->name + "_" + a_name);
      m_server->RegisterCommand(cmdname.c_str(),
          (bind_name + "->Clear()").c_str());
    }
    {
      auto cmdname = std::string("/LinLog/") +
          CleanName(page->name + "_" + a_name);
      m_server->RegisterCommand(cmdname.c_str(),
          (bind_name + "->SetDrawOption()").c_str());
    }
  }
  return ((uint32_t)m_page_vec.size() - 1) << 16 | ((uint32_t)vec.size() - 1);
}

std::string RootGui::CleanName(std::string const &a_name)
{
  std::string ret = a_name;
  for (auto it = ret.begin(); ret.end() != it; ++it) {
    auto c = *it;
    if ('_' != c && !isalnum(c)) {
      *it = '_';
    }
  }
  return ret;
}

bool RootGui::DoClear(uint32_t a_id)
{
  auto page_i = a_id >> 16;
  auto plot_i = a_id & 0xffff;
  auto page = m_page_vec.at(page_i);
  auto plot = page->plot_wrap_vec.at(plot_i);
  auto ret = plot->do_clear;
  plot->do_clear = false;
  return ret;
}

bool RootGui::Draw(double a_event_rate)
{
  std::cout << "\rEvent-rate: " << a_event_rate << "              " <<
      std::flush;
  for (auto it = m_page_vec.begin(); m_page_vec.end() != it; ++it) {
    auto page = *it;
    auto &vec = page->plot_wrap_vec;
    if (!page->canvas) {
      auto rows = (int)sqrt((double)vec.size());
      auto cols = ((int)vec.size() + rows - 1) / rows;
      page->canvas = new TCanvas(page->name.c_str(), page->name.c_str());
      page->canvas->Divide(cols, rows, 0.01f, 0.01f);
      m_server->Register("/", page->canvas);
    }
    int i = 1;
    for (auto it2 = vec.begin(); vec.end() != it2; ++it2) {
      auto plot_wrap = *it2;
      page->canvas->cd(i++);
      plot_wrap->plot->Draw(this);
    }
  }
  return !gSystem->ProcessEvents();
}

// TODO: Use axis transform.
void RootGui::DrawHist1(uint32_t a_id, Axis const &a_axis, LinearTransform
    const &a_transform, bool a_is_log_y, std::vector<uint32_t> const &a_v,
    std::vector<Peak> const &a_peak_vec)
{
  auto page_i = a_id >> 16;
  auto plot_i = a_id & 0xffff;
  auto page = m_page_vec.at(page_i);
  auto plot = page->plot_wrap_vec.at(plot_i);
  if (plot->h2) {
    throw std::runtime_error(__func__);
  }
  if (!plot->is_log_set) {
    plot->is_log_set = true;
    plot->is_log = a_is_log_y;
  }
  auto pad = page->canvas->cd(1 + (int)plot_i);
  pad->SetLogy(plot->is_log);
  if (plot->h1) {
    auto axis = plot->h1->GetXaxis();
    if (axis->GetNbins() != (int)a_axis.bins ||
        axis->GetXmin()  != (int)a_axis.min ||
        axis->GetXmax()  != (int)a_axis.max) {
      delete plot->h1;
      plot->h1 = nullptr;
    }
  }
  if (!plot->h1) {
    plot->h1 = new TH1I(plot->name.c_str(), plot->name.c_str(),
        (int)a_axis.bins, a_axis.min, a_axis.max);
    plot->h1->Draw();
  }
  for (size_t i = 0; i < a_axis.bins; ++i) {
    plot->h1->SetBinContent(1 + (int)i, a_v.at(i));
  }

  // Fit-peak graph.
  plot->gr_vec.resize(a_peak_vec.size());
  plot->tx_vec.resize(a_peak_vec.size());
  size_t gr_i = 0;
  for (auto it = a_peak_vec.begin(); a_peak_vec.end() != it; ++it) {
    auto &gr = plot->gr_vec.at(gr_i);
    auto &tx = plot->tx_vec.at(gr_i);
    gr.Set(20);
    gr.SetLineColor(kRed);
    //auto x = a_transform.ApplyAbs(it->peak_x);
    //auto std = a_transform.ApplyRel(it->std_x);
    auto x0 = it->peak_x;
    auto std = it->std_x;
    auto denom = 1 / (2 * std * std);
    auto left = x0 - 3 * std;
    auto right = x0 + 3 * std;
    auto scale = (right - left) / gr.GetN();
    for (int i = 0; i < gr.GetN(); ++i) {
      auto x = (i + 0.5) * scale + left;
      auto d = x - x0;
      auto y = it->ofs_y + it->amp_y * exp(-d*d * denom);
      gr.SetPoint(i, x, y);
    }
    gr.Draw("L");
    char buf[256];
    snprintf(buf, sizeof buf, "%.3f/%.3f", x0, std);
    auto text_y = it->ofs_y + it->amp_y;
    tx.SetTextFont(40);
    tx.SetTextSize(0.02f);
    tx.SetText(x0, text_y, buf);
    tx.Draw();
    ++gr_i;
  }
}

// TODO: Use axis transform.
void RootGui::DrawHist2(uint32_t a_id, Axis const &a_axis_x, Axis const
    &a_axis_y, LinearTransform const &a_tranform_x, LinearTransform const
    &a_transform_y, bool a_is_log_z, std::vector<uint32_t> const &a_v)
{
  auto page_i = a_id >> 16;
  auto plot_i = a_id & 0xffff;
  auto page = m_page_vec.at(page_i);
  auto plot = page->plot_wrap_vec.at(plot_i);
  if (plot->h1) {
    throw std::runtime_error(__func__);
  }
  if (!plot->is_log_set) {
    plot->is_log_set = true;
    plot->is_log = a_is_log_z;
  }
  auto pad = page->canvas->cd(1 + (int)plot_i);
  pad->SetLogz(plot->is_log);
  if (plot->h2) {
    auto axis_x = plot->h2->GetXaxis();
    auto axis_y = plot->h2->GetYaxis();
    if (axis_x->GetNbins() != (int)a_axis_x.bins ||
        axis_x->GetXmin()  != (int)a_axis_x.min ||
        axis_x->GetXmax()  != (int)a_axis_x.max ||
        axis_y->GetNbins() != (int)a_axis_y.bins ||
        axis_y->GetXmin()  != (int)a_axis_y.min ||
        axis_y->GetXmax()  != (int)a_axis_y.max) {
      delete plot->h2;
      plot->h2 = nullptr;
    }
  }
  if (!plot->h2) {
    plot->h2 = new TH2I(plot->name.c_str(), plot->name.c_str(),
        (int)a_axis_x.bins, a_axis_x.min, a_axis_x.max,
        (int)a_axis_y.bins, a_axis_y.min, a_axis_y.max);
    plot->h2->Draw("colz");
  }
  size_t k = 0;
  for (size_t i = 0; i < a_axis_y.bins; ++i) {
    for (size_t j = 0; j < a_axis_x.bins; ++j) {
      plot->h2->SetBinContent(1 + (int)j, 1 + (int)i, a_v.at(k++));
    }
  }
}

#endif
