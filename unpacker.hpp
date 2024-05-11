/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023  Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
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

#if PLUTT_UCESB

#ifndef UNPACKER_HPP
#define UNPACKER_HPP

#include <input.hpp>

class Config;

/*
 * Ucesb unpacker input.
 * argc/argv with non-main arguments are passed to the ctor, and should have
 * the full unpacker call, eg:
 *  argv[0] = path to the unpacker,
 *  argv[1] = lmd,
 *  argv[2] = --allow-errors etc.
 */
class Unpacker: public Input {
  public:
    Unpacker(Config &, int, char **);
    ~Unpacker();
    void Buffer();
    bool Fetch();
    std::pair<Input::Scalar const *, size_t> GetData(size_t);

  private:
    Unpacker(Unpacker const &);
    Unpacker &operator=(Unpacker const &);
    void BindSignal(Config &, std::vector<char> const &, std::string const &,
        char const *, char const *, size_t &, std::map<std::string, uint32_t>
        &, bool);
    std::vector<char> ExtractRange(std::vector<char> const &, char const *,
        char const *);
    char const *RewindNewline(std::vector<char> const &, char const *);

    std::string m_path;
    bool m_is_struct_writer;
    ext_data_clnt *m_clnt;
    FILE *m_pip;
    ext_data_struct_info m_struct_info;
    struct Entry {
      Entry(std::string const &a_name, int a_ext_type, size_t a_in_ofs, size_t
          a_out_ofs, size_t a_len):
        name(a_name),
        ext_type(a_ext_type),
        in_ofs(a_in_ofs),
        out_ofs(a_out_ofs),
        len(a_len)
      {
      }
      std::string name;
      int ext_type;
      size_t in_ofs;
      size_t out_ofs;
      size_t len;
    };
    std::vector<Entry> m_map;
    std::vector<uint8_t> m_event_buf;
    size_t m_out_size;
    std::vector<Input::Scalar> m_out_buf;
};

#endif

#endif
