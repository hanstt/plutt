/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2025  Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
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

#ifndef NODE_MERGE_HPP
#define NODE_MERGE_HPP

#include <node.hpp>

/*
 * Merges data from the same channels.
 */

class NodeValue;

struct MergeArg {
  MergeArg(std::string const &, NodeValue *);
  std::string loc;
  NodeValue *node;
  MergeArg *next;
  private:
    MergeArg(MergeArg const &);
    MergeArg &operator=(MergeArg const &);
};

class NodeMerge: public NodeValue {
  public:
    NodeMerge(std::string const &, MergeArg *);
    Value const &GetValue(uint32_t);
    void Process(uint64_t);

  private:
    struct Field {
      Field();
      Field(NodeValue *);
      NodeValue *node;
      Value const *value;
      uint32_t i;
      uint32_t vi;
    };

    NodeMerge(NodeMerge const &);
    NodeMerge &operator=(NodeMerge const &);

    std::vector<Field> m_source_vec;
    Value m_value;
};

#endif
