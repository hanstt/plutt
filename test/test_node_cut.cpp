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
#include <vector>
#include <node_cut.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_cut_;

class MockNode: public MockNodeCuttable {
  public:
    MOCK_NODE_CUTTABLE(MockNode)
    void ProcessUser(CutProducerList &a_cut_producer)
    {
      Input::Scalar x, y;
      y.dbl = x.dbl = 1;
      a_cut_producer.Test(Input::kDouble, x, Input::kDouble, y);
      y.dbl = x.dbl = 11;
      a_cut_producer.Test(Input::kDouble, x, Input::kDouble, y);
      y.dbl = x.dbl = 21;
      a_cut_producer.Test(Input::kDouble, x, Input::kDouble, y);
    }
};

void MyTest::Run()
{
  {
    auto p = new CutPolygon("test/poly.txt", true);
    NodeCut n("a", p);
    TestNodeBase(n, "a");
  }
  {
    MockNode nc("MockCuttable", Input::kUint64, 1);
    TEST_CMP(nc.GetTitle().compare("MockCuttable"), ==, 0);

    auto p = new CutPolygon("test/poly.txt", true);
    NodeCut n("", p);

    n.SetCuttable(&nc);
    nc.Preprocess(&n);

    TestNodeProcess(n, 1);

    auto const &v0 = n.GetValue(0);
    TEST_CMP(v0.GetV().size(), ==, 1U);
    TEST_CMP(std::abs(v0.GetV(0, false) - 11), <, 1e-6);

    auto const &v1 = n.GetValue(1);
    TEST_CMP(v1.GetV().size(), ==, 1U);
    TEST_CMP(std::abs(v1.GetV(0, false) - 11), <, 1e-6);
  }
  {
    MockNode nc("MockCuttable", Input::kUint64, 1);
    TEST_CMP(nc.GetTitle().compare("MockCuttable"), ==, 0);

    auto p = new CutPolygon("some_title", false);
    p->AddPoint(20, 20);
    p->AddPoint(30, 20);
    p->AddPoint(30, 30);
    p->AddPoint(20, 30);
    NodeCut n("", p);

    n.SetCuttable(&nc);
    nc.Preprocess(&n);

    TestNodeProcess(n, 1);

    auto const &v0 = n.GetValue(0);
    TEST_CMP(v0.GetV().size(), ==, 1U);
    TEST_CMP(std::abs(v0.GetV(0, false) - 21), <, 1e-6);

    auto const &v1 = n.GetValue(1);
    TEST_CMP(v1.GetV().size(), ==, 1U);
    TEST_CMP(std::abs(v1.GetV(0, false) - 21), <, 1e-6);
  }
}

}
