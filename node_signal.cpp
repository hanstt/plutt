/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2025
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

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <config.hpp>
#include <node_signal.hpp>

NodeSignal::NodeSignal(Config &a_config, std::string const &a_name):
  NodeValue(a_config.GetLocStr()),
  // This is nasty, but a_config will always outlive all nodes.
  m_config(&a_config),
  m_name(a_name),
  m_value(),
  m_id(),
  m_end(),
  m_v()
{
}

void NodeSignal::BindSignal(std::string const &a_name, MemberType
    a_member_type, size_t a_id, Input::Type a_type)
{
#define BIND_SIGNAL_ASSERT_UINT(member) do { \
    if (Input::kUint64 != a_type && Input::kInt64 != a_type) { \
      std::cerr << GetLocStr() << ": member '" << #member << \
          "' not integer (type id=" << a_type << ")!\n"; \
      throw std::runtime_error(__func__); \
    } \
  } while (0)
  Member **mem;
  char const *str;
  switch (a_member_type) {
  case kId:
    BIND_SIGNAL_ASSERT_UINT(id);
    mem = &m_id;
    str = "id";
    break;
  case kEnd:
    BIND_SIGNAL_ASSERT_UINT(end);
    mem = &m_end;
    str = "end";
    break;
  case kV:
    mem = &m_v;
    str = "v";
    break;
  default:
    abort();
  }
  if (*mem) {
    std::cerr << GetLocStr() << ": Signal member '" << m_name << ':' << str <<
        "' already set.\n";
    throw std::runtime_error(__func__);
  }
  *mem = new Member;
  (*mem)->type = a_type;
  (*mem)->id = a_id;
}

Value const &NodeSignal::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeSignal::Process(uint64_t a_evid)
{
  m_value.Clear();

#define FETCH_SIGNAL_DATA(SUFF) \
  if (!m_##SUFF) return; \
  auto const pair_##SUFF = m_config->GetInput()->GetData(m_##SUFF->id); \
  auto const p_##SUFF = pair_##SUFF.first; \
  auto const len_##SUFF = pair_##SUFF.second
#define SIGNAL_LEN_CHECK(l, op, r) do { \
  auto l_ = l; \
  auto r_ = r; \
  if (!(l_ op r_)) { \
    std::cerr << __FILE__ << ':' << __LINE__ << ':' << m_name << \
        ": Signal check failed: (" << \
        #l "=" << l_ << " " #op " " #r "=" << r_ << ").\n"; \
    return; \
  } \
} while (0)
#define IF_VALUE_NOP(v)
#define IF_VALUE_INVALID(v) if (!std::isnan(v.dbl) && !std::isinf(v.dbl))
  if (m_end) {
    // Multi-hit array.
    FETCH_SIGNAL_DATA(id);
    FETCH_SIGNAL_DATA(end);
    FETCH_SIGNAL_DATA(v);
    SIGNAL_LEN_CHECK(len_id, ==, len_end);
    SIGNAL_LEN_CHECK(len_id, <=, len_v);
    m_value.SetType(m_v->type);
    uint32_t v_i = 0;
    switch (m_v->type) {
#define COPY_M_HIT(input_type, kind) \
      case Input::input_type: \
        for (uint32_t i_ = 0; i_ < len_id; ++i_) { \
          auto id = (uint32_t)p_id[i_].u64; \
          auto end = (uint32_t)p_end[i_].u64; \
          for (; v_i < end; ++v_i) { \
            auto v_ = p_v[v_i]; \
            IF_VALUE_##kind(v_) { \
              m_value.Push(id, v_); \
            } \
          } \
        } \
        break
      COPY_M_HIT(kUint64, NOP);
      COPY_M_HIT(kInt64, NOP);
      COPY_M_HIT(kDouble, INVALID);
      case Input::kNone:
      default:
        throw std::runtime_error(__func__);
    }
  } else if (m_id) {
    // Single-hit array.
    FETCH_SIGNAL_DATA(id);
    FETCH_SIGNAL_DATA(v);
    SIGNAL_LEN_CHECK(len_id, ==, len_v);
    m_value.SetType(m_v->type);
    switch (m_v->type) {
#define COPY_S_HIT(input_type, kind) \
    case Input::input_type: \
      for (uint32_t i_ = 0; i_ < len_id; ++i_) { \
        auto mi = (uint32_t)p_id[i_].u64; \
        auto v_ = p_v[i_]; \
        IF_VALUE_##kind(v_) { \
          m_value.Push(mi, v_); \
        } \
      } \
      break
      COPY_S_HIT(kUint64, NOP);
      COPY_S_HIT(kInt64, NOP);
      COPY_S_HIT(kDouble, INVALID);
      case Input::kNone:
      default:
        throw std::runtime_error(__func__);
    }
  } else if (m_v) {
    // Scalar or simple array.
    FETCH_SIGNAL_DATA(v);
    m_value.SetType(m_v->type);
    switch (m_v->type) {
#define COPY_SCALAR(input_type, kind) \
      case Input::input_type: \
        for (uint32_t i_ = 0; i_ < len_v; ++i_) { \
          auto v_ = p_v[i_]; \
          IF_VALUE_##kind(v_) { \
            m_value.Push(0, v_); \
          } \
        } \
        break
      COPY_SCALAR(kUint64, NOP);
      COPY_SCALAR(kInt64, NOP);
      COPY_SCALAR(kDouble, INVALID);
      case Input::kNone:
      default:
        throw std::runtime_error(__func__);
    }
  } else {
    std::cerr << m_name << ": Has no value member!" << std::endl;
    throw std::runtime_error(__func__);
  }
}

void NodeSignal::SetLocStr(std::string const &a_loc)
{
  m_loc = a_loc;
}

void NodeSignal::UnbindSignal()
{
#define UNBIND(suff) do { \
  delete m_##suff; \
  m_##suff = nullptr; \
} while (0)
  UNBIND(id);
  UNBIND(end);
  UNBIND(v);
}
