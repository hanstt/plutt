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

#ifndef NODE_SIGNAL_USER_HPP
#define NODE_SIGNAL_USER_HPP

#include <node.hpp>

/*
 * Intermediary value.
 */
class NodeSignalUser: public NodeValue {
  public:
    NodeSignalUser(std::string const &, char const *, char const *, char const
        *);
    std::string const &GetID() const;
    std::string const &GetEnd() const;
    std::string const &GetV() const;
    Value const &GetValue(uint32_t);
    void Process(uint64_t);
    void SetSources(NodeValue *, NodeValue *, NodeValue *);

  private:
    NodeSignalUser(NodeSignalUser const &);
    NodeSignalUser &operator=(NodeSignalUser const &);

    std::string m_id_name;
    std::string m_end_name;
    std::string m_v_name;
    NodeValue *m_id;
    NodeValue *m_end;
    NodeValue *m_v;
    Value m_value;
};

#endif
