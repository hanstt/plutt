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

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_mexpr.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_mexpr_;

class MockNode0: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode0)
    void ProcessUser()
    {
      Input::Scalar s;
      s.u64 = 4;
      m_value[0].Push(2, s);
      s.u64 = 5;
      m_value[0].Push(3, s);
    }
};
class MockNode1: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode1)
    void ProcessUser()
    {
      Input::Scalar s;
      s.u64 = 6;
      m_value[0].Push(2, s);
      s.u64 = 7;
      m_value[0].Push(2, s);
      s.u64 = 8;
      m_value[0].Push(3, s);
    }
};
class MockNode2: public MockNodeValue {
  public:
    MOCK_NODE_VALUE(MockNode2)
    void ProcessUser()
    {
      Input::Scalar s;
      s.dbl = 0.1;
      m_value[0].Push(2, s);
      s.dbl = 0.3;
      m_value[0].Push(3, s);
      s.dbl = 1.1;
      m_value[0].Push(4, s);
    }
};

void MyTest::Run()
{
#define CHECK_SIZES1 do { \
    TEST_CMP(v.GetID().size(), ==, 2UL); \
    TEST_CMP(v.GetID().at(0), ==, 2UL); \
    TEST_CMP(v.GetID().at(1), ==, 3UL); \
    TEST_CMP(v.GetEnd().size(), ==, 2UL); \
    TEST_CMP(v.GetEnd().at(0), ==, 1UL); \
    TEST_CMP(v.GetEnd().at(1), ==, 2UL); \
    TEST_CMP(v.GetV().size(), ==, 2UL); \
} while(0)
#define CHECK_SIZES2 do { \
    TEST_CMP(v.GetID().size(), ==, 2UL); \
    TEST_CMP(v.GetID().at(0), ==, 2UL); \
    TEST_CMP(v.GetID().at(1), ==, 3UL); \
    TEST_CMP(v.GetEnd().size(), ==, 2UL); \
    TEST_CMP(v.GetEnd().at(0), ==, 1UL); \
    TEST_CMP(v.GetEnd().at(1), ==, 2UL); \
    TEST_CMP(v.GetV().size(), ==, 2UL); \
} while(0)
  /* ADD. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::ADD);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, 4 + 3.0);
    TEST_CMP(v.GetV(1, true), ==, 5 + 3.0);
  }
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", nullptr, &nv0, 3.0, NodeMExpr::ADD);
    auto const &v = n.GetValue(0);
    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);
    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, 3.0 + 4);
    TEST_CMP(v.GetV(1, true), ==, 3.0 + 5);
  }
  {
    MockNode0 nv0(Input::kUint64, 1);
    MockNode1 nv1(Input::kUint64, 1);
    NodeMExpr n("", &nv0, &nv1, 3.0, NodeMExpr::ADD);
    auto const &v = n.GetValue(0);
    nv0.Preprocess(&n);
    nv1.Preprocess(&n);
    TestNodeProcess(n, 1);
    CHECK_SIZES2;
    TEST_CMP(v.GetV(0, true), ==, 4 + 6);
    TEST_CMP(v.GetV(1, true), ==, 5 + 8);
  }

  /* SUB. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::SUB);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, 4 - 3.0);
    TEST_CMP(v.GetV(1, true), ==, 5 - 3.0);
  }
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", nullptr, &nv0, 3.0, NodeMExpr::SUB);
    auto const &v = n.GetValue(0);
    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);
    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, 3.0 - 4);
    TEST_CMP(v.GetV(1, true), ==, 3.0 - 5);
  }
  {
    MockNode0 nv0(Input::kUint64, 1);
    MockNode1 nv1(Input::kUint64, 1);
    NodeMExpr n("", &nv0, &nv1, 1.0, NodeMExpr::SUB);
    auto const &v = n.GetValue(0);
    nv0.Preprocess(&n);
    nv1.Preprocess(&n);
    TestNodeProcess(n, 1);
    CHECK_SIZES2;
    TEST_CMP(v.GetV(0, true), ==, 4 - 6);
    TEST_CMP(v.GetV(1, true), ==, 5 - 8);
  }

  /* MUL. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::MUL);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, 4 * 3);
    TEST_CMP(v.GetV(1, true), ==, 5 * 3);
  }
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", nullptr, &nv0, 3.0, NodeMExpr::MUL);
    auto const &v = n.GetValue(0);
    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);
    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, 3 * 4);
    TEST_CMP(v.GetV(1, true), ==, 3 * 5);
  }
  {
    MockNode0 nv0(Input::kUint64, 1);
    MockNode1 nv1(Input::kUint64, 1);
    NodeMExpr n("", &nv0, &nv1, 3.0, NodeMExpr::MUL);
    auto const &v = n.GetValue(0);
    nv0.Preprocess(&n);
    nv1.Preprocess(&n);
    TestNodeProcess(n, 1);
    CHECK_SIZES2;
    TEST_CMP(v.GetV(0, true), ==, 4 * 6);
    TEST_CMP(v.GetV(1, true), ==, 5 * 8);
  }

  /* DIV. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::DIV);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, 4.0 / 3.0);
    TEST_CMP(v.GetV(1, true), ==, 5.0 / 3.0);
  }
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", nullptr, &nv0, 3.0, NodeMExpr::DIV);
    auto const &v = n.GetValue(0);
    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);
    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, 3.0 / 4.0);
    TEST_CMP(v.GetV(1, true), ==, 3.0 / 5.0);
  }
  {
    MockNode0 nv0(Input::kUint64, 1);
    MockNode1 nv1(Input::kUint64, 1);
    NodeMExpr n("", &nv0, &nv1, 3.0, NodeMExpr::DIV);
    auto const &v = n.GetValue(0);
    nv0.Preprocess(&n);
    nv1.Preprocess(&n);
    TestNodeProcess(n, 1);
    CHECK_SIZES2;
    TEST_CMP(v.GetV(0, true), ==, 4.0 / 6.0);
    TEST_CMP(v.GetV(1, true), ==, 5.0 / 8.0);
  }

  /* COS. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::COS);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, cos(4.0));
    TEST_CMP(v.GetV(1, true), ==, cos(5.0));
  }

  /* SIN. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::SIN);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, sin(4.0));
    TEST_CMP(v.GetV(1, true), ==, sin(5.0));
  }

  /* TAN. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::TAN);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, tan(4.0));
    TEST_CMP(v.GetV(1, true), ==, tan(5.0));
  }

  /* ACOS. */
  {
    MockNode2 nv2(Input::kDouble, 1);
    NodeMExpr n("", &nv2, nullptr, 3.0, NodeMExpr::ACOS);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv2.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, acos(0.1));
    TEST_CMP(v.GetV(1, true), ==, acos(0.3));
  }

  /* ASIN. */
  {
    MockNode2 nv2(Input::kDouble, 1);
    NodeMExpr n("", &nv2, nullptr, 3.0, NodeMExpr::ASIN);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv2.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, asin(0.1));
    TEST_CMP(v.GetV(1, true), ==, asin(0.3));
  }

  /* ATAN. */
  {
    MockNode2 nv2(Input::kDouble, 1);
    NodeMExpr n("", &nv2, nullptr, 3.0, NodeMExpr::ATAN);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv2.Preprocess(&n);
    TestNodeProcess(n, 1);

    TEST_CMP(v.GetID().size(), ==, 3UL);
    TEST_CMP(v.GetID().at(0), ==, 2UL);
    TEST_CMP(v.GetID().at(1), ==, 3UL);
    TEST_CMP(v.GetID().at(2), ==, 4UL);
    TEST_CMP(v.GetEnd().size(), ==, 3UL);
    TEST_CMP(v.GetEnd().at(0), ==, 1UL);
    TEST_CMP(v.GetEnd().at(1), ==, 2UL);
    TEST_CMP(v.GetEnd().at(2), ==, 3UL);
    TEST_CMP(v.GetV().size(), ==, 3UL);
    TEST_CMP(v.GetV(0, true), ==, atan(0.1));
    TEST_CMP(v.GetV(1, true), ==, atan(0.3));
    TEST_CMP(v.GetV(2, true), ==, atan(1.1));
  }

  /* SQRT. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::SQRT);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, sqrt(4.0));
    TEST_CMP(v.GetV(1, true), ==, sqrt(5.0));
  }

  /* EXP. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::EXP);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, exp(4.0));
    TEST_CMP(v.GetV(1, true), ==, exp(5.0));
  }

  /* LOG. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::LOG);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, log(4.0));
    TEST_CMP(v.GetV(1, true), ==, log(5.0));
  }

  /* ABS. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    NodeMExpr n("", &nv0, nullptr, 3.0, NodeMExpr::ABS);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, abs(4.0));
    TEST_CMP(v.GetV(1, true), ==, abs(5.0));
  }

  /* POW. */
  {
    MockNode0 nv0(Input::kUint64, 1);
    MockNode1 nv1(Input::kUint64, 1);
    NodeMExpr n("", &nv0, &nv1, 3.0, NodeMExpr::POW);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nv0.Preprocess(&n);
    nv1.Preprocess(&n);
    TestNodeProcess(n, 1);

    CHECK_SIZES1;
    TEST_CMP(v.GetV(0, true), ==, pow(4.0, 6.0));
    TEST_CMP(v.GetV(1, true), ==, pow(5.0, 8.0));
  }
}

}
