/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023, 2024
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

#include <sys/select.h>
#include <dirent.h>
#include <err.h>

#include <cmath>
#include <cassert>
#include <cstdarg>
#include <mutex>
#include <vector>

#include <util.hpp>

namespace {
  uint64_t g_time_ms;
}

LinearTransform::LinearTransform(double a_k, double a_m):
  m_k(a_k),
  m_m(a_m)
{
}

double LinearTransform::ApplyAbs(double a_v) const
{
  return a_v * m_k + m_m;
}

double LinearTransform::ApplyRel(double a_v) const
{
  return a_v * m_k;
}

void FitLinear(double const *a_x, size_t a_xs, double const *a_y, size_t a_ys,
    size_t a_n, double *a_k, double *a_m)
{
  auto xp = (uint8_t const *)a_x;
  auto yp = (uint8_t const *)a_y;
  double sum_x = 0.0;
  double sum_x2 = 0.0;
  double sum_y = 0.0;
  double sum_xy = 0.0;
  for (size_t i = 0; i < a_n; ++i) {
    auto x = *(double const *)xp;
    auto y = *(double const *)yp;
    sum_x += x;
    sum_x2 += x * x;
    sum_y += y;
    sum_xy += x * y;
    xp += a_xs;
    yp += a_ys;
  }
  double d = (double)a_n * sum_x2 - sum_x * sum_x;
  *a_k = ((double)a_n * sum_xy - sum_x * sum_y) / d;
  *a_m = (sum_y - *a_k * sum_x) / (double)a_n;
}

bool LineClip(
    int a_r_x1, int a_r_y1, int a_r_x2, int a_r_y2,
    int *a_x1, int *a_y1, int *a_x2, int *a_y2)
{
  auto p1 = (float)-(*a_x2 - *a_x1);
  auto p2 = -p1;
  auto p3 = (float)-(*a_y2 - *a_y1);
  auto p4 = -p3;

  auto q1 = (float)(*a_x1 - a_r_x1);
  auto q2 = (float)(a_r_x2 - *a_x1);
  auto q3 = (float)(*a_y1 - a_r_y1);
  auto q4 = (float)(a_r_y2 - *a_y1);

  float posarr[5], negarr[5];
  int posind = 1, negind = 1;

  posarr[0] = 1;
  negarr[0] = 0;

  if ((p1 == 0 && q1 < 0) ||
      (p2 == 0 && q2 < 0) ||
      (p3 == 0 && q3 < 0) ||
      (p4 == 0 && q4 < 0)) {
      return false;
  }
  if (p1 != 0) {
    float r1 = q1 / p1;
    float r2 = q2 / p2;
    if (p1 < 0) {
      negarr[negind++] = r1;
      posarr[posind++] = r2;
    } else {
      negarr[negind++] = r2;
      posarr[posind++] = r1;
    }
  }
  if (p3 != 0) {
    float r3 = q3 / p3;
    float r4 = q4 / p4;
    if (p3 < 0) {
      negarr[negind++] = r3;
      posarr[posind++] = r4;
    } else {
      negarr[negind++] = r4;
      posarr[posind++] = r3;
    }
  }

  float rn1 = negarr[0];
  for (int i = 1; i < negind; ++i) {
    rn1 = std::max(rn1, negarr[i]);
  }
  float rn2 = posarr[0];
  for (int i = 1; i < posind; ++i) {
    rn2 = std::min(rn2, posarr[i]);
  }

  if (rn1 > rn2)  {
    return false;
  }

  *a_x2 = (int)((float)*a_x1 + p2 * rn2);
  *a_y2 = (int)((float)*a_y1 + p4 * rn2);
  *a_x1 += (int)(p2 * rn1);
  *a_y1 += (int)(p4 * rn1);
  return true;
}

double Log10Soft(double a_v, double a_exp_min)
{
  return a_v > pow(10, a_exp_min) ? log10(a_v) : a_exp_min;
}

std::vector<uint32_t> Rebin1(std::vector<uint32_t> const &a_hist,
    size_t a_bins_old, double a_min_old, double a_max_old,
    size_t a_bins_new, double a_min_new, double a_max_new)
{
  assert(a_hist.size() == a_bins_old);
  std::vector<uint32_t> nh(a_bins_new);
  auto x_from_i = (a_max_old - a_min_old) / (double)a_bins_old;
  auto ni_from_x = (double)a_bins_new / (a_max_new - a_min_new);
  for (size_t i = 0; i < a_bins_old; ++i) {
    double l = a_min_old + x_from_i * (double)(i + 0);
    double r = a_min_old + x_from_i * (double)(i + 1);

    double f_l = ni_from_x * (l - a_min_new);
    double f_r = ni_from_x * (r - a_min_new);

    auto i_l = (int)floor(f_l);
    auto i_r = (int)ceil(f_r - 1);
    i_r = std::min(i_r, (int)a_bins_new);

    // TODO: This is so nasty, can it be optimized?
    auto v = a_hist.at(i);
    // Spread 'v' to all but the last destination.
    if (i_l < 0) {
      // Fast-forward to x=0.
      v = (uint32_t)(v * (1 - -f_l / (f_r - f_l)));
      f_l = 0;
      i_l = 0;
    }
    while (i_l < i_r) {
      auto n = floor(f_l + 1);
      auto s = (n - f_l) / (f_r - f_l);
      uint32_t dv = (uint32_t)(v * s + 0.5);
      nh.at((size_t)i_l) += dv;
      v -= dv;
      f_l = n;
      ++i_l;
    }
    // Then dump the rest at the end, this way we don't lose any count.
    // TODO: Better tactic, eg pick random bin?
    if (i_l < (int)a_bins_new) {
      nh.at((size_t)i_l) += v;
    }
  }
  return nh;
}

std::vector<uint32_t> Rebin2(std::vector<uint32_t> const &a_hist,
    size_t a_binsx_old, double a_minx_old, double a_maxx_old,
    size_t a_binsy_old, double a_miny_old, double a_maxy_old,
    size_t a_binsx_new, double a_minx_new, double a_maxx_new,
    size_t a_binsy_new, double a_miny_new, double a_maxy_new)
{
  assert(a_hist.size() == a_binsx_old * a_binsy_old);
  std::vector<uint32_t> nh(a_binsx_new * a_binsy_new);
  auto x_from_j = (a_maxx_old - a_minx_old) / (double)a_binsx_old;
  auto nj_from_x = (double)a_binsx_new / (a_maxx_new - a_minx_new);
  auto y_from_i = (a_maxy_old - a_miny_old) / (double)a_binsy_old;
  auto ni_from_y = (double)a_binsy_new / (a_maxy_new - a_miny_new);
  for (size_t i = 0; i < a_binsy_old; ++i) {
    double y_l = a_miny_old + y_from_i * (double)(i + 0);
    double y_r = a_miny_old + y_from_i * (double)(i + 1);

    double fy_l = ni_from_y * (y_l - a_miny_new);
    double fy_r = ni_from_y * (y_r - a_miny_new);

    auto i_l = (int)floor(fy_l);
    auto i_r = (int)ceil(fy_r - 1);
    i_r = std::min(i_r, (int)a_binsy_new - 1);

    for (size_t j = 0; j < a_binsx_old; ++j) {
      double x_l = a_minx_old + x_from_j * (double)(j + 0);
      double x_r = a_minx_old + x_from_j * (double)(j + 1);

      double fx_l = nj_from_x * (x_l - a_minx_new);
      double fx_r = nj_from_x * (x_r - a_minx_new);

      auto j_l = (int)floor(fx_l);
      auto j_r = (int)ceil(fx_r - 1);
      j_r = std::min(j_r, (int)a_binsx_new - 1);

      // TODO: This whole thing is so nasty, can it be optimized?
      auto v = (double)a_hist.at(i * a_binsx_old + j);
      if (i_l < 0) {
        // Fast-forward to y=0.
        v = (uint32_t)(v * (1 - -fy_l / (fy_r - fy_l)));
        fy_l = 0;
        i_l = 0;
      }
      if (j_l < 0) {
        // Fast-forward to y=0.
        v = (uint32_t)(v * (1 - -fx_l / (fx_r - fx_l)));
        fx_l = 0;
        j_l = 0;
      }
      // Used to keep track of rounding errors.
      double vs = 0.0;
      double ivs = 0.0;
      int last_i = -1;
      int last_j = -1;
      // Spread the source cell over destination cells.
      double fy = fy_l;
      for (auto ii = i_l; ii <= i_r; ++ii) {
        auto ny = floor(fy + 1);
        ny = std::min(ny, fy_r);
        auto sy = (ny - fy) / (fy_r - fy_l);
        auto ofs = (size_t)(ii * (int)a_binsx_new + j_l);
        double fx = fx_l;
        for (auto jj = j_l; jj <= j_r; ++jj) {
          auto nx = floor(fx + 1);
          nx = std::min(nx, fx_r);
          auto sx = (nx - fx) / (fx_r - fx_l);
          auto dv = v * sy * sx;
          auto idv = (uint32_t)dv;
          vs += dv;
          ivs += idv;
          if (vs + 1 > ivs) {
            // If the integer deltas lose counts, recover.
            auto d = floor(vs) - floor(ivs);
            idv += (uint32_t)d;
            ivs += d;
          }
          assert(ii >= 0);
          assert(ii < (int)a_binsy_new);
          assert(jj >= 0);
          assert(jj < (int)a_binsx_new);
          nh.at(ofs) += idv;
          last_i = ii;
          last_j = jj;
          ++ofs;
          fx = nx;
        }
        fy = ny;
      }
      if (last_i >= 0 && last_i < (int)a_binsy_new &&
          last_j >= 0 && last_j < (int)a_binsx_new) {
        nh.at((size_t)last_i * a_binsx_new + (size_t)last_j) +=
            (uint32_t)(v - ivs);
      }
    }
  }
  return nh;
}

std::vector<float> Snip(std::vector<uint32_t> const &a_v, uint32_t a_exp)
{
  std::vector<float> buf0(a_v.size());
  std::vector<float> buf1(a_v.size());
  float *v[] = {&buf0[0], &buf1[0]};
  // Simplified LLS.
  for (size_t i = 0; i < a_v.size(); ++i) {
    v[1][i] = v[0][i] = (float)log(a_v[i] + 2);
  }
  // Log2 filtering.
  size_t src_i = 0;
  for (uint32_t e = 0; e < a_exp; ++e) {
    size_t p = 1 << e;
    size_t dst_i = 1 ^ src_i;
    auto &v_src = v[src_i];
    auto &v_dst = v[dst_i];
    for (size_t j = 2 * p; j < a_v.size(); ++j) {
      auto i = j - p;
      auto v0 = v_src[i - p];
      auto v1 = v_src[i + 0];
      auto v2 = v_src[i + p];
      v_dst[i] = std::min(v1, (v0 + v2) / 2);
    }
    src_i ^= 1;
  }
  // Undo LLS.
  auto &v_src = v[src_i];
  for (size_t i = 0; i < a_v.size(); ++i) {
    v_src[i] = (float)exp(v_src[i]) - 2;
  }
  return 0 == src_i ? buf0 : buf1;
}

std::vector<float> Snip2(std::vector<uint32_t> const &a_v, size_t a_w, size_t
    a_h, uint32_t a_exp)
{
  assert(a_v.size() == a_w * a_h);
  std::vector<float> buf0(a_v.size());
  std::vector<float> buf1(a_v.size());
  float *v[] = {&buf0[0], &buf1[0]};
  for (size_t i = 0; i < a_v.size(); ++i) {
    v[1][i] = v[0][i] = (float)log(a_v[i] + 2);
  }
  // Log2 filtering.
  size_t src_i = 0;
  for (uint32_t e = 0; e < a_exp; ++e) {
    size_t p = 1 << e;
    size_t pw = a_w << e;
    size_t dst_i = 1 ^ src_i;
    auto &v_src = v[src_i];
    auto &v_dst = v[dst_i];
    for (size_t k = 2 * p; k < a_h; ++k) {
      auto i = k - p;
      auto ofs = i * a_w + p;
      for (size_t l = 2 * p; l < a_w; ++l) {
        auto v0 = v_src[i - pw];
        auto v1 = v_src[i - p];
        auto v2 = v_src[i + 0];
        auto v3 = v_src[i + p];
        auto v4 = v_src[i + pw];
        auto v04 = (v0 + v4) / 2;
        auto v13 = (v1 + v3) / 2;
        assert(ofs < a_v.size());
        v_dst[ofs++] = std::min(v2, std::min(v04, v13));
      }
    }
    src_i ^= 1;
  }
  auto &v_src = v[src_i];
  for (size_t i = 0; i < a_v.size(); ++i) {
    v_src[i] = (float)exp(v_src[i]) - 2;
  }
  return 0 == src_i ? buf0 : buf1;
}

namespace {
  std::mutex g_status_mutex;
  std::string g_status;
}

std::string Status_get()
{
  std::unique_lock<std::mutex> lock(g_status_mutex);
  return g_status;
}

void Status_set(char const *a_fmt, ...)
{
  time_t tt;
  time(&tt);
  char buf[1024];
  snprintf(buf, sizeof buf, "%s", ctime(&tt));
  int ofs;
  for (ofs = 0; buf[ofs]; ++ofs) {
    if ('\n' == buf[ofs]) {
      buf[ofs] = '\0';
      break;
    }
  }
  ofs += snprintf(buf + ofs, sizeof buf - (size_t)ofs, ": ");
  va_list args;
  va_start(args, a_fmt);
  vsnprintf(buf + ofs, sizeof buf - (size_t)ofs, a_fmt, args);
  va_end(args);
  std::unique_lock<std::mutex> lock(g_status_mutex);
  g_status = buf;
}

double SubModDbl(double a_l, double a_r, double a_range)
{
  double d = a_l - a_r;
  if (d < 0) {
    auto n = ceil(-d / a_range);
    d += n * a_range;
  }
  return fmod(d + 0.5 * a_range, a_range) - 0.5 * a_range;
}

double SubModU64(uint64_t a_l, uint64_t a_r, double a_range)
{
  double d = (double)(int64_t)(a_l - a_r);
  if (d < 0) {
    auto n = ceil(-d / a_range);
    d += n * a_range;
  }
  return fmod(d + 0.5 * a_range, a_range) - 0.5 * a_range;
}

std::string TabComplete(std::string const &a_path)
{
  // Separate dir and filename.
  std::string dir, filename;
  auto slash = a_path.find_last_of('/');
  if (a_path.npos == slash) {
    dir = "./";
    filename = a_path;
  } else {
    dir = a_path.substr(0, slash);
    filename = a_path.substr(slash + 1);
  }

  // Traverse files in dir.
  auto dirp = opendir(dir.c_str());
  if (!dirp) {
    warn("opendir(%s)", dir.c_str());
    return "";
  }
  std::string suggestion;
  struct dirent *dp;
  while ((dp = readdir(dirp))) {
    std::string curr = dp->d_name;
    if (0 == curr.compare(0, filename.size(), filename)) {
      if (suggestion.empty()) {
        suggestion = curr;
      } else {
        size_t i;
        for (i = filename.size();
            i < suggestion.size() && i < curr.size(); ++i) {
          if (suggestion[i] != curr[i]) {
            break;
          }
        }
        suggestion.resize(i);
      }
    }
  }
  if (0 != closedir(dirp)) {
    warn("closedir(%s)", dir.c_str());
  }
  return (a_path.npos == slash ? "" : dir) + suggestion;
}

uint64_t Time_get_ms()
{
  return g_time_ms;
}

void Time_set_ms(uint64_t a_time_ms)
{
  // Start at 10 min, so timeouts initialized to 0 are indeed timed out.
  g_time_ms = 10 * 60 * 1000 + a_time_ms;
}

void Time_wait_ms(uint32_t a_time_ms)
{
  struct timeval tv;
  tv.tv_sec = a_time_ms / 1000;
  tv.tv_usec = a_time_ms % 1000;
  if (-1 == select(0, NULL, NULL, NULL, &tv)) {
    warn("select");
  }
}

ssize_t Utf8Left(std::string const &a_str, size_t a_ofs)
{
  if (a_ofs < 1) {
    return -1;
  }
  if (0x00 == (0x80 & a_str[a_ofs - 1])) {
    return (ssize_t)a_ofs - 1;
  }
  if (a_ofs < 2) {
    return -1;
  }
  if (0xc0 == (0xe0 & a_str[a_ofs - 2]) &&
      0x80 == (0xc0 & a_str[a_ofs - 1])) {
    return (ssize_t)a_ofs - 2;
  }
  if (a_ofs < 3) {
    return -1;
  }
  if (0xe0 == (0xf0 & a_str[a_ofs - 3]) &&
      0x80 == (0xc0 & a_str[a_ofs - 2]) &&
      0x80 == (0xc0 & a_str[a_ofs - 1])) {
    return (ssize_t)a_ofs - 3;
  }
  if (a_ofs < 4) {
    return -1;
  }
  if (0xf0 == (0xf8 & a_str[a_ofs - 4]) &&
      0x80 == (0xc0 & a_str[a_ofs - 3]) &&
      0x80 == (0xc0 & a_str[a_ofs - 2]) &&
      0x80 == (0xc0 & a_str[a_ofs - 1])) {
    return (ssize_t)a_ofs - 4;
  }
  return -1;
}

ssize_t Utf8Right(std::string const &a_str, size_t a_ofs)
{
  if (a_ofs >= a_str.size()) {
    return -1;
  }
  if (0x00 == (0x80 & a_str[a_ofs])) {
    return (ssize_t)a_ofs + 1;
  }
  if (a_ofs + 1 >= a_str.size()) {
    return -1;
  }
  if (0xc0 == (0xe0 & a_str[a_ofs]) &&
      0x80 == (0xc0 & a_str[a_ofs + 1])) {
    return (ssize_t)a_ofs + 2;
  }
  if (a_ofs + 2 >= a_str.size()) {
    return -1;
  }
  if (0xe0 == (0xf0 & a_str[a_ofs]) &&
      0x80 == (0xc0 & a_str[a_ofs + 1]) &&
      0x80 == (0xc0 & a_str[a_ofs + 2])) {
    return (ssize_t)a_ofs + 3;
  }
  if (a_ofs + 3 >= a_str.size()) {
    return -1;
  }
  if (0xf0 == (0xf8 & a_str[a_ofs]) &&
      0x80 == (0xc0 & a_str[a_ofs + 1]) &&
      0x80 == (0xc0 & a_str[a_ofs + 2]) &&
      0x80 == (0xc0 & a_str[a_ofs + 3])) {
    return (ssize_t)a_ofs + 4;
  }
  return -1;
}
