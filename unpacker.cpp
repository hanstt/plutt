/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2025
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

#if PLUTT_UCESB

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <ext_data_client.h>
#include <ext_data_struct_info.hh>
#include <ext_data_clnt.hh>

#include <config.hpp>
#include <unpacker.hpp>

#define MATCH_WORD(p, str) (0 == strncmp(p, str, sizeof str - 1) && \
    !isalnum(p[sizeof str - 1]) && '_' != p[sizeof str - 1])

#if 0
typedef struct EXT_STR_h101_t
{
  /* RAW */
  uint32_t CALIFA_ENE /* [0,512] */;
  uint32_t CALIFA_ENEI[512 EXT_STRUCT_CTRL(CALIFA_ENE)] /* [1,512] */;
  uint32_t CALIFA_ENEv[512 EXT_STRUCT_CTRL(CALIFA_ENE)] /* [0,65535] */;
  uint32_t CALIFA_TSLSB /* [0,512] */;
  uint32_t CALIFA_TSLSBI[512 EXT_STRUCT_CTRL(CALIFA_TSLSB)] /* [1,512] */;
  uint32_t CALIFA_TSLSBv[512 EXT_STRUCT_CTRL(CALIFA_TSLSB)] /* [-1,-1] */;
  uint32_t CALIFA_TSMSB /* [0,512] */;
  uint32_t CALIFA_TSMSBI[512 EXT_STRUCT_CTRL(CALIFA_TSMSB)] /* [1,512] */;
  uint32_t CALIFA_TSMSBv[512 EXT_STRUCT_CTRL(CALIFA_TSMSB)] /* [-1,-1] */;
  uint32_t CALIFA_NF /* [0,512] */;
  uint32_t CALIFA_NFI[512 EXT_STRUCT_CTRL(CALIFA_NF)] /* [1,512] */;
  uint32_t CALIFA_NFv[512 EXT_STRUCT_CTRL(CALIFA_NF)] /* [0,65535] */;
  uint32_t CALIFA_NS /* [0,512] */;
  uint32_t CALIFA_NSI[512 EXT_STRUCT_CTRL(CALIFA_NS)] /* [1,512] */;
  uint32_t CALIFA_NSv[512 EXT_STRUCT_CTRL(CALIFA_NS)] /* [0,65535] */;

} EXT_STR_h101;
typedef struct EXT_STR_h101_t
{
  /* DEF */
  ...
  uint32_t caen_n /* [0,1023] */;
  uint32_t caen_id[1024 EXT_STRUCT_CTRL(caen_n)] /* [-1,-1] */;
  uint32_t caen_ts_h[1024 EXT_STRUCT_CTRL(caen_n)] /* [-1,-1] */;
  uint32_t caen_ts_l[1024 EXT_STRUCT_CTRL(caen_n)] /* [-1,-1] */;
  uint32_t caen_adc_a[1024 EXT_STRUCT_CTRL(caen_n)] /* [-1,-1] */;
  uint32_t caen_adc_b[1024 EXT_STRUCT_CTRL(caen_n)] /* [-1,-1] */;
  uint32_t caen_adc_c[1024 EXT_STRUCT_CTRL(caen_n)] /* [-1,-1] */;
  uint32_t caen_adc_d[1024 EXT_STRUCT_CTRL(caen_n)] /* [-1,-1] */;
  ...

} EXT_STR_h101;
#endif

Unpacker::Unpacker(Config &a_config, int a_argc, char **a_argv):
  m_path(),
  m_is_struct_writer(),
  m_clnt(),
  m_pip(),
  m_struct_info(),
  m_map(),
  m_event_buf(),
  m_out_size(),
  m_out_buf()
{
  // Expand unpacker path.
  wordexp_t exp = {0};
  if (0 != wordexp(a_argv[0], &exp, 0)) {
    perror("wordexp");
    _exit(EXIT_FAILURE);
  }
  if (1 != exp.we_wordc) {
    std::cerr << m_path << ": Does not resolve to single unpacker.\n";
    _exit(EXIT_FAILURE);
  }
  m_path = exp.we_wordv[0];
  std::string host;
  if (std::string::npos != m_path.find("struct_writer")) {
    m_is_struct_writer = true;
    host = a_argv[1];
  }

  // Make string of signals.
  std::string signals_str;
  auto signal_list = a_config.GetSignalList();
  for (auto it = signal_list.begin(); signal_list.end() != it; ++it) {
    auto sig = *it;
    signals_str += sig->name + ',';
    if (!sig->id.empty()) {
      signals_str += sig->id + ',';
    }
    if (!sig->end.empty()) {
      signals_str += sig->end + ',';
    }
    if (!sig->v.empty()) {
      signals_str += sig->v + ',';
    }
  }

  // Generate struct file.
  auto temp_path_c = strdup("pluttXXXXXX.h");
  auto fd = mkstemps(temp_path_c, 2);
  if (-1 == fd) {
    perror("mkstemps");
    free(temp_path_c);
    throw std::runtime_error(__func__);
  }
  close(fd);
  std::string temp_path(temp_path_c);
  free(temp_path_c);
  auto pid = fork();
  if (-1 == pid) {
    perror("fork");
    remove(temp_path.c_str());
    throw std::runtime_error(__func__);
  }
  if (0 == pid) {
    std::ostringstream oss;
    if (m_is_struct_writer) {
      oss << "--header=" << temp_path;
      std::cout << "Struct call: " << m_path << ' ' << m_path << ' ' << host
          << ' ' << oss.str() << '\n';
      execl(m_path.c_str(), m_path.c_str(), host.c_str(), oss.str().c_str(),
          nullptr);
    } else {
      oss << "--ntuple=STRUCT_HH," << signals_str << "id=plutt," << temp_path;
      std::cout << "Struct call: " << m_path << ' ' << m_path << ' ' <<
          oss.str() << '\n';
      execl(m_path.c_str(), m_path.c_str(), oss.str().c_str(), nullptr);
    }
    perror("execl");
    _exit(EXIT_FAILURE);
  } else {
    int status = 0;
    if (-1 == wait(&status)) {
      perror("wait");
      remove(temp_path.c_str());
      throw std::runtime_error(__func__);
    }
    if (!WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
      std::cerr << m_path << ": failed to get struct.\n";
      remove(temp_path.c_str());
      throw std::runtime_error(__func__);
    }
  }

  // Prepare parsing of struct.
  struct stat st;
  if (-1 == stat(temp_path.c_str(), &st)) {
    perror("stat");
    throw std::runtime_error(__func__);
  }
  std::vector<char> buf_file(static_cast<size_t>(st.st_size) + 1);
  std::ifstream ifile(temp_path.c_str());
  ifile.read(&buf_file.at(0), st.st_size);
  ifile.close();
  remove(temp_path.c_str());

  auto buf_struct = ExtractRange(buf_file,
      "typedef struct EXT_STR_h101_t",
      "} EXT_STR_h101;");

  // Find signals in struct and allocate scalars/vectors.
  std::set<std::string> signal_set;
  size_t event_buf_i = 0;
  for (auto it = signal_list.begin(); signal_list.end() != it; ++it) {
    auto sig = *it;
    if (!sig->id.empty() && !sig->v.empty()) {
      BindSignal(&a_config, buf_struct, sig->name, sig->id, NodeSignal::kId,
          event_buf_i, signal_set);
      if (!sig->end.empty()) {
        BindSignal(&a_config, buf_struct, sig->name, sig->end,
            NodeSignal::kEnd, event_buf_i, signal_set);
      }
      BindSignal(&a_config, buf_struct, sig->name, sig->v, NodeSignal::kV,
          event_buf_i, signal_set);
      continue;
    }
    auto has_MI = FindSignal(buf_struct, sig->name + "MI", signal_set);
    auto has_ME = FindSignal(buf_struct, sig->name + "ME", signal_set);
    auto has_v = FindSignal(buf_struct, sig->name + "v", signal_set);
    if (has_MI && has_ME && has_v) {
      BindSignal(&a_config, buf_struct, sig->name, sig->name + "MI",
          NodeSignal::kId, event_buf_i, signal_set);
      BindSignal(&a_config, buf_struct, sig->name, sig->name + "ME",
          NodeSignal::kEnd, event_buf_i, signal_set);
      BindSignal(&a_config, buf_struct, sig->name, sig->name + "v",
          NodeSignal::kV, event_buf_i, signal_set);
      continue;
    }
    auto has_I = FindSignal(buf_struct, sig->name + "I", signal_set);
    if (has_I && has_v) {
      BindSignal(&a_config, buf_struct, sig->name, sig->name + "I",
          NodeSignal::kId, event_buf_i, signal_set);
      BindSignal(&a_config, buf_struct, sig->name, sig->name + "v",
          NodeSignal::kV, event_buf_i, signal_set);
      continue;
    }
    BindSignal(&a_config, buf_struct, sig->name, sig->name, NodeSignal::kV,
        event_buf_i, signal_set);
  }
  m_event_buf.resize(event_buf_i);
  m_out_buf.resize(m_out_size);

  /* Run unpacker and connect. */
  std::string cmd;
  if (m_is_struct_writer) {
    cmd = m_path + " " + host + " --stdout";
  } else {
    cmd = m_path + " --ntuple=RAW," + signals_str + "STRUCT,-";
    for (int arg_i = 1; arg_i < a_argc; ++arg_i) {
      cmd += " ";
      cmd += a_argv[arg_i];
    }
  }
  std::cout << "popen(" << cmd << ")\n";
  m_pip = popen(cmd.c_str(), "r");
  if (!m_pip) {
    perror("popen");
    throw std::runtime_error(__func__);
  }
  m_clnt = new ext_data_clnt;
  fd = fileno(m_pip);
  if (-1 == fd) {
    perror("fileno");
    throw std::runtime_error(__func__);
  }
  if (!m_clnt->connect(fd)) {
    perror("ext_data_clnt::ext_data_from_fd");
    throw std::runtime_error(__func__);
  }
  uint32_t success = 0;
  if (-1 == m_clnt->setup(nullptr, 0, &m_struct_info, &success, event_buf_i))
  {
    perror("ext_data_clnt::setup");
    std::cerr << m_clnt->last_error() << '\n';
    throw std::runtime_error(__func__);
  }
  uint32_t map_ok = EXT_DATA_ITEM_MAP_OK | EXT_DATA_ITEM_MAP_NO_DEST;
  if (success & ~map_ok) {
    perror("ext_data_clnt::setup");
    ext_data_struct_info_print_map_success(m_struct_info, stderr, map_ok);
    throw std::runtime_error(__func__);
  }
}

Unpacker::~Unpacker()
{
  delete m_clnt;
  if (m_pip) {
    if (-1 == pclose(m_pip)) {
      perror("pclose");
    }
  }
}

void Unpacker::BindSignal(Config *a_config, std::vector<char> const
    &a_buf_struct, std::string const &a_base_name, std::string const &a_name,
    NodeSignal::MemberType a_member_type, size_t &a_event_buf_i,
    std::set<std::string> &a_signal_set)
{
  auto it = a_signal_set.find(a_name);
  if (a_signal_set.end() != it) {
    // Already bound.
    return;
  }
  a_signal_set.insert(a_name);
  auto p0 = &a_buf_struct.at(0);
  auto p = p0;
  auto name_space = a_name + ' ';
  auto name_brack = a_name + '[';
  auto p_name = strstr(p, name_space.c_str());
  if (!p_name) {
    p_name = strstr(p, name_brack.c_str());
  }
  if (!p_name) {
    std::cerr << a_name << ": mandatory signal not mapped.\n";
    throw std::runtime_error(__func__);
  }
  p = p_name;

  // Find type.
  int struct_info_type;
  Type output_type;
  size_t in_type_bytes;
  auto q = p;
  for (; '\n' != *q; --q) {
    if (p0 == q) {
      std::cerr << a_name << ": could not find type.\n";
      throw std::runtime_error(__func__);
    }
  }
  for (++q; isblank(*q); ++q)
    ;
  if (MATCH_WORD(q, "uint32_t")) {
    struct_info_type = EXT_DATA_ITEM_TYPE_UINT32;
    output_type = kUint64;
    in_type_bytes = sizeof(uint32_t);
  } else {
    auto str = std::string(q, static_cast<size_t>(p - q));
    std::cerr << a_name << ": unsupported type '" << str << "'.\n";
    throw std::runtime_error(__func__);
  }

  // Find array size and max value.
  size_t arr_n; // Max size.
  std::string ctrl; // ctrl-variable, holds runtime array size.
  size_t len_ofs; // Offset in output buffer to runtime array size.
  uint32_t max;
  p += a_name.length();
  if (' ' == *p) {
    arr_n = 1;
    len_ofs = -1;
    // Get max.
    p = strchr(p, ',');
    if (!p) {
      std::cerr << a_name << ": could not find max value.\n";
      throw std::runtime_error(__func__);
    }
    char *end;
    max = (uint32_t)strtol(p + 1, &end, 0);
  } else if ('[' == *p) {
    // Get array size.
    char *end;
    arr_n = strtol(p + 1, &end, 0);
    if (' ' != *end) {
      std::cerr << a_name << ": could not extract array size.\n";
      throw std::runtime_error(__func__);
    }
    // Find limit reference.
    p = end + 1;
    if (!strcmp(p, "EXT_STRUCT_CTRL(")) {
      std::cerr << a_name << ": could not find ctrl name.\n";
      throw std::runtime_error(__func__);
    }
    p += 16;
    auto p_ctrl = p;
    for (; isalnum(*p) || '_' == *p; ++p)
      ;
    if (')' != *p) {
      std::cerr << a_name << ": could not extract ctrl name.\n";
      throw std::runtime_error(__func__);
    }
    ctrl = std::string(p_ctrl, static_cast<size_t>(p - p_ctrl));
    auto it_ctrl = a_signal_set.find(ctrl);
    if (a_signal_set.end() == it_ctrl) {
      len_ofs = m_out_size;
      BindSignal(nullptr, a_buf_struct, a_base_name, ctrl, NodeSignal::kV,
          a_event_buf_i, a_signal_set);
      it_ctrl = a_signal_set.find(ctrl);
      assert(a_signal_set.end() != it_ctrl);
    } else {
      // Find entry for ctrl to get offset.
      for (auto it2 = m_map.begin();; ++it2) {
        assert(m_map.end() != it2);
        if (0 == it2->name.compare(ctrl)) {
          len_ofs = it2->out_ofs;
          break;
        }
      }
    }
  } else {
    std::cerr << a_name << ": struct syntax error.\n";
    throw std::runtime_error(__func__);
  }
  auto in_bytes = in_type_bytes * arr_n;
  if (a_config) {
    a_config->BindSignal(a_base_name, a_member_type, m_map.size(),
        output_type);
  }

  // std::cout << a_name << ": " << a_event_buf_i << ' ' << in_type_bytes <<
  //     '*' << arr_n << '\n';

#if EXT_DATA_ITEM_FLAGS_OPTIONAL
# define FLAG_OPT , 0
#else
# define FLAG_OPT
#endif

  // Setup links.
  int ok = 1;
  if ((size_t)-1 == len_ofs) {
    // Unlimited (max=-1) or limited scalar.
// std::cout << "0: " << a_event_buf_i << ' ' << in_bytes << ' ' <<
// struct_info_type << ' ' << a_name << ' ' << max << '\n';
    ok &= 0 == ext_data_struct_info_item(m_struct_info, a_event_buf_i,
        in_bytes, struct_info_type, "", -1, a_name.c_str(), "", max FLAG_OPT);
  } else {
// std::cout << "1: " << a_event_buf_i << ' ' << in_bytes << ' ' <<
// struct_info_type << ' ' << a_name << ' ' << ctrl << '\n';
    ok &= 0 == ext_data_struct_info_item(m_struct_info, a_event_buf_i,
        in_bytes, struct_info_type, "", -1, a_name.c_str(), ctrl.c_str(), -1
        FLAG_OPT);
  }
  if (!ok) {
    perror("ext_data_struct_info_item");
    std::cerr << a_name << ": failed to set struct-info.\n";
    throw std::runtime_error(__func__);
  }

  m_map.push_back(Entry(a_name, struct_info_type, a_event_buf_i, m_out_size,
      arr_n, len_ofs));

  a_event_buf_i += in_bytes;
  // 32-bit align.
  a_event_buf_i = (a_event_buf_i + 3U) & ~3U;
  m_out_size += arr_n;
}

void Unpacker::Buffer()
{
  // Convert ucesb event-buffer.
  for (auto it = m_map.begin(); m_map.end() != it; ++it) {
#define COPY_BUF_TYPE(TYPE, in_type, out_member) do { \
    if (EXT_DATA_ITEM_TYPE_##TYPE == it->ext_type) { \
      auto pin = (in_type const *)&m_event_buf[it->in_ofs]; \
      auto pout = &m_out_buf[it->out_ofs]; \
      auto len_ = GetLen(*it); \
      for (size_t i = 0; i < len_; ++i) { \
        pout->out_member = *pin++; \
        ++pout; \
      } \
    } \
  } while (0)
    COPY_BUF_TYPE(UINT32, uint32_t, u64);
  }
}

std::vector<char> Unpacker::ExtractRange(std::vector<char> const &a_buf, char
    const *a_begin, char const *a_end)
{
  auto begin = strstr(&a_buf.at(0), a_begin);
  if (!begin) {
    std::cerr << "Missing range start.\n";
    throw std::runtime_error(__func__);
  }
  auto begin_i = begin - &a_buf.at(0);
  auto end = strstr(begin, a_end);
  if (!end) {
    std::cerr << "Missing range end.\n";
    throw std::runtime_error(__func__);
  }
  auto end_i = end - &a_buf.at(0);
  std::vector<char> buf(a_buf.begin() + begin_i, a_buf.begin() + end_i);
  buf.push_back('\0');
  return buf;
}

bool Unpacker::Fetch()
{
  memset(m_event_buf.data(), 0, m_event_buf.size());
  auto ret = m_clnt->fetch_event(m_event_buf.data(), m_event_buf.size());
  if (0 == ret) {
    return false;
  }
  if (-1 == ret) {
    perror("ext_data_clnt::fetch_event");
    throw std::runtime_error(__func__);
  }
  return true;
}

bool Unpacker::FindSignal(std::vector<char> const &a_buf_struct, std::string
    const &a_name, std::set<std::string> &a_signal_set)
{
  auto it = a_signal_set.find(a_name);
  if (a_signal_set.end() != it) {
    // Signal already bound.
    return true;
  }
  // Find signal in struct.
  auto p = &a_buf_struct.at(0);
  auto name_space = a_name + ' ';
  if (strstr(p, name_space.c_str())) {
    return true;
  }
  auto name_brack = a_name + '[';
  return !!strstr(p, name_brack.c_str());
}

std::pair<Input::Scalar const *, size_t> Unpacker::GetData(size_t a_id)
{
  auto &entry = m_map.at(a_id);
  return std::make_pair(&m_out_buf.at(entry.out_ofs), GetLen(entry));
}

size_t Unpacker::GetLen(Entry const &a_entry)
{
  return (size_t)-1 == a_entry.len_ofs
      ? a_entry.arr_n
      : *(uint32_t const *)&m_out_buf.at(a_entry.len_ofs);
}

#endif
