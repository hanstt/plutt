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

#ifndef NODE_SIGNAL_HPP
#define NODE_SIGNAL_HPP

#include <node.hpp>

class Config;

/*
 * Data source, called signal for the bling.
 */
class NodeSignal: public NodeValue {
  public:
    NodeSignal(Config &, char const *);
    void BindSignal(std::string const &, size_t, Input::Type);
    Value const &GetValue(uint32_t);
    void Process(uint64_t);
    void SetLocStr(std::string const &);
    void UnbindSignal();

  private:
    NodeSignal(NodeSignal const &);
    NodeSignal &operator=(NodeSignal const &);

    struct Member {
      Input::Type type;
      size_t id;
    };
    Config *m_config;
    std::string m_name;
    Value m_value;
    Member *m_M;
    Member *m_MI;
    Member *m_ME;
    Member *m_;
    Member *m_v;
};

#endif
