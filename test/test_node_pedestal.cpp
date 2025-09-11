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
#include <node_pedestal.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_pedestal_;

class MockNode: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode)
    void ProcessUser()
    {
      m_value[0].Clear();
      Input::Scalar s;
      s.u64 = rand() & 7;
      m_value[0].Push(2, s);
      s.u64 = rand() & 15;
      m_value[0].Push(3, s);
    }
};

void MyTest::Run()
{
  MockNode nv(Input::kUint64, 1);
  NodePedestal n("", &nv, 0.1, nullptr);

  auto const &v = n.GetValue(0);
  TEST_BOOL(v.GetV().empty());
  auto const &s = n.GetValue(1);
  TEST_BOOL(s.GetV().empty());

  nv.Preprocess(&n);
  for (int i = 0; i < 11000; ++i) {
    TestNodeProcess(n, 1 + i);
  }

  TEST_CMP(s.GetID().size(), ==, 2UL);
  TEST_CMP(s.GetID().at(0), ==, 2UL);
  TEST_CMP(s.GetID().at(1), ==, 3UL);
  TEST_CMP(s.GetEnd().size(), ==, 2UL);
  TEST_CMP(s.GetEnd().at(0), ==, 1UL);
  TEST_CMP(s.GetEnd().at(1), ==, 2UL);
  TEST_CMP(s.GetV().size(), ==, 2UL);
  TEST_CMP(std::abs(s.GetV().at(0).dbl - 2.3), <, 1e-1);
  TEST_CMP(std::abs(s.GetV().at(1).dbl - 4.6), <, 1e-1);

  // TODO: Test tpat.
}

}
