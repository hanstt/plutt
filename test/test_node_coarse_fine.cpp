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

#include <iostream>
#include <map>
#include <vector>
#include <node_coarse_fine.hpp>
#include <test/mock_node.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_node_coarse_fine_;

uint32_t g_evid;

uint32_t EvidToFineRaw()
{
  // 2/3 fines -> 0, 1/3 fines -> 1.
  return 2 == g_evid % 3;
}

void ProcessExtraCoarse(MockNodeValue &a_nv)
{
  Input::Scalar s;
  s.u64 = g_evid;
  a_nv.m_value[0].Push(1, s);
}

void ProcessExtraFine(MockNodeValue &a_nv)
{
  Input::Scalar s;
  s.u64 = EvidToFineRaw();
  a_nv.m_value[0].Push(1, s);
}

void MyTest::Run()
{
  {
    NodeCoarseFine n("a", nullptr, nullptr, 0);
    TestNodeBase(n, "a");
  }
  {
    MockNodeValue nvc(Input::kUint64, 1, ProcessExtraCoarse);
    MockNodeValue nvf(Input::kUint64, 1, ProcessExtraFine);

#define PERIOD 10.0
    NodeCoarseFine n("", &nvc, &nvf, PERIOD);

    auto const &v = n.GetValue(0);
    TEST_BOOL(v.GetV().empty());

    nvc.Preprocess(&n);
    nvf.Preprocess(&n);

    for (g_evid = 1; g_evid <= 10; ++g_evid) {
      TestNodeProcess(n, g_evid);
      TEST_CMP(v.GetV().size(), ==, 1U);

      auto coarse = PERIOD * (g_evid + 1);
      TEST_CMP(std::abs(v.GetV(0, false) - coarse), <, 1e-6);
    }

    for (; g_evid <= 20; ++g_evid) {
      TestNodeProcess(n, g_evid);
      TEST_CMP(v.GetV().size(), ==, 1U);

      auto coarse = PERIOD * (g_evid + 1);
      auto fine = EvidToFineRaw();

      auto time = v.GetV(0, false);
      if (0 == fine) {
        TEST_CMP(time, >, coarse - 2*PERIOD/3 - 1e-3);
        TEST_CMP(time, <, coarse + 1e-3);
      } else {
        TEST_CMP(time, >, coarse - PERIOD - 1e-3);
        TEST_CMP(time, <, coarse - 2*PERIOD/3 + 1e-3);
      }
    }
  }
}

}
