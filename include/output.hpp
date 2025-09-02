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

#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include <input.hpp>

/*
 * For writing to "disk".
 */
class Output {
  public:
    struct Var {
      uint32_t id;
    };

    virtual ~Output() {}
    // Add variable to output.
    virtual void Add(Var *, std::string const &) = 0;
    // Fill variable.
    virtual void Fill(Var const &, double) = 0;
    // Finish event with filled variables.
    virtual void FinishEvent() = 0;
};

extern Output *g_output;

#endif
