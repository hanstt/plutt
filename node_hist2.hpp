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

#ifndef NODE_HIST2_HPP
#define NODE_HIST2_HPP

#include <node.hpp>
#include <visual.hpp>

/*
 * Collects a vs b in a 2D histogram, actual histogramming is performed in
 * visual.*.
 */
class NodeHist2: public NodeCuttable {
  public:
    NodeHist2(std::string const &, char const *, size_t, NodeValue *,
        NodeValue *, uint32_t, uint32_t, LinearTransform const &,
        LinearTransform const &, char const *, bool, double, unsigned,
        double);
    void Process(uint64_t);

  private:
    NodeHist2(NodeHist2 const &);
    NodeHist2 &operator=(NodeHist2 const &);

    NodeValue *m_x;
    NodeValue *m_y;
    uint32_t m_xb;
    uint32_t m_yb;
    VisualHist2 m_visual_hist2;
};

#endif
