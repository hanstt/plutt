/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023  Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
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

#ifndef NODE_HPP
#define NODE_HPP

#include <cut.hpp>

// TODO: Consider encapsulation over inheritance?

// Asserts something, on failure prints location in config file.
#define NODE_ASSERT(l, op, r) do {\
    auto l_ = l;\
    auto r_ = r;\
    if (!(l_ op r_)) {\
      std::cerr << __FILE__ << ':' << __LINE__ << ": " <<\
          GetLocStr() << ": Assertion '" << #l << "'='" << l_ << "' "#op \
          " '" << #r << "'='" << r_ << "' failed!\n";\
      throw std::runtime_error(__func__);\
    }\
  } while (0)
// Should always be called if a child node is processed!
#define NODE_PROCESS_GUARD(evid) \
    if (IsActive()) { \
      std::cerr << GetLocStr() + ": Node loop!\n"; \
      throw std::runtime_error(__func__); \
    } \
    if (IsEvent(evid)) { \
      return; \
    } \
    Node::ProcessGuard node_process_guard_(this, evid)
// Use this to process a child node, NOT the method directly!
#define NODE_PROCESS(node, evid) do { \
  assert(IsActive()); \
  node->Process(evid); \
} while (0)

class CutPolygon;
struct NodeCutValue;
class Value;

/* Base node stuff. */
class Node {
  public:
    class ProcessGuard {
      public:
        ProcessGuard(Node *, uint64_t);
        ~ProcessGuard();
      private:
        ProcessGuard(ProcessGuard const &);
        ProcessGuard &operator=(ProcessGuard const &);

        Node *m_node;
    };

    Node(std::string const &);
    virtual ~Node() {}
    std::string GetLocStr() const;
    bool IsActive() const;
    bool IsEvent(uint64_t) const;
    // Do NOT call this! Use the macro! Macros are good. Really. Sometimes.
    virtual void Process(uint64_t) = 0;

  protected:
    std::string m_loc;
  private:
    uint64_t m_evid;
    bool m_is_active;
};

/* Node interface which holds a value. */
class NodeValue: public Node {
  public:
    NodeValue(std::string const &);
    virtual ~NodeValue() {}
    virtual Value const &GetValue(uint32_t = 0) = 0;
};

/* Node which produces/consumes cuts. */
class NodeCuttable: public Node {
  public:
    NodeCuttable(std::string const &, std::string const &);
    // Hits inside the polygon are recorded in the returned object.
    NodeCutValue *CutDataAdd(CutPolygon const *);
    // If the given node has no hit inside the given polygon, this node should
    // stop processing the even.
    void CutEventAdd(NodeCuttable *, CutPolygon const *);
    void CutReset();
    std::string const &GetTitle() const;

  protected:
    std::string m_title;
    // The list of cut conditions on other nodes.
    CutConsumerList m_cut_consumer;
    // The list of cuts to populate by this node.
    CutProducerList m_cut_producer;
};

#endif
