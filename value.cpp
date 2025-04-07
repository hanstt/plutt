/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2025
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

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <value.hpp>

Value::Value():
  m_type(Input::kNone),
  m_id(),
  m_end(),
  m_v()
{
}

void Value::Clear()
{
  m_id.clear();
  m_end.clear();
  m_v.clear();
}

int Value::Cmp(Input::Scalar const &a_l, Input::Scalar const &a_r) const
{
  switch (m_type) {
    case Input::kUint64:
      if (a_l.u64 < a_r.u64) return -1;
      if (a_l.u64 > a_r.u64) return 1;
      return 0;
    case Input::kInt64:
      if (a_l.i64 < a_r.i64) return -1;
      if (a_l.i64 > a_r.i64) return 1;
      return 0;
    case Input::kDouble:
      if (a_l.dbl < a_r.dbl) return -1;
      if (a_l.dbl > a_r.dbl) return 1;
      return 0;
    case Input::kNone:
      break;
  }
  throw std::runtime_error(__func__);
}

Input::Type Value::GetType() const
{
  return m_type;
}

Vector<uint32_t> const &Value::GetID() const
{
  return m_id;
}

Vector<uint32_t> const &Value::GetEnd() const
{
  return m_end;
}

Vector<Input::Scalar> const &Value::GetV() const
{
  return m_v;
}

double Value::GetV(uint32_t a_i, bool a_do_signed) const
{
  switch (m_type) {
    case Input::kUint64:
      {
        auto u64 = m_v.at(a_i).u64;
        if (a_do_signed) {
          return (double)(int64_t)u64;
        }
        return (double)u64;
      }
    case Input::kInt64:
      {
        auto i64 = m_v.at(a_i).i64;
        return (double)i64;
      }
    case Input::kDouble:
      return m_v.at(a_i).dbl;
    case Input::kNone:
      break;
  }
  throw std::runtime_error(__func__);
}

void Value::Push(uint32_t a_i, Input::Scalar const &a_v)
{
  if (m_id.empty() || m_id.back() != a_i) {
    m_id.push_back(a_i);
    auto end_prev = m_end.empty() ? 0 : m_end.back();
    m_end.push_back(end_prev + 1);
  } else {
    ++m_end.back();
  }
  m_v.push_back(a_v);
}

void Value::SetType(Input::Type a_type)
{
  if (Input::kNone != m_type && a_type != m_type) {
    std::runtime_error("Value cannot change type!");
  }
  m_type = a_type;
}
