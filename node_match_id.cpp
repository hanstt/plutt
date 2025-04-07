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
 * EndRCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_match_id.hpp>

NodeMatchId::NodeMatchId(std::string const &a_loc, NodeValue *a_l,
    NodeValue *a_r):
  NodeValue(a_loc),
  m_node_l(a_l),
  m_node_r(a_r),
  m_val_l(),
  m_val_r()
{
}

Value const &NodeMatchId::GetValue(uint32_t a_ret_i)
{
  switch (a_ret_i) {
    case 0:
      return m_val_l;
    case 1:
      return m_val_r;
    default:
      throw std::runtime_error(__func__);
  }
}

void NodeMatchId::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_node_l, a_evid);
  NODE_PROCESS(m_node_r, a_evid);

  m_val_l.Clear();
  m_val_r.Clear();

  auto const &val_l = m_node_l->GetValue();
  auto const &val_r = m_node_r->GetValue();
  m_val_l.SetType(val_l.GetType());
  m_val_r.SetType(val_r.GetType());

  uint32_t i_l = 0;
  uint32_t i_r = 0;
  while (i_l < val_l.GetID().size() && i_r < val_r.GetID().size()) {
    auto mi_l = val_l.GetID()[i_l];
    auto mi_r = val_r.GetID()[i_r];
    if (mi_l < mi_r) {
      ++i_l;
    } else if (mi_l > mi_r) {
      ++i_r;
    } else {
#define MATCH_INDEX_PUSH(sub) do {\
      auto me_0 = 0 == i_##sub ? 0 : val_##sub.GetEnd().at(i_##sub - 1);\
      auto me_1 = val_##sub.GetEnd().at(i_##sub);\
      for (; me_0 < me_1; ++me_0) {\
        m_val_##sub.Push(mi_##sub, val_##sub.GetV()[me_0]);\
      }\
    } while (0)
      MATCH_INDEX_PUSH(l);
      MATCH_INDEX_PUSH(r);

      ++i_l;
      ++i_r;
    }
  }
}
