/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2025
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

#ifndef NODE_ANNULAR_HPP
#define NODE_ANNULAR_HPP

#include <node.hpp>
#include <visual.hpp>

/*
 * Collects r and phi in radial histogram, actual histogramming is performed
 * in visual.*.
 */
class NodeAnnular: public NodeCuttable {
  public:
    NodeAnnular(std::string const &, char const *, NodeValue *, double,
        double, NodeValue *, double, bool, double, unsigned, double);
    void Process(uint64_t);

  private:
    NodeAnnular(NodeAnnular const &);
    NodeAnnular &operator=(NodeAnnular const &);

    NodeValue *m_r;
    NodeValue *m_phi;
    double m_phi0;
    VisualAnnular m_visual_annular;
};

#endif
