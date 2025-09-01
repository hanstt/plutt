/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2024
 * Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
 * Håkan T Johansson <f96hajo@chalmers.se>
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

#ifndef INPUT_HPP
#define INPUT_HPP

/*
 * Input base stuff.
 *
 * NOTE: Input fetches data from source with Fetch() and buffers in a second
 * location with Buffer(), in parallel Config processes the last buffered
 * event. A time-line could therefore look like:
 *
 * fn = input->Fetch(), ev=n
 * bn = input->Buffer(), ev=n
 * pn = config->Process(), ev=n
 *
 *  time  thread1  thread2
 *  1     f1
 *  2     b1
 *  3     f2        p1
 *  4     b2
 *  5     f3        p2
 *  6               p2
 *  7     b3
 *  8     f4        p3
 *  9     f4
 *  10    b4
 *  11    f5        p4
 *  ...
 */
class Input {
  public:
    enum Type {
      kNone,
      kUint64,
      kInt64,
      kDouble
    };
    union Scalar {
      double GetDouble(Type) const;
      uint64_t u64;
      int64_t i64;
      double dbl;
    };

    static bool IsTypeInt(Type);

    virtual ~Input() {}
    // Copy data to read buffers.
    virtual void Buffer() = 0;
    // Fetches data.
    virtual bool Fetch() = 0;
    // Gets event-buffer by ID, check Config::BindSignal.
    virtual std::pair<Scalar const *, size_t> GetData(size_t) = 0;
};

#endif
