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

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <node.hpp>
#include <cut.hpp>

NodeCutValue::NodeCutValue():
  x(),
  y()
{
}

CutPolygon::Point::Point(double a_x, double a_y):
  x(a_x),
  y(a_y)
{
}

CutPolygon::CutPolygon(char const *a_str, bool a_is_path):
  m_title(),
  m_dim(),
  m_point_vec()
{
  if (a_is_path) {
    // No inline points, expect path.
    std::ifstream ifile(a_str);
    if (!ifile.is_open()) {
      std::cerr << a_str << ": Could not open.\n";
      throw std::runtime_error(__func__);
    }
    if (!std::getline(ifile, m_title)) {
      std::cerr << a_str << ": Missing histo title.\n";
      throw std::runtime_error(__func__);
    }
    for (int line_no = 1;; ++line_no) {
      std::string line;
      if (!std::getline(ifile, line)) {
        break;
      }
      std::istringstream iss(line);
      double x;
      if (!(iss >> x)) {
        std::cerr << a_str << ':' << line_no << ": Invalid x-coord.\n";
        throw std::runtime_error(__func__);
      }
      double y;
      if (!(iss >> y)) {
        AddPoint(x);
      } else {
        AddPoint(x, y);
      }
    }
  } else {
    m_title = a_str;
  }
}

void CutPolygon::AddPoint(double a_x)
{
  if (0 == m_dim) {
    m_dim = 1;
  } else if (1 != m_dim) {
    std::cerr << m_title << ": 1d cut became 2d?\n";
    throw std::runtime_error(__func__);
  }
  Point p(a_x, 0.0);
  m_point_vec.push_back(p);
}

void CutPolygon::AddPoint(double a_x, double a_y)
{
  if (0 == m_dim) {
    m_dim = 2;
  } else if (2 != m_dim) {
    std::cerr << m_title << ": 2d cut became 1d?.\n";
    throw std::runtime_error(__func__);
  }
  Point p(a_x, a_y);
  m_point_vec.push_back(p);
}

std::string const &CutPolygon::GetTitle() const
{
  return m_title;
}

bool CutPolygon::Test(double a_x) const
{
  if (1 != m_dim) {
    std::cerr << m_title <<
        ": CutPolygon dim=" << m_dim << " but 1d Test.\n";
    throw std::runtime_error(__func__);
  }
  if (2 != m_point_vec.size()) {
    std::cerr << m_title <<
        ": CutPolygon 1d Test has " << m_point_vec.size() << "!=2 points.\n";
    throw std::runtime_error(__func__);
  }
  return m_point_vec[0].x <= a_x && a_x < m_point_vec[1].x;
}

bool CutPolygon::Test(double a_x, double a_y) const
{
  if (2 != m_dim) {
    std::cerr << m_title <<
        ": CutPolygon dim=" << m_dim << " but 2d Test.\n";
    throw std::runtime_error(__func__);
  }
  if (m_point_vec.size() < 3) {
    std::cerr << m_title <<
        ": CutPolygon 2d Test has " << m_point_vec.size() << "<3 points.\n";
    throw std::runtime_error(__func__);
  }
  // This is so much more work than the 1D case...
  // - Create a x+ ray from the point to test.
  // - The ray points in (+1,0) so the winding = cross product is trivial.
  // - Set sum winding to 0.
  // - For each line:
  //   - If the line is horizontal, do nothing.
  //   - If the ray intersects either end and the neighbour has the same
  //     winding it is a crossing.
  //   - If the ray intersects the line, it is a crossing.
  //   - Accumulate each winding.
  // - If the sum winding is not 0, the point is inside the polygon.
  int winding_sum = 0;
  auto it2 = m_point_vec.begin();
  auto it1 = it2++;
  int winding_last = -1;
  while (m_point_vec.end() != it1) {
    bool do_intersect = false;
    if (it1->y == it2->y) {
    } else if ((a_y == it1->y && a_x < it1->x) ||
        (a_y == it2->y && a_x < it2->x)) {
      int winding = it2->y >= it1->y;
      if (-1 != winding_last && winding == winding_last) {
        do_intersect = true;
      }
      winding_last = winding;
    } else {
      // Ray-line intersection.
      // x + s = x1 + t(x2-x1)
      // y = y1 + t(y2-y1)
      // t = (y-y1) / (y2-y1), 0<t<1
      // s = x1 + t(x2-x1) - x, s >= 0
      auto t = (a_y - it1->y) / (it2->y - it1->y);
      if (0 < t && t < 1) {
        auto s = it1->x + t * (it2->x - it1->x) - a_x;
        do_intersect = s >= 0;
      }
    }
    if (do_intersect) {
      auto winding = it2->y >= it1->y ? 1 : -1;
      winding_sum += winding;
    }
    ++it1;
    ++it2;
    if (m_point_vec.end() == it2) {
      it2 = m_point_vec.begin();
    }
  }
  return 0 != winding_sum;
}

CutConsumerList::Entry::Entry(NodeCuttable *a_node, bool *a_is_ok):
  node(a_node),
  is_ok(a_is_ok)
{
}

CutConsumerList::CutConsumerList():
  m_cut_vec()
{
}

void CutConsumerList::Add(NodeCuttable *a_node, bool *a_is_ok)
{
  m_cut_vec.push_back(Entry(a_node, a_is_ok));
}

bool CutConsumerList::IsOk() const
{
  for (auto it = m_cut_vec.begin(); m_cut_vec.end() != it; ++it) {
    if (!*it->is_ok) {
      return false;
    }
  }
  return true;
}

void CutConsumerList::Process(uint64_t a_evid)
{
  for (auto it = m_cut_vec.begin(); m_cut_vec.end() != it; ++it) {
    it->node->Process(a_evid);
  }
}

CutProducerList::EntryData::EntryData(CutPolyMap::iterator a_it):
  cut_poly_it(a_it),
  value()
{
}

CutProducerList::EntryEvent::EntryEvent(CutPolyMap::iterator a_it):
  cut_poly_it(a_it),
  is_ok()
{
}

CutProducerList::CutProducerList():
  m_cut_poly_map(),
  m_cut_data_vec(),
  m_cut_event_vec()
{
}

CutProducerList::~CutProducerList()
{
  for (auto it = m_cut_data_vec.begin(); m_cut_data_vec.end() != it; ++it) {
    delete *it;
  }
}

NodeCutValue *CutProducerList::AddData(CutPolygon const *a_poly)
{
  auto it = AddCutPolygon(a_poly);
  m_cut_data_vec.push_back(new EntryData(it));
  return &m_cut_data_vec.back()->value;
}

bool *CutProducerList::AddEvent(CutPolygon const *a_poly)
{
  auto it = AddCutPolygon(a_poly);
  m_cut_event_vec.push_back(EntryEvent(it));
  return &m_cut_event_vec.back().is_ok;
}

CutProducerList::CutPolyMap::iterator
CutProducerList::AddCutPolygon(CutPolygon const *a_poly)
{
  return m_cut_poly_map.insert(std::make_pair(a_poly, false)).first;
}

void CutProducerList::Reset()
{
  for (auto it = m_cut_data_vec.begin(); m_cut_data_vec.end() != it; ++it) {
    auto entry = *it;
    entry->value.x.Clear();
    entry->value.y.Clear();
  }
  for (auto it = m_cut_event_vec.begin(); m_cut_event_vec.end() != it; ++it) {
    it->is_ok = false;
  }
}

void CutProducerList::Test(Input::Type a_type, Input::Scalar const &a_x)
{
  auto x = a_x.GetDouble(a_type);
  // Test polys.
  for (auto it = m_cut_poly_map.begin(); m_cut_poly_map.end() != it; ++it) {
    it->second = it->first->Test(x);
  }
  // Save hits inside cut.
  for (auto it = m_cut_data_vec.begin(); m_cut_data_vec.end() != it; ++it) {
    auto entry = *it;
    if (entry->cut_poly_it->second) {
      // TODO: Channel?
      entry->value.x.SetType(a_type);
      entry->value.x.Push(0, a_x);
    }
  }
  // Accumulate event cuts.
  for (auto it = m_cut_event_vec.begin(); m_cut_event_vec.end() != it; ++it) {
    it->is_ok |= it->cut_poly_it->second;
  }
}

void CutProducerList::Test(Input::Type a_x_type, Input::Scalar const &a_x,
    Input::Type a_y_type, Input::Scalar const &a_y)
{
  auto x = a_x.GetDouble(a_x_type);
  auto y = a_y.GetDouble(a_y_type);
  // Test polys.
  for (auto it = m_cut_poly_map.begin(); m_cut_poly_map.end() != it; ++it) {
    it->second = it->first->Test(x, y);
  }
  // Pass through data cuts.
  for (auto it = m_cut_data_vec.begin(); m_cut_data_vec.end() != it; ++it) {
    auto entry = *it;
    if (entry->cut_poly_it->second) {
      // TODO: Channel?
      entry->value.x.SetType(a_x_type);
      entry->value.x.Push(0, a_x);
      entry->value.y.SetType(a_y_type);
      entry->value.y.Push(0, a_y);
    }
  }
  // Accumulate event cuts.
  for (auto it = m_cut_event_vec.begin(); m_cut_event_vec.end() != it; ++it) {
    it->is_ok |= it->cut_poly_it->second;
  }
}
