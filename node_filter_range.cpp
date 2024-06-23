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

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_filter_range.hpp>

NodeFilterRange::NodeFilterRange(std::string const &a_loc, CondVec const
    &a_cond_vec, std::vector<NodeValue *> const &a_src_vec):
  NodeValue(a_loc),
  m_cond_vec(a_cond_vec),
  m_arg_vec()
{
  for (auto it = a_src_vec.begin(); a_src_vec.end() != it; ++it) {
    m_arg_vec.push_back(Arg());
    auto &a = m_arg_vec.back();
    a.node = *it;
    a.value = new Value;
  }
}

Value const &NodeFilterRange::GetValue(uint32_t a_ret_i)
{
  return *m_arg_vec.at(a_ret_i).value;
}

void NodeFilterRange::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  for (auto it = m_cond_vec.begin(); m_cond_vec.end() != it; ++it) {
    NODE_PROCESS(it->node, a_evid);
  }
  for (auto it = m_arg_vec.begin(); m_arg_vec.end() != it; ++it) {
    NODE_PROCESS(it->node, a_evid);

    it->value->Clear();

    auto const &val = it->node->GetValue();
    it->value->SetType(val.GetType());
  }

  auto const &val0 = m_cond_vec.begin()->node->GetValue();
  auto const &miv0 = val0.GetMI();
  auto const &mev0 = val0.GetME();
  uint32_t vi = 0;
  for (uint32_t i = 0; i < miv0.size(); ++i) {
    auto mi0 = miv0[i];
    auto me0 = mev0[i];
    for (; vi < me0; ++vi) {
      bool ok = true;
      for (auto it = m_cond_vec.begin(); m_cond_vec.end() != it; ++it) {
        auto const &val = it->node->GetValue();
        auto const &miv = val.GetMI();
        auto const &mev = val.GetME();
        auto mi = miv.at(i);
        auto me = mev.at(i);
        NODE_ASSERT(mi, ==, mi0);
        NODE_ASSERT(me, ==, me0);
        // TODO: This double conversion should work the same for the values
        // and the limits, but should maybe do this properly?
        auto dbl = val.GetV(vi, false);
        ok &= it->lower_le ? it->lower <= dbl : it->lower < dbl;
        ok &= it->upper_le ? dbl <= it->upper : dbl < it->upper;
      }
      if (ok) {
        for (auto it = m_arg_vec.begin(); m_arg_vec.end() != it; ++it) {
          auto const &val = it->node->GetValue();
	  auto const &miv = val.GetMI();
	  // We _should_ have equal layout, but don't crash if we don't...
	  if (i < miv.size()) {
            auto mi = miv.at(i);
            NODE_ASSERT(mi, ==, mi0);
            auto const &s = val.GetV().at(vi);
            it->value->Push(mi, s);
	  }
        }
      }
    }
  }
}
