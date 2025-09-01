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

#ifndef NODE_CUT_HPP
#define NODE_CUT_HPP

#include <node.hpp>

class CutPolygon;
struct NodeCutValue;

/*
 * Histogram cut pushes data to this node.
 */
class NodeCut: public NodeValue {
  public:
    NodeCut(std::string const &, CutPolygon *);
    ~NodeCut();
    CutPolygon const &GetCutPolygon() const;
    Value const &GetValue(uint32_t);
    void Process(uint64_t);
    void SetCuttable(NodeCuttable *);

  private:
    NodeCut(NodeCut const &);
    NodeCut &operator=(NodeCut const &);

    CutPolygon *m_cut_poly;
    NodeCuttable *m_cuttable;
    NodeCutValue *m_value;
};

#endif
