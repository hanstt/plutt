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

#include <config.hpp>
#include <iostream>
#include <sstream>
#include <err.h>
#include <config_parser.hpp>
#include <implutt.hpp>
#include <util.hpp>

#include <node_alias.hpp>
#include <node_bitfield.hpp>
#include <node_cluster.hpp>
#include <node_coarse_fine.hpp>
#include <node_cut.hpp>
#include <node_hist1.hpp>
#include <node_hist2.hpp>
#include <node_match_index.hpp>
#include <node_match_value.hpp>
#include <node_max.hpp>
#include <node_mean_arith.hpp>
#include <node_mean_geom.hpp>
#include <node_member.hpp>
#include <node_pedestal.hpp>
#include <node_select_index.hpp>
#include <node_signal.hpp>
#include <node_sub_mod.hpp>
#include <node_tot.hpp>
#include <node_tpat.hpp>
#include <node_trig_map.hpp>
#include <node_zero_suppress.hpp>

extern FILE *yycpin;
extern Config *g_config;

extern void yycperror(char const *);
extern int yycpparse();

extern char const *yycppath;

Config::Config(char const *a_path):
  m_path(a_path),
  m_line(),
  m_col(),
  m_trig_map(),
  m_alias_map(),
  m_signal_map(),
  m_cut_node_list(),
  m_cuttable_map(),
  m_cut_poly_map(),
  m_cut_poly_list(),
  m_cut_ref_map(),
  m_fit_map(),
  m_clock_match(),
  m_colormap(ImPlutt::ColormapGet(nullptr)),
  m_evid(),
  m_input()
{
  // config_parser relies on this global!
  g_config = this;

  // Let the bison roam.
  yycpin = fopen(a_path, "rb");
  if (!yycpin) {
    warn("fopen(%s)", a_path);
    throw std::runtime_error(__func__);
  }
  yycppath = a_path;
  yycplloc.last_line = 1;
  yycplloc.last_column = 1;
  std::cout << a_path << ": Parsing...\n";
  yycpparse();
  std::cout << a_path << ": Done!\n";
  fclose(yycpin);

  // Link unassigned aliases to signal-map, should be ucesb signals.
  for (auto it = m_alias_map.begin(); m_alias_map.end() != it; ++it) {
    auto alias = it->second;
    if (!alias->GetSource()) {
      auto signal = new NodeSignal(*this, it->first.c_str());
      signal->SetLocStr(alias->GetLocStr());
      alias->SetSource(GetLocStr(), signal);
      m_signal_map.insert(std::make_pair(it->first, signal));
    }
  }
  for (auto it = m_signal_map.begin(); m_signal_map.end() != it; ++it) {
    std::cout << "Signal=" << it->first << '\n';
  }

  if (!m_cut_poly_list.empty()) {
    throw std::runtime_error(__func__);
  }
  // Now all histos exist, change from name refs to direct pointers for cuts.
  for (auto dst_it = m_cut_node_list.begin(); m_cut_node_list.end() != dst_it;
      ++dst_it) {
    auto &dst = *dst_it;
    auto const &title = dst->GetCutPolygon().GetTitle();
    auto src_it = m_cuttable_map.find(title);
    if (m_cuttable_map.end() == src_it) {
      std::cerr << dst->GetLocStr() << ": Histogram '" << title <<
          "' does not exist!\n";
      throw std::runtime_error(__func__);
    }
    dst->SetCuttable(src_it->second);
  }
  for (auto it = m_cut_ref_map.begin(); m_cut_ref_map.end() != it; ++it) {
    auto const &dst_title = it->first;
    auto const &poly_list = it->second;
    auto dst_it = m_cuttable_map.find(dst_title);
    assert(m_cuttable_map.end() != dst_it);
    auto dst_node = dst_it->second;
    for (auto it2 = poly_list.begin(); poly_list.end() != it2; ++it2) {
      auto src_it = m_cuttable_map.find((*it2)->GetTitle());
      if (m_cuttable_map.end() == src_it) {
        std::cerr << (*it2)->GetTitle() << ": Histogram does not exist!\n";
        throw std::runtime_error(__func__);
      }
      auto src_node = src_it->second;
      dst_node->CutEventAdd(src_node, *it2);
    }
  }
  m_cut_ref_map.clear();
}

Config::~Config()
{
  for (auto it = m_alias_map.begin(); m_alias_map.end() != it; ++it) {
    delete it->second;
  }
  for (auto it = m_signal_map.begin(); m_signal_map.end() != it; ++it) {
    delete it->second;
  }
  for (auto it = m_cuttable_map.begin(); m_cuttable_map.end() != it; ++it) {
    delete it->second;
  }
}

NodeValue *Config::AddAlias(char const *a_name, NodeValue *a_value, uint32_t
    a_ret_i)
{
  NodeAlias *alias;
  auto it = m_alias_map.find(a_name);
  if (m_alias_map.end() == it) {
    alias = new NodeAlias(GetLocStr(), a_value, a_ret_i);
    m_alias_map.insert(std::make_pair(a_name, alias));
  } else {
    alias = it->second;
    alias->SetSource(GetLocStr(), a_value);
  }
  return alias;
}

NodeValue *Config::AddBitfield(BitfieldArg *a_arg)
{
  auto node = new NodeBitfield(GetLocStr(), a_arg);
  return node;
}

NodeValue *Config::AddCluster(NodeValue *a_node)
{
  auto node = new NodeCluster(GetLocStr(), a_node);
  return node;
}

NodeValue *Config::AddCoarseFine(NodeValue *a_coarse, NodeValue *a_fine,
    double a_fine_range)
{
  auto node = new NodeCoarseFine(GetLocStr(), a_coarse, a_fine, a_fine_range);
  return node;
}

NodeValue *Config::AddCut(char const *a_str, std::vector<CutPolygon::Point>
    const &a_vec)
{
  auto node = new NodeCut(GetLocStr(), a_str, a_vec);
  NodeCutAdd(node);
  return node;
}

void Config::AddFit(char const *a_name, double a_k, double a_m)
{
  auto ret = m_fit_map.insert(std::make_pair(a_name, FitEntry()));
  if (!ret.second) {
    std::cerr << a_name << ": Fit already exists.\n";
    throw std::runtime_error(__func__);
  }
  auto it = ret.first;
  it->second.k = a_k;
  it->second.m = a_m;
}

void Config::AddHist1(char const *a_title, NodeValue *a_x, uint32_t a_xb, char
    const *a_transform, char const *a_fit, bool a_log_y, double a_drop_old_s)
{
  double k = 1.0;
  double m = 0.0;
  if (a_transform) {
    auto it = m_fit_map.find(a_transform);
    if (m_fit_map.end() == it) {
      std::cerr << a_transform <<
          ": Fit must be defined before histogram!\n";
      throw std::runtime_error(__func__);
    }
    k = it->second.k;
    m = it->second.m;
  }
  auto node = new NodeHist1(GetLocStr(), a_title,
      a_x, a_xb, LinearTransform(k, m), a_fit, a_log_y, a_drop_old_s);
  NodeCuttableAdd(node);
}

void Config::AddHist2(char const *a_title, NodeValue *a_y, NodeValue *a_x,
    uint32_t a_yb, uint32_t a_xb, char const *a_transformy, char const
    *a_transformx, char const *a_fit, bool a_log_z, double a_drop_old_s)
{
  double kx = 1.0;
  double mx = 0.0;
  if (a_transformx) {
    auto it = m_fit_map.find(a_transformx);
    if (m_fit_map.end() == it) {
      std::cerr << a_transformx <<
          ": Fit must be defined before histogram!\n";
      throw std::runtime_error(__func__);
    }
    kx = it->second.k;
    mx = it->second.m;
  }
  double ky = 1.0;
  double my = 0.0;
  if (a_transformy) {
    auto it = m_fit_map.find(a_transformy);
    if (m_fit_map.end() == it) {
      std::cerr << a_transformy <<
          ": Fit must be defined before histogram!\n";
      throw std::runtime_error(__func__);
    }
    ky = it->second.k;
    my = it->second.m;
  }
  auto node = new NodeHist2(GetLocStr(), a_title, m_colormap,
      a_y, a_x, a_yb, a_xb, LinearTransform(ky, my), LinearTransform(kx, mx),
      a_fit, a_log_z, a_drop_old_s);
  NodeCuttableAdd(node);
}

NodeValue *Config::AddMatchIndex(NodeValue *a_l, NodeValue *a_r)
{
  auto node = new NodeMatchIndex(GetLocStr(), a_l, a_r);
  return node;
}

NodeValue *Config::AddMatchValue(NodeValue *a_l, NodeValue *a_r, double
    a_cutoff)
{
  auto node = new NodeMatchValue(GetLocStr(), a_l, a_r, a_cutoff);
  return node;
}

NodeValue *Config::AddMax(NodeValue *a_value)
{
  auto node = new NodeMax(GetLocStr(), a_value);
  return node;
}

NodeValue *Config::AddMeanArith(NodeValue *a_l, NodeValue *a_r)
{
  auto node = new NodeMeanArith(GetLocStr(), a_l, a_r);
  return node;
}

NodeValue *Config::AddMeanGeom(NodeValue *a_l, NodeValue *a_r)
{
  auto node = new NodeMeanGeom(GetLocStr(), a_l, a_r);
  return node;
}

NodeValue *Config::AddMember(NodeValue *a_node, char const *a_suffix)
{
  auto node = new NodeMember(GetLocStr(), a_node, a_suffix);
  return node;
}

void Config::AddPage(char const *a_label)
{
  plot_page_create(a_label);
}

NodeValue *Config::AddPedestal(NodeValue *a_value, double a_cutoff, NodeValue
    *a_tpat)
{
  auto node = new NodePedestal(GetLocStr(), a_value, a_cutoff, a_tpat);
  return node;
}

NodeValue *Config::AddSelectIndex(NodeValue *a_child, uint32_t a_first,
    uint32_t a_last)
{
  auto node = new NodeSelectIndex(GetLocStr(), a_child, a_first, a_last);
  return node;
}

NodeValue *Config::AddSubMod(NodeValue *a_left, NodeValue *a_right, double
    a_range)
{
  auto node = new NodeSubMod(GetLocStr(), a_left, a_right, a_range);
  return node;
}

NodeValue *Config::AddTot(NodeValue *a_leading, NodeValue *a_trailing, double
    a_range)
{
  auto node = new NodeTot(GetLocStr(), a_leading, a_trailing, a_range);
  return node;
}

NodeValue *Config::AddTpat(NodeValue *a_tpat, uint32_t a_mask)
{
  auto node = new NodeTpat(GetLocStr(), a_tpat, a_mask);
  return node;
}

NodeValue *Config::AddTrigMap(char const *a_path, char const *a_prefix,
    NodeValue *a_left, NodeValue *a_right, double a_range)
{
  auto prefix = m_trig_map.LoadPrefix(a_path, a_prefix);
  auto node = new NodeTrigMap(GetLocStr(), prefix, a_left, a_right, a_range);
  return node;
}

NodeValue *Config::AddZeroSuppress(NodeValue *a_value, double a_cutoff)
{
  auto node = new NodeZeroSuppress(GetLocStr(), a_value, a_cutoff);
  return node;
}

void Config::BindSignal(std::string const &a_name, char const *a_suffix,
    size_t a_id, Input::Type a_type)
{
  auto it = m_signal_map.find(a_name);
  if (m_signal_map.end() == it) {
    std::cerr << a_name <<
        ": Input told to bind, but config doesn't know?!\n";
    throw std::runtime_error(__func__);
  }
  auto signal = it->second;
  signal->BindSignal(a_suffix ? a_suffix : "", a_id, a_type);
}

void Config::AppearanceSet(char const *a_name)
{
  ImPlutt::Style style;
  if (0 == strcmp("light", a_name)) {
    style = ImPlutt::STYLE_LIGHT;
  } else if (0 == strcmp("dark", a_name)) {
    style = ImPlutt::STYLE_DARK;
  } else {
    std::cerr << GetLocStr() << ": Unknown style '" << a_name << "'.\n";
    throw std::runtime_error(__func__);
  }
  ImPlutt::StyleSet(style);
}

void Config::ClockMatch(NodeValue *a_value, double a_s_from_ts)
{
  m_clock_match.node = a_value;
  m_clock_match.s_from_ts = a_s_from_ts;
}

void Config::ColormapSet(char const *a_name)
{
  try {
    m_colormap = ImPlutt::ColormapGet(a_name);
  } catch (...) {
    std::cerr << GetLocStr() << ": Could not set palette.\n";
    throw std::runtime_error(__func__);
  }
}

void Config::HistCutAdd(char const *a_path, std::vector<CutPolygon::Point>
    const &a_vec)
{
  // Manage cache of cut polygons.
  auto it = m_cut_poly_map.find(a_path);
  if (m_cut_poly_map.end() == it) {
    auto ret = m_cut_poly_map.insert(
        std::make_pair(a_path, new CutPolygon(a_path, a_vec)));
    it = ret.first;
  }
  auto poly = it->second;
  // Temp list of cuts for histogram under construction.
  m_cut_poly_list.push_back(poly);
}

void Config::CutListBind(std::string const &a_dst_title)
{
  // Bind temp list to histogram via its name.
  m_cut_ref_map.insert(std::make_pair(a_dst_title, m_cut_poly_list));
  m_cut_poly_list.clear();
}

void Config::NodeCutAdd(NodeCut *a_node)
{
  // This node is pending cut-assignments when everything has been loaded.
  m_cut_node_list.push_back(a_node);
}

void Config::NodeCuttableAdd(NodeCuttable *a_node)
{
  // This node is pending cut-assignments when everything has been loaded.
  auto it = m_cuttable_map.find(a_node->GetTitle());
  if (m_cuttable_map.end() != it) {
    std::cerr << a_node->GetLocStr() << ": Histogram title already used at "
        << it->second->GetLocStr() << "\n";
    throw std::runtime_error(__func__);
  }
  m_cuttable_map.insert(std::make_pair(a_node->GetTitle(), a_node));
  CutListBind(a_node->GetTitle());
}

void Config::DoEvent(Input *a_input)
{
  m_input = a_input;

  if (m_clock_match.node) {
    // Match virtual event-rate with given signal.
    m_clock_match.node->Process(m_evid);
    auto const &val = m_clock_match.node->GetValue();
    auto const &v = val.GetV();
    if (!v.empty()) {
      auto const &s = v.at(0);
      double dts;
#define CLOCK_MATCH_PREP(field) do { \
  if (s.field <= m_clock_match.ts_prev.field) { \
    std::cerr << "Non-monotonic clock for rate-matching!\n"; \
    std::cerr << "Prev=" << s.field << \
      " -> Curr=" << m_clock_match.ts_prev.field << ".\n"; \
    throw std::runtime_error(__func__); \
  } \
  m_clock_match.ts_prev.field = s.field; \
  if (0 == m_clock_match.ts0.field) { \
    m_clock_match.ts0.field = s.field; \
  } \
  dts = m_clock_match.s_from_ts * \
      (double)(s.field - m_clock_match.ts0.field); \
} while (0)
      switch (val.GetType()) {
        case Value::kUint64:
          CLOCK_MATCH_PREP(u64);
          break;
        case Value::kDouble:
          CLOCK_MATCH_PREP(dbl);
          break;
        default:
          throw std::runtime_error(__func__);
      }
      if (0 == m_clock_match.t0) {
        m_clock_match.t0 = 1e-3 * (double)Time_get_ms();
      }
      double dt = 1e-3 * (double)Time_get_ms() - m_clock_match.t0;
      if (dts > dt) {
        Time_wait_ms((uint32_t)(1e3 * (dts - dt)));
      }
    }
  }

  for (auto it = m_cuttable_map.begin(); m_cuttable_map.end() != it; ++it) {
    auto node = it->second;
    node->CutReset();
  }
  for (auto it = m_cuttable_map.begin(); m_cuttable_map.end() != it; ++it) {
    auto node = it->second;
    node->Process(m_evid);
  }

  m_input = nullptr;
  ++m_evid;
}

std::string Config::GetLocStr() const
{
  std::ostringstream oss;
  oss << m_path << ':' << m_line << ':' << m_col;
  return oss.str();
}

std::list<std::string> Config::GetSignalList() const
{
  std::list<std::string> list;
  for (auto it = m_signal_map.begin(); m_signal_map.end() != it; ++it) {
    list.push_back(it->first);
  }
  return list;
}

Input const *Config::GetInput() const
{
  assert(m_input);
  return m_input;
}

Input *Config::GetInput()
{
  assert(m_input);
  return m_input;
}

void Config::SetLoc(int a_line, int a_col)
{
  m_line = a_line;
  m_col = a_col;
}
