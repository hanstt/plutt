/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023, 2024
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
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <config.hpp>
#include <node_signal.hpp>

NodeSignal::NodeSignal(Config &a_config, char const *a_name):
  NodeValue(a_config.GetLocStr()),
  // This is nasty, but a_config will always outlive all nodes.
  m_config(&a_config),
  m_name(a_name),
  m_value(),
  m_M(),
  m_MI(),
  m_ME(),
  m_(),
  m_v()
{
}

void NodeSignal::BindSignal(std::string const &a_suffix, size_t a_id,
    Input::Type a_type)
{
#define BIND_SIGNAL_ASSERT_UINT do { \
    if (Input::kUint64 != a_type) { \
      std::cerr << GetLocStr() << ": 'M' member not integer!\n"; \
      throw std::runtime_error(__func__); \
    } \
  } while (0)
  Member **mem;
  if (0 == a_suffix.compare("")) {
    mem = &m_;
  } else if (0 == a_suffix.compare("M")) {
    BIND_SIGNAL_ASSERT_UINT;
    mem = &m_M;
  } else if (0 == a_suffix.compare("I") || 0 == a_suffix.compare("MI")) {
    BIND_SIGNAL_ASSERT_UINT;
    mem = &m_MI;
  } else if (0 == a_suffix.compare("ME")) {
    BIND_SIGNAL_ASSERT_UINT;
    mem = &m_ME;
  } else if (0 == a_suffix.compare("v")) {
    mem = &m_v;
  } else {
    std::cerr << GetLocStr() << ": Weird signal suffix '" << a_suffix <<
        "'.\n";
    throw std::runtime_error(__func__);
  }
  if (*mem) {
    std::cerr << GetLocStr() << ": Signal member '" << m_name << a_suffix <<
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
  if (m_ME) {
    // Multi-hit array.
    FETCH_SIGNAL_DATA(M);
    FETCH_SIGNAL_DATA(MI);
    FETCH_SIGNAL_DATA(ME);
    FETCH_SIGNAL_DATA();
    FETCH_SIGNAL_DATA(v);
    SIGNAL_LEN_CHECK(1U, ==, len_M);
    SIGNAL_LEN_CHECK(len_ME, ==, len_MI);
    SIGNAL_LEN_CHECK(1U, ==, len_);
    SIGNAL_LEN_CHECK(p_->u64, <=, len_v);
    m_value.SetType(m_v->type);
    uint32_t v_i = 0;
    switch (m_v->type) {
#define COPY_M_HIT(input_type, kind) \
      case Input::input_type: \
        for (uint32_t i = 0; i < p_M->u64; ++i) { \
          auto mi = (uint32_t)p_MI[i].u64; \
          auto me = (uint32_t)p_ME[i].u64; \
          for (; v_i < me; ++v_i) { \
            auto v_ = p_v[v_i]; \
            IF_VALUE_##kind(v_) { \
              m_value.Push(mi, v_); \
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
  } else if (m_MI) {
    // Single-hit array.
    FETCH_SIGNAL_DATA();
    FETCH_SIGNAL_DATA(MI);
    FETCH_SIGNAL_DATA(v);
    SIGNAL_LEN_CHECK(1U, ==, len_);
    SIGNAL_LEN_CHECK(p_->u64, <=, len_MI);
    SIGNAL_LEN_CHECK(p_->u64, <=, len_v);
    m_value.SetType(m_v->type);
    switch (m_v->type) {
#define COPY_S_HIT(input_type, kind) \
    case Input::input_type: \
      for (uint32_t i = 0; i < p_->u64; ++i) { \
        auto mi = (uint32_t)p_MI[i].u64; \
        auto v_ = p_v[i]; \
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
    // Non-indexed array.
    FETCH_SIGNAL_DATA();
    FETCH_SIGNAL_DATA(v);
    SIGNAL_LEN_CHECK(1U, ==, len_);
    SIGNAL_LEN_CHECK(p_->u64, <=, len_v);
    m_value.SetType(m_v->type);
    switch (m_v->type) {
#define COPY_INDEX(input_type, kind) \
      case Input::input_type: \
        for (uint32_t i = 0; i < p_->u64; ++i) { \
          auto v_ = p_v[i]; \
          IF_VALUE_##kind(v_) { \
            m_value.Push(0, v_); \
          } \
        } \
        break
      COPY_INDEX(kUint64, NOP);
      COPY_INDEX(kInt64, NOP);
      COPY_INDEX(kDouble, INVALID);
      case Input::kNone:
      default:
        throw std::runtime_error(__func__);
    }
  } else {
    // Scalar or simple array.
    FETCH_SIGNAL_DATA();
    m_value.SetType(m_->type);
    switch (m_->type) {
#define COPY_SCALAR(input_type, kind) \
      case Input::input_type: \
        for (uint32_t i = 0; i < len_; ++i) { \
          auto v_ = p_[i]; \
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
  UNBIND();
  UNBIND(M);
  UNBIND(MI);
  UNBIND(ME);
  UNBIND(v);
}
