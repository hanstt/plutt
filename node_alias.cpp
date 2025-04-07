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
#include <node_alias.hpp>

NodeAlias::NodeAlias(std::string const &a_loc, NodeValue *a_source, uint32_t
    a_ret_i):
  NodeValue(a_loc),
  m_source(a_source),
  m_ret_i(a_ret_i)
{
}

NodeValue *NodeAlias::GetSource()
{
  return m_source;
}

Value const &NodeAlias::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_source->GetValue(m_ret_i);
}

void NodeAlias::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_source, a_evid);
}

void NodeAlias::SetSource(std::string const &a_loc, NodeValue *a_source)
{
  if (a_source) {
    if (m_source) {
      std::cerr << a_loc << ": Already aliased at " << GetLocStr() << "!\n";
      throw std::runtime_error(__func__);
    }
    m_source = a_source;
  }
}
