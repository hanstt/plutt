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

#ifndef VECTOR_HPP
#define VECTOR_HPP

// Stupid fast vector version that only grows, never shrinks. It's so stupid
// you shouldn't use it unless you know what you're doing, and maybe not even
// then.
template <class T>
class Vector {
  public:
    typedef T *it;

    Vector():
      m_array(),
      m_capacity(),
      m_size() {
    }
    Vector(size_t a_len):
      m_array(new T [a_len]),
      m_capacity(a_len),
      m_size(a_len) {
    }
    ~Vector() {
      delete [] m_array;
    }
    T &at(size_t a_i) {
      if (a_i < m_size) {
        return m_array[a_i];
      }
      throw std::runtime_error("Vector.at overflow.");
    }
    T const &at(size_t a_i) const {
      if (a_i < m_size) {
        return m_array[a_i];
      }
      throw std::runtime_error("Vector.at overflow.");
    }
    T &back() {
      if (0 == m_size) {
        throw std::runtime_error("Vector.back on empty vector.");
      }
      return m_array[m_size - 1];
    }
    T const &back() const {
      if (0 == m_size) {
        throw std::runtime_error("Vector.back on empty vector.");
      }
      return m_array[m_size - 1];
    }
    it const begin() const {
      return m_array;
    }
    void clear() {
      m_size = 0;
    }
    bool empty() const {
      return 0 == m_size;
    }
    it const end() const {
      // Maybe verbose, but don't deref an invalid pointer.
      return m_array ? &m_array[m_size] : nullptr;
    }
    T &front() {
      if (0 == m_size) {
        throw std::runtime_error("Vector.front on empty vector.");
      }
      return m_array[0];
    }
    void push_back(T const &a_t) {
      if (m_size == m_capacity) {
        ++m_capacity;
        auto array = new T [m_capacity];
        if (m_array) {
          memcpy(array, m_array, m_size * sizeof(T));
          delete [] m_array;
        }
        m_array = array;
      }
      m_array[m_size++] = a_t;
    }
    void resize(size_t a_size) {
      if (a_size > m_capacity) {
        m_capacity = a_size;
        auto array = new T [m_capacity];
        if (m_array) {
          memcpy(array, m_array, m_size * sizeof(T));
          delete [] m_array;
        }
        m_array = array;
      }
      m_size = a_size;
    }
    size_t size() const {
      return m_size;
    }
    T &operator[](size_t a_i) {
      return at(a_i);
    }
    T const &operator[](size_t a_i) const {
      return at(a_i);
    }

  private:
    Vector(Vector const &);
    Vector &operator=(Vector const &);

    T *m_array;
    size_t m_capacity;
    size_t m_size;
};

#endif
