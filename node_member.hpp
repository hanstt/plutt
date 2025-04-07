/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023, 2025
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

#ifndef NODE_MEMBER_HPP
#define NODE_MEMBER_HPP

#include <node.hpp>

/*
 * Transforms I or v into a flat Value, ie all indices = 0.
 */
class NodeMember: public NodeValue {
  public:
    NodeMember(std::string const &, NodeValue *, char const *);
    Value const &GetValue(uint32_t);
    void Process(uint64_t);

  private:
    NodeMember(NodeMember const &);
    NodeMember &operator=(NodeMember const &);

    Value m_value;
    NodeValue *m_child;
    bool m_is_id;
};

#endif
