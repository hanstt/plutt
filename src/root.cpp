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

#if PLUTT_ROOT

#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <config.hpp>
#include <filewatcher.hpp>
#include <util.hpp>
#include <root.hpp>

class RootChain {
  public:
    RootChain(Config &, Root *, int, char **);
    ~RootChain();
    void Buffer();
    bool Fetch();

  private:
    RootChain(RootChain const &);
    RootChain &operator=(RootChain const &);

    void BindBranch(Config &, std::string const &, std::string const &,
        NodeSignal::MemberType);
    bool FindBranch(std::string const &);

    Root *m_root;
    TChain m_chain;
    TTreeReader m_reader;
    struct Entry {
      Entry(std::string a_name, EDataType a_in_type, Input::Type a_out_type,
            bool a_is_vector):
        name(a_name),
        in_type(a_in_type),
        out_type(a_out_type),
        is_vector(a_is_vector),
        val_Char_t(),
        arr_Char_t(),
        val_Short_t(),
        arr_Short_t(),
        val_Int_t(),
        arr_Int_t(),
        val_Long_t(),
        arr_Long_t(),
        val_UChar_t(),
        arr_UChar_t(),
        val_UShort_t(),
        arr_UShort_t(),
        val_UInt_t(),
        arr_UInt_t(),
        val_ULong_t(),
        arr_ULong_t(),
        val_Float_t(),
        arr_Float_t(),
        val_Double_t(),
        arr_Double_t()
      {
      }
      Entry(Entry const &a_e):
        name(),
        in_type(),
        out_type(),
        is_vector(),
        val_Char_t(),
        arr_Char_t(),
        val_Short_t(),
        arr_Short_t(),
        val_Int_t(),
        arr_Int_t(),
        val_Long_t(),
        arr_Long_t(),
        val_UChar_t(),
        arr_UChar_t(),
        val_UShort_t(),
        arr_UShort_t(),
        val_UInt_t(),
        arr_UInt_t(),
        val_ULong_t(),
        arr_ULong_t(),
        val_Float_t(),
        arr_Float_t(),
        val_Double_t(),
        arr_Double_t()
      {
        Copy(a_e);
      }
      Entry &operator=(Entry const &a_e)
      {
        Copy(a_e);
        return *this;
      }
      std::string name;
      EDataType in_type;
      Input::Type out_type;
      bool is_vector;
      TTreeReaderValue<Char_t> *val_Char_t;
      TTreeReaderArray<Char_t> *arr_Char_t;
      TTreeReaderValue<Short_t> *val_Short_t;
      TTreeReaderArray<Short_t> *arr_Short_t;
      TTreeReaderValue<Int_t> *val_Int_t;
      TTreeReaderArray<Int_t> *arr_Int_t;
      TTreeReaderValue<Long_t> *val_Long_t;
      TTreeReaderArray<Long_t> *arr_Long_t;
      TTreeReaderValue<UChar_t> *val_UChar_t;
      TTreeReaderArray<UChar_t> *arr_UChar_t;
      TTreeReaderValue<UShort_t> *val_UShort_t;
      TTreeReaderArray<UShort_t> *arr_UShort_t;
      TTreeReaderValue<UInt_t> *val_UInt_t;
      TTreeReaderArray<UInt_t> *arr_UInt_t;
      TTreeReaderValue<ULong_t> *val_ULong_t;
      TTreeReaderArray<ULong_t> *arr_ULong_t;
      TTreeReaderValue<Float_t> *val_Float_t;
      TTreeReaderArray<Float_t> *arr_Float_t;
      TTreeReaderValue<Double_t> *val_Double_t;
      TTreeReaderArray<Double_t> *arr_Double_t;
      private:
      void Copy(Entry const &a_e)
      {
        name = a_e.name;
        in_type = a_e.in_type;
        out_type = a_e.out_type;
        is_vector = a_e.is_vector;
        // With great power comes great guns to shoot your foot with.
        // We can cheat a bit here:
        // If we never copy m_branch_vec, copying is only done on resizing so
        // there's really only one place for these.
        // The ownership is really with RootChain since it allocates, so it
        // must also delete.
        val_Char_t = a_e.val_Char_t;
        arr_Char_t = a_e.arr_Char_t;
        val_Short_t = a_e.val_Short_t;
        arr_Short_t = a_e.arr_Short_t;
        val_Int_t = a_e.val_Int_t;
        arr_Int_t = a_e.arr_Int_t;
        val_Long_t = a_e.val_Long_t;
        arr_Long_t = a_e.arr_Long_t;
        val_UChar_t = a_e.val_UChar_t;
        arr_UChar_t = a_e.arr_UChar_t;
        val_UShort_t = a_e.val_UShort_t;
        arr_UShort_t = a_e.arr_UShort_t;
        val_UInt_t = a_e.val_UInt_t;
        arr_UInt_t = a_e.arr_UInt_t;
        val_ULong_t = a_e.val_ULong_t;
        arr_ULong_t = a_e.arr_ULong_t;
        val_Float_t = a_e.val_Float_t;
        arr_Float_t = a_e.arr_Float_t;
        val_Double_t = a_e.val_Double_t;
        arr_Double_t = a_e.arr_Double_t;
      }
    };
    std::vector<Entry> m_branch_vec;
    Long64_t m_ev_n;
    Long64_t m_ev_i;
    Long64_t m_ev_i_latch;
    uint64_t m_progress_t_last;
};

RootChain::RootChain(Config &a_config, Root *a_root, int a_argc, char
    **a_argv):
  m_root(a_root),
  m_chain(a_argv[0]),
  m_reader(&m_chain),
  m_branch_vec(),
  m_ev_n(),
  m_ev_i(),
  m_ev_i_latch(),
  m_progress_t_last()
{
  auto signal_list = a_config.GetSignalList();

  if (a_argc < 2) {
    std::cerr << "Missing input files.\n";
    throw std::runtime_error(__func__);
  }

  // Setup chain.
  for (int i = 1; i < a_argc; ++i) {
    std::cout << a_argv[i] << ": Adding.\n";
    m_chain.Add(a_argv[i]);
  }
  m_ev_n = m_chain.GetEntries();

  // Make sure we have a working tree if weird files were added.
  if (m_chain.GetNtrees() > 0) {
    // Look for branches for each signal.
    for (auto it = signal_list.begin(); signal_list.end() != it; ++it) {
      auto name = *it;
      auto has_ = FindBranch(name);
      auto has_MI = FindBranch(name + "MI");
      auto has_ME = FindBranch(name + "ME");
      auto has_v = FindBranch(name + "v");
      if (has_ && has_MI && has_ME && has_v) {
        BindBranch(a_config, name, name + "MI", NodeSignal::kId);
        BindBranch(a_config, name, name + "ME", NodeSignal::kEnd);
        BindBranch(a_config, name, name + "v", NodeSignal::kV);
        continue;
      }
      auto has_I = FindBranch(name + "I");
      auto has_E = FindBranch(name + "E");
      if (has_ && has_I && (has_v || has_E)) {
        BindBranch(a_config, name, name + "I", NodeSignal::kId);
        if (has_v) {
          BindBranch(a_config, name, name + "v", NodeSignal::kV);
        } else {
          BindBranch(a_config, name, name + "E", NodeSignal::kV);
        }
        continue;
      }
      if (has_ && (has_v || has_E)) {
        if (has_v) {
          BindBranch(a_config, name, name + "v", NodeSignal::kV);
        } else {
          BindBranch(a_config, name, name + "E", NodeSignal::kV);
        }
        continue;
      }
      BindBranch(a_config, name, name, NodeSignal::kV);
    }
  }
}

RootChain::~RootChain()
{
  for (auto it = m_branch_vec.begin(); m_branch_vec.end() != it; ++it) {
    delete it->val_Char_t;
    delete it->arr_Char_t;
    delete it->val_Short_t;
    delete it->arr_Short_t;
    delete it->val_Int_t;
    delete it->arr_Int_t;
    delete it->val_Long_t;
    delete it->arr_Long_t;
    delete it->val_UChar_t;
    delete it->arr_UChar_t;
    delete it->val_UShort_t;
    delete it->arr_UShort_t;
    delete it->val_UInt_t;
    delete it->arr_UInt_t;
    delete it->val_ULong_t;
    delete it->arr_ULong_t;
    delete it->val_Float_t;
    delete it->arr_Float_t;
    delete it->val_Double_t;
    delete it->arr_Double_t;
  }
}

void RootChain::BindBranch(Config &a_config, std::string const &a_base_name,
    std::string const &a_name, NodeSignal::MemberType a_member_type)
{
  // branch.GetFullName() gives only the full name which we already know:
  //  GetBranch("m").GetFullName() = "b.m".
  //  GetBranch("b.m").GetFullName() = "b.m".
  // branch.GetTitle() gives vector and type info, but only the leaf name:
  //  GetBranch("m").GetTitle() = "m[arr_ref]/i".
  //  GetBranch("b.m").GetTitle() = "m[arr_ref]/i".
  auto dot = a_name.find_last_of('.');
  std::string member = a_name.npos == dot ? a_name : a_name.substr(dot + 1);

  auto branch = m_chain.GetBranch(a_name.c_str());
  if (!branch) {
    std::cerr << a_name << ": Could not find branch for signal.\n";
    throw std::runtime_error(__func__);
  }

  // The title looks like "name/type" or "name[arr_ref]/type".
  auto title = branch->GetTitle();
  if (0 != strncmp(member.c_str(), title, member.length())) {
    std::cerr << a_name << ": member=" << member << " title=" << title <<
        " mismatch.\n";
    throw std::runtime_error(__func__);
  }
  auto bracket = &title[member.length()];
  bool is_vector = '[' == *bracket;

  TClass *exp_cls;
  EDataType exp_type;
  branch->GetExpectedType(exp_cls, exp_type);
  if (exp_cls) {
    std::cerr << a_name << ": Class members not supported.\n";
    throw std::runtime_error(__func__);
  }
  Input::Type out_type;
  /*
   * Cast away enum, new ROOT primitive types are out of our control.
   * Happens a few times in this file!
   */
  switch ((unsigned)exp_type) {
    case kUChar_t:
    case kUShort_t:
    case kUInt_t:
    case kULong_t:
    case kULong64_t:
      out_type = Input::kUint64;
      break;
    case kChar_t:
    case kShort_t:
    case kInt_t:
    case kLong_t:
    case kLong64_t:
      out_type = Input::kInt64;
      break;
    case kFloat_t:
    case kDouble_t:
      out_type = Input::kDouble;
      break;
    default:
      std::cerr << a_name << ": Unsupported ROOT type " << exp_type << ".\n";
      throw std::runtime_error(__func__);
  }

  auto id = m_branch_vec.size();
  m_branch_vec.push_back(
      RootChain::Entry(a_name, exp_type, out_type, is_vector));
  auto &entry = m_branch_vec.back();

  // Reader instantiation ladder.
  switch ((unsigned)exp_type) {
#define READER_MAKE_TYPE(root_type) \
    case k##root_type: \
      if (is_vector) { \
        entry.arr_##root_type = new \
            TTreeReaderArray<root_type>(m_reader, a_name.c_str()); \
      } else { \
        entry.val_##root_type = new \
            TTreeReaderValue<root_type>(m_reader, a_name.c_str()); \
      } \
      break

    READER_MAKE_TYPE(Char_t);
    READER_MAKE_TYPE(Short_t);
    READER_MAKE_TYPE(Int_t);
    READER_MAKE_TYPE(Long_t);
    READER_MAKE_TYPE(UChar_t);
    READER_MAKE_TYPE(UShort_t);
    READER_MAKE_TYPE(UInt_t);
    READER_MAKE_TYPE(ULong_t);
    READER_MAKE_TYPE(Float_t);
    READER_MAKE_TYPE(Double_t);
    default:
      std::cerr << a_name <<
          ": Non-implemented input type " << out_type << ".\n";
      throw std::runtime_error(__func__);
  }

  a_config.BindSignal(a_base_name, a_member_type, id, out_type);
  m_root->BindSignal(id);
}

bool RootChain::FindBranch(std::string const &a_name)
{
  return nullptr != m_chain.GetBranch(a_name.c_str());
}

void RootChain::Buffer()
{
  // Copy from readers to vectors in Root.
  for (size_t id = 0; id < m_branch_vec.size(); ++id) {
    auto it = &m_branch_vec.at(id);
    auto &buf = m_root->GetBuffer(id); \
    // TODO: Error-checking!
    switch ((unsigned)it->in_type) {
#define BUF_COPY_TYPE(root_type, s_type, s_member) \
      case k##root_type: \
        if (it->is_vector) { \
          auto const size = it->arr_##root_type->GetSize(); \
          buf.resize(size); \
          for (size_t i = 0; i < size; ++i) { \
            buf[i].s_member = (s_type)it->arr_##root_type->At(i); \
          } \
        } else { \
          buf.resize(1); \
          buf[0].s_member = (s_type)**it->val_##root_type; \
        } \
        break
      BUF_COPY_TYPE(Char_t,   uint64_t, u64);
      BUF_COPY_TYPE(Short_t,  uint64_t, u64);
      BUF_COPY_TYPE(Int_t,    uint64_t, u64);
      BUF_COPY_TYPE(Long_t,   uint64_t, u64);
      BUF_COPY_TYPE(UChar_t,  uint64_t, u64);
      BUF_COPY_TYPE(UShort_t, uint64_t, u64);
      BUF_COPY_TYPE(UInt_t,   uint64_t, u64);
      BUF_COPY_TYPE(ULong_t,  uint64_t, u64);
      BUF_COPY_TYPE(Float_t,  double,   dbl);
      BUF_COPY_TYPE(Double_t, double,   dbl);
      default:
        std::cerr << it->name << ": Non-implemented input type.\n";
        throw std::runtime_error(__func__);
    }
  }
}

bool RootChain::Fetch()
{
  if (!m_chain.GetTree()) {
    return false;
  }
  if (!m_reader.Next()) {
    return false;
  }

  // Progress meter.
  if (Time_get_ms() > m_progress_t_last + 1000 ||
      m_ev_i + 1 == m_ev_n) {
    auto rate = m_ev_i - m_ev_i_latch;
    std::string prefix = "";
    if (rate > 1000) {
      rate /= 1000;
      prefix = "k";
    }
    std::cout << "Event: " <<
        m_ev_i << "/" << m_ev_n <<
        " (" << rate << prefix << "/s)" <<
        "\033[0K\r" << std::flush;
    m_progress_t_last = Time_get_ms();
    m_ev_i_latch = m_ev_i;
  }
  ++m_ev_i;

  return true;
}

Root::Root(bool a_is_files, Config *a_config, int a_argc, char **a_argv):
  m_watcher(),
  m_chain(),
  m_buf_vec()
{
  if (a_is_files) {
    m_chain = new RootChain(*a_config, this, a_argc, a_argv);
  } else {
    /* Setup directory watching. */
    m_watcher.config = a_config;
    m_watcher.tree_name = a_argv[0];
    std::vector<std::string> v;
    for (int i = 1; i < a_argc; ++i) {
      v.push_back(a_argv[i]);
    }
    m_watcher.file_watcher = new FileWatcher(v);
  }
}

Root::~Root()
{
  delete m_chain;
  delete m_watcher.file_watcher;
}

void Root::BindSignal(size_t a_id)
{
  if (a_id >= m_buf_vec.size()) {
    auto i = m_buf_vec.size();
    m_buf_vec.resize(a_id + 1);
    for (; i < m_buf_vec.size(); ++i) {
      m_buf_vec.at(i) = new Vector<Input::Scalar>();
    }
  }
}

Vector<Input::Scalar> &Root::GetBuffer(size_t a_id)
{
  return *m_buf_vec.at(a_id);
}

void Root::Buffer()
{
  if (m_chain) {
    m_chain->Buffer();
  }
}

bool Root::Fetch()
{
  if (!m_chain) {
    // If there's a newly written + closed file, start a chain on it.
    auto path = m_watcher.file_watcher->WaitFile(1000);
    if (path.size() > 0) {
      char *argv[2];
      argv[0] = (char *)m_watcher.tree_name.c_str();
      argv[1] = (char *)path.c_str();
      m_chain = new RootChain(*m_watcher.config, this, LENGTH(argv), argv);
    }
  }
  if (m_chain) {
    auto ok = m_chain->Fetch();
    if (!m_watcher.file_watcher) {
      // Only auto-quit input if we have fixed files.
      return ok;
    }
    if (!ok) {
      m_watcher.config->UnbindSignals();
      delete m_chain;
      m_chain = nullptr;
    }
  }
  return true;
}

std::pair<Input::Scalar const *, size_t> Root::GetData(size_t a_id)
{
  if (!m_chain) {
    return std::make_pair(nullptr, 0);
  }
  auto const &v = GetBuffer(a_id);
  if (v.empty()) {
    return std::make_pair(nullptr, 0);
  }
  return std::make_pair(v.begin(), v.size());
}

#endif
