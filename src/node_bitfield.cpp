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

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_bitfield.hpp>

BitfieldArg::BitfieldArg(std::string const &a_loc, NodeValue *a_node, uint32_t
    a_bits):
  loc(a_loc),
  node(a_node),
  bits(a_bits),
  next()
{
}

NodeBitfield::Field::Field():
  node(),
  bits(),
  value(),
  i(),
  vi()
{
}

NodeBitfield::Field::Field(NodeValue *a_node, uint32_t a_bits):
  node(a_node),
  bits(a_bits),
  value(),
  i(),
  vi()
{
}

NodeBitfield::NodeBitfield(std::string const &a_loc, BitfieldArg *a_arg_list):
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
    m_source_vec.at(n) = Field(a_arg_list->node, a_arg_list->bits);
    delete a_arg_list;
    a_arg_list = next;
  }
}

Value const &NodeBitfield::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeBitfield::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  for (auto it = m_source_vec.begin(); m_source_vec.end() != it; ++it) {
    NODE_PROCESS(it->node, a_evid);
    it->value = &it->node->GetValue();
    if (Input::kUint64 != it->value->GetType()) {
      std::cerr << "Bitfield signals must have integer type!\n";
      throw std::runtime_error(__func__);
    }
    it->i = 0;
    it->vi = 0;
  }

  m_value.Clear();
  m_value.SetType(Input::kUint64);

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
    uint64_t val = 0;
    uint32_t ofs = 0;
    for (auto it = m_source_vec.begin(); m_source_vec.end() != it; ++it,
        ofs += it->bits) {
      auto const &vmi = it->value->GetID();
      if (it->i >= vmi.size()) {
        continue;
      }
      auto mi = vmi[it->i];
      if (mi != min_i) {
        continue;
      }
      auto me = it->value->GetEnd()[it->i];
      if (it->vi >= me) {
        continue;
      }
      auto const &vv = it->value->GetV();
      assert(it->vi < vv.size());
      uint64_t part = vv[it->vi].u64;
      auto mask = (1ULL << it->bits) - 1;
      if (part > mask) {
        std::cerr << it->node->GetLocStr() <<
            ": Value=" << std::hex << part <<
            " larger than # bits=" << std::dec << it->bits << "!\n";
        throw std::runtime_error(__func__);
      }
      val |= part << ofs;
      ++it->vi;
      if (it->vi == me) {
        ++it->i;
      }
    }
    Input::Scalar s;
    s.u64 = val;
    m_value.Push(min_i, s);
  }
}
