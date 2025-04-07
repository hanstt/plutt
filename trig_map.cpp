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

#include <cstring>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
#include <trig_map_parser.tab.h>
#include <trig_map.hpp>

extern int yytmparse();
extern int yytmlex_destroy();

extern FILE *yytmin;
extern char const *yytmpath;
extern TrigMap *yytm_trig_map;

TrigMap::Prefix::Prefix():
  m_map_vec()
{
}

TrigMap::Prefix::~Prefix()
{
}

bool TrigMap::Prefix::GetTrig(uint32_t a_sig_i, uint32_t *a_trig_i) const
{
  if (a_sig_i >= m_map_vec.size()) {
    return false;
  }
  *a_trig_i = m_map_vec.at(a_sig_i);
  return true;
}

void TrigMap::Prefix::SetTrig(uint32_t a_sig_i, uint32_t a_trig_i)
{
  if (a_sig_i >= m_map_vec.size()) {
    m_map_vec.resize(a_sig_i + 1);
  }
  m_map_vec.at(a_sig_i) = a_trig_i;
}

TrigMap::TrigMap():
  m_file_map(),
  m_prefix_map_last()
{
}

TrigMap::~TrigMap()
{
}

void TrigMap::AddMapping(char const *a_prefix_name, uint32_t a_sig_ch,
    uint32_t a_trig_ch)
{
  auto it = m_prefix_map_last->find(a_prefix_name);
  if (m_prefix_map_last->end() == it) {
    auto pair = m_prefix_map_last->insert(std::make_pair(a_prefix_name,
        Prefix()));
    it = pair.first;
  }
  it->second.SetTrig(a_sig_ch, a_trig_ch);
}

TrigMap::Prefix const *TrigMap::LoadPrefix(char const *a_path, char const
    *a_prefix_name)
{
  auto path_it = m_file_map.find(a_path);
  if (m_file_map.end() == path_it) {
    // Parse new file.
    auto pair = m_file_map.insert(std::make_pair(a_path, PrefixMap()));
    path_it = pair.first;
    m_prefix_map_last = &path_it->second;
    yytmin = fopen(a_path, "rb");
    if (!yytmin) {
      std::cerr << a_path << ':' << a_prefix_name <<
          ": Could not open: " << strerror(errno) << ".\n";
      throw std::runtime_error(__func__);
    }
    yytmpath = a_path;
    yytmlloc.last_line = 1;
    yytmlloc.last_column = 1;
    yytm_trig_map = this;
    std::cout << a_path << ": Parsing...\n";
    yytmparse();
    fclose(yytmin);
    yytmlex_destroy();
    std::cout << a_path << ": Done!\n";
    yytm_trig_map = nullptr;
  }
  auto &prefix_map = path_it->second;
  auto prefix_it = prefix_map.find(a_prefix_name);
  if (prefix_map.end() == prefix_it) {
    std::cerr << a_path << ':' << a_prefix_name <<
        ": Prefix not in file.\n";
    throw std::runtime_error(__func__);
  }
  return &prefix_it->second;
}
