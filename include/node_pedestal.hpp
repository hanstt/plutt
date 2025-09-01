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

#ifndef NODE_PEDESTAL_HPP
#define NODE_PEDESTAL_HPP

#include <node.hpp>

/*
 * On-the-fly pedestal subtraction.
 */
class NodePedestal: public NodeValue {
  public:
    NodePedestal(std::string const &, NodeValue *, double, NodeValue *);
    Value const &GetValue(uint32_t);
    void Process(uint64_t);

  private:
    NodePedestal(NodePedestal const &);
    NodePedestal &operator=(NodePedestal const &);
    void StatsFit(uint32_t);

    struct StatPage {
      uint32_t num;
      double mean;
      double M2;
      double var;
      void Add(double);
    };
    struct Stats {
      StatPage page[2];
      size_t write_i;
      void Get(double *, double *);
    };
    NodeValue *m_child;
    double m_cutoff;
    NodeValue *m_tpat;
    Value m_value;
    Value m_sigma;
    std::vector<Stats> m_stats;
};

#endif
