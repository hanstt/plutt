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

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
#include <node_cluster.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_cluster_;

void ProcessExtra(MockNodeValue &a_nv)
{
  Input::Scalar s;

  s.u64 = 4;
  a_nv.m_value[0].Push(1, s);
  s.u64 = 5;
  a_nv.m_value[0].Push(2, s);
  s.u64 = 6;
  a_nv.m_value[0].Push(3, s);
  s.u64 = 7;
  a_nv.m_value[0].Push(5, s);
}

void MyTest::Run()
{
  {
    NodeCluster n("a", nullptr);
    TestNodeBase(n, "a");
  }
  {
    MockNodeValue nv(Input::kUint64, 1, ProcessExtra);

    NodeCluster n("", &nv);

    auto const &x = n.GetValue(0);
    TEST_BOOL(x.GetV().empty());
    auto const &e = n.GetValue(1);
    TEST_BOOL(e.GetV().empty());
    auto const &eta = n.GetValue(2);
    TEST_BOOL(eta.GetV().empty());

    nv.Preprocess(&n);
    TestNodeProcess(n, 1);

    TEST_CMP(x.GetV().size(), ==, 2U);
    TEST_CMP(e.GetV().size(), ==, 2U);
    TEST_CMP(eta.GetV().size(), ==, 2U);

    auto mid = (double)(1*4+2*5+3*6) / (4+5+6);

    TEST_CMP(x.GetV(0, false), ==, mid);
    TEST_CMP(x.GetV(1, false), ==, 5);

    TEST_CMP(e.GetV(0, false), ==, 15);
    TEST_CMP(e.GetV(1, false), ==, 7);

    TEST_CMP(eta.GetV(0, false), ==, mid - floor(mid));
    TEST_CMP(eta.GetV(1, false), ==, 0);
  }
}

}
