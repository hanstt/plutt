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

#ifndef TRIG_MAP_HPP
#define TRIG_MAP_HPP

/*
 * Loads trigger mapping from files, each file can have any number of
 * prefixes, and each prefix any number of channels. Lookup is O(1).
 * File syntax: prefix:sig_ch = trig_ch
 */
class TrigMap {
  public:
    class Prefix {
      // Mapping of all channels with specific prefix from one file.
      public:
        Prefix();
        ~Prefix();
        bool GetTrig(uint32_t, uint32_t *) const;
        void SetTrig(uint32_t, uint32_t);
      private:
        std::vector<uint32_t> m_map_vec;
    };

    TrigMap();
    ~TrigMap();
    // Adds single mapping to one prefix.
    void AddMapping(char const *, uint32_t, uint32_t);
    // Loads file and returns the mapping for one prefix.
    Prefix const *LoadPrefix(char const *, char const *);

  private:
    TrigMap(TrigMap const &);
    TrigMap &operator=(TrigMap const &);

    typedef std::map<std::string, Prefix> PrefixMap;
    std::map<std::string, PrefixMap> m_file_map;
    PrefixMap *m_prefix_map_last;
};

#endif
