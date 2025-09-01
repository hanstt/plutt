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

#ifndef NODE_TRIG_MAP_HPP
#define NODE_TRIG_MAP_HPP

#include <node.hpp>
#include <trig_map.hpp>

/*
 * First loads trigger map from file and gets mapping for prefix.
 * In runtime does "A[i] - B[prefix.map[i]]", ie subtracts mapped trigger
 * times.
 */
class NodeTrigMap: public NodeValue {
  public:
    NodeTrigMap(std::string const &, TrigMap::Prefix const *, NodeValue *,
        NodeValue *, double);
    Value const &GetValue(uint32_t);
    void Process(uint64_t);

  private:
    NodeTrigMap(NodeTrigMap const &);
    NodeTrigMap &operator=(NodeTrigMap const &);

    TrigMap::Prefix const *m_prefix;
    NodeValue *m_sig;
    NodeValue *m_trig;
    double m_range;
    Value m_value;
};

#endif
