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

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <node_mexpr.hpp>

NodeMExpr::NodeMExpr(std::string const &a_loc, NodeValue *a_l, NodeValue *a_r,
    double a_d, Operation a_op):
  NodeValue(a_loc),
  m_l(a_l),
  m_r(a_r),
  m_d(a_d),
  m_mix(),
  m_op(a_op),
  m_value()
{
  if (a_l && a_r) {
    m_mix = 0;
  } else if (a_l) {
    m_mix = 1;
  } else {
    m_mix = 2;
  }
}

Value const &NodeMExpr::GetValue(uint32_t a_ret_i)
{
  assert(0 == a_ret_i);
  return m_value;
}

void NodeMExpr::Process(uint64_t a_evid)
{
  NODE_PROCESS_GUARD(a_evid);
  Value const *val_l = nullptr;
  Value const *val_r = nullptr;
  if (m_l) {
    NODE_PROCESS(m_l, a_evid);
    val_l = &m_l->GetValue();
    if (Input::kNone == val_l->GetType() ||
        val_l->GetID().empty()) {
      return;
    }
  }
  if (m_r) {
    NODE_PROCESS(m_r, a_evid);
    val_r = &m_r->GetValue();
    if (Input::kNone == val_r->GetType() ||
        val_r->GetID().empty()) {
      return;
    }
  }

  m_value.Clear();
  m_value.SetType(Input::kDouble);

  uint32_t i_l = 0;
  uint32_t i_r = 0;
  for (;;) {
    uint32_t mi;
    uint32_t me_l;
    uint32_t me_r;
    uint32_t vi_l;
    uint32_t vi_r;
    if (0 == m_mix) {
      if (i_l >= val_l->GetID().size() ||
          i_r >= val_r->GetID().size()) {
        break;
      }
      auto mi_l = val_l->GetID()[i_l];
      auto mi_r = val_r->GetID()[i_r];
      if (mi_l < mi_r) {
        ++i_l;
        continue;
      } else if (mi_r < mi_l) {
        ++i_r;
        continue;
      }
      mi = mi_l;
      me_l = val_l->GetEnd()[i_l];
      me_r = val_r->GetEnd()[i_r];
      vi_l = 0 == i_l ? 0 : val_l->GetEnd()[i_l - 1];
      vi_r = 0 == i_r ? 0 : val_r->GetEnd()[i_r - 1];
    } else if (1 == m_mix) {
      if (i_l >= val_l->GetID().size()) {
        break;
      }
      mi = val_l->GetID()[i_l];
      me_l = val_l->GetEnd()[i_l];
      vi_l = 0 == i_l ? 0 : val_l->GetEnd()[i_l - 1];
    } else {
      if (i_r >= val_r->GetID().size()) {
        break;
      }
      mi = val_r->GetID()[i_r];
      me_r = val_r->GetEnd()[i_r];
      vi_r = 0 == i_r ? 0 : val_r->GetEnd()[i_r - 1];
    }
    for (;;) {
      double v, l = 0.0, r = 0.0;
      if (0 == m_mix) {
        if (vi_l >= me_l || vi_r >= me_r) {
          break;
        }
        l = val_l->GetV(vi_l++, true);
        r = val_r->GetV(vi_r++, true);
      } else if (1 == m_mix) {
        if (vi_l >= me_l) {
          break;
        }
        l = val_l->GetV(vi_l++, true);
        r = m_d;
      } else {
        if (vi_r >= me_r) {
          break;
        }
        l = m_d;
        r = val_r->GetV(vi_r++, true);
      }
      switch (m_op) {
        case ADD:  v = l + r; break;
        case SUB:  v = l - r; break;
        case MUL:  v = l * r; break;
        case DIV:  v = l / r; break;
        case COS:  v = cos(l); break;
        case SIN:  v = sin(l); break;
        case TAN:  v = tan(l); break;
        case ACOS: v = acos(l); break;
        case ASIN: v = asin(l); break;
        case ATAN: v = atan(l); break;
        case SQRT: v = sqrt(l); break;
        case EXP:  v = exp(l); break;
        case LOG:  v = (1 == m_mix) ? log(l) : log(r) / log(m_d); break;
        case ABS:  v = std::abs(l); break;
        case POW:  v = pow(l, r); break;
        default: abort();
      }
      if (!std::isnan(v) && !std::isinf(v)) {
        Input::Scalar s;
        s.dbl = v;
        m_value.Push(mi, s);
      }
    }
    ++i_l;
    ++i_r;
  }
}
