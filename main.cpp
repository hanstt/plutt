/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023
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
#include <iostream>
#include <thread>
#if PLUTT_SDL2
# include <implutt.hpp>
# include <sdl_gui.hpp>
#endif
#include <config.hpp>
#if PLUTT_ROOT
# include <root.hpp>
# include <root_gui.hpp>
#endif
#include <unpacker.hpp>
#include <util.hpp>

extern Config *g_config;
extern Gui *g_gui;

namespace {

  enum InputType {
#if PLUTT_ROOT
#       define ROOT_ARGOPT "r"
    INPUT_ROOT,
#else
#       define ROOT_ARGOPT
#endif
#if PLUTT_UCESB
#       define UCESB_ARGOPT "u"
    INPUT_UCESB,
#else
#       define UCESB_ARGOPT
#endif
    INPUT_NONE
  };
  enum GuiType {
#if PLUTT_SDL2
    GUI_SDL,
#endif
#if PLUTT_ROOT
    GUI_ROOT,
#endif
    GUI_NONE
  };

  char const *g_arg0;
  char const *g_conf_path;
  long g_jobs;
  Input *g_input;
  bool g_data_running;

  void help(char const *a_msg)
  {
    if (a_msg) {
      std::cerr << a_msg << '\n';
    }
    std::cout << "Usage: " << g_arg0 <<
        " -f config [-g gui] [-j jobs] input...\n";
    std::cout << "-g values:";
#if PLUTT_SDL2
    std::cout << " sdl";
#endif
#if PLUTT_ROOT
    std::cout << " root";
#endif
    std::cout << "\n";
    std::cout << "Input options:\n";
#if PLUTT_ROOT
    std::cout << " -r tree-name root-files...\n";
#endif
#if PLUTT_UCESB
    std::cout << " -u unpacker args...\n";
#endif
    exit(a_msg ? EXIT_FAILURE : EXIT_SUCCESS);
  }

  uint64_t g_input_i, g_event_i;
  std::mutex g_input_event_mutex;
  std::condition_variable g_input_cv;
  std::condition_variable g_event_cv;

  void main_input(int argc, char **argv)
  {
    std::cout << "Starting input loop.\n";
    for (;;) {
      // Fetch event and wait until buffered event is done.
      if (!g_input->Fetch()) {
        g_data_running = false;
      }
      std::unique_lock<std::mutex> lock(g_input_event_mutex);
      g_input_cv.wait(lock, []{
          return g_input_i == g_event_i || !g_data_running;
      });
      if (!g_data_running) {
        lock.unlock();
        break;
      }

      // Buffer fetched data and wake up the event thread.
      g_input->Buffer();
      ++g_input_i;
      lock.unlock();
      g_event_cv.notify_one();
    }
    g_event_cv.notify_one();
    std::cout << "Exited input loop.\n";
  }

  void main_event(int argc, char **argv)
  {
    std::cout << "Starting event loop.\n";
    for (;;) {
      // Wait until there's a new buffered event.
      std::unique_lock<std::mutex> lock(g_input_event_mutex);
      g_event_cv.wait(lock, []{
          return g_input_i > g_event_i || !g_data_running;
      });
      if (!g_data_running) {
        lock.unlock();
        break;
      }

      // Process buffered event, then wake up the input thread.
      g_config->DoEvent(g_input);
      ++g_event_i;
      lock.unlock();
      g_input_cv.notify_one();
    }
    std::cout << "Exited event loop.\n";
  }

}

int main(int argc, char **argv)
{
  g_arg0 = argv[0];

  // Print some niceties.
  std::cout << "Built with:";
#if PLUTT_SDL2
  std::cout << " SDL2,freetype2";
#endif
#if PLUTT_NLOPT
  std::cout << " nlopt";
#endif
#if PLUTT_ROOT
  std::cout << " ROOT";
#endif
#if PLUTT_UCESB
  std::cout << " ucesb";
#endif
  std::cout << ".\n";

  // Handle arguments.
  enum InputType input_type = INPUT_NONE;
  enum GuiType gui_type = GUI_NONE;
#if PLUTT_ROOT
  gui_type = GUI_ROOT;
  RootGui *root_gui = nullptr;
#endif
#if PLUTT_SDL2
  gui_type = GUI_SDL;
  SdlGui *sdl_gui = nullptr;
#endif
  int c;
  while ((c = getopt(argc, argv, "hf:g:j:" ROOT_ARGOPT UCESB_ARGOPT)) != -1) {
    switch (c) {
      case 'h':
        help(nullptr);
      case 'f':
        g_conf_path = optarg;
        break;
      case 'g':
        gui_type = GUI_NONE;
#if PLUTT_SDL2
        if (0 == strcmp(optarg, "sdl")) {
          gui_type = GUI_SDL;
        }
#endif
#if PLUTT_ROOT
        if (0 == strcmp(optarg, "root")) {
          gui_type = GUI_ROOT;
        }
#endif
        if (GUI_NONE == gui_type) {
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
        input_type = INPUT_ROOT;
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

  // Start GUI.
  switch (gui_type) {
#if PLUTT_SDL2
    case GUI_SDL:
      ImPlutt::Setup();
      g_gui = sdl_gui = new SdlGui("plutt", 800, 600);
      break;
#endif
#if PLUTT_ROOT
    case GUI_ROOT:
      g_gui = root_gui = new RootGui(8080);
      break;
#endif
    default:
      help("ROOT GUI not built in.");
  }

  // Config figures out requested signals and asks the input to deliver blobs
  // of arrays.
  // The ctor sets g_config by itself, nice hack bro.
  new Config(g_conf_path);
  switch (input_type) {
#if PLUTT_ROOT
    case INPUT_ROOT:
      g_input = new Root(*g_config, argc, argv);
      break;
#endif
#if PLUTT_UCESB
    case INPUT_UCESB:
      g_input = new Unpacker(*g_config, argc, argv);
      break;
#endif
    default:
      throw std::runtime_error(__func__);
  }

  Status_set("Started.");

  // Start data thread.
  g_data_running = true;
  std::thread thread_input(main_input, argc, argv);
  std::thread thread_event(main_event, argc, argv);

  uint64_t event_i0 = 0;
  unsigned loop_n = 0;
  double event_rate = 0.0;

  std::cout << "Entering main loop...\n";
  for (bool is_running = true; is_running;) {

    switch (gui_type) {
#if PLUTT_SDL2
      case GUI_SDL:
        {
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
          Time_set_ms(t_end);
        }
        break;
#endif
#if PLUTT_ROOT
      case GUI_ROOT:
        // This is medium-latency presentation, let's take it easy.
        sleep(3);
        break;
#endif
      default:
        abort();
    }

    if (!g_gui->Draw(event_rate)) {
      is_running = false;
    }

    ++loop_n;
#define RATE_PER_SECOND 2
    if (g_config->UIRateGet() / RATE_PER_SECOND == loop_n) {
      auto event_i1 = g_event_i;
      event_rate = (double)(event_i1 - event_i0) * RATE_PER_SECOND;
      event_i0 = event_i1;
      loop_n = 0;
    }
  }
  std::cout << "Exiting main loop...\n";
  g_data_running = false;
  g_input_cv.notify_one();
  g_event_cv.notify_one();
  thread_input.join();
  thread_event.join();

  switch (gui_type) {
#if PLUTT_SDL2
    case GUI_SDL:
      delete sdl_gui;
      ImPlutt::Destroy();
      break;
#endif
#if PLUTT_ROOT
    case GUI_ROOT:
      delete root_gui;
      break;
#endif
    default:
      break;
  }

  delete g_input;
  delete g_config;

  return 0;
}
