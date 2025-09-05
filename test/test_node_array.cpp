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
#include <node_array.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_array_;

void ProcessExtra(MockNodeValue &a_nv)
{
  Input::Scalar s;
  s.u64 = 2;
  a_nv.m_value[0].Push(1, s);
  s.u64 = 3;
  a_nv.m_value[0].Push(1, s);
  s.u64 = 4;
  a_nv.m_value[0].Push(3, s);
  s.u64 = 5;
  a_nv.m_value[0].Push(3, s);
}

void MyTest::Run()
{
  {
    MockNodeValue nv(Input::kUint64, 1, ProcessExtra);
    NodeArray n("", &nv, 0, 0);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv.Preprocess(&n);
    TestNodeProcess(n, 1);

    TEST_CMP(v.GetID().at(0), ==, 1UL);
    TEST_CMP(v.GetEnd().at(0), ==, 1UL);
    TEST_CMP(v.GetV(0, true), ==, 2);
  }
  {
    MockNodeValue nv(Input::kUint64, 1, ProcessExtra);
    NodeArray n("", &nv, 1, 1);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv.Preprocess(&n);
    TestNodeProcess(n, 1);

    TEST_CMP(v.GetID().at(0), ==, 3UL);
    TEST_CMP(v.GetEnd().at(0), ==, 1UL);
    TEST_CMP(v.GetV(0, true), ==, 5);
  }
}

}
