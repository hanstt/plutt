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
#include <string>
#include <vector>
#include <fit.hpp>

#if PLUTT_NLOPT

# include <nlopt.h>
# include <iostream>

# define BENCHMARK 0

namespace {
  struct Region {
    std::vector<uint32_t> const *vec;
    uint32_t left;
    uint32_t right;
    unsigned itn;
  };

  enum {
    OFS = 0,
    EXP_GAUSS_PHASE = 1,
    EXP_GAUSS_TAU,
    EXP_GAUSS_AMP,
    EXP_GAUSS_MEAN,
    EXP_GAUSS_WIDTH,
    GAUSS_AMP = 1,
    GAUSS_MEAN,
    GAUSS_WIDTH
  };

  double ExpGauss(unsigned a_n, double const *a_x, double *a_grad, void
      *a_data)
  {
    auto r = (Region *)a_data;
    /*
     * f(t) = x1 + exp[(t-x2) / x3] + x4 exp[-(t-x5)^2 / x6]
     * err = sum [f(i) - v_i]^2
     * derr/dx = 2 sum [f(i) - v_i]df(i)/dx
     * df(t)/dx1 = 1
     * df(t)/dx2 = -1/x3 exp[(t-x2) / x3]
     * df(t)/dx3 = -(t-x2)/x3^2 exp[(t-x2) / x3]
     * df(t)/dx4 = exp[-(t-x5)^2 / x6]
     * df(t)/dx5 = 2 x4 (t-x5) / x6 exp[-(t-x5)^2 / x6]
     * df(t)/dx6 = x4 (t-x5)^2 / x6^2 exp[-(t-x5)^2 / x6]
     */
    auto x1 = a_x[OFS];
    auto x2 = a_x[EXP_GAUSS_PHASE];
    auto x3 = a_x[EXP_GAUSS_TAU];
    auto x4 = a_x[EXP_GAUSS_AMP];
    auto x5 = a_x[EXP_GAUSS_MEAN];
    auto x6 = a_x[EXP_GAUSS_WIDTH];
    double sum = 0.0;
    if (a_grad) {
      a_grad[OFS            ] = 0.0;
      a_grad[EXP_GAUSS_PHASE] = 0.0;
      a_grad[EXP_GAUSS_TAU  ] = 0.0;
      a_grad[EXP_GAUSS_AMP  ] = 0.0;
      a_grad[EXP_GAUSS_MEAN ] = 0.0;
      a_grad[EXP_GAUSS_WIDTH] = 0.0;
    }
    for (uint32_t i = r->left; i <= r->right; ++i) {
      auto v_i = r->vec->at(i);
      auto dx1 = i - x2;
      double e1 = exp(dx1 / x3);
      auto dx2 = i - x5;
      double e2 = exp(-dx2*dx2 / x6);
      auto f_i = x1 + e1 + x4 * e2;
      auto dv = f_i - v_i;
      sum += dv * dv;
      if (a_grad) {
        dv *= 2;
        a_grad[OFS            ] += dv;
        a_grad[EXP_GAUSS_PHASE] += dv * -1/x3 * e1;
        a_grad[EXP_GAUSS_TAU  ] += dv * -dx1 / (x3*x3) * e1;
        a_grad[EXP_GAUSS_AMP  ] += dv * e2;
        a_grad[EXP_GAUSS_MEAN ] += dv * x4 * e2 * 2 * dx2 / x6;
        a_grad[EXP_GAUSS_WIDTH] += dv * x4 * e2 * dx2*dx2 / (x6*x6);
      }
    }
    ++r->itn;
    return sum;
  }

  double Gauss(unsigned a_n, double const *a_x, double *a_grad, void *a_data)
  {
    auto r = (Region *)a_data;
    /*
     * f(t) = x1 + x2 exp[-(t-x3)^2 / x4]
     * err = sum [f(i) - v_i]^2
     * derr/dx = 2 sum [f(i) - v_i]df(i)/dx
     * df(t)/dx1 = 1
     * df(t)/dx2 = exp[-(t-x3)^2 / x4]
     * df(t)/dx3 = 2 x2 (t-x3) / x4 exp[-(t-x3)^2 / x4]
     * df(t)/dx4 = x2 (t-x3)^2 / x4^2 exp[-(t-x3)^2 / x4]
     */
    auto x1 = a_x[OFS];
    auto x2 = a_x[GAUSS_AMP];
    auto x3 = a_x[GAUSS_MEAN];
    auto x4 = a_x[GAUSS_WIDTH];
    double sum = 0.0;
    if (a_grad) {
      a_grad[OFS        ] = 0.0;
      a_grad[GAUSS_AMP  ] = 0.0;
      a_grad[GAUSS_MEAN ] = 0.0;
      a_grad[GAUSS_WIDTH] = 0.0;
    }
    for (uint32_t i = r->left; i <= r->right; ++i) {
      auto v_i = r->vec->at(i);
      auto dx = i - x3;
      double e = exp(-dx*dx / x4);
      auto f_i = x1 + x2 * e;
      auto dv = f_i - v_i;
      sum += dv * dv;
      if (a_grad) {
        dv *= 2;
        a_grad[OFS        ] += dv;
        a_grad[GAUSS_AMP  ] += dv * e;
        a_grad[GAUSS_MEAN ] += dv * x2 * e * 2 * dx / x4;
        a_grad[GAUSS_WIDTH] += dv * x2 * e * dx*dx / (x4*x4);
      }
    }
    ++r->itn;
    return sum;
  }

}

#define NLOPT_CALL(func, args) do { \
  auto res_ = func args; \
  if (res_ < 0) { \
    auto str = nlopt_result_to_string(res_); \
    std::cerr << "nlopt failed: " << str << ".\n"; \
    throw std::runtime_error(__func__); \
  } \
} while (0)

/*
 * nlopt tests:
 *  solver             y       cnt  time
 *  ln_praxis          9.3283  2055 0.0022
 *  ld_tnewton         9.3283    58 0.0009
 *  ld_tnewton_precond 9.3283    33 0.0008
 *  ld_mma             9.85733  100 0.00018
 *  ln_newuoa          9.3283    33 0.0014
 *  ln_neldermead      9.33134  501 0.00055
 *  ln_sbplx           11.1854 1647 0.0017
 *  ld_ccsaq           9.33496  123 0.00021
 */

FitExpGauss::FitExpGauss(std::vector<uint32_t> const &a_hist, double a_max_y,
    uint32_t a_left, uint32_t a_right):
  m_y(),
  m_phase(),
  m_tau(),
  m_amp(),
  m_mean(),
  m_std()
{
  auto opt = nlopt_create(NLOPT_LD_MMA, 6);
  Region r;
  r.vec = &a_hist;
  r.left = a_left;
  r.right = a_right;
  r.itn = 0;
  NLOPT_CALL(nlopt_set_min_objective, (opt, ExpGauss, &r));
  NLOPT_CALL(nlopt_set_ftol_rel, (opt, 1e-5));
  double x[6];
  x[OFS            ] = 0.0;
  x[EXP_GAUSS_PHASE] = 0.0;
  x[EXP_GAUSS_TAU  ] = -1.0;
  x[EXP_GAUSS_AMP  ] = a_max_y / 2;
  x[EXP_GAUSS_MEAN ] = 0.5 * (a_left + a_right);
  x[EXP_GAUSS_WIDTH] = 0.5 * (a_right - a_left);
  double y;
#if BENCHMARK
  double t0;
  {
    timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    t0 = (double)tp.tv_sec + 1e-9 * (double)tp.tv_nsec;
  }
#endif
  NLOPT_CALL(nlopt_optimize, (opt, x, &y));
#if BENCHMARK
  double t1;
  {
    timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    t1 = (double)tp.tv_sec + 1e-9 * (double)tp.tv_nsec;
  }
  std::cout
      << y << ' '
      << r.itn << ' '
      << t1-t0 << ' '
      << '\n';
#endif
  nlopt_destroy(opt);
  m_y = x[OFS];
  m_phase = x[EXP_GAUSS_PHASE];
  m_tau = x[EXP_GAUSS_TAU];
  m_amp = x[EXP_GAUSS_AMP];
  m_mean = x[EXP_GAUSS_MEAN];
  m_std = sqrt(x[EXP_GAUSS_WIDTH] / 2);
}

FitGauss::FitGauss(std::vector<uint32_t> const &a_hist, double a_max_y,
    uint32_t a_left, uint32_t a_right):
  m_y(),
  m_amp(),
  m_mean(),
  m_std()
{
  auto opt = nlopt_create(NLOPT_LD_MMA, 4);
  Region r;
  r.vec = &a_hist;
  r.left = a_left;
  r.right = a_right;
  r.itn = 0;
  NLOPT_CALL(nlopt_set_min_objective, (opt, Gauss, &r));
  NLOPT_CALL(nlopt_set_ftol_rel, (opt, 1e-5));
  double x[4];
  x[OFS] = 0.0;
  x[GAUSS_AMP] = a_max_y;
  x[GAUSS_MEAN] = 0.5 * (a_left + a_right);
  x[GAUSS_WIDTH] = 0.5 * (a_right - a_left);
  double y;
#if BENCHMARK
  double t0;
  {
    timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    t0 = (double)tp.tv_sec + 1e-9 * (double)tp.tv_nsec;
  }
#endif
  NLOPT_CALL(nlopt_optimize, (opt, x, &y));
#if BENCHMARK
  double t1;
  {
    timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    t1 = (double)tp.tv_sec + 1e-9 * (double)tp.tv_nsec;
  }
  std::cout
      << y << ' '
      << r.itn << ' '
      << t1-t0 << ' '
      << '\n';
#endif
  nlopt_destroy(opt);
  m_y = x[OFS];
  m_amp = x[GAUSS_AMP];
  m_mean = x[GAUSS_MEAN];
  m_std = sqrt(x[GAUSS_WIDTH] / 2);
}

#else

FitExpGauss::FitExpGauss(std::vector<uint32_t> const &, double, uint32_t,
    uint32_t):
  m_y(),
  m_phase(),
  m_tau(),
  m_amp(),
  m_mean(),
  m_std()
{
}

FitGauss::FitGauss(std::vector<uint32_t> const &, double, uint32_t, uint32_t):
  m_y(),
  m_amp(),
  m_mean(),
  m_std()
{
}

#endif

FitExpGauss::~FitExpGauss()
{
}

double FitExpGauss::GetY() const { return m_y; }
double FitExpGauss::GetExpPhase() const { return m_phase; }
double FitExpGauss::GetExpTau() const { return m_tau; }
double FitExpGauss::GetGaussAmp() const { return m_amp; }
double FitExpGauss::GetGaussMean() const { return m_mean; }
double FitExpGauss::GetGaussStd() const { return m_std; }

FitGauss::~FitGauss()
{
}

double FitGauss::GetY() const { return m_y; }
double FitGauss::GetAmp() const { return m_amp; }
double FitGauss::GetMean() const { return m_mean; }
double FitGauss::GetStd() const { return m_std; }
