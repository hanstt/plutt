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

#ifndef NODE_MEXPR_HPP
#define NODE_MEXPR_HPP

#include <node.hpp>

/*
 * Simple mathematical expressions of the form f(node, constant).
 */
class NodeMExpr: public NodeValue {
  public:
    enum Operation {
      ADD,
      SUB,
      MUL,
      DIV,
      COS,
      SIN,
      TAN,
      ACOS,
      ASIN,
      ATAN,
      SQRT,
      EXP,
      LOG,
      ABS,
      POW
    };
    NodeMExpr(std::string const &, NodeValue *, NodeValue *, double,
        Operation);
    Value const &GetValue(uint32_t);
    void Process(uint64_t);

  private:
    NodeMExpr(NodeMExpr const &);
    NodeMExpr &operator=(NodeMExpr const &);

    NodeValue *m_l;
    NodeValue *m_r;
    double m_d;
    int m_mix;
    Operation m_op;
    Value m_value;
};

#endif
