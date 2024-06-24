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

#include <stdexcept>
#include <input.hpp>

bool Input::IsTypeInt(Type a_type)
{
  switch (a_type) {
    case kUint64:
    case kInt64:
      return true;
    case kDouble:
      return false;
    case kNone:
      break;
  }
  throw std::runtime_error(__func__);
}

double Input::Scalar::GetDouble(Input::Type a_type) const
{
  switch (a_type) {
    case kUint64:
      return (double)u64;
    case kInt64:
      return (double)i64;
    case kDouble:
      return dbl;
    case kNone:
      break;
  }
  throw std::runtime_error(__func__);
}
