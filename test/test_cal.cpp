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
#include <vector>
#include <cal.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_cal_;

void MyTest::Run()
{
  {
    // Single fine-time.
    CalFineTime c;
    for (uint32_t i = 0; i < 1000; ++i) {
      TEST_CMP(c.Get(0), <, 1e-6);
    }
  }

  {
    // Two fine-times at 3:1 occurrence.
    CalFineTime c;
    for (uint32_t i = 0; i < 1000; ++i) {
      c.Get((i & 3) / 3);
    }
    TEST_CMP(c.Get(0), <, 1e-6);
    TEST_CMP(std::abs(c.Get(1) - 0.75), <, 1 / (1000 / 2.));
  }

  {
    // Lots of uniform fine-times.
    CalFineTime c;
    for (uint32_t i = 0; i < 10000; ++i) {
      c.Get(i & 127);
    }
    for (uint32_t i = 0; i < 128; ++i) {
      TEST_CMP(std::abs(c.Get(i) - i/128.0), <, 1 / (10000 / 128.));
    }
  }
}

}
