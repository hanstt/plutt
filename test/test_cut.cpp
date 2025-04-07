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
#include <cut.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_cut_;

void MyTest::Run()
{
  // 1d.
  {
    CutPolygon c("c1", false);
    c.AddPoint(0);
    c.AddPoint(2);

    // Test outside.
    TEST_BOOL(!c.Test(-1));
    TEST_BOOL(!c.Test(3));

    // Test inside.
    TEST_BOOL(c.Test(1));

    // Same tests again should be the same.
    TEST_BOOL(!c.Test(-1));
    TEST_BOOL(!c.Test(3));
    TEST_BOOL(c.Test(1));

    // Check the boundaries.
    TEST_BOOL(!c.Test(2+1e-6));
    TEST_BOOL(c.Test(2-1e-6));

    TEST_BOOL(!c.Test(0-1e-6));
    TEST_BOOL(c.Test(0+1e-6));
  }

  // 2d.
  {
    // Square.
    CutPolygon c("c2", false);
    c.AddPoint(0.0, 0.0);
    c.AddPoint(1.0, 0.0);
    c.AddPoint(1.0, 1.0);
    c.AddPoint(0.0, 1.0);

    // Points far outside.
    double v[] = {0-1e-6, 0.0, 0.5, 1.0, 1+1e-6};
    for (int i = 0; i < 5; ++i) {
      TEST_BOOL(!c.Test(v[i], 0-1e-6));
      TEST_BOOL(!c.Test(v[i], 1+1e-6));
    }
    for (int i = 0; i < 5; ++i) {
      TEST_BOOL(!c.Test(0-1e-6, v[i]));
      TEST_BOOL(!c.Test(1+1e-6, v[i]));
    }

    // Points inside.
    TEST_BOOL(c.Test(0+1e-6, 0+1e-6));
    TEST_BOOL(c.Test(0+1e-6, 1-1e-6));
    TEST_BOOL(c.Test(1-1e-6, 0+1e-6));
    TEST_BOOL(c.Test(1-1e-6, 1-1e-6));
  }
  {
    // Diamond.
    CutPolygon c("c3", false);
    c.AddPoint( 0.0, -1.0);
    c.AddPoint(+1.0,  0.0);
    c.AddPoint( 0.0, +1.0);
    c.AddPoint(-1.0,  0.0);

    // Outside around the left.
    TEST_BOOL(!c.Test(-1.0, 1e-6));
    TEST_BOOL(!c.Test(-1.0-1e-6, 0.0));
    TEST_BOOL(!c.Test(-1.0, -1e-6));

    // Outside around the bottom.
    TEST_BOOL(!c.Test(-1e-6, -1.0));
    TEST_BOOL(!c.Test(0.0, -1.0-1e-6));
    TEST_BOOL(!c.Test(1e-6, -1.0));

    // Outside around the right.
    TEST_BOOL(!c.Test(1.0, 1e-6));
    TEST_BOOL(!c.Test(1.0+1e-6, 0.0));
    TEST_BOOL(!c.Test(1.0, -1e-6));

    // Outside around the top.
    TEST_BOOL(!c.Test(-1e-6, 1.0));
    TEST_BOOL(!c.Test(0.0, 1.0+1e-6));
    TEST_BOOL(!c.Test(1e-6, 1.0));

    // Points inside.
    TEST_BOOL(c.Test(0, 0));
    TEST_BOOL(c.Test(-1e-6, 0));
    TEST_BOOL(c.Test(0, -1e-6));
    TEST_BOOL(c.Test(1e-6, 0));
    TEST_BOOL(c.Test(0, 1e-6));
  }
}

}
