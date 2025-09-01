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
#include <node_tpat.hpp>

NodeTpat::NodeTpat(std::string const &a_loc, NodeValue *a_tpat, uint32_t
    a_mask):
  NodeValue(a_loc),
  m_tpat(a_tpat),
  m_mask(a_mask),
  m_value()
{
}

Value const &NodeTpat::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeTpat::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_tpat, a_evid);

  m_value.Clear();

  auto const &val = m_tpat->GetValue();
  NODE_ASSERT(val.GetType(), ==, Input::kUint64);
  m_value.SetType(Input::kUint64);

  if (val.GetV().empty()) {
    return;
  }
  NODE_ASSERT(val.GetV().size(), ==, 1U);
  if (val.GetV().at(0).u64 & m_mask) {
    Input::Scalar s;
    s.u64 = 1;
    m_value.Push(0, s);
  }
}

bool NodeTpat::Test(Node *a_parent, NodeValue *a_tpat)
{
  auto const &val = a_tpat->GetValue();
  auto const &v = val.GetV();
  if (v.empty()) {
    return false;
  }
  if (val.GetType() != Input::kUint64 ||
      v.size() != 1 ||
      1 != v.at(0).u64) {
    std::cerr <<
        a_parent->GetLocStr() << ": Given invalid tpat selector from " <<
        a_tpat->GetLocStr() << "!\n";
    throw std::runtime_error(__func__);
  }
  return true;
}
