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
#include <stdexcept>
#include <vector>
#include <node_signal_user.hpp>

NodeSignalUser::NodeSignalUser(std::string const &a_loc, char const *a_id,
    char const *a_end, char const *a_v):
  NodeValue(a_loc),
  m_id_name(a_id),
  m_end_name(a_end ? a_end : ""),
  m_v_name(a_v),
  m_id(),
  m_end(),
  m_v(),
  m_value()
{
}

std::string const &NodeSignalUser::GetID() const
{
  return m_id_name;
}

std::string const &NodeSignalUser::GetEnd() const
{
  return m_end_name;
}

std::string const &NodeSignalUser::GetV() const
{
  return m_v_name;
}

Value const &NodeSignalUser::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeSignalUser::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);

  if (!m_id || !m_v) {
    return;
  }

  NODE_PROCESS(m_id, a_evid);
  auto const &idval = m_id->GetValue();
  if (Input::kUint64 != idval.GetType() &&
      Input::kInt64 != idval.GetType()) {
    std::cerr << GetLocStr() << ": Invalid type for ID!\n";
    throw std::runtime_error(__func__);
  }
  auto const &idv = idval.GetV();

  NODE_PROCESS(m_v, a_evid);
  auto const &vval = m_v->GetValue();
  auto const &vv = vval.GetV();

  m_value.Clear();
  m_value.SetType(vval.GetType());

  if (m_end) {
    NODE_PROCESS(m_end, a_evid);
    auto const &endval = m_end->GetValue();
    if (Input::kUint64 != endval.GetType() &&
        Input::kInt64 != endval.GetType()) {
      std::cerr << GetLocStr() << ": Invalid type for end!\n";
      throw std::runtime_error(__func__);
    }
    auto const &endv = endval.GetV();
    if (idv.size() != endv.size()) {
      std::cerr << GetLocStr() << ": ID and end size mismatch!\n";
      throw std::runtime_error(__func__);
    }
    unsigned vi = 0;
    for (unsigned i = 0; i < idv.size(); ++i) {
      auto id = (uint32_t)idv.at(i).u64;
      auto end = (uint32_t)endv.at(i).u64;
      for (; vi < end; ++vi) {
        m_value.Push(id, vv.at(vi));
      }
    }
  } else {
    if (idv.size() != vv.size()) {
      std::cerr << GetLocStr() << ": ID and data size mismatch!\n";
      throw std::runtime_error(__func__);
    }
    for (unsigned i = 0; i < idv.size(); ++i) {
      auto id = (uint32_t)idv.at(i).u64;
      m_value.Push(id, vv.at(i));
    }
  }
}

void NodeSignalUser::SetSources(NodeValue *a_id, NodeValue *a_end, NodeValue
    *a_v)
{
  if (m_id || m_end || m_v) {
    std::cerr << "Already set at " << GetLocStr() << "!\n";
    throw std::runtime_error(__func__);
  }
  m_id = a_id;
  m_end = a_end;
  m_v = a_v;
}
