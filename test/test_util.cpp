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
#include <vector>
#include <util.hpp>
#include <test/test.hpp>

namespace {

void test_line_clip()
{
  {
    int x1 = 1;
    int y1 = 1;
    int x2 = 9;
    int y2 = 9;
    auto ret = LineClip(0, 0, 10, 10, &x1, &y1, &x2, &y2);
    TEST_BOOL(ret);
    TEST_CMP(x1, ==, 1);
    TEST_CMP(y1, ==, 1);
    TEST_CMP(x2, ==, 9);
    TEST_CMP(y2, ==, 9);
  }
  {
    int x1 = -1;
    int y1 = -1;
    int x2 = 11;
    int y2 = 11;
    auto ret = LineClip(0, 0, 10, 10, &x1, &y1, &x2, &y2);
    TEST_BOOL(ret);
    TEST_CMP(x1, ==, 0);
    TEST_CMP(y1, ==, 0);
    TEST_CMP(x2, ==, 10);
    TEST_CMP(y2, ==, 10);
  }
  {
    int x1 = 1;
    int y1 = -1;
    int x2 = 9;
    int y2 = -1;
    auto ret = LineClip(0, 0, 10, 10, &x1, &y1, &x2, &y2);
    TEST_BOOL(!ret);
  }
  {
    int x1 = 1;
    int y1 = 11;
    int x2 = 9;
    int y2 = 11;
    auto ret = LineClip(0, 0, 10, 10, &x1, &y1, &x2, &y2);
    TEST_BOOL(!ret);
  }
  {
    int x1 = -1;
    int y1 = 1;
    int x2 = -1;
    int y2 = 9;
    auto ret = LineClip(0, 0, 10, 10, &x1, &y1, &x2, &y2);
    TEST_BOOL(!ret);
  }
  {
    int x1 = 11;
    int y1 = 1;
    int x2 = 11;
    int y2 = 9;
    auto ret = LineClip(0, 0, 10, 10, &x1, &y1, &x2, &y2);
    TEST_BOOL(!ret);
  }
}

void test_rebin1()
{
  {
    // Old bin fits completely inside new bin.
    std::vector<uint32_t> a(3);
    a.at(1) = 1;
    auto b = Rebin1(a,
        3, 1, 2,
        3, 0, 3);
    TEST_CMP(b.size(), ==, 3U);
    TEST_CMP(b.at(0), ==, 0U);
    TEST_CMP(b.at(1), ==, 1U);
    TEST_CMP(b.at(2), ==, 0U);
  }

  {
    // Old bin matches new bin.
    std::vector<uint32_t> a(3);
    a.at(0) = 1;
    a.at(1) = 2;
    a.at(2) = 3;
    auto b = Rebin1(a,
        3, 0, 3,
        3, 0, 3);
    TEST_CMP(b.size(), ==, 3U);
    TEST_CMP(b.at(0), ==, 1U);
    TEST_CMP(b.at(1), ==, 2U);
    TEST_CMP(b.at(2), ==, 3U);
  }

  {
    // Bins shifted.
    std::vector<uint32_t> a(3);
    a.at(0) = 1;
    a.at(1) = 10;
    a.at(2) = 100;
    auto b = Rebin1(a,
        3, 0, 3,
        3, 1, 4);
    TEST_CMP(b.size(), ==, 3U);
    TEST_CMP(b.at(0), ==,  10U);
    TEST_CMP(b.at(1), ==, 100U);
    TEST_CMP(b.at(2), ==,   0U);
  }

  {
    // Old bin straddles two new bins.
    std::vector<uint32_t> a(3);
    a.at(1) = 10;
    auto b = Rebin1(a,
        3,   0,   3,
        3, 0.5, 3.5);
    TEST_CMP(b.size(), ==, 3U);
    TEST_CMP(b.at(0), ==, 5U);
    TEST_CMP(b.at(1), ==, 5U);
    TEST_CMP(b.at(2), ==, 0U);
  }

  {
    // Old bin straddles three new bins.
    std::vector<uint32_t> a(3);
    a.at(1) = 10;
    auto b = Rebin1(a,
        3,   0,   3,
        3, 0.5, 2.5);
    TEST_CMP(b.size(), ==, 3U);
    TEST_CMP(b.at(0), ==, 2U); // 1/6 of 10 = 1.6.. ~= 2
    TEST_CMP(b.at(1), ==, 6U); // 4/5 of 8 = 6.4 ~= 6
    TEST_CMP(b.at(2), ==, 2U); // The remainder
  }
}

void test_rebin2()
{
  {
    // Old bin fits completely inside new bin.
    std::vector<uint32_t> a(3 * 3);
    a.at(4) = 10;
    auto b = Rebin2(a,
        3, 1, 2,  3, 1, 2,
        3, 0, 3,  3, 0, 3);
    TEST_CMP(b.size(), ==, 3U * 3U);
    TEST_CMP(b.at(0), ==,  0U);
    TEST_CMP(b.at(1), ==,  0U);
    TEST_CMP(b.at(2), ==,  0U);

    TEST_CMP(b.at(3), ==,  0U);
    TEST_CMP(b.at(4), ==, 10U);
    TEST_CMP(b.at(5), ==,  0U);

    TEST_CMP(b.at(6), ==,  0U);
    TEST_CMP(b.at(7), ==,  0U);
    TEST_CMP(b.at(8), ==,  0U);
  }

  {
    // Old bin matches new bin.
    std::vector<uint32_t> a(3 * 3);
    for (size_t i = 0; i < a.size(); ++i) {
      a.at(i) = (uint32_t)i + 1;
    }
    auto b = Rebin2(a,
        3, 0, 3,  3, 0, 3,
        3, 0, 3,  3, 0, 3);
    TEST_CMP(b.size(), ==, 3U * 3U);
    for (size_t i = 0; i < b.size(); ++i) {
      TEST_CMP(b.at(i), ==, i + 1);
    }
  }

  {
    // Bins shifted.
    std::vector<uint32_t> a(3 * 3);
    for (size_t i = 0; i < a.size(); ++i) {
      a.at(i) = (uint32_t)i + 1;
    }
    auto b = Rebin2(a,
        3, 0, 3,  3, 0, 3,
        3, 1, 4,  3, 1, 4);
    TEST_CMP(b.size(), ==, 3U * 3U);
    TEST_CMP(b.at(0), ==, 5U);
    TEST_CMP(b.at(1), ==, 6U);
    TEST_CMP(b.at(2), ==, 0U);

    TEST_CMP(b.at(3), ==, 8U);
    TEST_CMP(b.at(4), ==, 9U);
    TEST_CMP(b.at(5), ==, 0U);

    TEST_CMP(b.at(6), ==, 0U);
    TEST_CMP(b.at(7), ==, 0U);
    TEST_CMP(b.at(8), ==, 0U);
  }

  {
    // Old bin straddles two new bins.
    std::vector<uint32_t> a(3 * 3);
    a.at(0) = 4;
    a.at(8) = 4;
    auto b = Rebin2(a,
        3,   0,   3,  3,   0,   3,
        3, 0.5, 3.5,  3, 0.5, 3.5);
    TEST_CMP(b.size(), ==, 3U * 3U);
    TEST_CMP(b.at(0), ==,  1U);
    TEST_CMP(b.at(1), ==,  0U);
    TEST_CMP(b.at(2), ==,  0U);

    TEST_CMP(b.at(3), ==,  0U);
    TEST_CMP(b.at(4), ==,  1U);
    TEST_CMP(b.at(5), ==,  1U);

    TEST_CMP(b.at(6), ==,  0U);
    TEST_CMP(b.at(7), ==,  1U);
    TEST_CMP(b.at(8), ==,  1U);
  }

  {
    // Old bin straddles three new bins.
    std::vector<uint32_t> a(3 * 3);
    a.at(4) = 100;
    auto b = Rebin2(a,
        3,   0,   3,  3,   0,   3,
        3, 0.5, 2.5,  3, 0.5, 2.5);
    TEST_CMP(b.size(), ==, 3U * 3U);
    TEST_CMP(b.at(0), ==,  2U);
    TEST_CMP(b.at(1), ==, 11U);
    TEST_CMP(b.at(2), ==,  3U);

    TEST_CMP(b.at(3), ==, 11U);
    TEST_CMP(b.at(4), ==, 45U);
    TEST_CMP(b.at(5), ==, 11U);

    TEST_CMP(b.at(6), ==,  3U);
    TEST_CMP(b.at(7), ==, 11U);
    TEST_CMP(b.at(8), ==,  3U);
    uint32_t sum = 0;
    for (auto it = b.begin(); b.end() != it; ++it) {
      sum += *it;
    }
    TEST_CMP(sum, ==, 100U);
  }
}

void test_utf8()
{
  {
    char s[] = "ab";
    TEST_CMP( Utf8Left(s, 0), ==, -1);
    TEST_CMP( Utf8Left(s, 1), ==,  0);
    TEST_CMP( Utf8Left(s, 2), ==,  1);
    TEST_CMP(Utf8Right(s, 0), ==,  1);
    TEST_CMP(Utf8Right(s, 1), ==,  2);
    TEST_CMP(Utf8Right(s, 2), ==, -1);
  }
  {
    char s[] = {
      (char)0xc3, (char)0x84,
      (char)0xc3, (char)0x96,
      0};
    TEST_CMP( Utf8Left(s, 0), ==, -1);
    TEST_CMP( Utf8Left(s, 1), ==, -1);
    TEST_CMP( Utf8Left(s, 2), ==,  0);
    TEST_CMP( Utf8Left(s, 3), ==, -1);
    TEST_CMP( Utf8Left(s, 4), ==,  2);
    TEST_CMP(Utf8Right(s, 0), ==,  2);
    TEST_CMP(Utf8Right(s, 1), ==, -1);
    TEST_CMP(Utf8Right(s, 2), ==,  4);
    TEST_CMP(Utf8Right(s, 3), ==, -1);
    TEST_CMP(Utf8Right(s, 4), ==, -1);
  }
  {
    char s[] = {
      (char)0xe1, (char)0xb8, (char)0x8b,
      (char)0xe1, (char)0xb8, (char)0x92,
      0};
    TEST_CMP( Utf8Left(s, 0), ==, -1);
    TEST_CMP( Utf8Left(s, 1), ==, -1);
    TEST_CMP( Utf8Left(s, 2), ==, -1);
    TEST_CMP( Utf8Left(s, 3), ==,  0);
    TEST_CMP( Utf8Left(s, 4), ==, -1);
    TEST_CMP( Utf8Left(s, 5), ==, -1);
    TEST_CMP( Utf8Left(s, 6), ==,  3);
    TEST_CMP(Utf8Right(s, 0), ==,  3);
    TEST_CMP(Utf8Right(s, 1), ==, -1);
    TEST_CMP(Utf8Right(s, 2), ==, -1);
    TEST_CMP(Utf8Right(s, 3), ==,  6);
    TEST_CMP(Utf8Right(s, 4), ==, -1);
    TEST_CMP(Utf8Right(s, 5), ==, -1);
    TEST_CMP(Utf8Right(s, 6), ==, -1);
  }
  {
    char s[] = {
      (char)0xf0, (char)0x93, (char)0x82, (char)0xb3,
      (char)0xf0, (char)0x93, (char)0x82, (char)0xbb,
      0};
    TEST_CMP( Utf8Left(s, 0), ==, -1);
    TEST_CMP( Utf8Left(s, 1), ==, -1);
    TEST_CMP( Utf8Left(s, 2), ==, -1);
    TEST_CMP( Utf8Left(s, 3), ==, -1);
    TEST_CMP( Utf8Left(s, 4), ==,  0);
    TEST_CMP( Utf8Left(s, 5), ==, -1);
    TEST_CMP( Utf8Left(s, 6), ==, -1);
    TEST_CMP( Utf8Left(s, 7), ==, -1);
    TEST_CMP( Utf8Left(s, 8), ==,  4);
    TEST_CMP(Utf8Right(s, 0), ==,  4);
    TEST_CMP(Utf8Right(s, 1), ==, -1);
    TEST_CMP(Utf8Right(s, 2), ==, -1);
    TEST_CMP(Utf8Right(s, 3), ==, -1);
    TEST_CMP(Utf8Right(s, 4), ==,  8);
    TEST_CMP(Utf8Right(s, 5), ==, -1);
    TEST_CMP(Utf8Right(s, 6), ==, -1);
    TEST_CMP(Utf8Right(s, 7), ==, -1);
    TEST_CMP(Utf8Right(s, 8), ==, -1);
  }
}

class MyTest: public Test {
  void Run();
};
MyTest g_test_util_;

void MyTest::Run()
{
  test_line_clip();

  TEST_CMP(Log10Soft(0, -3), ==, -3);
  TEST_CMP(Log10Soft(1, -3), ==, 0);

  test_rebin1();
  test_rebin2();

  TEST_CMP(SubModDbl(0, 0, 8), ==, 0);

  TEST_CMP(SubModDbl(1, 0, 8), ==,  1);
  TEST_CMP(SubModDbl(2, 0, 8), ==,  2);
  TEST_CMP(SubModDbl(3, 0, 8), ==,  3);
  TEST_CMP(SubModDbl(4, 0, 8), ==, -4);
  TEST_CMP(SubModDbl(5, 0, 8), ==, -3);
  TEST_CMP(SubModDbl(6, 0, 8), ==, -2);
  TEST_CMP(SubModDbl(7, 0, 8), ==, -1);
  TEST_CMP(SubModDbl(8, 0, 8), ==,  0);

  TEST_CMP(SubModDbl(0, 1, 8), ==, -1);
  TEST_CMP(SubModDbl(0, 2, 8), ==, -2);
  TEST_CMP(SubModDbl(0, 3, 8), ==, -3);
  TEST_CMP(SubModDbl(0, 4, 8), ==, -4);
  TEST_CMP(SubModDbl(0, 5, 8), ==,  3);
  TEST_CMP(SubModDbl(0, 6, 8), ==,  2);
  TEST_CMP(SubModDbl(0, 7, 8), ==,  1);
  TEST_CMP(SubModDbl(0, 8, 8), ==,  0);

  TEST_CMP(SubModU64(0, 0, 8), ==, 0);

  TEST_CMP(SubModU64(1, 0, 8), ==,  1);
  TEST_CMP(SubModU64(2, 0, 8), ==,  2);
  TEST_CMP(SubModU64(3, 0, 8), ==,  3);
  TEST_CMP(SubModU64(4, 0, 8), ==, -4);
  TEST_CMP(SubModU64(5, 0, 8), ==, -3);
  TEST_CMP(SubModU64(6, 0, 8), ==, -2);
  TEST_CMP(SubModU64(7, 0, 8), ==, -1);
  TEST_CMP(SubModU64(8, 0, 8), ==,  0);

  TEST_CMP(SubModU64(0, 1, 8), ==, -1);
  TEST_CMP(SubModU64(0, 2, 8), ==, -2);
  TEST_CMP(SubModU64(0, 3, 8), ==, -3);
  TEST_CMP(SubModU64(0, 4, 8), ==, -4);
  TEST_CMP(SubModU64(0, 5, 8), ==,  3);
  TEST_CMP(SubModU64(0, 6, 8), ==,  2);
  TEST_CMP(SubModU64(0, 7, 8), ==,  1);
  TEST_CMP(SubModU64(0, 8, 8), ==,  0);

  Time_set_ms(0);
  TEST_CMP((int)Time_get_ms(), ==, 10 * 60 * 1000);
  Time_set_ms(1);
  TEST_CMP((int)Time_get_ms(), ==, 10 * 60 * 1000 + 1);

  test_utf8();
}

}
