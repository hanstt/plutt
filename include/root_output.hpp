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

#ifndef ROOT_OUTPUT_HPP
#define ROOT_OUTPUT_HPP

#include <output.hpp>

class TBranch;
class TFile;
class TTree;

/*
 * Writes ROOT tree.
 */
class RootOutput: public Output {
  public:
    RootOutput(std::string const &, std::string const &);
    ~RootOutput();
    void Add(Var *, std::string const &);
    void Fill(Var const &, double);
    void FinishEvent();

  private:
    RootOutput(RootOutput const &);
    RootOutput &operator=(RootOutput const &);

    struct Entry {
      double dbl;
    };
    TFile *m_file;
    TTree *m_tree;
    std::list<std::string> m_proto_list;
    std::vector<Entry> m_var_vec;
};

#endif

#endif
