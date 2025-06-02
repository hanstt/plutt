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

#ifndef INHAX_HPP
#define INHAX_HPP

#include <input.hpp>

class Config;

/*
 * "Hax" input.
 * A stupid and inefficient streaming protocol:
 *  8 * 8-bit: "Haxelhax"
 *  32-bit: num-signals {
 *   c-string: name
 *   32-bit: num-values {
 *    32-bit: value
 *   }
 *  }
 * argc/argv with non-main arguments are passed to the ctor, the only argument
 * is an optional path to a file to read from, otherwise read from stdin:
 *  argv[0] = path.
 */
class Inhax: public Input {
  public:
    Inhax(Config &, int, char **);
    ~Inhax();
    void Buffer();
    bool Fetch();
    std::pair<Input::Scalar const *, size_t> GetData(size_t);

  private:
    struct Entry {
      Entry():
        buf()
      {
      }
      std::vector<Input::Scalar> buf[2];
    };

    Inhax(Inhax const &);
    Inhax &operator=(Inhax const &);
    void BindSignal(Config &, std::string const &, std::string const &,
        NodeSignal::MemberType);
    void const *Fetch(unsigned);
    void Shift(unsigned);

    typedef std::map<std::string, Entry> EntryMap;
    std::string m_path;
    int m_fd;
    EntryMap m_map;
    std::vector<EntryMap::iterator> m_map_lu;
    size_t m_in_buf_bytes;
    std::vector<uint8_t> m_in_buf;
    /* 0 = fetch into Entry::buf[0], GetData from buf[1]. */
    size_t m_fetch_i;
};

#endif
