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

#if PLUTT_ROOT

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <TFile.h>
#include <TTree.h>
#include <root_output.hpp>
#include <util.hpp>

RootOutput::RootOutput(std::string const &a_path, std::string const &a_name):
  m_file(new TFile(a_path.c_str(), "RECREATE")),
  m_tree(new TTree(a_name.c_str(), "Plutt was here")),
  m_proto_list(),
  m_var_vec()
{
  std::cout << "Created " << a_path << ":" << a_name << ".\n";
}

RootOutput::~RootOutput()
{
  std::cout << "Closing output ROOT file.\n";
  m_tree->Write();
  m_file->Close();
  // TODO: Can't delete tree?
  delete m_file;
}

void RootOutput::Add(Output::Var *a_var, std::string const &a_name)
{
  a_var->id = (uint32_t)m_proto_list.size();
  auto clean_name = CleanName(a_name);
  m_proto_list.push_back(clean_name);
}

void RootOutput::Fill(Var const &a_var, double a_dbl)
{
  if (!m_proto_list.empty()) {
    m_var_vec.resize(m_proto_list.size());
    size_t i = 0;
    while (!m_proto_list.empty()) {
      m_tree->Branch(m_proto_list.front().c_str(), &m_var_vec.at(i++).dbl);
      m_proto_list.pop_front();
    }
  }
  m_var_vec.at(a_var.id).dbl = a_dbl;
}

void RootOutput::FinishEvent()
{
  m_tree->Fill();
  for (auto it = m_var_vec.begin(); m_var_vec.end() != it; ++it) {
    it->dbl = 0;
  }
}

#endif
