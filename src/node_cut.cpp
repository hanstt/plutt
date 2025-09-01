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
#include <node_cut.hpp>

NodeCut::NodeCut(std::string const &a_loc, CutPolygon *a_poly):
  NodeValue(a_loc),
  m_cut_poly(a_poly),
  m_cuttable(),
  m_value()
{
}

NodeCut::~NodeCut()
{
  delete m_cut_poly;
}

CutPolygon const &NodeCut::GetCutPolygon() const
{
  return *m_cut_poly;
}

Value const &NodeCut::GetValue(uint32_t a_ret_i)
{
  switch (a_ret_i) {
    case 0:
      return m_value->x;
    case 1:
      return m_value->y;
    default:
      throw std::runtime_error(__func__);
  }
}

void NodeCut::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  NODE_PROCESS(m_cuttable, a_evid);
}

void NodeCut::SetCuttable(NodeCuttable *a_node)
{
  m_cuttable = a_node;
  m_value = m_cuttable->CutDataAdd(m_cut_poly);
}
