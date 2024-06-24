/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2024
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

#ifndef ROOT_HPP
#define ROOT_HPP

#include <input.hpp>

class Config;
// Root has less strict headers, hide everything in root.cpp...
class RootChain;

/*
 * Root input.
 * Takes argc/argv after main arguments and hopes they are all Root files.
 */
class Root: public Input {
  public:
    Root(bool, Config *, int, char **);
    ~Root();
    void Buffer();
    bool Fetch();
    std::pair<Input::Scalar const *, size_t> GetData(size_t);

  private:
    Root(Root const &);
    Root &operator=(Root const &);

    bool m_is_files;
    struct Notify {
      Notify():
        config(),
        tree_name(),
        fd(),
        wd_map()
      {
      }
      Notify(Notify const &);
      Notify &operator=(Notify const &);
      Config *config;
      std::string tree_name;
      int fd;
      std::map<int, std::string> wd_map;
    } m_inotify;
    RootChain *m_chain;
};

#endif

#endif
