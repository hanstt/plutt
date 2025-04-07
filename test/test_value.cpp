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
#include <value.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_value_;

void MyTest::Run()
{
  // Scalars.
  {
    Input::Scalar s;
    s.u64 = 100;
    TEST_CMP(std::abs(s.GetDouble(Input::kUint64) - 100.0), <, 1e-9);
    s.dbl = 200.0;
    TEST_CMP(std::abs(s.GetDouble(Input::kDouble) - 200.0), <, 1e-9);
  }

  // Values.
  {
    Value v;
    TEST_CMP(v.GetType(), ==, Input::kNone);
    v.SetType(Input::kUint64);
    TEST_CMP(v.GetType(), ==, Input::kUint64);
    TEST_BOOL(v.GetID().empty());
    TEST_BOOL(v.GetEnd().empty());
    TEST_BOOL(v.GetV().empty());

    Input::Scalar s;
    s.u64 = 10;
    v.Push(1, s);
    TEST_CMP(v.GetID().size(), ==, 1U);
    TEST_CMP(v.GetEnd().size(), ==, 1U);
    TEST_CMP(v.GetV().size(), ==, 1U);
    TEST_CMP(v.GetID().at(0), ==, 1U);
    TEST_CMP(v.GetEnd().at(0), ==, 1U);
    TEST_CMP(v.GetV().at(0).u64, ==, 10U);

    s.u64 = 100;
    v.Push(1, s);
    TEST_CMP(v.GetID().size(), ==, 1U);
    TEST_CMP(v.GetEnd().size(), ==, 1U);
    TEST_CMP(v.GetV().size(), ==, 2U);
    TEST_CMP(v.GetID().at(0), ==, 1U);
    TEST_CMP(v.GetEnd().at(0), ==, 2U);
    TEST_CMP(v.GetV().at(0).u64, ==, 10U);
    TEST_CMP(v.GetV().at(1).u64, ==, 100U);

    s.u64 = 1000;
    v.Push(2, s);
    TEST_CMP(v.GetID().size(), ==, 2U);
    TEST_CMP(v.GetEnd().size(), ==, 2U);
    TEST_CMP(v.GetV().size(), ==, 3U);
    TEST_CMP(v.GetID().at(0), ==, 1U);
    TEST_CMP(v.GetID().at(1), ==, 2U);
    TEST_CMP(v.GetEnd().at(0), ==, 2U);
    TEST_CMP(v.GetEnd().at(1), ==, 3U);
    TEST_CMP(v.GetV().at(0).u64, ==, 10U);
    TEST_CMP(v.GetV().at(1).u64, ==, 100U);
    TEST_CMP(v.GetV().at(2).u64, ==, 1000U);

    v.Clear();
    TEST_CMP(v.GetType(), ==, Input::kUint64);
    TEST_BOOL(v.GetID().empty());
    TEST_BOOL(v.GetEnd().empty());
    TEST_BOOL(v.GetV().empty());
  }
}

}
