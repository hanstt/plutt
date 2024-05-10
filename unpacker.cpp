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

#if PLUTT_UCESB

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
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

Unpacker::Unpacker(Config &a_config, int a_argc, char **a_argv):
  m_path(a_argv[0]),
  m_clnt(),
  m_pip(),
  m_struct_info(),
  m_map(),
  m_event_buf(),
  m_out_size(),
  m_out_buf()
{
  // Make string of signals.
  std::string signals_str;
  auto signal_list = a_config.GetSignalList();
  for (auto it = signal_list.begin(); signal_list.end() != it; ++it) {
    signals_str += *it;
    signals_str += ',';
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
    oss << "--ntuple=STRUCT_HH," << signals_str << "id=plutt," << temp_path;
    wordexp_t exp = {0};
    if (0 != wordexp(m_path.c_str(), &exp, 0)) {
      perror("wordexp");
      _exit(EXIT_FAILURE);
    }
    if (1 != exp.we_wordc) {
      std::cerr << m_path << ": Does not resolve to single unpacker.\n";
      _exit(EXIT_FAILURE);
    }
    std::string path = exp.we_wordv[0];
    std::cout << "Struct call: " << path << ' ' << path << ' ' << oss.str() <<
        '\n';
    execl(path.c_str(), path.c_str(), oss.str().c_str(), nullptr);
    perror("exec");
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

  auto buf_macro = ExtractRange(buf_file, "_ITEMS_INFO", " while (0)");

  // Find signals in struct and allocate scalars/vectors.
  std::map<std::string, uint32_t> signal_map;
  size_t event_buf_i = 0;
  for (auto it = signal_list.begin(); signal_list.end() != it; ++it) {
    BindSignal(a_config, buf_macro, *it, nullptr, nullptr, event_buf_i,
        signal_map, false);
    BindSignal(a_config, buf_macro, *it,  "M",  "M", event_buf_i, signal_map,
        true);
    BindSignal(a_config, buf_macro, *it,  "I",  "I", event_buf_i, signal_map,
        true);
    BindSignal(a_config, buf_macro, *it, "MI", "MI", event_buf_i, signal_map,
        true);
    BindSignal(a_config, buf_macro, *it, "ME", "ME", event_buf_i, signal_map,
        true);
    BindSignal(a_config, buf_macro, *it,  "v",  "v", event_buf_i, signal_map,
        true);
    BindSignal(a_config, buf_macro, *it,  "E",  "v", event_buf_i, signal_map,
        true);
  }
  m_event_buf.resize(event_buf_i);
  m_out_buf.resize(m_out_size);

  /* Run unpacker and connect. */
  std::string cmd = std::string(m_path) + " --ntuple=RAW," + signals_str +
      "STRUCT,-";
  for (int arg_i = 1; arg_i < a_argc; ++arg_i) {
    cmd += " ";
    cmd += a_argv[arg_i];
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

void Unpacker::BindSignal(Config &a_config, std::vector<char> const
    &a_buf_macro, std::string const &a_name, char const *a_suffix, char const
    *a_config_suffix, size_t &a_event_buf_i, std::map<std::string, uint32_t>
    &a_signal_map, bool a_optional)
{
  // Find signal in macro.
  std::string full_name = a_name + (a_suffix ? a_suffix : "");
  auto it = a_signal_map.find(full_name);
  if (a_signal_map.end() != it) {
    return;
  }
  auto ret = a_signal_map.insert(std::make_pair(full_name, 0));
  char const *p = &a_buf_macro.at(0);
  for (;; ++p) {
    p = strstr(p, full_name.c_str());
    if (!p) {
      if (a_optional) {
        return;
      }
      std::cerr << full_name << ": mandatory signal not mapped.\n";
      throw std::runtime_error(__func__);
    }
    if (',' == p[full_name.size()]) {
      break;
    }
  }

  // Find macro name.
  auto macro = RewindNewline(a_buf_macro, p);
  macro = RewindNewline(a_buf_macro, macro - 1);
  macro = strstr(macro, "EXT_STR_ITEM_INFO");
  if (!macro || macro > p) {
    std::cerr << full_name << ": missing struct-info macro name.\n";
    throw std::runtime_error(__func__);
  }

  // Find type.
  p += full_name.size() + 1;
  for (; isblank(*p); ++p)
    ;
  int struct_info_type;
  Type output_type;
  size_t in_type_bytes;
  size_t arr_n;
  if (MATCH_WORD(p, "UINT32,")) {
    struct_info_type = EXT_DATA_ITEM_TYPE_UINT32;
    output_type = kUint64;
    in_type_bytes = sizeof(uint32_t);
  } else {
    auto q = p;
    for (; isalnum(*p); ++p)
      ;
    std::string str(q, static_cast<size_t>(p - q));
    std::cerr << full_name << ": unsupported type '" << str << "'.\n";
    throw std::runtime_error(__func__);
  }
  std::string full_name_ptn = "\"";
  full_name_ptn += full_name + "\"";
  p = strstr(p, full_name_ptn.c_str());
  if (!p) {
    std::cerr << full_name << ": could not find max value/reference.\n";
    throw std::runtime_error(__func__);
  }

  // Find array size.
  p += full_name_ptn.size() + 1;
  std::string ctrl;
  if (!a_suffix || 0 == strcmp("M", a_suffix)) {
    // If no suffix = "" or "M", find max value, possibly future array limit.
    char *end;
    // Can be unlimited scalar, limit = 0.
    ret.first->second = static_cast<uint32_t>(strtol(p, &end, 10));
    arr_n = 1;
  } else {
    // Otherwise find reference limit.
    for (; isblank(*p); ++p)
      ;
    auto q = p + 1;
    if ('"' == *p) {
      for (++p; isalnum(*p) || '_' == *p; ++p)
        ;
    }
    if ('"' != *p) {
      std::cerr << full_name << ": could not find reference.\n";
      throw std::runtime_error(__func__);
    }
    ctrl = std::string(q, static_cast<size_t>(p - q));
    auto it2 = a_signal_map.find(ctrl);
    if (a_signal_map.end() == it2) {
      std::cerr << full_name << ": reference '" << ctrl << "' not seen.\n";
      throw std::runtime_error(__func__);
    }
    arr_n = it2->second;
  }
  if (0 == arr_n) {
    std::cerr << full_name << ": 0 array limit.\n";
    throw std::runtime_error(__func__);
  }
  auto in_bytes = in_type_bytes * arr_n;
  // 32-bit align.
  a_event_buf_i = (a_event_buf_i + 3U) & ~3U;
  a_config.BindSignal(a_name, a_config_suffix, m_map.size(), output_type);

  // std::cout << full_name << ": " << a_event_buf_i << ' ' << in_type_bytes <<
  //     '*' << arr_n << '\n';

#if EXT_DATA_ITEM_FLAGS_OPTIONAL
# define FLAG_OPT , 0
#else
# define FLAG_OPT
#endif

  // Setup links.
  int ok = 1;
  if (MATCH_WORD(macro, "EXT_STR_ITEM_INFO") ||
      MATCH_WORD(macro, "EXT_STR_ITEM_INFO2")) {
    // Unlimited scalar.
// std::cout << "0: " << a_event_buf_i << ' ' << in_bytes << ' ' <<
// struct_info_type << ' ' << full_name << '\n';
    ok &= 0 == ext_data_struct_info_item(m_struct_info, a_event_buf_i,
        in_bytes, struct_info_type, "", -1, full_name.c_str(), "", -1
        FLAG_OPT);
  } else if (MATCH_WORD(macro, "EXT_STR_ITEM_INFO_LIM") ||
      MATCH_WORD(macro, "EXT_STR_ITEM_INFO2_LIM")) {
    // Limited scalar.
// std::cout << "1: " << a_event_buf_i << ' ' << in_bytes << ' ' <<
// struct_info_type << ' ' << full_name << ' ' << ret.first->second << '\n';
    ok &= 0 == ext_data_struct_info_item(m_struct_info, a_event_buf_i,
        in_bytes, struct_info_type, "", -1, full_name.c_str(), "",
        static_cast<int>(ret.first->second) FLAG_OPT);
  } else if (MATCH_WORD(macro, "EXT_STR_ITEM_INFO_ZZP") ||
      MATCH_WORD(macro, "EXT_STR_ITEM_INFO2_ZZP")) {
// std::cout << "2: " << a_event_buf_i << ' ' << in_bytes << ' ' <<
// struct_info_type << ' ' << full_name << ' ' << ctrl << '\n';
    ok &= 0 == ext_data_struct_info_item(m_struct_info, a_event_buf_i,
        in_bytes, struct_info_type, "", -1, full_name.c_str(), ctrl.c_str(),
        -1 FLAG_OPT);
  } else {
    std::cerr << full_name << ": unknown struct-info macro.\n";
    throw std::runtime_error(__func__);
  }
  if (!ok) {
    perror("ext_data_struct_info_item");
    std::cerr << full_name << ": failed to set struct-info.\n";
    throw std::runtime_error(__func__);
  }

  m_map.push_back(
      Entry(full_name, struct_info_type, a_event_buf_i, m_out_size, arr_n));

  a_event_buf_i += in_bytes;
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
      for (size_t i = 0; i < it->len; ++i) { \
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

std::pair<Input::Scalar const *, size_t> Unpacker::GetData(size_t a_id)
{
  auto &entry = m_map.at(a_id);
  return std::make_pair(&m_out_buf.at(entry.out_ofs), entry.len);
}


char const *Unpacker::RewindNewline(std::vector<char> const &a_buf, char const
    *a_p)
{
  auto p = a_p;
  for (; '\n' != *p; --p) {
    if (&a_buf.at(0) >= p) {
      std::cerr << "No newline found.\n";
      throw std::runtime_error(__func__);
    }
  }
  return p;
}

#endif
