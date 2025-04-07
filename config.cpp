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

#include <err.h>

#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <cal.hpp>
#include <util.hpp>
#include <visual.hpp>
#if PLUTT_SDL2
# include <SDL.h>
# include <implutt.hpp>
#endif

#include <node.hpp>
#include <node_alias.hpp>
#include <node_bitfield.hpp>
#include <node_cluster.hpp>
#include <node_coarse_fine.hpp>
#include <node_cut.hpp>
#include <node_filter_range.hpp>
#include <node_hist1.hpp>
#include <node_hist2.hpp>
#include <node_length.hpp>
#include <node_match_id.hpp>
#include <node_match_value.hpp>
#include <node_max.hpp>
#include <node_mean_arith.hpp>
#include <node_mean_geom.hpp>
#include <node_member.hpp>
#include <node_mexpr.hpp>
#include <node_pedestal.hpp>
#include <node_select_id.hpp>
#include <node_signal.hpp>
#include <node_sub_mod.hpp>
#include <node_tot.hpp>
#include <node_tpat.hpp>
#include <node_trig_map.hpp>
#include <node_zero_suppress.hpp>

#include <config.hpp>
#include <config_parser.hpp>
#include <config_parser.tab.h>

#define DEFAULT_UI_RATE 20U

extern FILE *yycpin;
extern Config *g_config;

extern void yycperror(char const *);
extern int yycpparse();
extern int yycplex_destroy();

extern char const *yycppath;

Config::Signal::Signal(std::string const &a_loc, std::string const &a_name,
    std::string const &a_id, std::string const &a_end, std::string const
    &a_v):
  loc(a_loc), name(a_name), id(a_id), end(a_end), v(a_v)
{}

Config::Config(char const *a_path):
  m_path(a_path),
  m_line(),
  m_col(),
  m_trig_map(),
  m_node_value_map(),
  m_alias_map(),
  m_signal_descr_map(),
  m_signal_map(),
  m_cut_node_list(),
  m_cuttable_map(),
  m_cut_poly_list(),
  m_cut_ref_map(),
  m_fit_map(),
  m_clock_match(),
  m_colormap(),
  m_ui_rate(DEFAULT_UI_RATE),
  m_evid(),
  m_input()
{
  // config_parser relies on this global!
  g_config = this;

#if PLUTT_SDL2
  m_colormap = ImPlutt::ColormapGet(nullptr);
#endif

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
  fclose(yycpin);
  yycplex_destroy();
  std::cout << a_path << ": Done!\n";

  // Copy unassigned aliases to signal-description map, should come from
  // Input.
  for (auto it = m_alias_map.begin(); m_alias_map.end() != it; ++it) {
    auto alias = it->second;
    if (!alias->GetSource()) {
      auto const &name = it->first;
      auto it2 = m_signal_descr_map.find(name);
      if (m_signal_descr_map.end() == it2) {
        m_signal_descr_map.insert(std::make_pair(name,
            Signal(alias->GetLocStr(), name, "", "", "")));
      }
    }
  }
  // Create signals from description map.
  for (auto it = m_signal_descr_map.begin(); m_signal_descr_map.end() != it;
      ++it) {
    auto const &name = it->first;
    auto signal = new NodeSignal(*this, name);
    signal->SetLocStr(it->second.loc);
    auto it2 = m_alias_map.find(name);
    if (m_alias_map.end() != it2) {
      auto alias = it2->second;
      alias->SetSource(GetLocStr(), signal);
    }
    m_signal_map.insert(std::make_pair(it->first, signal));
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
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_name << ',' << a_value << ',' << a_ret_i;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    auto it = m_alias_map.find(a_name);
    NodeAlias *alias;
    if (m_alias_map.end() == it) {
      alias = new NodeAlias(GetLocStr(), a_value, a_ret_i);
      m_alias_map.insert(std::make_pair(a_name, alias));
    } else {
      alias = it->second;
      alias->SetSource(GetLocStr(), a_value);
    }
    NodeValueAdd(key, node = alias);
  }
  return node;
}

NodeValue *Config::AddBitfield(BitfieldArg *a_arg)
{
  std::ostringstream oss;
  oss << __LINE__;
  for (auto arg = a_arg; arg; arg = arg->next) {
    oss << ',' << arg->node << ',' << arg->bits;
  }
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeBitfield(GetLocStr(), a_arg));
  }
  return node;
}

NodeValue *Config::AddCluster(NodeValue *a_node)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_node;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeCluster(GetLocStr(), a_node));
  }
  return node;
}

NodeValue *Config::AddCoarseFine(NodeValue *a_coarse, NodeValue *a_fine,
    double a_fine_range)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_coarse << ',' << a_fine << ',' << a_fine_range;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeCoarseFine(GetLocStr(), a_coarse, a_fine,
        a_fine_range));
  }
  return node;
}

NodeValue *Config::AddCut(CutPolygon *a_poly)
{
  // TODO: De-duplicate this properly...
  auto node_cut = new NodeCut(GetLocStr(), a_poly);
  NodeCutAdd(node_cut);
  return node_cut;
}

NodeValue *Config::AddFilterRange(
    std::vector<FilterRangeCond> const &a_cond_vec,
    std::vector<NodeValue *> const &a_arg_vec)
{
  std::ostringstream oss;
  oss << __LINE__;
  for (auto it = a_cond_vec.begin(); a_cond_vec.end() != it; ++it) {
    oss << ',' << it->node << ',' << it->lower << ',' << it->lower_le << ',' <<
        it->upper << ',' << it->upper_le;
  }
  for (auto it = a_arg_vec.begin(); a_arg_vec.end() != it; ++it) {
    oss << ',' << *it;
  }
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key,
        node = new NodeFilterRange(GetLocStr(), a_cond_vec, a_arg_vec));
  }
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
    const *a_transform, char const *a_fit, bool a_log_y, bool a_contour,
    double a_drop_counts_s, unsigned a_drop_counts_num, double a_drop_stats_s)
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

  if (a_drop_counts_s > 0.0 && a_drop_stats_s > 0.0) {
    std::cerr << a_title <<
        ": Can only drop one of counts and stats!\n";
    throw std::runtime_error(__func__);
  }
  if (a_drop_counts_num > 5) {
    std::cerr << a_title <<
        ": Cannot allow more than 5 drop-counts slices!\n";
    throw std::runtime_error(__func__);
  }
  // We must have at least 1 slice for the standard histos.
  a_drop_counts_num = std::max(a_drop_counts_num, 1U);

  auto node = new NodeHist1(GetLocStr(), a_title, a_x, a_xb,
      LinearTransform(k, m), a_fit, a_log_y, a_contour, a_drop_counts_s,
      a_drop_counts_num, a_drop_stats_s);
  NodeCuttableAdd(node);
}

void Config::AddHist2(char const *a_title, NodeValue *a_y, NodeValue *a_x,
    uint32_t a_yb, uint32_t a_xb, char const *a_transformy, char const
    *a_transformx, char const *a_fit, bool a_log_z, double a_drop_counts_s,
    unsigned a_drop_counts_num, double a_drop_stats_s)
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

  if (a_drop_counts_s > 0.0 && a_drop_stats_s > 0.0) {
    std::cerr << a_title <<
        ": Can only drop one of counts and stats!\n";
    throw std::runtime_error(__func__);
  }
  if (a_drop_counts_num > 5) {
    std::cerr << a_title <<
        ": Cannot allow more than 5 drop-counts slices!\n";
    throw std::runtime_error(__func__);
  }
  a_drop_counts_num = std::max(a_drop_counts_num, 1U);

  auto node = new NodeHist2(GetLocStr(), a_title, m_colormap, a_y, a_x,
      a_yb, a_xb, LinearTransform(ky, my), LinearTransform(kx, mx), a_fit,
      a_log_z, a_drop_counts_s, a_drop_counts_num, a_drop_stats_s);
  NodeCuttableAdd(node);
}

NodeValue *Config::AddLength(NodeValue *a_value)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_value;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeLength(GetLocStr(), a_value));
  }
  return node;
}

NodeValue *Config::AddMatchId(NodeValue *a_l, NodeValue *a_r)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_l << ',' << a_r;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeMatchId(GetLocStr(), a_l, a_r));
  }
  return node;
}

NodeValue *Config::AddMatchValue(NodeValue *a_l, NodeValue *a_r, double
    a_cutoff)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_l << ',' << a_r << ',' << a_cutoff;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key,
        node = new NodeMatchValue(GetLocStr(), a_l, a_r, a_cutoff));
  }
  return node;
}

NodeValue *Config::AddMax(NodeValue *a_value)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_value;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeMax(GetLocStr(), a_value));
  }
  return node;
}

NodeValue *Config::AddMeanArith(NodeValue *a_l, NodeValue *a_r)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_l << ',' << a_r;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeMeanArith(GetLocStr(), a_l, a_r));
  }
  return node;
}

NodeValue *Config::AddMeanGeom(NodeValue *a_l, NodeValue *a_r)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_l << ',' << a_r;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeMeanGeom(GetLocStr(), a_l, a_r));
  }
  return node;
}

NodeValue *Config::AddMember(NodeValue *a_node, char const *a_suffix)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_node << ',' << a_suffix;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeMember(GetLocStr(), a_node, a_suffix));
  }
  return node;
}

NodeValue *Config::AddMExpr(NodeValue *a_l, NodeValue *a_r, double a_d,
    NodeMExpr::Operation a_op)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_l << ',' << a_r << ',' << a_d << ',' << a_op;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeMExpr(GetLocStr(), a_l, a_r, a_d, a_op));
  }
  return node;
}

NodeValue *Config::AddPedestal(NodeValue *a_value, double a_cutoff, NodeValue
    *a_tpat)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_value << ',' << a_cutoff << ',' << a_tpat;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key,
        node = new NodePedestal(GetLocStr(), a_value, a_cutoff, a_tpat));
  }
  return node;
}

NodeValue *Config::AddSelectId(NodeValue *a_child, uint32_t a_first,
    uint32_t a_last)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_child << ',' << a_first << ',' << a_last;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key,
        node = new NodeSelectId(GetLocStr(), a_child, a_first, a_last));
  }
  return node;
}

void Config::AddSignal(char const *a_name, char const *a_id, char const
    *a_end, char const *a_v)
{
  m_signal_descr_map.insert(
      std::make_pair(a_name, Signal(GetLocStr(), a_name, a_id, a_end ? a_end :
      "", a_v)));
}

NodeValue *Config::AddSubMod(NodeValue *a_left, NodeValue *a_right, double
    a_range)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_left << ',' << a_right << ',' << a_range;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key,
        node = new NodeSubMod(GetLocStr(), a_left, a_right, a_range));
  }
  return node;
}

NodeValue *Config::AddTot(NodeValue *a_leading, NodeValue *a_trailing, double
    a_range)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_leading << ',' << a_trailing << ',' << a_range;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key,
        node = new NodeTot(GetLocStr(), a_leading, a_trailing, a_range));
  }
  return node;
}

NodeValue *Config::AddTpat(NodeValue *a_tpat, uint32_t a_mask)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_tpat << ',' << a_mask;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key, node = new NodeTpat(GetLocStr(), a_tpat, a_mask));
  }
  return node;
}

NodeValue *Config::AddTrigMap(char const *a_path, char const *a_prefix,
    NodeValue *a_left, NodeValue *a_right, double a_range)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_path << ',' << a_prefix << ',' << a_left <<
      ',' << a_right << ',' << a_range;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    auto prefix = m_trig_map.LoadPrefix(a_path, a_prefix);
    NodeValueAdd(key, node = new NodeTrigMap(GetLocStr(), prefix, a_left,
        a_right, a_range));
  }
  return node;
}

NodeValue *Config::AddZeroSuppress(NodeValue *a_value, double a_cutoff)
{
  std::ostringstream oss;
  oss << __LINE__ << ',' << a_value << ',' << a_cutoff;
  auto key = oss.str();
  auto node = NodeValueGet(key);
  if (!node) {
    NodeValueAdd(key,
        node = new NodeZeroSuppress(GetLocStr(), a_value, a_cutoff));
  }
  return node;
}

void Config::BindSignal(std::string const &a_name, NodeSignal::MemberType
    a_member_type, size_t a_id, Input::Type a_type)
{
  auto it = m_signal_map.find(a_name);
  if (m_signal_map.end() == it) {
    std::cerr << a_name <<
        ": Input told to bind, but config doesn't know?!\n";
    throw std::runtime_error(__func__);
  }
  auto signal = it->second;
  signal->BindSignal(a_name, a_member_type, a_id, a_type);
}

void Config::UnbindSignals()
{
  for (auto it = m_signal_map.begin(); m_signal_map.end() != it; ++it) {
    auto signal = it->second;
    signal->UnbindSignal();
  }
}

void Config::AppearanceSet(char const *a_name)
{
#if PLUTT_SDL2
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
#endif
}

void Config::ClockMatch(NodeValue *a_value, double a_s_from_ts)
{
  m_clock_match.node = a_value;
  m_clock_match.s_from_ts = a_s_from_ts;
}

void Config::ColormapSet(char const *a_name)
{
#if PLUTT_SDL2
  try {
    m_colormap = ImPlutt::ColormapGet(a_name);
  } catch (...) {
    std::cerr << GetLocStr() << ": Could not set palette.\n";
    throw std::runtime_error(__func__);
  }
#endif
}

void Config::HistCutAdd(CutPolygon *a_poly)
{
  m_cut_poly_list.push_back(a_poly);
}

unsigned Config::UIRateGet() const
{
  return m_ui_rate;
}

void Config::UIRateSet(unsigned a_ui_rate)
{
  m_ui_rate = std::min(a_ui_rate, DEFAULT_UI_RATE);
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

void Config::NodeValueAdd(std::string const &a_key, NodeValue *a_node)
{
  m_node_value_map.insert(std::make_pair(a_key, a_node));
}

NodeValue *Config::NodeValueGet(std::string const &a_key)
{
  auto it = m_node_value_map.find(a_key);
  if (m_node_value_map.end() == it) {
    return nullptr;
  }
  // std::cout << "Re-using node _" << a_key << "_\n";
  return it->second;
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
      // Init needed for some compilers.
      double dts = 0.0;
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
        case Input::kUint64:
          CLOCK_MATCH_PREP(u64);
          break;
        case Input::kInt64:
          CLOCK_MATCH_PREP(u64);
          break;
        case Input::kDouble:
          CLOCK_MATCH_PREP(dbl);
          break;
        case Input::kNone:
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

std::list<Config::Signal const *> Config::GetSignalList() const
{
  std::list<Signal const *> list;
  for (auto it = m_signal_descr_map.begin(); m_signal_descr_map.end() != it;
      ++it) {
    list.push_back(&it->second);
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
