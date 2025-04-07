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
#include <node_alias.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_alias_;

void ProcessExtra(MockNodeValue &a_nv)
{
  Input::Scalar s;
  s.u64 = 2;
  a_nv.m_value[0].Push(1, s);
  s.u64 = 3;
  a_nv.m_value[1].Push(1, s);
}

void MyTest::Run()
{
  {
    NodeAlias n("a", nullptr, 0);
    TestNodeBase(n, "a");
    TEST_BOOL(!n.GetSource());

    n.SetSource("", nullptr);
    TestNodeBase(n, "a");
    TEST_BOOL(!n.GetSource());

    MockNodeValue v(Input::kUint64, 1);
    n.SetSource("", &v);
    TEST_CMP(n.GetSource(), ==, &v);
    NodeAlias n2("", &v, 0);
    TEST_CMP(n.GetSource(), ==, &v);
  }
  {
    MockNodeValue nv(Input::kUint64, 2, ProcessExtra);
    NodeAlias n0("", &nv, 0);
    NodeAlias n1("", &nv, 1);

    auto const &v0 = n0.GetValue(0);
    TEST_BOOL(v0.GetV().empty());
    auto const &v1 = n1.GetValue(0);
    TEST_BOOL(v1.GetV().empty());

    nv.Preprocess(&n0);
    TestNodeProcess(n0, 1);
    nv.Preprocess(&n1);
    TestNodeProcess(n1, 1);
    TEST_CMP(v0.GetV(0, true), ==, 2.0);
    TEST_CMP(v1.GetV(0, true), ==, 3.0);
  }
}

}
