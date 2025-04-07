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

#ifndef VALUE_HPP
#define VALUE_HPP

#include <input.hpp>
#include <vector.hpp>

/*
 * Keeps zero-suppressed multi-hit style arrays.
 */
class Value {
  public:
    Value();
    void Clear();
    // Compares two scalars assumed to be of the same type as current object.
    int Cmp(Input::Scalar const &, Input::Scalar const &) const;
    Input::Type GetType() const;
    Vector<uint32_t> const &GetID() const;
    Vector<uint32_t> const &GetEnd() const;
    Vector<Input::Scalar> const &GetV() const;
    // Converts whatever v-type to double. This is online, not paper plots,
    // but developers need to be careful still!
    double GetV(uint32_t, bool) const;
    // Pushes scalar to given channel.
    void Push(uint32_t, Input::Scalar const &);
    void SetType(Input::Type);

  private:
    Input::Type m_type;
    Vector<uint32_t> m_id;
    Vector<uint32_t> m_end;
    Vector<Input::Scalar> m_v;
};

#endif
