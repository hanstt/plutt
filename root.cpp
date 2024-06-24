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

#if PLUTT_ROOT

#include <linux/limits.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <sys/stat.h>

#include <TChain.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <config.hpp>
#include <util.hpp>
#include <root.hpp>

class RootChain {
  public:
    RootChain(Config &, int, char **);
    ~RootChain();
    void Buffer();
    bool Fetch();
    std::pair<Input::Scalar const *, size_t> GetData(size_t);

  private:
    void BindBranch(Config &, std::string const &, char const *, char const *,
        bool);

    TChain m_chain;
    TTreeReader m_reader;
    struct Entry {
      Entry(std::string a_name, EDataType a_in_type, Input::Type a_out_type,
            bool a_is_vector):
        name(a_name),
        in_type(a_in_type),
        out_type(a_out_type),
        is_vector(a_is_vector),
        val_char(),
        arr_char(),
        val_short(),
        arr_short(),
        val_int(),
        arr_int(),
        val_long(),
        arr_long(),
        val_uchar(),
        arr_uchar(),
        val_ushort(),
        arr_ushort(),
        val_uint(),
        arr_uint(),
        val_ulong(),
        arr_ulong(),
        val_float(),
        arr_float(),
        val_double(),
        arr_double(),
        buf()
      {
      }
      Entry(Entry const &a_e):
        name(),
        in_type(),
        out_type(),
        is_vector(),
        val_char(),
        arr_char(),
        val_short(),
        arr_short(),
        val_int(),
        arr_int(),
        val_long(),
        arr_long(),
        val_uchar(),
        arr_uchar(),
        val_ushort(),
        arr_ushort(),
        val_uint(),
        arr_uint(),
        val_ulong(),
        arr_ulong(),
        val_float(),
        arr_float(),
        val_double(),
        arr_double(),
        buf()
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
      TTreeReaderValue<Char_t> *val_char;
      TTreeReaderArray<Char_t> *arr_char;
      TTreeReaderValue<Short_t> *val_short;
      TTreeReaderArray<Short_t> *arr_short;
      TTreeReaderValue<Int_t> *val_int;
      TTreeReaderArray<Int_t> *arr_int;
      TTreeReaderValue<Long_t> *val_long;
      TTreeReaderArray<Long_t> *arr_long;
      TTreeReaderValue<UChar_t> *val_uchar;
      TTreeReaderArray<UChar_t> *arr_uchar;
      TTreeReaderValue<UShort_t> *val_ushort;
      TTreeReaderArray<UShort_t> *arr_ushort;
      TTreeReaderValue<UInt_t> *val_uint;
      TTreeReaderArray<UInt_t> *arr_uint;
      TTreeReaderValue<ULong_t> *val_ulong;
      TTreeReaderArray<ULong_t> *arr_ulong;
      TTreeReaderValue<Float_t> *val_float;
      TTreeReaderArray<Float_t> *arr_float;
      TTreeReaderValue<Double_t> *val_double;
      TTreeReaderArray<Double_t> *arr_double;
      Vector<Input::Scalar> buf;
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
        val_char = a_e.val_char;
        arr_char = a_e.arr_char;
        val_short = a_e.val_short;
        arr_short = a_e.arr_short;
        val_int = a_e.val_int;
        arr_int = a_e.arr_int;
        val_long = a_e.val_long;
        arr_long = a_e.arr_long;
        val_uchar = a_e.val_uchar;
        arr_uchar = a_e.arr_uchar;
        val_ushort = a_e.val_ushort;
        arr_ushort = a_e.arr_ushort;
        val_uint = a_e.val_uint;
        arr_uint = a_e.arr_uint;
        val_ulong = a_e.val_ulong;
        arr_ulong = a_e.arr_ulong;
        val_float = a_e.val_float;
        arr_float = a_e.arr_float;
        val_double = a_e.val_double;
        arr_double = a_e.arr_double;
      }
    };
    std::vector<Entry> m_branch_vec;
    Long64_t m_ev_n;
    Long64_t m_ev_i;
    Long64_t m_ev_i_latch;
    uint64_t m_progress_t_last;
};

RootChain::RootChain(Config &a_config, int a_argc, char **a_argv):
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
    struct stat st;
    auto rc = stat(a_argv[i], &st);
    if (0 != rc) {
      std::cerr << a_argv[i] << ": Could not stat.\n";
      throw std::runtime_error(__func__);
    }
    std::cout << a_argv[i] << ": Adding.\n";
    m_chain.Add(a_argv[i]);
  }
  m_ev_n = m_chain.GetEntries();

  // Look for branches for each signal.
  for (auto it = signal_list.begin(); signal_list.end() != it; ++it) {
    BindBranch(a_config, *it,   "",   "", false);
    BindBranch(a_config, *it,  "M",  "M", true);
    BindBranch(a_config, *it,  "I",  "I", true);
    BindBranch(a_config, *it, "MI", "MI", true);
    BindBranch(a_config, *it, "ME", "ME", true);
    BindBranch(a_config, *it,  "v",  "v", true);
    BindBranch(a_config, *it,  "E",  "v", true);
  }
}

RootChain::~RootChain()
{
  for (auto it = m_branch_vec.begin(); m_branch_vec.end() != it; ++it) {
    delete it->val_uchar;
    delete it->arr_uchar;
    delete it->val_ushort;
    delete it->arr_ushort;
    delete it->val_uint;
    delete it->arr_uint;
    delete it->val_ulong;
    delete it->arr_ulong;
    delete it->val_float;
    delete it->arr_float;
    delete it->val_double;
    delete it->arr_double;
  }
}

void RootChain::BindBranch(Config &a_config, std::string const &a_name, char
    const *a_suffix, char const *a_config_suffix, bool a_optional)
{
  // full = name + suffix
  // If full = "m*", then GetBranch("m*").GetTitle() = "m*".
  // If full = "b.m*", then GetBranch("b.m*").GetTitle() = "m*".
  auto full_name = a_name + (a_suffix ? a_suffix : "");
  auto dot = full_name.find_first_of('.');
  std::string member =
      full_name.npos == dot ? full_name : full_name.substr(dot + 1);

  auto branch = m_chain.GetBranch(full_name.c_str());
  if (!branch) {
    if (a_optional) {
      return;
    }
    std::cerr << full_name << ": Could not find branch for signal.\n";
    throw std::runtime_error(__func__);
  }

  // The title looks like "full_name*" or "full_name[arr_ref]*".
  auto title = branch->GetTitle();
  if (0 != strncmp(member.c_str(), title, member.length())) {
    std::cerr << full_name << ": member=" << member <<
        " title=" << title << " mismatch.\n";
    throw std::runtime_error(__func__);
  }
  auto bracket = &title[member.length()];
  bool is_vector = '[' == *bracket;

  TClass *exp_cls;
  EDataType exp_type;
  branch->GetExpectedType(exp_cls, exp_type);
  if (exp_cls) {
    std::cerr << full_name << ": Class members not supported.\n";
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
      std::cerr << full_name <<
          ": Unsupported ROOT type " << exp_type << ".\n";
      throw std::runtime_error(__func__);
  }

  auto id = m_branch_vec.size();
  m_branch_vec.push_back(RootChain::Entry(full_name, exp_type, out_type,
        is_vector));
  auto &entry = m_branch_vec.back();

  // Reader instantiation ladder.
  switch ((unsigned)exp_type) {
#define READER_MAKE_TYPE(root_type, c_type, reader_type)     \
    case root_type: \
      if (is_vector) { \
        entry.arr_##reader_type = new \
            TTreeReaderArray<c_type>(m_reader, full_name.c_str()); \
      } else { \
        entry.val_##reader_type = new                                        \
            TTreeReaderValue<c_type>(m_reader, full_name.c_str()); \
      } \
      break

    READER_MAKE_TYPE(kChar_t,   Char_t,   char);
    READER_MAKE_TYPE(kShort_t,  Short_t,  short);
    READER_MAKE_TYPE(kInt_t,    Int_t,    int);
    READER_MAKE_TYPE(kLong_t,   Long_t,   long);
    READER_MAKE_TYPE(kUChar_t,  UChar_t,  uchar);
    READER_MAKE_TYPE(kUShort_t, UShort_t, ushort);
    READER_MAKE_TYPE(kUInt_t,   UInt_t,   uint);
    READER_MAKE_TYPE(kULong_t,  ULong_t,  ulong);
    READER_MAKE_TYPE(kFloat_t,  float,    float);
    READER_MAKE_TYPE(kDouble_t, double,   double);
    default:
      std::cerr << full_name <<
          ": Non-implemented input type " << out_type << ".\n";
      throw std::runtime_error(__func__);
  }

  a_config.BindSignal(a_name, a_config_suffix, id, out_type);
}

void RootChain::Buffer()
{
  // Copy from readers to vectors.
  for (auto it = m_branch_vec.begin(); m_branch_vec.end() != it; ++it) {
    // TODO: Error-checking!
    switch ((unsigned)it->in_type) {
#define BUF_COPY_TYPE(root_type, reader_type, s_type, s_member) \
      case root_type: \
        if (it->is_vector) { \
          auto const size = it->arr_##reader_type->GetSize(); \
          it->buf.resize(size); \
          for (size_t i = 0; i < size; ++i) { \
            it->buf[i].s_member = (s_type)it->arr_##reader_type->At(i); \
          } \
        } else { \
          it->buf.resize(1); \
          it->buf[0].s_member = (s_type)**it->val_##reader_type; \
        } \
        break
      BUF_COPY_TYPE(kChar_t,   char,   uint64_t, u64);
      BUF_COPY_TYPE(kShort_t,  short,  uint64_t, u64);
      BUF_COPY_TYPE(kInt_t,    int,    uint64_t, u64);
      BUF_COPY_TYPE(kLong_t,   long,   uint64_t, u64);
      BUF_COPY_TYPE(kUChar_t,  uchar,  uint64_t, u64);
      BUF_COPY_TYPE(kUShort_t, ushort, uint64_t, u64);
      BUF_COPY_TYPE(kUInt_t,   uint,   uint64_t, u64);
      BUF_COPY_TYPE(kULong_t,  ulong,  uint64_t, u64);
      BUF_COPY_TYPE(kFloat_t,  float,  double,   dbl);
      BUF_COPY_TYPE(kDouble_t, double, double,   dbl);
      default:
        std::cerr << it->name << ": Non-implemented input type.\n";
        throw std::runtime_error(__func__);
    }
  }
}

bool RootChain::Fetch()
{
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

std::pair<Input::Scalar const *, size_t> RootChain::GetData(size_t a_id)
{
  auto const &entry = m_branch_vec.at(a_id);
  if (entry.is_vector) {
    if (entry.buf.empty()) {
      return std::make_pair(nullptr, 0);
    }
    return std::make_pair(&entry.buf.at(0), entry.buf.size());
  }
  return std::make_pair(&entry.buf.at(0), 1);
}

Root::Root(bool a_is_files, Config *a_config, int a_argc, char **a_argv):
  m_is_files(a_is_files),
  m_inotify(),
  m_chain()
{
  if (m_is_files) {
    m_chain = new RootChain(*a_config, a_argc, a_argv);
  } else {
    /* Setup directory watching. */
    m_inotify.config = a_config;
    m_inotify.tree_name = a_argv[0];
    m_inotify.fd = inotify_init();
    if (m_inotify.fd < 0) {
      std::cerr << "inotify_init: " << strerror(errno) << ".\n";
      throw std::runtime_error(__func__);
    }
    for (int i = 1; i < a_argc; ++i) {
      int wd;

      wd = inotify_add_watch(m_inotify.fd, a_argv[i], IN_CLOSE_WRITE);
      if (wd < 0) {
        std::cerr << "inotify_add_watch(" << a_argv[i] << "): " <<
            strerror(errno) << ".\n";
        throw std::runtime_error(__func__);
      }
      m_inotify.wd_map.insert(std::make_pair(wd, a_argv[i]));
    }
  }
}

Root::~Root()
{
  delete m_chain;
  if (!m_is_files) {
    for (auto it = m_inotify.wd_map.begin(); m_inotify.wd_map.end() != it;
        ++it) {
      if (inotify_rm_watch(m_inotify.fd, it->first) < 0) {
        std::cerr << "inotify_rm_watch: " << strerror(errno) << ".\n";
      }
    }
    if (close(m_inotify.fd) < 0) {
      std::cerr << "close(inotify): " << strerror(errno) << ".\n";
    }
  }
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
    // If there's a newly written + close file, start a chain on it.
    int nfds;
    {
      struct pollfd fds[1];
      fds[0].fd = m_inotify.fd;
      fds[0].events = POLLIN;
      nfds = poll(fds, LENGTH(fds), 1000);
      if (nfds < 0) {
        std::cerr << "poll: " << strerror(errno) << ".\n";
        throw std::runtime_error(__func__);
      }
    }
    if (nfds > 0) {
      char buffer[sizeof(inotify_event) + NAME_MAX];
      auto rc = read(m_inotify.fd, buffer, sizeof buffer);
      if (rc < 0) {
        std::cerr << "read: " << strerror(errno) << ".\n";
        throw std::runtime_error(__func__);
      }
      auto ev = (struct inotify_event *)buffer;
      if (ev->mask & IN_CLOSE_WRITE) {
        std::cout << "Found " << ev->name << ".\n";
        auto it = m_inotify.wd_map.find(ev->wd);
        auto dir = it->second;
        auto path = dir + '/' + ev->name;
        char *argv[2];
        argv[0] = (char *)m_inotify.tree_name.c_str();
        argv[1] = (char *)path.c_str();
        m_chain = new RootChain(*m_inotify.config, LENGTH(argv), argv);
      }
    }
  }
  if (m_chain) {
    auto ok = m_chain->Fetch();
    if (m_is_files) {
      // Only auto-quit input if we have fixed files.
      return ok;
    }
    if (!ok) {
      delete m_chain;
      m_chain = nullptr;
      m_inotify.config->UnbindSignals();
    }
  }
  return true;
}

std::pair<Input::Scalar const *, size_t> Root::GetData(size_t a_id)
{
  if (m_chain) {
    return m_chain->GetData(a_id);
  }
  return std::make_pair(nullptr, 0);
}

#endif
