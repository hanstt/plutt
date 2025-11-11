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
#include <fit.hpp>
#include <test/test.hpp>

#if PLUTT_NLOPT

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_fit_;

void MyTest::Run()
{
  {
    std::vector<uint32_t> v(100);
    double y = 1e3;
    double phase = 70.0;
    double tau = -10.0;
    double amp = 3e3;
    double mean = 40;
    double std = 10;
    double denom = 1 / (2 * std * std);
    for (uint32_t i = 0; i < v.size(); ++i) {
      auto d1 = i - phase;
      auto d2 = i - mean;
      v.at(i) = (uint32_t)(y + exp(d1 / tau) + amp * exp(-d2 * d2 * denom));
    }
    FitExpGauss f(v, y + amp, 0, (uint32_t)(v.size() - 1));
    TEST_CMP(std::abs(f.GetY()         -     y), <, std::abs(    y * 1e-2));
    TEST_CMP(std::abs(f.GetExpPhase()  - phase), <, std::abs(phase * 1e-2));
    TEST_CMP(std::abs(f.GetExpTau()    -   tau), <, std::abs(  tau * 1e-2));
    TEST_CMP(std::abs(f.GetGaussAmp()  -   amp), <, std::abs(  amp * 1e-2));
    TEST_CMP(std::abs(f.GetGaussMean() -  mean), <, std::abs( mean * 1e-2));
    TEST_CMP(std::abs(f.GetGaussStd()  -   std), <, std::abs(  std * 1e-2));
  }
  {
    std::vector<uint32_t> v(100);
    double y = 1e3;
    double amp = 3e3;
    double mean = 40;
    double std = 10;
    double denom = 1 / (2 * std * std);
    for (uint32_t i = 0; i < v.size(); ++i) {
      auto d = i - mean;
      v.at(i) = (uint32_t)(y + amp * exp(-d * d * denom));
    }
    FitGauss f(v, y + amp, 0, (uint32_t)(v.size() - 1));
    TEST_CMP(std::abs(f.GetY()    -    y), <, std::abs(   y * 1e-2));
    TEST_CMP(std::abs(f.GetAmp()  -  amp), <, std::abs(amp  * 1e-2));
    TEST_CMP(std::abs(f.GetMean() - mean), <, std::abs(mean * 1e-2));
    TEST_CMP(std::abs(f.GetStd()  -  std), <, std::abs( std * 1e-2));
  }
}

}

#endif
