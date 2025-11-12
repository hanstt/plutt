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

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <fit.hpp>
#include <node_mexpr.hpp>
#include <node_signal.hpp>
#include <trig_map.hpp>

struct BitfieldArg;
struct FilterRangeCond;
struct FilterRangeArg;
struct MergeArg;
class CutPolygon;
class Node;
class NodeAlias;
class NodeCut;
class NodeCuttable;
class NodeSignalUser;
class NodeValue;

/*
 * Config, ie node graph builder.
 */
class Config {
  public:
    Config(char const *, char const *);
    ~Config();

    NodeValue *AddAlias(char const *, NodeValue *, uint32_t);
    void AddAnnular(char const *, NodeValue *, double, double, NodeValue *,
        double, bool, double, unsigned, double);
    NodeValue *AddArray(NodeValue *, uint64_t, uint64_t = 0);
    NodeValue *AddBitfield(BitfieldArg *);
    NodeValue *AddCluster(NodeValue *);
    NodeValue *AddCoarseFine(NodeValue *, NodeValue *, double);
    NodeValue *AddCut(CutPolygon *);
    NodeValue *AddFilterRange(std::vector<FilterRangeCond> const &,
        std::vector<NodeValue *> const &);
    void AddFit(char const *, double, double);
    NodeValue *AddFloor(NodeValue *);
    void AddHist1(char const *, NodeValue *, uint32_t, char const *,
        PeakFitVec const &, bool, bool, double, unsigned, double);
    void AddHist2(char const *, NodeValue *, NodeValue *, uint32_t, uint32_t,
        char const *, char const *, bool, double, unsigned, double, double,
        bool);
    NodeValue *AddLength(NodeValue *);
    NodeValue *AddMatchId(NodeValue *, NodeValue *);
    NodeValue *AddMatchValue(NodeValue *, NodeValue *, double);
    NodeValue *AddMax(NodeValue *);
    NodeValue *AddMeanArith(NodeValue *, NodeValue *);
    NodeValue *AddMeanGeom(NodeValue *, NodeValue *);
    NodeValue *AddMember(NodeValue *, char const *);
    NodeValue *AddMExpr(NodeValue *, NodeValue *, double,
        NodeMExpr::Operation);
    NodeValue *AddMerge(MergeArg *);
    NodeValue *AddPedestal(NodeValue *, double, NodeValue *);
    NodeValue *AddSelectId(NodeValue *, uint32_t, uint32_t);
    NodeValue *AddSignalUser(NodeValue *, NodeValue *, NodeValue *);
    NodeValue *AddSubMod(NodeValue *, NodeValue *, double);
    NodeValue *AddTot(NodeValue *, NodeValue *, double);
    NodeValue *AddTpat(NodeValue *, uint32_t);
    NodeValue *AddTrigMap(char const *, char const *, NodeValue *, NodeValue
        *, double);
    NodeValue *AddZeroSuppress(NodeValue *, double);

    void AppearanceSet(char const *);
    void ClockMatch(NodeValue *, double);
    void ColormapSet(char const *);
    void HistCutAdd(CutPolygon *);
    unsigned UIRateGet() const;
    void UIRateSet(unsigned);

    std::string GetLocStr() const;
    void SetLoc(int, int);

    void BindSignal(std::string const &, NodeSignal::MemberType, size_t,
        Input::Type);
    void DoEvent(Input *);
    Input const *GetInput() const;
    Input *GetInput();
    std::list<std::string> GetSignalList() const;
    void UnbindSignals();

  private:
    Config(Config const &);
    Config &operator=(Config const &);
    void CutListBind(std::string const &);
    void DotAddLink(Node *, Node *, size_t = 0);
    void DotAddNode(Node *, std::string const &, std::vector<std::string>
        const & = {"in"});
    void NodeCutAdd(NodeCut *);
    void NodeCuttableAdd(NodeCuttable *);
    void NodeValueAdd(std::string const &, NodeValue *);
    NodeValue *NodeValueGet(std::string const &);

    struct FitEntry {
      double k;
      double m;
    };
    struct DotEntry {
      std::string name;
      std::vector<std::string> outs;
    };
    std::string m_path;
    std::map<Node *, DotEntry> m_dot_node_map;
    std::map<std::string, uintptr_t> m_dot_link_map;
    int m_line, m_col;
    TrigMap m_trig_map;
    // For de-duplicating nodes.
    // The key is "__LINE__,arg_0,...,arg_n".
    std::map<std::string, NodeValue *> m_node_value_map;
    // config_parser identifiers start out as unassigned aliases, are assigned
    // by assignment operations, and unassigned ones at the end are considered
    // signals that an Input must provide.
    std::map<std::string, NodeAlias *> m_alias_map;
    // User signals are assigned when all aliases are available.
    std::list<NodeSignalUser *> m_signal_user_list;
    // Unassigned aliases and signal descriptors are moved here.
    std::map<std::string, NodeSignal *> m_signal_map;
    // Cutting is special due to "soft" name-based dependencies.
    std::list<NodeCut *> m_cut_node_list;
    std::map<std::string, NodeCuttable *> m_cuttable_map;
    // Temps to keep cut-links to be resolved when all histograms exist.
    typedef std::list<CutPolygon *> CutPolyList;
    CutPolyList m_cut_poly_list;
    std::map<std::string, CutPolyList> m_cut_ref_map;
    std::map<std::string, FitEntry> m_fit_map;
    struct {
      NodeValue *node;
      double s_from_ts;
      Input::Scalar ts_prev;
      Input::Scalar ts0;
      double t0;
    } m_clock_match;
    size_t m_colormap;
    unsigned m_ui_rate;
    uint64_t m_evid;
    Input *m_input;
};

#endif
