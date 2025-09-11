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
#include <node_tpat.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_tpat_;

class MockNode0: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode0)
    void ProcessUser()
    {
      Input::Scalar s;
      s.u64 = 0x8;
      m_value[0].Push(0, s);
    }
};
class MockNode1: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode1)
    void ProcessUser()
    {
      Input::Scalar s;
      s.u64 = 0x2;
      m_value[0].Push(0, s);
    }
};

void MyTest::Run()
{
  {
    MockNode0 nv(Input::kUint64, 1);
    NodeTpat n("", &nv, 0x6);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv.Preprocess(&n);
    TestNodeProcess(n, 1);

    TEST_CMP(v.GetID().size(), ==, 0UL);
    TEST_CMP(v.GetEnd().size(), ==, 0UL);
    TEST_CMP(v.GetV().size(), ==, 0UL);
  }
  {
    MockNode1 nv(Input::kUint64, 1);
    NodeTpat n("", &nv, 0x6);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv.Preprocess(&n);
    TestNodeProcess(n, 1);

    TEST_CMP(v.GetID().size(), ==, 1UL);
    TEST_CMP(v.GetID().at(0), ==, 0UL);
    TEST_CMP(v.GetEnd().size(), ==, 1UL);
    TEST_CMP(v.GetEnd().at(0), ==, 1UL);
    TEST_CMP(v.GetV().size(), ==, 1UL);
    TEST_CMP(v.GetV(0, true), ==, 1);
  }
}

}
