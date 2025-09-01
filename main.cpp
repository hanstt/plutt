/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2025
 * Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
 * Bastian Loeher <b.loeher@gsi.de>
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

#include <getopt.h>
#include <unistd.h>

#include <condition_variable>
#include <csignal>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <thread>
#include <vector>

#include <config.hpp>
#include <gui.hpp>
#if PLUTT_SDL2
# include <SDL_compat.h>
# include <SDL.h>
# include <implutt.hpp>
# include <sdl_gui.hpp>
#endif
#if PLUTT_ROOT
# include <root.hpp>
#endif
#if PLUTT_ROOT_HTTP
# include <root_gui.hpp>
#endif
#if PLUTT_UCESB
# include <ext_data_struct_info.hh>
# include <ext_data_clnt.hh>
# include <unpacker.hpp>
#endif
#include <inhax.hpp>
#include <util.hpp>

extern Config *g_config;
extern GuiCollection g_gui;

namespace {

  enum InputType {
#if PLUTT_ROOT
#       define ROOT_ARGOPT "rR"
    INPUT_ROOT_FILES,
    INPUT_ROOT_DIR,
#else
#       define ROOT_ARGOPT
#endif
#if PLUTT_UCESB
#       define UCESB_ARGOPT "u"
    INPUT_UCESB,
#else
#       define UCESB_ARGOPT
#endif
    INPUT_HAX,
    INPUT_NONE
  };
  enum GuiType {
    GUI_NONE = 0,
#if PLUTT_SDL2
    GUI_SDL = 1 << 0,
#endif
#if PLUTT_ROOT_HTTP
    GUI_ROOT = 1 << 1,
#endif
    GUI_LAST
  };

  char const *g_arg0;
  char const *g_conf_path;
  long g_jobs;
  Input *g_input;
  bool g_main_running = true;

  void help(char const *a_msg)
  {
    std::cout << "\n";
    if (a_msg) {
      std::cerr << a_msg << '\n';
      std::cout << "\n";
    }
    std::cout << "Usage: " << g_arg0 <<
        " -f config [-g gui] [-j jobs] input...\n";
    std::cout << " -g   values (this arg can be repeated):";
#if PLUTT_SDL2
    std::cout << " sdl";
#endif
#if PLUTT_ROOT_HTTP
    std::cout << " root(:port)";
#endif
    std::cout << "\n";
    std::cout << "Input options:\n";
#if PLUTT_ROOT
    std::cout << " -r   tree-name root-files...\n";
    std::cout << " -R   tree-name directories...\n";
#endif
#if PLUTT_UCESB
    std::cout << " -u   unpacker args...\n";
#endif
    exit(a_msg ? EXIT_FAILURE : EXIT_SUCCESS);
  }

  struct Blupp {
    Blupp():
      mutex(),
      input_i(),
      event_i(),
      running(),
      input_cv(),
      event_cv() {}
    std::mutex mutex;
    uint64_t input_i;
    uint64_t event_i;
    bool running;
    std::condition_variable input_cv;
    std::condition_variable event_cv;
  } g_inp;

  void main_input(int argc, char **argv)
  {
    std::cout << "Starting input loop.\n";
    for (;;) {
      // Fetch event and wait until buffered event is done.
      if (!g_input->Fetch()) {
        sleep(1);
        continue;
      }
      std::unique_lock<std::mutex> lock(g_inp.mutex);
      g_inp.input_cv.wait(lock, []{
          return g_inp.input_i == g_inp.event_i || !g_inp.running;
      });
      if (!g_inp.running) {
        lock.unlock();
        break;
      }

      // Buffer fetched data and wake up the event thread.
      g_input->Buffer();
      ++g_inp.input_i;
      lock.unlock();
      g_inp.event_cv.notify_one();
    }
    g_inp.event_cv.notify_one();
    std::cout << "Exited input loop.\n";
  }

  void main_event(int argc, char **argv)
  {
    std::cout << "Starting event loop.\n";
    for (;;) {
      // Wait until there's a new buffered event.
      std::unique_lock<std::mutex> lock(g_inp.mutex);
      g_inp.event_cv.wait(lock, []{
          return g_inp.input_i > g_inp.event_i || !g_inp.running;
      });
      if (!g_inp.running) {
        lock.unlock();
        break;
      }

      // Process buffered event, then wake up the input thread.
      g_config->DoEvent(g_input);
      ++g_inp.event_i;
      lock.unlock();
      g_inp.input_cv.notify_one();
    }
    std::cout << "Exited event loop.\n";
  }

  static int ctr = 0;
  void sighandler(int a_signum)
  {
    if (3 == ctr) {
      abort();
    }
    ++ctr;
    g_main_running &= SIGINT != a_signum;
  }

}

int main(int argc, char **argv)
{
  g_arg0 = argv[0];

  signal(SIGINT, sighandler);

  // Print some niceties.
  std::cout << "Built with: ";
  std::string with;
#if PLUTT_SDL2
  with += "SDL2+freetype2";
#endif
#if PLUTT_NLOPT
  if (!with.empty()) with += ',';
  with += "nlopt";
#endif
#if PLUTT_ROOT
  if (!with.empty()) with += ',';
  with += "ROOT";
#endif
#if PLUTT_ROOT_HTTP
  if (!with.empty()) with += ',';
  with += "ROOT-HTTP";
  uint16_t web_port = 8080;
#endif
#if PLUTT_UCESB
  if (!with.empty()) with += ',';
  with += "ucesb";
#endif
  std::cout << with << ".\n";

  // Handle arguments.
  enum InputType input_type = INPUT_NONE;
  unsigned gui_type = GUI_NONE;
  (void)gui_type;
  int c;
  while ((c = getopt(argc, argv, "hf:g:j:x" ROOT_ARGOPT UCESB_ARGOPT)) != -1) {
    switch (c) {
      case 'h':
        help(nullptr);
      case 'f':
        g_conf_path = optarg;
        break;
      case 'g':
#if PLUTT_SDL2
        if (0 == strcmp(optarg, "sdl")) {
          gui_type |= GUI_SDL;
        } else
#endif
#if PLUTT_ROOT_HTTP
        if (0 == strncmp(optarg, "root", 4)) {
          if ('\0' != optarg[4]) {
            if (':' != optarg[4]) {
              help("Invalid GUI type for -g.");
            }
            char *end;
            auto port = strtol(optarg + 5, &end, 0);
            if (port < 1 || port > 65535) {
              help("Invalid port for -g ROOT gui.");
            }
            if ('\0' != *end) {
              help("Invalid ROOT GUI argument for -g.");
            }
            web_port = (uint16_t)port;
          }
          gui_type |= GUI_ROOT;
        } else
#endif
        {
          help("Invalid GUI type for -g.");
        }
        break;
      case 'j':
        {
          char *end;
          g_jobs = strtol(optarg, &end, 10);
          if ('\0' != *end) {
            help("Invalid integer jobs.");
          }
        }
        break;
#if PLUTT_ROOT
      case 'r':
        if (argc - optind < 2) {
          help("Not enough parameters for -r.");
        }
        input_type = INPUT_ROOT_FILES;
        break;
      case 'R':
        if (argc - optind < 2) {
          help("Not enough parameters for -R.");
        }
        input_type = INPUT_ROOT_DIR;
        break;
#endif
#if PLUTT_UCESB
      case 'u':
        if (argc - optind < 2) {
          help("Not enough parameters for -u.");
        }
        input_type = INPUT_UCESB;
        break;
#endif
      case 'x':
        input_type = INPUT_HAX;
        break;
      default:
        help("Invalid argument.");
        break;
    }
    if (INPUT_NONE != input_type) {
      break;
    }
  }
  argc -= optind;
  argv += optind;
  if (!g_conf_path) {
    help("I need a config file, see -f!");
  }
  if (INPUT_NONE == input_type) {
    help("I need an input!");
  }

  // Make sure there's a default GUI.
#if PLUTT_SDL2
  if (!gui_type) {
    gui_type = GUI_SDL;
  }
#endif
#if PLUTT_ROOT_HTTP
  if (!gui_type) {
    gui_type = GUI_ROOT;
  }
#endif

  // Start GUI.
#if PLUTT_SDL2
  SdlGui *sdl_gui = nullptr;
  if (GUI_SDL & gui_type) {
    std::cout << "Creating SDL GUI...\n";
    ImPlutt::Setup();
    sdl_gui = new SdlGui("plutt", 800, 600);
    g_gui.AddGui(sdl_gui);
  }
#endif
#if PLUTT_ROOT_HTTP
  RootGui *root_gui = nullptr;
  if (GUI_ROOT & gui_type) {
    std::cout << "Creating ROOT GUI on port " << web_port << "...\n";
    root_gui = new RootGui(web_port);
    g_gui.AddGui(root_gui);
  }
#endif

  // Config figures out requested signals and asks the input to deliver blobs
  // of arrays.
  // The ctor sets g_config by itself, nice hack bro.
  new Config(g_conf_path);
  switch (input_type) {
#if PLUTT_ROOT
    case INPUT_ROOT_FILES:
      g_input = new Root(true, g_config, argc, argv);
      break;
    case INPUT_ROOT_DIR:
      g_input = new Root(false, g_config, argc, argv);
      break;
#endif
#if PLUTT_UCESB
    case INPUT_UCESB:
      g_input = new Unpacker(*g_config, argc, argv);
      break;
#endif
    case INPUT_HAX:
      g_input = new Inhax(*g_config, argc, argv);
      break;
    case INPUT_NONE:
    default:
      throw std::runtime_error(__func__);
  }

  Status_set("Started.");

  // Start data thread.
  g_inp.running = true;
  std::thread thread_input(main_input, argc, argv);
  std::thread thread_event(main_event, argc, argv);

  uint64_t event_i0 = 0;
  double event_rate = 0.0;

  std::cout << "Entering main loop...\n";
  while (g_main_running) {
    bool allow_auto_quit = true;
    bool is_throttled = false;
    (void)is_throttled;

#if PLUTT_SDL2
    if (GUI_SDL & gui_type) {
      allow_auto_quit = false;
      // Use event timeout to cap UI rate.
      auto t_end = SDL_GETTICKS() + 1000 / g_config->UIRateGet();
      for (;;) {
        auto t_cur = SDL_GETTICKS();
        if (t_cur > t_end) {
          break;
        }
        auto timeout = (int)(t_end - t_cur);
        SDL_Event event;
        if (!SDL_WaitEventTimeout(&event, timeout)) {
          break;
        }
        ImPlutt::ProcessEvent(event);
      }
      is_throttled = true;
      Time_set_ms(t_end);
    }
#endif
#if PLUTT_ROOT_HTTP
    if (GUI_ROOT & gui_type) {
      if (!is_throttled) {
        // This is medium-latency presentation, let's take it easy.
        sleep(3);
        is_throttled = true;
      }
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      auto ms = (uint64_t)
          (1000 * (double)ts.tv_sec + 1e-6 * (double)ts.tv_nsec);
      Time_set_ms(ms);
    }
#endif

    {
      std::unique_lock<std::mutex> lock(g_inp.mutex);
      if (allow_auto_quit && !g_inp.running) {
        // If no data is coming in and the GUI is not interactive, break out.
        break;
      }
    }

    g_main_running &= g_gui.Draw(event_rate);

#define RATE_PER_SECOND 2
    {
      static uint64_t t_prev = 0;
      uint64_t t = Time_get_ms();
      if (t_prev + 1000 / RATE_PER_SECOND < t) {
        std::unique_lock<std::mutex> lock(g_inp.mutex);
        auto event_i1 = g_inp.event_i;
        event_rate = (double)(event_i1 - event_i0) * RATE_PER_SECOND;
        event_i0 = event_i1;
        t_prev = t;
      }
    }
  }
  std::cout << "Exiting main loop...\n";
  {
    std::unique_lock<std::mutex> lock(g_inp.mutex);
    g_inp.running = false;
  }
  g_inp.input_cv.notify_one();
  g_inp.event_cv.notify_one();
  thread_input.join();
  thread_event.join();

#if PLUTT_SDL2
  if (GUI_SDL & gui_type) {
    delete sdl_gui;
    ImPlutt::Destroy();
  }
#endif
#if PLUTT_ROOT_HTTP
  if (GUI_ROOT & gui_type) {
    delete root_gui;
  }
#endif

  delete g_input;
  delete g_config;

  return 0;
}
