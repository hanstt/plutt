/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023  Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
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

#include <test/test.hpp>
#include <node_filter_range.hpp>
#include <test/mock_node.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_cluster_;

void ProcessExtraCond(MockNodeValue &a_nv)
{
  Value::Scalar s;

  s.u64 = 1;
  a_nv.m_value[0].Push(1, s);
  s.u64 = 2;
  a_nv.m_value[0].Push(3, s);
  s.u64 = 3;
  a_nv.m_value[0].Push(3, s);
}

void ProcessExtraArg(MockNodeValue &a_nv)
{
  Value::Scalar s;

  s.u64 = 4;
  a_nv.m_value[0].Push(1, s);
  s.u64 = 5;
  a_nv.m_value[0].Push(3, s);
  s.u64 = 6;
  a_nv.m_value[0].Push(3, s);
}

void MyTest::Run()
{
  {
    NodeFilterRange n("a", NodeFilterRange::CondVec(),
        std::vector<NodeValue *>());
    TestNodeBase(n, "a");
  }
  {
    MockNodeValue nv_cond(Value::kUint64, 1, ProcessExtraCond);
    MockNodeValue nv_arg(Value::kUint64, 1, ProcessExtraArg);

    NodeFilterRange::CondVec cv;
    cv.push_back(FilterRangeCond());
    auto &c = cv.back();
    c.node = &nv_cond;
    c.lower = 1;
    c.lower_le = 1;
    c.upper = 3;
    c.upper_le = 0;

    std::vector<NodeValue *> av;
    av.push_back(&nv_arg);

    NodeFilterRange n("", cv, av);

    auto const &x = n.GetValue(0);
    TEST_BOOL(x.GetV().empty());

    nv_cond.Preprocess(&n);
    nv_arg.Preprocess(&n);
    TestNodeProcess(n, 1);

    TEST_CMP(x.GetMI().size(), ==, 2U);
    TEST_CMP(x.GetME().size(), ==, 2U);
    TEST_CMP(x.GetV().size(), ==, 2U);

    TEST_CMP(x.GetMI()[0], ==, 1U);
    TEST_CMP(x.GetMI()[1], ==, 3U);

    TEST_CMP(x.GetME()[0], ==, 1U);
    TEST_CMP(x.GetME()[1], ==, 2U);

    TEST_CMP(x.GetV(0, false), ==, 4);
    TEST_CMP(x.GetV(1, false), ==, 5);
  }
}

}
