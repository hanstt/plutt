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

#include <err.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <stdexcept>
#include <vector>

#include <config.hpp>
#include <inhax.hpp>

Inhax::Inhax(Config &a_config, int a_argc, char **a_argv):
  m_path(),
  m_fd(),
  m_map(),
  m_map_lu(),
  m_in_buf_bytes(),
  m_in_buf(1 << 16),
  m_fetch_i()
{
  if (0 == a_argc) {
    m_path = "-";
    m_fd = STDIN_FILENO;
  } else {
    m_path = a_argv[0];
    m_fd = open(a_argv[0], O_RDONLY);
    if (-1 == m_fd) {
      err(EXIT_FAILURE, "open(%s)", a_argv[0]);
    }
  }

  // Prepare map of signals.
  auto signal_list = a_config.GetSignalList();
  for (auto it = signal_list.begin(); signal_list.end() != it; ++it) {
    auto name = *it;
    BindSignal(a_config, name);
  }
}

Inhax::~Inhax()
{
  if (STDIN_FILENO != m_fd) {
    if (-1 == close(m_fd)) {
      warn("close(%s)", m_path.c_str());
    }
  }
}

void Inhax::BindSignal(Config &a_config, std::string const &a_name)
{
  auto ret = m_map.insert(std::make_pair(a_name, Entry()));
  m_map_lu.push_back(ret.first);
  a_config.BindSignal(a_name, NodeSignal::kV, m_map_lu.size() - 1,
      Input::kUint64);
}

void Inhax::Buffer()
{
  // Flip buffers.
  m_fetch_i ^= 1;
}

bool Inhax::Fetch()
{
  // Clear buffers.
  for (auto it = m_map.begin(); m_map.end() != it; ++it) {
    auto &entry = it->second;
    entry.buf[m_fetch_i].resize(0);
  }

  void const *p;

  // Look for signature.
  for (;;) {
    p = Fetch(8);
    if (!p) {
      return false;
    }
    if (0 == strncmp((char const *)p, "Haxelhax", 8)) {
      Shift(8);
      break;
    }
    Shift(1);
  }

  p = Fetch(4);
  if (!p) {
    return false;
  }
  auto sig_n = *(uint32_t const *)p;
  Shift(4);
  for (uint32_t sig_i = 0; sig_i < sig_n; ++sig_i) {
    std::string name;
    for (;;) {
      p = Fetch(1);
      if (!p) {
        return false;
      }
      auto c = *(char const *)p;
      Shift(1);
      if (0 == c) {
        break;
      }
      name += c;
    }
    std::vector<Input::Scalar> *buf = nullptr;
    auto it = m_map.find(name);
    if (m_map.end() != it) {
      auto &entry = it->second;
      buf = &entry.buf[m_fetch_i];
    }
    p = Fetch(4);
    if (!p) {
      return false;
    }
    auto v_n = *(uint32_t const *)p;
    Shift(4);
    for (uint32_t v_i = 0; v_i < v_n; ++v_i) {
      p = Fetch(4);
      if (!p) {
        return false;
      }
      auto v = *(uint32_t const *)p;
      Shift(4);
      if (buf) {
        buf->push_back(Input::Scalar());
        auto &s = buf->back();
        s.u64 = v;
      }
    }
  }
  return true;
}

void const *Inhax::Fetch(unsigned a_bytes)
{
  while (a_bytes > m_in_buf_bytes) {
    // Fetch bytes from input.
    ssize_t rc;

    rc = read(m_fd, &m_in_buf.at(m_in_buf_bytes), a_bytes - m_in_buf_bytes);
    if (rc < 0) {
      warn("read(%s)", m_path.c_str());
      return nullptr;
    }
    if (0 == rc) {
      std::cout << "End of file." << std::endl;
      return nullptr;
    }
    m_in_buf_bytes += (size_t)rc;
  }
  return &m_in_buf.at(0);
}

std::pair<Input::Scalar const *, size_t> Inhax::GetData(size_t a_id)
{
  auto it = m_map_lu.at(a_id);
  auto &entry = it->second;
  auto &buf = entry.buf[!m_fetch_i];
  if (buf.empty()) {
    return std::make_pair(nullptr, 0);
  }
  return std::make_pair(&buf.at(0), buf.size());
}

void Inhax::Shift(unsigned a_bytes)
{
  // Shift away data.
  assert(a_bytes <= m_in_buf_bytes);
  memmove(&m_in_buf.at(0), &m_in_buf.at(a_bytes), m_in_buf_bytes - a_bytes);
  m_in_buf_bytes -= a_bytes;
}
