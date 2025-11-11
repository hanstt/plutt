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

#ifndef NODE_HIST1_HPP
#define NODE_HIST1_HPP

#include <fit.hpp>
#include <node.hpp>
#include <output.hpp>
#include <visual.hpp>

/*
 * Collects in a 1D histogram, actual histogramming is performed in visual.*.
 */
class NodeHist1: public NodeCuttable {
  public:
    NodeHist1(std::string const &, char const *, NodeValue *, uint32_t,
        LinearTransform const &, PeakFitVec const &, bool, bool, double,
        unsigned, double);
    void Process(uint64_t);

  private:
    NodeHist1(NodeHist1 const &);
    NodeHist1 &operator=(NodeHist1 const &);

    NodeValue *m_x;
    uint32_t m_xb;
    VisualHist m_visual_hist;
    Output::Var m_out;
};

#endif
