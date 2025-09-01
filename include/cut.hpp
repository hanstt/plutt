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

#ifndef CUT_HPP
#define CUT_HPP

#include <value.hpp>

class NodeCut;
class NodeCuttable;

struct NodeCutValue {
  NodeCutValue();
  Value x;
  Value y;
};

// Contains polygon vertices and tests if 1d/2d coords are inside.
class CutPolygon {
  public:
    struct Point {
      Point(double, double);
      double x;
      double y;
    };

    CutPolygon(char const *, bool);
    void AddPoint(double);
    void AddPoint(double, double);
    std::string const &GetTitle() const;
    bool Test(double) const;
    bool Test(double, double) const;

  private:
    std::string m_title;
    int m_dim;
    std::vector<Point> m_point_vec;
};

// Tests 1d/2d coords against a list of polygons.
class CutProducerList {
  public:
    CutProducerList();
    ~CutProducerList();
    NodeCutValue *AddData(CutPolygon const *);
    bool *AddEvent(CutPolygon const *);
    void Reset();
    void Test(Input::Type, Input::Scalar const &);
    void Test(Input::Type, Input::Scalar const &,
        Input::Type, Input::Scalar const &);

  private:
    typedef std::map<CutPolygon const *, bool> CutPolyMap;
    CutPolyMap::iterator AddCutPolygon(CutPolygon const *);
    struct EntryData {
      EntryData(CutPolyMap::iterator);
      CutPolyMap::iterator cut_poly_it;
      NodeCutValue value;
    };
    struct EntryEvent {
      EntryEvent(CutPolyMap::iterator);
      CutPolyMap::iterator cut_poly_it;
      bool is_ok;
    };
    CutPolyMap m_cut_poly_map;
    std::vector<EntryData *> m_cut_data_vec;
    std::vector<EntryEvent> m_cut_event_vec;
};

// Processes a list of cuttable nodes and points to their results.
class CutConsumerList {
  public:
    CutConsumerList();
    void Add(NodeCuttable *, bool *);
    bool IsOk() const;
    void Process(uint64_t);

  private:
    struct Entry {
      Entry(NodeCuttable *, bool *);
      NodeCuttable *node;
      bool *is_ok;
    };
    std::vector<Entry> m_cut_vec;
};

#endif
