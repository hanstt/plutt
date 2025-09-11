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

#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_sub_mod.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_sub_mod_;

class MockNode0: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode0)
    void ProcessUser()
    {
      Input::Scalar s;
      s.u64 = 2;
      m_value[0].Push(2, s);
      s.u64 = 30;
      m_value[0].Push(3, s);
    }
};
class MockNode1: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode1)
    void ProcessUser()
    {
      Input::Scalar s;
      s.u64 = 1;
      m_value[0].Push(2, s);
      s.u64 = 2;
      m_value[0].Push(3, s);
      s.u64 = 30;
      m_value[0].Push(3, s);
    }
};

void MyTest::Run()
{
  MockNode0 nv0(Input::kUint64, 1);
  MockNode1 nv1(Input::kUint64, 1);
  NodeSubMod n("", &nv0, &nv1, 20);

  auto const &v = n.GetValue(0);
  TEST_BOOL(v.GetV().empty());

  nv0.Preprocess(&n);
  nv1.Preprocess(&n);
  TestNodeProcess(n, 1);

  TEST_CMP(v.GetID().size(), ==, 2UL);
  TEST_CMP(v.GetID().at(0), ==, 2UL);
  TEST_CMP(v.GetID().at(1), ==, 3UL);
  TEST_CMP(v.GetEnd().size(), ==, 2UL);
  TEST_CMP(v.GetEnd().at(0), ==, 1UL);
  TEST_CMP(v.GetEnd().at(1), ==, 2UL);
  TEST_CMP(v.GetV().size(), ==, 2UL);
  TEST_CMP(v.GetV(0, true), ==, 1);
  TEST_CMP(v.GetV(1, true), ==, 8);
}

}
