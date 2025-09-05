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

#ifndef MOCK_NODE_HPP
#define MOCK_NODE_HPP

#include <node.hpp>

#define MOCK_NODE_CUTTABLE(name) \
    name(std::string const &a_title, Input::Type a_type, unsigned a_val_n): \
      MockNodeCuttable(a_title, a_type, a_val_n) {}
#define MOCK_NODE_VALUE(name) \
    name(Input::Type a_type, unsigned a_val_n): \
      MockNodeValue(a_type, a_val_n) {}

class MockNodeCuttable: public NodeCuttable {
  public:
    MockNodeCuttable(std::string const &, Input::Type, unsigned);
    CutPolygon const &GetCutPolygon() const;
    Value const &GetValue(uint32_t);
    void Preprocess(Node *);
    void Process(uint64_t);
    virtual void ProcessUser(CutProducerList &);

    std::vector<Value> m_value;

  private:
    MockNodeCuttable(MockNodeCuttable const &);
    MockNodeCuttable &operator=(MockNodeCuttable const &);

    Node *m_parent;
};

class MockNodeValue: public NodeValue {
  public:
    MockNodeValue(Input::Type, unsigned);
    Value const &GetValue(uint32_t);
    void Preprocess(Node *);
    void Process(uint64_t);
    virtual void ProcessUser();

    std::vector<Value> m_value;

  private:
    MockNodeValue(MockNodeValue const &);
    MockNodeValue &operator=(MockNodeValue const &);

    Node *m_parent;
};

void TestNodeBase(Node &, std::string const &);
void TestNodeProcess(Node &, uint64_t);

#endif
