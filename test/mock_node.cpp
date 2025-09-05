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
#include <test/mock_node.hpp>
#include <test/test.hpp>

MockNodeCuttable::MockNodeCuttable(std::string const &a_title, Input::Type
    a_type, unsigned a_val_num):
  NodeCuttable("", a_title),
  m_value(a_val_num),
  m_parent()
{
  for (auto it = m_value.begin(); m_value.end() != it; ++it) {
    it->SetType(a_type);
  }
}

Value const &MockNodeCuttable::GetValue(uint32_t a_ret_i)
{
  return m_value.at(a_ret_i);
}
void MockNodeCuttable::Preprocess(Node *a_parent)
{
  m_parent = a_parent;
}

void MockNodeCuttable::Process(uint64_t a_evid)
{
  assert(m_parent);
  TEST_BOOL(m_parent->IsActive());
  TEST_BOOL(m_parent->IsEvent(a_evid));
  for (auto it = m_value.begin(); m_value.end() != it; ++it) {
    it->Clear();
  }
  m_cut_consumer.Process(a_evid);
  if (!m_cut_consumer.IsOk()) {
    return;
  }
  ProcessUser( m_cut_producer);
}

void MockNodeCuttable::ProcessUser(CutProducerList &)
{
}

MockNodeValue::MockNodeValue(Input::Type a_type, unsigned a_val_num):
  NodeValue(""),
  m_value(a_val_num),
  m_parent()
{
  for (auto it = m_value.begin(); m_value.end() != it; ++it) {
    it->SetType(a_type);
  }
}

Value const &MockNodeValue::GetValue(uint32_t a_ret_i)
{
  return m_value.at(a_ret_i);
}

void MockNodeValue::Preprocess(Node *a_parent)
{
  m_parent = a_parent;
}

void MockNodeValue::Process(uint64_t a_evid)
{
  assert(m_parent);
  TEST_BOOL(m_parent->IsActive());
  TEST_BOOL(m_parent->IsEvent(a_evid));
  for (auto it = m_value.begin(); m_value.end() != it; ++it) {
    it->Clear();
  }
  ProcessUser();
}

void MockNodeValue::ProcessUser()
{
}

void TestNodeBase(Node &a_node, std::string const &a_loc)
{
  TEST_CMP(a_node.GetLocStr(), ==, a_loc);
  TEST_BOOL(!a_node.IsActive());
  TEST_BOOL(a_node.IsEvent(0));
  a_node.Process(0);
  TEST_BOOL(!a_node.IsActive());
  TEST_BOOL(a_node.IsEvent(0));
}

void TestNodeProcess(Node &a_node, uint64_t a_evid)
{
  TEST_BOOL(!a_node.IsActive());
  TEST_BOOL(!a_node.IsEvent(a_evid));
  a_node.Process(a_evid);
  TEST_BOOL(!a_node.IsActive());
  TEST_BOOL(a_node.IsEvent(a_evid));
}
