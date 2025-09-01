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

#ifndef NODE_TPAT_HPP
#define NODE_TPAT_HPP

#include <node.hpp>

/*
 * If any requested bit is set, populate value with a u64 1.
 */
class NodeTpat: public NodeValue {
  public:
    static bool Test(Node *, NodeValue *);

    NodeTpat(std::string const &, NodeValue *, uint32_t);
    Value const &GetValue(uint32_t);
    void Process(uint64_t);

  private:
    NodeTpat(NodeTpat const &);
    NodeTpat &operator=(NodeTpat const &);

    NodeValue *m_tpat;
    uint32_t m_mask;
    Value m_value;
};

#endif
