/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2025
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
#include <TH2Poly.h>
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

class RootGui::ClearMany: public TNamed
{
  public:
    ClearMany(std::string const &a_name, std::list<Bind *> *a_list):
      TNamed(a_name.c_str(), a_name.c_str()),
      m_list(a_list)
    {
    }
    void Clear(Option_t *) {
      for (auto it = m_list->begin(); m_list->end() != it; ++it) {
        (*it)->Clear(nullptr);
      }
    }

  private:
    ClearMany(ClearMany const &);
    ClearMany &operator=(ClearMany const &);

    std::list<Bind *> *m_list;
};

RootGui::PlotWrap::PlotWrap():
  name(),
  plot(),
  h1(),
  h2(),
  poly(),
  gr_vec(),
  tx_vec(),
  do_clear(),
  is_log_set(),
  is_log()
{
  poly.hp = nullptr;
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
  {
    // Add clearer for all histograms.
    std::string bind_name = "plutt_BindClearALL";
    auto clearer = new ClearMany(bind_name, &m_bind_list);
    gROOT->Append(clearer);
    auto cmdname = std::string("/Clear/ALL");
    m_server->RegisterCommand(cmdname.c_str(),
        (bind_name + "->Clear()").c_str());
  }
}

RootGui::~RootGui()
{
  m_server->SetTerminate();
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
  {
    std::string clean_name = CleanName(page->name);
    std::string bind_name = "plutt_BindClearPage_" + clean_name;
    auto clearer = new ClearMany(bind_name, &m_bind_list);
    gROOT->Append(clearer);
    auto cmdname = std::string("/Clear/PAGE_") + clean_name;
    m_server->RegisterCommand(cmdname.c_str(),
        (bind_name + "->Clear()").c_str());
  }
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
    std::string clean_name = CleanName(page->name + "_" + a_name);
    auto bind_name = std::string("plutt_BindClear_") + clean_name;
    auto clearer = new Bind(bind_name, plot_wrap);
    gROOT->Append(clearer);
    m_bind_list.push_back(clearer);
    page->m_bind_list.push_back(clearer);

    {
      auto cmdname = std::string("/Clear/HIST_") + clean_name;
      m_server->RegisterCommand(cmdname.c_str(),
          (bind_name + "->Clear()").c_str());
    }
    {
      auto cmdname = std::string("/LinLog/HIST_") + clean_name;
      m_server->RegisterCommand(cmdname.c_str(),
          (bind_name + "->SetDrawOption()").c_str());
    }
  }
  return ((uint32_t)m_page_vec.size() - 1) << 16 | ((uint32_t)vec.size() - 1);
}

std::string RootGui::CleanName(std::string const &a_name)
{
  std::string ret;
  int prev = -1;
  for (auto it = a_name.begin(); a_name.end() != it; ++it) {
    int c = *it;
    if ('_' != c && !isalnum(c)) {
      c = '_';
    }
    if (-1 != prev && ('_' != prev || '_' != c)) {
      ret += (char)prev;
    }
    prev = c;
  }
  if ('_' != prev) {
    ret += (char)prev;
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

void RootGui::DrawAnnular(uint32_t a_id, Axis const &a_axis_r, double a_r_min,
    double a_r_max, Axis const &a_axis_p, double a_phi0, bool a_is_log_z,
    std::vector<uint32_t> const &a_v)
{
  auto page_i = a_id >> 16;
  auto plot_i = a_id & 0xffff;
  auto page = m_page_vec.at(page_i);
  auto plot = page->plot_wrap_vec.at(plot_i);
  if (plot->h1 || plot->h2) {
    throw std::runtime_error(__func__);
  }
  if (!plot->is_log_set) {
    plot->is_log_set = true;
    plot->is_log = a_is_log_z;
  }
  auto pad = page->canvas->cd(1 + (int)plot_i);
  pad->SetLogz(plot->is_log);
  if (plot->poly.hp) {
    auto &axis_r = plot->poly.axis_r;
    auto &axis_p = plot->poly.axis_p;
    if (axis_r.bins != a_axis_r.bins ||
        axis_r.min  != a_axis_r.min ||
        axis_r.max  != a_axis_r.max ||
        axis_p.bins != a_axis_p.bins ||
        axis_p.min  != a_axis_p.min ||
        axis_p.max  != a_axis_p.max) {
      delete plot->poly.hp;
      plot->poly.hp = nullptr;
      axis_r = a_axis_r;
      axis_p = a_axis_p;
    }
  }
  if (!plot->poly.hp) {
    plot->poly.bin_vec.resize(a_axis_r.bins * a_axis_p.bins);
    size_t bin_i = 0;
    double x[4], y[4];
    auto extent = 1.1 * a_r_max;
    plot->poly.hp = new TH2Poly(plot->name.c_str(), plot->name.c_str(),
        -extent, extent, -extent, extent);
    plot->poly.hp->Draw("colz");
    auto dphi = (2 * M_PI * (a_phi0 < 0.0 ? -1 : 1)) / a_axis_p.bins;
    auto phi0 = 2 * M_PI * fabs(a_phi0) / 360;
    auto dr = (a_r_max - a_r_min) / a_axis_r.bins;
    for (size_t i = 0; i < a_axis_p.bins; ++i) {
      auto p0 = dphi * ((double)i + 0.05) + phi0;
      auto p1 = dphi * ((double)i + 0.95) + phi0;
      for (size_t j = 0; j < a_axis_r.bins; ++j) {
        auto r0 = dr * ((double)j + 0.1) + a_r_min;
        auto r1 = dr * ((double)j + 0.9) + a_r_min;
        x[0] = r0 * cos(p0);
        x[1] = r1 * cos(p0);
        x[2] = r1 * cos(p1);
        x[3] = r0 * cos(p1);
        y[0] = r0 * sin(p0);
        y[1] = r1 * sin(p0);
        y[2] = r1 * sin(p1);
        y[3] = r0 * sin(p1);
        plot->poly.bin_vec.at(bin_i++) = plot->poly.hp->AddBin(4, x, y);
      }
    }
  }
  size_t k = 0;
  for (size_t i = 0; i < a_axis_p.bins; ++i) {
    for (size_t j = 0; j < a_axis_r.bins; ++j) {
      plot->poly.hp->SetBinContent(plot->poly.bin_vec.at(k), a_v.at(k));
      ++k;
    }
  }
  plot->poly.hp->ResetStats();
}

// TODO: Use axis transform.
void RootGui::DrawHist1(uint32_t a_id, Axis const &a_axis, LinearTransform
    const &a_transform, bool a_is_log_y, bool a_is_contour,
    std::vector<uint32_t> const &a_v, std::vector<Peak> const &a_peak_vec)
{
  auto page_i = a_id >> 16;
  auto plot_i = a_id & 0xffff;
  auto page = m_page_vec.at(page_i);
  auto plot = page->plot_wrap_vec.at(plot_i);
  if (plot->h2 || plot->poly.hp) {
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
  plot->h1->ResetStats();

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
  if (plot->h1 || plot->poly.hp) {
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
  plot->h2->ResetStats();
}

#endif
