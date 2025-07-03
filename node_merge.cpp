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

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
#include <node_merge.hpp>

MergeArg::MergeArg(std::string const &a_loc, NodeValue *a_node):
  loc(a_loc),
  node(a_node),
  next()
{
}

NodeMerge::Field::Field():
  node(),
  value(),
  i(),
  vi()
{
}

NodeMerge::Field::Field(NodeValue *a_node):
  node(a_node),
  value(),
  i(),
  vi()
{
}

NodeMerge::NodeMerge(std::string const &a_loc, MergeArg *a_arg_list):
  NodeValue(a_loc),
  m_source_vec(),
  m_value()
{
  // The parser built the arg list in reverse order!
  unsigned n = 0;
  for (auto p = a_arg_list; p; p = p->next) {
    ++n;
  }
  m_source_vec.resize(n);
  while (n-- > 0) {
    auto next = a_arg_list->next;
    m_source_vec.at(n) = Field(a_arg_list->node);
    delete a_arg_list;
    a_arg_list = next;
  }
}

Value const &NodeMerge::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeMerge::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  auto type = Input::kNone;
  for (auto it = m_source_vec.begin(); m_source_vec.end() != it; ++it) {
    NODE_PROCESS(it->node, a_evid);
    it->value = &it->node->GetValue();
    auto it_type = it->value->GetType();
    if (Input::kNone != type && Input::kNone != it_type && it_type != type) {
      std::cerr << GetLocStr() << ": Merging with different types (had=" <<
          type << " got=" << it_type << ")!\n";
      throw std::runtime_error(__func__);
    }
    type = it_type;
    it->i = 0;
    it->vi = 0;
  }

  m_value.Clear();
  if (Input::kNone == type) {
    return;
  }
  m_value.SetType(type);

  for (;;) {
    uint32_t min_i = UINT32_MAX;
    for (auto it = m_source_vec.begin(); m_source_vec.end() != it; ++it) {
      // Get min channel.
      auto const &vmi = it->value->GetID();
      if (it->i < vmi.size()) {
        auto mi = vmi[it->i];
        min_i = std::min(min_i, mi);
      }
    }
    if (UINT32_MAX == min_i) {
      break;
    }
    for (auto it = m_source_vec.begin(); m_source_vec.end() != it; ++it) {
      auto const &vmi = it->value->GetID();
      if (it->i >= vmi.size()) {
        continue;
      }
      auto mi = vmi[it->i];
      if (mi != min_i) {
        continue;
      }
      auto me = it->value->GetEnd()[it->i];
      auto const &vv = it->value->GetV();
      for (; it->vi < me; ++it->vi) {
        m_value.Push(mi, vv[it->vi]);
      }
      ++it->i;
    }
  }
}
