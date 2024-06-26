/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2024
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

#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <filewatcher.hpp>

#ifdef __linux__

#       include <linux/limits.h>
#       include <sys/inotify.h>
#       include <sys/poll.h>

class FileWatcherImpl {
  public:
    FileWatcherImpl(std::vector<std::string> const &);
    ~FileWatcherImpl();

    std::string WaitFile(unsigned);

  private:
    int m_fd;
    std::map<int, std::string> m_wd_map;
};

FileWatcherImpl::FileWatcherImpl(std::vector<std::string> const &a_vec)
  m_fd(),
  m_wd_map()
{
  m_fd = inotify_init();
  if (m_fd < 0) {
    std::cerr << "inotify_init: " << strerror(errno) << ".\n";
    throw std::runtime_error(__func__);
  }
  for (auto it = a_vec.begin(); a_vec.end() != it; ++it) {
    auto wd = inotify_add_watch(m_fd, it->c_str(), IN_CLOSE_WRITE);
    if (wd < 0) {
      std::cerr << "inotify_add_watch(" << *it << "): " << strerror(errno) <<
          ".\n";
      throw std::runtime_error(__func__);
    }
    m_wd_map.insert(std::make_pair(wd, *it));
  }
}

FileWatcherImpl::~FileWatcherImpl()
{
  for (auto it = m_wd_map.begin(); m_wd_map.end() != it; ++it) {
    if (inotify_rm_watch(m_fd, it->first) < 0) {
      std::cerr << "inotify_rm_watch: " << strerror(errno) << ".\n";
    }
  }
  if (close(m_fd) < 0) {
    std::cerr << "close(inotify): " << strerror(errno) << ".\n";
  }
}

std::string FileWatcherImpl::WaitFile(unsigned a_timeout_ms)
{
  int nfds;
  {
    struct pollfd fds[1];

    fds[0].fd = m_fd;
    fds[0].events = POLLIN;
    nfds = poll(fds, LENGTH(fds), a_timeout_ms);
    if (nfds < 0) {
      std::cerr << "poll: " << strerror(errno) << ".\n";
      throw std::runtime_error(__func__);
    }
  }
  if (nfds > 0) {
    char buffer[sizeof(inotify_event) + NAME_MAX];
    auto rc = read(m_fd, buffer, sizeof buffer);
    if (rc < 0) {
      std::cerr << "read: " << strerror(errno) << ".\n";
      throw std::runtime_error(__func__);
    }
    auto ev = (struct inotify_event *)buffer;
    if (ev->mask & IN_CLOSE_WRITE) {
      std::cout << "Found " << ev->name << ".\n";
      auto it = m_wd_map.find(ev->wd);
      auto dir = it->second;
      auto path = dir + '/' + ev->name;
      return path;
    }
  }
  return "";
}

#endif

#ifdef __APPLE__

#       include <CoreServices/CoreServices.h>

class FileWatcherImpl {
  public:
    FileWatcherImpl(std::vector<std::string> const &);
    ~FileWatcherImpl();

    void Push(std::string const &);
    std::string WaitFile(unsigned);

  private:
    static void callback(ConstFSEventStreamRef, void *, size_t, void *,
        FSEventStreamEventFlags const[],
        FSEventStreamEventId const[]);

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::list<std::string> m_path_list;
    FSEventStreamRef m_stream;
    dispatch_queue_t m_queue;
};

FileWatcherImpl::FileWatcherImpl(std::vector<std::string> const &a_vec):
  m_mutex(),
  m_cv(),
  m_path_list(),
  m_stream(),
  m_queue()
{
  std::vector<CFStringRef> path_vec;
  for (auto it = a_vec.begin(); a_vec.end() != it; ++it) {
    auto path = CFStringCreateWithCString(nullptr, it->c_str(),
        kCFStringEncodingUTF8);
    path_vec.push_back(path);
  }
  auto path_array = CFArrayCreate(nullptr, (void const **)path_vec.data(),
      (int)path_vec.size(), nullptr);

  FSEventStreamContext context;
  context.version = 0;
  context.info = this;
  context.retain = nullptr;
  context.release = nullptr;
  context.copyDescription = nullptr;

  CFAbsoluteTime latency = 1.0;

  m_stream = FSEventStreamCreate(
      nullptr,
      &FileWatcherImpl::callback,
      &context,
      path_array,
      kFSEventStreamEventIdSinceNow,
      latency,
      kFSEventStreamCreateFlagFileEvents);

  m_queue = dispatch_queue_create("plutt:FileWatcherImpl", nullptr);
  FSEventStreamSetDispatchQueue(m_stream, m_queue);
  FSEventStreamStart(m_stream);

  for (auto it = path_vec.begin(); path_vec.end() != it; ++it) {
    CFRelease(*it);
  }
}

FileWatcherImpl::~FileWatcherImpl()
{
  dispatch_release(m_queue);
  FSEventStreamStop(m_stream);
  FSEventStreamInvalidate(m_stream);
  FSEventStreamRelease(m_stream);
}

void FileWatcherImpl::callback(ConstFSEventStreamRef, void *a_user_info,
    size_t a_ev_num, void *a_path_array,
    FSEventStreamEventFlags const a_flag_array[],
    FSEventStreamEventId const a_id_array[])
{
  auto watcher = (FileWatcherImpl *)a_user_info;
  char **path_array = (char **)a_path_array;
  for (size_t i = 0; i < a_ev_num; i++) {
    // Where are the flags?
    // /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/\
    //     Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/\
    //     CoreServices.framework/Versions/A/Frameworks/FSEvents.framework/\
    //     Versions/A/Headers/FSEvents.h
    FSEventStreamEventFlags flags =
        kFSEventStreamEventFlagItemIsFile |
        kFSEventStreamEventFlagItemModified;
    if ((flags & a_flag_array[i]) == flags) {
      std::cout << "Found " << path_array[i] << ".\n";
      watcher->Push(path_array[i]);
    }
  }
}

void FileWatcherImpl::Push(std::string const &a_path)
{
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_path_list.push_back(a_path);
  }
  m_cv.notify_one();
}

std::string FileWatcherImpl::WaitFile(unsigned a_timeout_ms)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_cv.wait_for(lock, std::chrono::milliseconds(a_timeout_ms), [this]{
      return !m_path_list.empty();
      });
  if (m_path_list.empty()) {
    return "";
  }
  std::string path = m_path_list.front();
  m_path_list.pop_front();
  return path;
}

#endif

FileWatcher::FileWatcher(std::vector<std::string> const &a_vec):
  m_impl(new FileWatcherImpl(a_vec))
{
}

FileWatcher::~FileWatcher()
{
  delete m_impl;
}

std::string FileWatcher::WaitFile(unsigned a_timeout_ms)
{
  return m_impl->WaitFile(a_timeout_ms);
}
