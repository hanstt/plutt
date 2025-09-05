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

#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_bitfield.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_bitfield_;

class MockNode0: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode0)
    void ProcessUser()
    {
      Input::Scalar s;
      s.u64 = 1;
      m_value[0].Push(1, s);
      s.u64 = 2;
      m_value[0].Push(2, s);
    }
};
class MockNode1: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode1)
    void ProcessUser()
    {
      Input::Scalar s;
      s.u64 = 3;
      m_value[0].Push(1, s);
      s.u64 = 4;
      m_value[0].Push(1, s);
      s.u64 = 5;
      m_value[0].Push(3, s);
    }
};

void MyTest::Run()
{
  {
    NodeBitfield n("a", nullptr);
    TestNodeBase(n, "a");
  }
  {
    MockNode0 nv0(Input::kUint64, 1);
    auto a0 = new BitfieldArg("a0", &nv0, 8);
    MockNode1 nv1(Input::kUint64, 1);
    auto a1 = new BitfieldArg("a1", &nv1, 8);
    // Parser builds arg-list in reverse order!
    a1->next = a0;

    NodeBitfield n("", a1);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    nv1.Preprocess(&n);
    TestNodeProcess(n, 1);
    TEST_CMP(v.GetV().at(0).u64, ==, 0x0301U);
    TEST_CMP(v.GetV().at(1).u64, ==, 0x0400U);
    TEST_CMP(v.GetV().at(2).u64, ==, 0x0002U);
    TEST_CMP(v.GetV().at(3).u64, ==, 0x0500U);
  }
}

}
