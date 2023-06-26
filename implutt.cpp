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

#include <implutt.hpp>
#include <sys/stat.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <ttf.hpp>
#include <util.hpp>

//
// Some notes:
// Render* functions work in "current" space, ie (0,0) is the top-left corner
// of the current level.
// g_input values are in global space, so remember to transform with eg
// Rect::ContainsLocal.
// This imgui tries hard to minimize internal/global state and relies on user
// code for persistent logic by working on transparent structs.
//

// Padding outside a widget, eg outside a button.
#define PAD_EXT 2
// Padding insite a widget, eg between a button and its label.
#define PAD_INT 2
#define PAD (PAD_EXT + PAD_INT)
// Time to stay in "tension" display mode, eg zoomed in.
#define TENSION_TIMEOUT_MS (60 * 1000)
// Zoom factor per scroll wheel step.
#define ZOOM_FACTOR 0.9
#define TICK_SIZE 5

#define SDL_CALL_THROW_(func) do { \
    std::cerr << __FILE__ << ':' << __LINE__ << ": " << SDL_GetError() << \
      ".\n"; \
    throw std::runtime_error(__func__); \
} while (0)
#define SDL_CALL_VOID(func, args) do { \
  if (func args) { \
    SDL_CALL_THROW_(func); \
  } \
} while (0)
#define SDL_CALL(ret, func, args) do { \
  ret = func args; \
  if (!ret) { \
    SDL_CALL_THROW_(func); \
  } \
} while (0)

#define TRUNC(x, min, max) \
    (x) < (min) ? (min) : (x) >= (max) ? (max) : (x)

namespace ImPlutt {

  namespace {
    SDL_Cursor *g_cursor_arrow;
    SDL_Cursor *g_cursor_xhair;
    Style g_style_i;
    std::set<Window *> g_window_set;
    bool g_do_quit;

    enum {
      STYLE_BG,
      STYLE_FG,
      STYLE_HOVER,
      STYLE_PLOT_AXIS,
      STYLE_PLOT_BG,
      STYLE_PLOT_FG,
      STYLE_PLOT_FIT,
      STYLE_PLOT_OVERLAY,
      STYLE_PLOT_USER_PROJ,
      STYLE_PLOT_USER_ZOOM,
      STYLE_SELECTED,
      STYLE_SHADOW,
      STYLE_TEXT,
      STYLE_TYPE_MAX_
    };
    SDL_Color g_style[STYLE_MAX_][STYLE_TYPE_MAX_] = {
      {
        // Bright.
        {240, 240, 240, 255},
        {255, 255, 255, 255},
        {245, 245, 245, 255},
        {  0,   0,   0, 255},
        {255, 255, 255, 255},
        {  0,   0, 255, 255},
        {255,   0,   0, 255},
        {255, 255, 255, 192},
        {255, 128, 128,  64},
        {128, 128, 255,  64},
        {  0, 150, 255, 255},
        {182, 182, 182, 255},
        {  0,   0,   0, 255},
      },
      {
        // Dark.
        { 45,  45,  45, 255},
        {100, 100, 100, 255},
        {120, 120, 120, 255},
        {255, 255, 255, 255},
        {  0,   0,   0, 255},
        {100, 100, 255, 255},
        {255,   0,   0, 255},
        {  0,   0,   0, 192},
        {255, 128, 128,  64},
        {128, 128, 255,  64},
        { 30, 125, 240, 255},
        { 40,  40,  40, 255},
        {255, 255, 255, 255},
      }
    };
#include <roma.hpp>

    Font *g_font;

    bool g_pointer_up;

    bool InputPointerButtonUp()
    {
      return g_pointer_up;
    }

  }

  Level::Level(Rect const &a_rect):
    rect(a_rect),
    cursor(a_rect.x, a_rect.y),
    max_h()
  {
  }

  //
  // Text texture cache.
  //

  TextKey::TextKey():
    str(),
    font_style(),
    style_i(),
    style_sub_i()
  {
  }

  bool TextKey::operator()(TextKey const &a_l, TextKey const &a_r) const {
#define CMP(field) \
    if (a_l.field < a_r.field) return true; \
    if (a_l.field > a_r.field) return false
    int cmp = a_l.str.compare(a_r.str);
    if (cmp) return cmp < 0;
    CMP(font_style);
    CMP(style_i);
    CMP(style_sub_i);
    return false;
  }

  Window::Window(char const *a_title, int a_width, int a_height):
    m_window(),
    m_window_id(),
    m_renderer(),
    m_checkmark_tex(),
    m_wsize(),
    m_rsize(),
    m_event_list(),
    m_pointer(),
    m_level_stack(),
    m_text_tex_map(),
    m_do_close(),
    m_tex_destroy_list()
  {
    // Find a position that causes little overlap.
    // Silly approach: Move the window in large steps, choose what causes the
    // least overlap, then randomise a little.
    struct {
      Pos pos;
      int area;
    } best = {Pos(), 1 << 30};
    SDL_DisplayMode dm;
    SDL_CALL_VOID(SDL_GetCurrentDisplayMode, (0, &dm));
    auto dw = dm.w - a_width;
    auto dh = dm.h - a_height;
    if (dw > 0 && dh > 0) {
      for (int i = 0; i < 10; ++i) {
        auto y1 = i * dh / 10;
        auto y2 = y1 + a_height;
        for (int j = 0; j < 10; ++j) {
          auto x1 = j * dw / 10;
          auto x2 = x1 + a_width;
          int area = 0;
          for (auto it = g_window_set.begin(); g_window_set.end() != it; ++it)
          {
            auto window = (*it)->m_window;
            int tx1, ty1, w, h, tx2, ty2;
            SDL_GetWindowPosition(window, &tx1, &ty1);
            SDL_GetWindowSize(window, &w, &h);
            tx2 = tx1 + w;
            ty2 = ty1 + h;
            tx1 = std::max(tx1, x1);
            ty1 = std::max(ty1, y1);
            tx2 = std::min(tx2, x2);
            ty2 = std::min(ty2, y2);
            auto tw = std::max(tx2 - tx1, 0);
            auto th = std::max(ty2 - ty1, 0);
            area += tw * th;
          }
          if (area < best.area) {
            best.pos.x = x1 + rand() % 100;
            best.pos.y = y1 + rand() % 100;
            best.area = area;
          }
        }
      }
    }
    m_window = SDL_CreateWindow(a_title,
        best.pos.x, best.pos.y, a_width, a_height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!m_window) {
      std::cerr << "SDL_CreateWindow: " << SDL_GetError() << ".\n";
      throw std::runtime_error(__func__);
    }
    m_window_id = SDL_GetWindowID(m_window);
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
      std::cerr << "SDL_CreateRenderer: " << SDL_GetError() << ".\n";
      throw std::runtime_error(__func__);
    }

    // Check-mark.
    auto h = FontGetHeight(g_font);
    // TODO: ctor + dtor is slooow, cache this.
    std::vector<uint8_t> pixels((size_t)(h * h * 4));
    int d = h / 10;
    auto const &fg = g_style[g_style_i][STYLE_SELECTED];
    size_t ofs = 0;
    for (int i = 0; i < h; ++i) {
      for (int j = 0; j < h; ++j) {
        uint8_t a =
            (j > i - d && j < i + d) ||
            (j > (h-1-i) - d && j < (h-1-i) + d) ? 255 : 0;
        pixels.at(ofs++) = a;
        pixels.at(ofs++) = fg.b;
        pixels.at(ofs++) = fg.g;
        pixels.at(ofs++) = fg.r;
      }
    }
    SDL_Surface *surf;
    SDL_CALL(surf, SDL_CreateRGBSurfaceWithFormatFrom,
        (&pixels[0], h, h, 32, h * 4, SDL_PIXELFORMAT_RGBA8888));
    SDL_CALL(m_checkmark_tex,
        SDL_CreateTextureFromSurface, (m_renderer, surf));
    SDL_FreeSurface(surf);
    SDL_CALL_VOID(SDL_SetTextureBlendMode,
        (m_checkmark_tex, SDL_BLENDMODE_BLEND));

    g_window_set.insert(this);
  }

  Window::~Window()
  {
    auto it = g_window_set.find(this);
    assert(g_window_set.end() != it);
    g_window_set.erase(it);
    for (auto it2 = m_text_tex_map.begin(); m_text_tex_map.end() != it2;
        ++it2) {
      SDL_DestroyTexture(it2->second.tex);
    }
    SDL_DestroyTexture(m_checkmark_tex);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
  }

  //
  // 2D stack.
  //

  Level &Window::LevelGet()
  {
    assert(!m_level_stack.empty());
    return m_level_stack.back();
  }

  void Window::LevelAdvance(Pos const &a_size)
  {
    auto &l = LevelGet();
    l.cursor.x += a_size.x;
    l.max_h = std::max(l.max_h, a_size.y);
  }

  void Window::LevelPop()
  {
    m_level_stack.pop_back();
    if (!m_level_stack.empty()) {
      auto const &r = LevelGet().rect;
      SDL_Rect sdl_r;
      sdl_r.x = r.x;
      sdl_r.y = r.y;
      sdl_r.w = r.w;
      sdl_r.h = r.h;
      SDL_CALL_VOID(SDL_RenderSetClipRect, (m_renderer, &sdl_r));
    }
  }

  void Window::LevelPush(Rect const &a_rect)
  {
    Rect r = a_rect;
    if (!m_level_stack.empty()) {
      // Clip requested rect against available rect.
      auto const &cur = LevelGet().rect;
      auto x1 = r.x;
      auto y1 = r.y;
      auto x2 = r.x + r.w;
      auto y2 = r.y + r.h;
      auto x3 = cur.x;
      auto y3 = cur.y;
      auto x4 = cur.x + cur.w;
      auto y4 = cur.y + cur.h;
      x1 = std::max(x1, x3);
      y1 = std::max(y1, y3);
      x2 = std::min(x2, x4);
      y2 = std::min(y2, y4);
      r.x = x1;
      r.y = y1;
      r.w = x2 - x1;
      r.h = y2 - y1;
    }
    m_level_stack.push_back(Level(r));
    SDL_Rect sdl_r;
    sdl_r.x = r.x;
    sdl_r.y = r.y;
    sdl_r.w = r.w;
    sdl_r.h = r.h;
    SDL_CALL_VOID(SDL_RenderSetClipRect, (m_renderer, &sdl_r));
  }

  //
  // Simple classes.
  //

  Pos::Pos():
    x(0),
    y(0)
  {
  }

  Pos::Pos(int a_x, int a_y):
    x(a_x),
    y(a_y)
  {
  }

  Point::Point():
    x(0.0f),
    y(0.0f)
  {
  }

  Point::Point(double a_x, double a_y):
    x(a_x),
    y(a_y)
  {
  }

  Event::Event():
    sdl_ev(),
    pointer(-1, -1)
  {
  }

  // Checks pos inside rect with all global coords.
  bool Window::ContainsWorld(Rect const &a_rect, Pos const &a_pos)
  {
    return
        a_pos.x >= a_rect.x && a_pos.x < a_rect.x + a_rect.w &&
        a_pos.y >= a_rect.y && a_pos.y < a_rect.y + a_rect.h;
  }

  // Checks global pos inside local rect.
  bool Window::ContainsLocal(Rect const &a_rect, Pos const &a_pos)
  {
    auto const &l = LevelGet();
    return ContainsWorld(a_rect, Pos(
        a_pos.x - l.cursor.x,
        a_pos.y - l.cursor.y));
  }

  void Window::DrawLineClipped(Pos const &a_p1, Pos const &a_p2)
  {
    // Metal crashes with crazy lines, let's clip ourselves.
    SDL_Rect r;
    SDL_RenderGetClipRect(m_renderer, &r);
    Pos p1 = a_p1;
    Pos p2 = a_p2;
    if (LineClip(r.x, r.y, r.x + r.w, r.y + r.h,
        &p1.x, &p1.y, &p2.x, &p2.y)) {
      SDL_CALL_VOID(SDL_RenderDrawLine,
          (m_renderer, p1.x, p1.y, p2.x, p2.y));
    }
  }

  bool Window::InputPointerButtonDown(Event const &a_event, Rect const
      &a_rect)
  {
    return SDL_MOUSEBUTTONDOWN == a_event.sdl_ev.type &&
          ContainsLocal(a_rect, a_event.pointer);
  }

  TextInputState::TextInputState():
    str(),
    pos(),
    comp_str(),
    comp_ofs()
  {
    SDL_StartTextInput();
  }

  TextInputState::~TextInputState()
  {
    SDL_StopTextInput();
  }

  //
  // Basic rendering.
  //

  void Window::RenderColor(SDL_Color a_color)
  {
    SDL_CALL_VOID(SDL_SetRenderDrawColor,
        (m_renderer, a_color.r, a_color.g, a_color.b, a_color.a));
  }

  void Window::RenderLine(Pos const &a_p1, Pos const &a_p2)
  {
    auto const &l = LevelGet();
    Pos p1(l.cursor.x + a_p1.x, l.cursor.y + a_p1.y);
    Pos p2(l.cursor.x + a_p2.x, l.cursor.y + a_p2.y);

    DrawLineClipped(p1, p2);
  }

  void Window::RenderLineDashed(Pos const &a_p1, Pos const &a_p2, double
      a_solid, double a_open)
  {
    auto const &l = LevelGet();
    Pos p1(l.cursor.x + a_p1.x, l.cursor.y + a_p1.y);
    Pos p2(l.cursor.x + a_p2.x, l.cursor.y + a_p2.y);
    auto dx = a_p2.x - a_p1.x;
    auto dy = a_p2.y - a_p1.y;
    auto d = sqrt(dx * dx + dy * dy);
    auto s = a_solid / (a_solid + a_open);
    auto dt = (a_solid + a_open) / d;
    for (double t = 0; t < 1.0; t += dt) {
      Pos p3(
          (int)(p1.x + t * (p2.x - p1.x)),
          (int)(p1.y + t * (p2.y - p1.y)));
      auto t2 = t + s * dt;
      t2 = std::min(t2, 1.0);
      Pos p4(
          (int)(p1.x + t2 * (p2.x - p1.x)),
          (int)(p1.y + t2 * (p2.y - p1.y)));
      DrawLineClipped(p3, p4);
    }
  }

  void Window::RenderRect(Rect const &a_r, bool a_do_shadow)
  {
    auto const &l = LevelGet();
    SDL_Rect sdl_r;
    sdl_r.x = l.cursor.x + a_r.x;
    sdl_r.y = l.cursor.y + a_r.y;
    sdl_r.w = a_r.w;
    sdl_r.h = a_r.h;
    SDL_CALL_VOID(SDL_RenderFillRect, (m_renderer, &sdl_r));
    if (a_do_shadow) {
      RenderColor(g_style[g_style_i][STYLE_SHADOW]);
      RenderLine(
          Pos(a_r.x +     1, a_r.y + a_r.h),
          Pos(a_r.x + a_r.w, a_r.y + a_r.h));
      RenderLine(
          Pos(a_r.x + a_r.w, a_r.y +     1),
          Pos(a_r.x + a_r.w, a_r.y + a_r.h));
    }
  }

  void Window::RenderTexture(SDL_Texture *a_tex, Rect const &a_r)
  {
    auto &l = LevelGet();
    SDL_Rect sdl_r;
    sdl_r.x = l.cursor.x + a_r.x;
    sdl_r.y = l.cursor.y + a_r.y;
    sdl_r.w = a_r.w;
    sdl_r.h = a_r.h;
    SDL_CALL_VOID(SDL_RenderCopy, (m_renderer, a_tex, nullptr, &sdl_r));
  }

  void Window::RenderBox(Rect const &a_r, size_t a_fg, size_t a_bg)
  {
    RenderColor(g_style[g_style_i][a_fg]);
    Pos p1(a_r.x        , a_r.y        );
    Pos p2(a_r.x + a_r.w, a_r.y        );
    Pos p3(a_r.x + a_r.w, a_r.y + a_r.h);
    Pos p4(a_r.x        , a_r.y + a_r.h);
    RenderLine(p1, p2);
    RenderLine(p2, p3);
    RenderLine(p3, p4);
    RenderLine(p4, p1);
    RenderColor(g_style[g_style_i][a_bg]);
    Rect r;
    r.x = a_r.x + 1;
    r.y = a_r.y + 1;
    r.w = a_r.w - 2;
    r.h = a_r.h - 2;
    RenderTransparent(true);
    RenderRect(r, false);
    RenderTransparent(false);
  }

  void Window::RenderCross(Pos const &a_pos, int a_size)
  {
    RenderLine(
        Pos(a_pos.x-a_size, a_pos.y       ),
        Pos(a_pos.x+a_size, a_pos.y       ));
    RenderLine(
        Pos(a_pos.x       , a_pos.y-a_size),
        Pos(a_pos.x       , a_pos.y+a_size));
  }

  void Window::RenderText(char const *a_str, TextStyle a_font_style, int
      a_style_i, Pos const &a_ofs)
  {
    // Check cache.
    TextKey key;
    key.str = a_str;
    key.font_style = a_font_style;
    key.style_i = g_style_i;
    key.style_sub_i = a_style_i;
    auto it = m_text_tex_map.find(key);
    if (m_text_tex_map.end() == it) {
      auto rgb = g_style[g_style_i][a_style_i];
      FontSetBold(g_font, TEXT_BOLD == a_font_style);
      auto printed = FontPrintf(g_font, a_str, rgb.r, rgb.g, rgb.b);
      SDL_Surface *surf;
      SDL_CALL(surf, SDL_CreateRGBSurfaceWithFormatFrom,
          (printed.bmap.data(), (int)printed.size.w, (int)printed.size.h,
           32, (int)printed.size.w * 4, SDL_PIXELFORMAT_RGBA8888));
      TextTexture value;
      SDL_CALL(value.tex, SDL_CreateTextureFromSurface, (m_renderer, surf));
      value.r.x = printed.size.x;
      value.r.y = printed.size.y;
      value.r.w = surf->w;
      value.r.h = surf->h;
      SDL_FreeSurface(surf);
      auto pair = m_text_tex_map.insert(std::make_pair(key, value));
      it = pair.first;
    }
    auto &value = it->second;
    Rect r;
    r.x = value.r.x + a_ofs.x;
    r.y = value.r.y + a_ofs.y;
    r.w = value.r.w;
    r.h = value.r.h;
    RenderTexture(value.tex, r);
    value.t_last = Time_get_ms();
  }

  Pos Window::RenderTextMeasure(char const *a_str, TextStyle a_style)
  {
    // TODO: Cache?
    FontSetBold(g_font, TEXT_BOLD == a_style);
    auto size = FontMeasure(g_font, a_str);
    return Pos((int)size.w, (int)size.h);
  }

  void Window::RenderTransparent(bool a_yes)
  {
    SDL_SetRenderDrawBlendMode(m_renderer,
        a_yes ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
  }

  //
  // Context.
  //

  void Setup()
  {
    // SDL setup.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
      std::cerr << "SDL_Init: " << SDL_GetError() << ".\n";
      throw std::runtime_error(__func__);
    }

    g_cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    g_cursor_xhair = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);

    // Fonts.
    {
      TtfSetup();
      struct FontPair {
        char const *path;
        int size;
      };
      FontPair font_array[] = {
        {"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
          12},
        {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
          12},
        // Traditional OSX font, nicer than Helvetica Neue in cramped GUI:s.
        {"/System/Library/Fonts/LucidaGrande.ttc",
          20}
      };
      for (size_t i = 0; i < LENGTH(font_array); ++i) {
        auto const &f = font_array[i];
        g_font = FontLoad(f.path);
        if (g_font) {
          FontSetSize(g_font, f.size);
          break;
        }
      }
      if (!g_font) {
        std::cerr << "No fonts found!\n";
        throw std::runtime_error(__func__);
      }
    }
  }

  void Destroy()
  {
    FontUnload(&g_font);
    TtfShutdown();
    SDL_FreeCursor(g_cursor_xhair);
    SDL_FreeCursor(g_cursor_arrow);
    SDL_Quit();
  }

  void StyleSet(Style a_style)
  {
    g_style_i = a_style;
  }

  void Begin()
  {
    assert(!g_pointer_up);
  }

  void End()
  {
    g_pointer_up = false;
  }

  //
  // Event handling.
  //

  void ProcessEvent(SDL_Event const &a_event)
  {
    if (SDL_MOUSEBUTTONUP == a_event.type) {
      g_pointer_up = true;
    }
    for (auto it = g_window_set.begin(); g_window_set.end() != it; ++it) {
      (*it)->ProcessEvent(a_event);
    }
  }

  bool Window::DoClose() const
  {
    return m_do_close;
  }

  void Window::ProcessEvent(SDL_Event const &a_sdl_ev)
  {
    auto flags = SDL_GetWindowFlags(m_window);
    auto mouse_focus = !!(SDL_WINDOW_MOUSE_FOCUS & flags);
    auto input_focus = !!(SDL_WINDOW_INPUT_FOCUS & flags);

    bool ok = true;
    Event e;
    e.sdl_ev = a_sdl_ev;
    auto pointer = m_pointer;
    switch (e.sdl_ev.type) {
      case SDL_KEYDOWN:
        ok &= input_focus;
        if (((KMOD_LCTRL | KMOD_RCTRL | KMOD_LGUI | KMOD_RGUI) &
             e.sdl_ev.key.keysym.mod)) {
          if (SDLK_q == e.sdl_ev.key.keysym.sym) {
            g_do_quit = true;
          }
          if (SDLK_w == e.sdl_ev.key.keysym.sym &&
              SDL_GetWindowID(m_window) == e.sdl_ev.key.windowID) {
            m_do_close = true;
          }
        }
        break;
      case SDL_MOUSEMOTION:
        ok &= mouse_focus;
        if (m_wsize.x > 0) {
          // Scale for high-DPI display attributes.
          e.pointer.x = m_rsize.x * e.sdl_ev.motion.x / m_wsize.x;
          e.pointer.y = m_rsize.y * e.sdl_ev.motion.y / m_wsize.y;
          pointer = e.pointer;
        }
        break;
      case SDL_MOUSEBUTTONDOWN:
        ok &= mouse_focus;
        /* FALLTHROUGH */
      case SDL_MOUSEBUTTONUP:
        if (m_wsize.x > 0) {
          e.pointer.x = m_rsize.x * e.sdl_ev.button.x / m_wsize.x;
          e.pointer.y = m_rsize.y * e.sdl_ev.button.y / m_wsize.y;
          pointer = e.pointer;
        }
        break;
      case SDL_MOUSEWHEEL:
        ok &= mouse_focus;
        break;
      case SDL_QUIT:
        g_do_quit = true;
        break;
      case SDL_WINDOWEVENT:
        if (SDL_WINDOWEVENT_CLOSE == e.sdl_ev.window.event &&
            m_window_id == e.sdl_ev.window.windowID) {
          m_do_close = true;
        }
        break;
    }
    if (ok) {
      e.pointer = m_pointer = pointer;
      m_event_list.push_back(e);
    }
  }

  //
  // Basics.
  //

  bool DoQuit()
  {
    return g_do_quit;
  }

  void Window::Begin()
  {
    // Just make sure it's not leaking.
    assert(m_event_list.size() < 1000);
    assert(m_level_stack.empty());

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
    SDL_RenderClear(m_renderer);

    SDL_GetWindowSize(m_window, &m_wsize.x, &m_wsize.y);
    SDL_CALL_VOID(SDL_GetRendererOutputSize,
        (m_renderer, &m_rsize.x, &m_rsize.y));

    RenderColor(g_style[g_style_i][STYLE_BG]);
    Rect r;
    r.x = 0;
    r.y = 0;
    r.w = m_rsize.x;
    r.h = m_rsize.y;
    LevelPush(r);
    RenderRect(r, false);
  }

  void Window::End()
  {
    LevelPop();
    assert(m_level_stack.empty());

    m_event_list.clear();

    SDL_RenderPresent(m_renderer);

    // Destroy old unused textures.
    for (auto it = m_text_tex_map.begin(); m_text_tex_map.end() != it;) {
      auto it_next = it;
      ++it_next;
      auto &val = it->second;
      if (val.t_last + 1000 < Time_get_ms()) {
        m_tex_destroy_list.push_back(val.tex);
        m_text_tex_map.erase(it);
      }
      it = it_next;
    }
    for (auto it = m_tex_destroy_list.begin(); m_tex_destroy_list.end() != it;
        ++it) {
      SDL_DestroyTexture(*it);
    }
    m_tex_destroy_list.clear();
  }

  Pos Window::GetSize()
  {
    auto const &l = LevelGet();
    auto const &r = l.rect;
    return Pos(r.w - (l.cursor.x - l.rect.x),
        r.h - (l.cursor.y - l.rect.y));
  }

  int Window::Newline()
  {
    auto &l = LevelGet();
    auto max_h = l.max_h;
    l.cursor.x = l.rect.x;
    l.cursor.y += l.max_h;
    l.max_h = 0;
    return max_h;
  }

  void Window::Pop()
  {
    auto const &l = LevelGet();
    // Copy the rect before the pop destroys it!
    auto r = l.rect;
    LevelPop();
    LevelAdvance(Pos(r.w, r.h));
  }

  void Window::Push(Rect const &a_rect)
  {
    auto const &l = LevelGet();
    Rect r;
    r.x = a_rect.x + l.cursor.x;
    r.y = a_rect.y + l.cursor.y;
    r.w = a_rect.w;
    r.h = a_rect.h;
    LevelPush(r);
  }

  void Window::Rewind()
  {
    LevelPop();
    assert(m_level_stack.empty());
    Rect r;
    r.x = 0;
    r.y = 0;
    r.w = m_rsize.x;
    r.h = m_rsize.y;
    LevelPush(r);
  }

  //
  // Colormaps.
  //

  namespace {

    struct RGB {
      RGB():
        r(),
        g(),
        b()
      {
      }
      RGB(uint8_t a_r, uint8_t a_g, uint8_t a_b):
        r(a_r),
        g(a_g),
        b(a_b)
      {
      }
      uint8_t lerp_channel(uint8_t a_l, uint8_t a_r, double a_t)
      {
        return (uint8_t)(a_l + (a_r - a_l) * a_t);
      }
      void lerp(RGB const &a_l, RGB const &a_r, double a_t)
      {
        r = lerp_channel(a_l.r, a_r.r, a_t);
        g = lerp_channel(a_l.g, a_r.g, a_t);
        b = lerp_channel(a_l.b, a_r.b, a_t);
      }
      uint8_t r;
      uint8_t g;
      uint8_t b;
    };
    struct Colormap {
      Colormap():
        name(),
        ramp()
      {
      }
      std::string name;
      std::vector<RGB> ramp;
    };
    std::vector<Colormap> g_cmap_vec;
    size_t g_cmap_i;
  }

  size_t ColormapGet(char const *a_name)
  {
    if (g_cmap_vec.empty()) {
      // Always have the default ones ready.
      {
        g_cmap_vec.push_back(Colormap());
        auto &cmap = g_cmap_vec.back();
        cmap.name = "roma";
        for (size_t i = 0; i < LENGTH(g_roma); ++i) {
          RGB rgb(
              g_roma[i][0],
              g_roma[i][1],
              g_roma[i][2]);
          cmap.ramp.push_back(rgb);
        }
      }
    }
    if (nullptr == a_name) {
      a_name = "roma";
    }
    for (size_t i = 0; i < g_cmap_vec.size(); ++i) {
      if (0 == g_cmap_vec.at(i).name.compare(a_name)) {
        return i;
      }
    }
    // Try scientific colormaps.
    {
      std::ostringstream oss;
      oss << "../ScientificColourMaps7/" << a_name << '/' << a_name << ".lut";
      auto path = oss.str();
      std::ifstream ifile(path.c_str());
      if (ifile.is_open()) {
        g_cmap_vec.push_back(Colormap());
        auto &cmap = g_cmap_vec.back();
        cmap.name = a_name;
        int r, g, b;
        while (ifile >> r >> g >> b) {
          cmap.ramp.push_back(RGB((uint8_t)r, (uint8_t)g, (uint8_t)b));
        }
        return g_cmap_vec.size() - 1;
      }
    }
    std::cerr << "Unknown palette \"" << a_name << "\".\n";
    throw std::runtime_error(__func__);
  }

  void ColormapPop()
  {
    g_cmap_i = 0;
  }

  void ColormapPush(size_t a_i)
  {
    g_cmap_i = a_i;
  }

  //
  // Standard widgets.
  //

  bool Window::Button(char const *a_label)
  {
    auto size = RenderTextMeasure(a_label, TEXT_NORMAL);

    Rect r;
    r.x = PAD_EXT;
    r.y = PAD_EXT;
    r.w = size.x + 2 * PAD_INT;
    r.h = size.y + 2 * PAD_INT;

    size_t bg_i = ContainsLocal(r, m_pointer) ? STYLE_HOVER : STYLE_FG;
    bool click = false;
    for (auto it = m_event_list.begin(); m_event_list.end() != it; ++it) {
      if (InputPointerButtonDown(*it, r)) {
        bg_i = STYLE_SELECTED;
        click = true;
      } else if (ContainsLocal(r, it->pointer) && !click) {
        bg_i = STYLE_HOVER;
      }
    }
    RenderColor(g_style[g_style_i][bg_i]);
    RenderRect(r, true);
    RenderText(a_label, TEXT_NORMAL, STYLE_TEXT, Pos(PAD, PAD));

    LevelAdvance(Pos(r.w + 2 * PAD_EXT, r.h + 2 * PAD_EXT));

    return click;
  }

  void Window::Checkbox(char const *a_label, CheckboxState *a_state)
  {
    auto h = FontGetHeight(g_font);
    Rect r_box;
    r_box.x = PAD;
    r_box.y = PAD;
    r_box.w = h;
    r_box.h = h;

    for (auto it = m_event_list.begin(); m_event_list.end() != it; ++it) {
      if (InputPointerButtonDown(*it, r_box) && !a_state->is_locked) {
        a_state->is_on ^= true;
        a_state->is_locked = true;
      } else if (InputPointerButtonUp()) {
        a_state->is_locked = false;
      }
    }
    size_t bg_i = ContainsLocal(r_box, m_pointer) ? STYLE_HOVER : STYLE_FG;
    RenderColor(g_style[g_style_i][bg_i]);
    RenderRect(r_box, true);

    if (a_state->is_on) {
      RenderTexture(m_checkmark_tex, r_box);
    }

    LevelAdvance(Pos(PAD + h + PAD_INT, h));

    auto label_size = RenderTextMeasure(a_label, TEXT_NORMAL);
    RenderText(a_label, TEXT_NORMAL, STYLE_TEXT, Pos(PAD_INT, PAD));

    LevelAdvance(Pos(PAD_INT + label_size.x + PAD, label_size.y + 2 * PAD));
  }

  void Window::Panel()
  {
    RenderColor(g_style[g_style_i][STYLE_BG]);
    auto size = GetSize();
    Rect r;
    r.x = 0;
    r.y = 0;
    r.w = size.x;
    r.h = size.y;
    RenderRect(r, false);

    RenderColor(g_style[g_style_i][STYLE_PLOT_AXIS]);
    Pos tl(r.x, r.y);
    Pos bl(r.x, r.y + r.h - 1);
    Pos br(r.x + r.w - 1, r.y + r.h - 1);
    Pos tr(r.x + r.w - 1, r.y);
    RenderLine(tl, bl);
    RenderLine(bl, br);
    RenderLine(br, tr);
    RenderLine(tr, tl);
  }

  void Window::Text(char const *a_fmt, ...)
  {
    char buf[1024];
    va_list args;
    va_start(args, a_fmt);
    vsnprintf(buf, sizeof buf, a_fmt, args);
    va_end(args);
    auto size = RenderTextMeasure(buf, TEXT_NORMAL);
    RenderText(buf, TEXT_NORMAL, STYLE_TEXT, Pos(PAD, PAD));
    LevelAdvance(Pos(size.x + 2 * PAD, size.y + 2 * PAD));
  }

  enum InputStatus Window::TextInput(TextInputState *a_state)
  {
    auto ret = STATUS_NOP;
    for (auto it = m_event_list.begin(); m_event_list.end() != it; ++it) {
      auto const &sdle = it->sdl_ev;
      switch (sdle.type) {
        case SDL_KEYDOWN:
          switch (sdle.key.keysym.sym) {
            case SDLK_BACKSPACE:
              if (a_state->pos > 0) {
                auto prev = Utf8Left(a_state->str, a_state->pos);
                if (prev == -1) {
                  prev = (ssize_t)a_state->pos - 1;
                }
                // Stitch substrings around removed character.
                auto tmp = a_state->str.substr(0, (size_t)prev) +
                    a_state->str.substr(a_state->pos, a_state->str.size() -
                        a_state->pos);
                a_state->str = tmp;
                a_state->pos = (size_t)prev;
              }
              break;
            case SDLK_DELETE:
              if (a_state->pos < a_state->str.size()) {
                auto next = Utf8Right(a_state->str, a_state->pos);
                if (next == -1) {
                  next = (ssize_t)a_state->pos + 1;
                }
                // Stitch substrings around removed character.
                auto tmp = a_state->str.substr(0, a_state->pos) +
                    a_state->str.substr((size_t)next, a_state->str.size() -
                        a_state->pos);
                a_state->str = tmp;
              }
              break;
            case SDLK_END:
              if (!a_state->str.empty()) {
                a_state->pos = a_state->str.size();
              }
              break;
            case SDLK_HOME:
              a_state->pos = 0;
              break;
            case SDLK_LEFT:
              if (a_state->pos > 0) {
                auto prev = Utf8Left(a_state->str, a_state->pos);
                if (prev == -1) {
                  prev = (ssize_t)a_state->pos - 1;
                }
                a_state->pos = (size_t)prev;
              }
              break;
            case SDLK_RETURN:
              ret = STATUS_OK;
              break;
            case SDLK_RIGHT:
              if (a_state->pos < a_state->str.size()) {
                auto next = Utf8Right(a_state->str, a_state->pos);
                if (next == -1) {
                  next = (ssize_t)a_state->pos + 1;
                }
                a_state->pos = (size_t)next;
              }
              break;
            case SDLK_TAB:
              // Tab-completion of filename part.
              auto suggestion = TabComplete(a_state->str);
              if (!suggestion.empty()) {
                a_state->str = suggestion;
                a_state->pos = a_state->str.size();
              }
              break;
          }
          break;
        case SDL_TEXTINPUT:
          {
            auto tmp =
                a_state->str.substr(0, a_state->pos) +
                sdle.text.text +
                a_state->str.substr(a_state->pos, a_state->str.size() -
                    a_state->pos);
            a_state->str = tmp;
            auto len = strlen(sdle.text.text);
            a_state->pos += len;
          }
          break;
        case SDL_TEXTEDITING:
          // Highlight sdle.edit.text in a_state->comp.
          // When done, text = "", start = 0, length = 0.
          a_state->comp_str = sdle.edit.text;
          a_state->comp_ofs = (size_t)sdle.edit.start;
          // TODO: sdle.edit.length?
          break;
      }
    }
    auto h = FontGetHeight(g_font);
    RenderColor(g_style[g_style_i][STYLE_FG]);
    Rect r;
    r.x = PAD_EXT;
    r.y = PAD_EXT;
    r.w = GetSize().x - 2 * PAD_EXT;
    r.h = h + 2 * PAD_INT;
    RenderRect(r, true);
    Pos cursor;
    Pos comp;
    std::string text =
        a_state->str.substr(0, a_state->pos) +
        a_state->comp_str +
        a_state->str.substr(a_state->pos, a_state->str.size() - a_state->pos);
    if (!text.empty()) {
      RenderText(text.data(), TEXT_NORMAL, STYLE_TEXT, Pos(PAD, PAD));
      if (a_state->pos > 0) {
        // Measure distance to cursor/composition start.
        auto tmp = text.substr(0, a_state->pos);
        comp = RenderTextMeasure(tmp.c_str(), TEXT_NORMAL);
      }
      if (0 == a_state->comp_ofs) {
        // No composition.
        cursor = comp;
      } else {
        // Measure distance to cursor offset by composition.
        auto tmp = text.substr(0, a_state->pos + a_state->comp_ofs);
        cursor = RenderTextMeasure(tmp.c_str(), TEXT_NORMAL);
      }
    }
    RenderColor(g_style[g_style_i][STYLE_TEXT]);
    // Draw cursor.
    RenderLine(
        Pos(PAD + cursor.x - 1, PAD),
        Pos(PAD + cursor.x - 1, PAD + h));
    if (0 != a_state->comp_ofs) {
      // Draw composition underline.
      RenderLine(
          Pos(PAD + comp.x - 1, PAD + h),
          Pos(PAD + cursor.x - 1, PAD + h));
    }
    LevelAdvance(Pos(GetSize().x, h + 2 * PAD));
    return ret;
  }

  //
  // Plotting.
  //

  PlotState::Cut::Cut():
    t(),
    title(),
    list()
  {
  }

  PlotState::Proj::Proj():
    state(),
    t(),
    title(),
    window(),
    plot_state(),
    vec(),
    point(),
    width(),
    band_r()
  {
  }

  PlotState::PlotState(uint32_t a_mask):
    state_mask(a_mask),
    user_state(),
    min_lin(),
    max_lin(),
    cut(),
    proj(),
    zooming()
  {
  }

  PlotState::~PlotState()
  {
    Unproject();
  }

  void PlotState::CutClear()
  {
    if (cut.t > 0) {
      SDL_SetCursor(g_cursor_arrow);
      if (PlotState::CUT == user_state) {
        user_state = PlotState::DEFAULT;
      }
      cut.t = 0;
      cut.list.clear();
    }
  }

  bool PlotState::GoTo(UserState a_state)
  {
    if ((a_state & state_mask)) {
      return false;
    }
    user_state = a_state;
    return true;
  }

  void PlotState::Project(UserState a_state, char const *a_title, Point const
      &a_min, Point const &a_max)
  {
    assert(PROJ_X == a_state || PROJ_Y == a_state);
    if (proj.window && proj.state == a_state) {
      return;
    }
    Unproject();
    if (GoTo(a_state)) {
      // Proj* is a transient state.
      user_state = DEFAULT;
      proj.state = a_state;
      proj.t = Time_get_ms();
      proj.title = a_title;
      proj.title += ": Projection";
      proj.title += a_state == PlotState::PROJ_X ? "X" : "Y";
      proj.window = new Window(proj.title.c_str(), 640, 480);
      proj.plot_state = new PlotState(CUT | PROJ_X | PROJ_Y);
      proj.width = 1;
      min_lin = a_min;
      max_lin = a_max;
    }
  }

  void PlotState::Unproject()
  {
    proj.t = 0;
    delete proj.window;
    proj.window = nullptr;
    delete proj.plot_state;
    proj.plot_state = nullptr;
    memset(&proj.band_r, 0, sizeof proj.band_r);
  }

#define PLOT_TMPL_INSTANTIATE \
  template PLOT_TMPL(uint32_t); \
  template PLOT_TMPL(float); \
  template PLOT_TMPL(double)

  struct PlotTicks {
    // Value of min tick.
    double min_val;
    // Position of min tick.
    int min_pos;
    // Max.
    double max_val;
    int max_pos;
    // # ticks to plot with label incl. min & max.
    int num;
  };

  // Figures out ticks along axis "a_size" long but don't go nearer than
  // "a_tip" to the max value.
  PlotTicks PlotGetTicks(int a_size, int a_tip, double a_min, double a_max)
  {
    auto size = a_size > a_tip ? a_size - a_tip : a_size;
    auto min = a_min;
    auto max = min + size * (a_max - a_min) / a_size;
    auto d = max - min;
    auto l = log10(d) - 0.1;
    auto l_i = floor(l);
    auto l_f = l - l_i;
    double i, e;
    if (l_f < 0.5) {
      i = 5;
      e = l_i - 1;
    } else if (l_f < 0.8) {
      i = 1;
      e = l_i;
    } else {
      i = 2;
      e = l_i;
    }
    auto step = i * pow(10, e);
    auto f = 1 / d;

    PlotTicks ticks;
    ticks.min_val = ceil(min / step) * step;
    ticks.min_pos = (int)(size * (ticks.min_val - min) * f);
    ticks.max_val = ceil(max / step) * step;
    ticks.max_pos = (int)(size * (ticks.max_val - min) * f);
    ticks.num = (int)((ticks.max_val - ticks.min_val) / step);
    return ticks;
  }

  void PlotPrintLabel(char *a_buf, size_t a_buf_bytes, bool a_is_log, double
      a_value)
  {
    snprintf(a_buf, a_buf_bytes, a_is_log ? "10^%g" : "%g", a_value);
  }

  // TODO: X-labels can be larger than Y-labels, take the max!
  void Plot::PlotXaxis(Rect const &a_rect, double a_min, double a_max)
  {
    auto const h = FontGetHeight(g_font);
    char buf[100];

    auto ticks = PlotGetTicks(a_rect.w, h, a_min, a_max);

    // Arrow.
    Pos tip(a_rect.x + a_rect.w - 1, a_rect.y);
    Pos base(a_rect.x, a_rect.y);
    m_window->RenderColor(g_style[g_style_i][STYLE_PLOT_AXIS]);
    m_window->RenderLine(tip, base);
    m_window->RenderLine(tip, Pos(tip.x - h/2, tip.y + h/2));

    // Ticks and labels.
    auto const dv = (ticks.max_val - ticks.min_val) / ticks.num;
    auto const dp = ticks.max_pos - ticks.min_pos;
    int x_prev = -10000;
    for (int i = 0; i < ticks.num; ++i) {
      auto val = ticks.min_val + dv * i;
      PlotPrintLabel(buf, sizeof buf, false, val);
      auto x = a_rect.x + ticks.min_pos + dp * (int)i / (int)ticks.num;
      m_window->RenderLine(Pos(x, a_rect.y), Pos(x, a_rect.y + TICK_SIZE));
      auto size = m_window->RenderTextMeasure(buf, Window::TEXT_NORMAL);
      // Avoid overlapping labels.
      int x_curr = x - size.x / 2;
      if (x_curr > x_prev) {
        m_window->RenderText(buf, Window::TEXT_NORMAL, STYLE_PLOT_AXIS,
            Pos(x_curr, a_rect.y + TICK_SIZE + PAD_INT));
        x_prev = x_curr + size.x;
      }
    }
  }

  Rect Plot::PlotYaxis(Rect const &a_rect, double a_min, double a_max, bool
      a_is_log)
  {
    auto const h = FontGetHeight(g_font);
    char buf[100];

    auto height = a_rect.h - (PAD_EXT + h + PAD_INT + TICK_SIZE);
    auto ticks = PlotGetTicks(height, h, a_min, a_max);

    // Measure label widths and find max.
    std::vector<int> width_vec((size_t)ticks.num);
    int label_x = 0;
    auto const dv = (ticks.max_val - ticks.min_val) / ticks.num;
    for (int i = 0; i < ticks.num; ++i) {
      auto val = ticks.min_val + dv * (double)i;
      PlotPrintLabel(buf, sizeof buf, a_is_log, val);
      auto size = m_window->RenderTextMeasure(buf, Window::TEXT_NORMAL);
      label_x = std::max(label_x, size.x);
      width_vec.at((size_t)i) = size.x;
    }

    // Arrow.
    Pos tip(PAD_EXT + label_x + PAD_INT + TICK_SIZE, 0);
    Pos base(tip.x, height);
    m_window->RenderColor(g_style[g_style_i][STYLE_PLOT_AXIS]);
    m_window->RenderLine(tip, base);
    m_window->RenderLine(tip, Pos(tip.x - h/2, tip.y + h/2));

    // Ticks and labels.
    auto const dp = ticks.max_pos - ticks.min_pos;
    for (int i = 0; i < ticks.num; ++i) {
      auto val = ticks.min_val + dv * i;
      PlotPrintLabel(buf, sizeof buf, a_is_log, val);
      auto pos = ticks.min_pos + dp * (int)i / (int)ticks.num;
      auto y = base.y - 1 - pos;
      m_window->RenderLine(Pos(label_x + 5, y), Pos(label_x + 10, y));
      m_window->RenderText(buf, Window::TEXT_NORMAL, STYLE_PLOT_AXIS,
          Pos(PAD_EXT + label_x - width_vec.at((size_t)i), y - h/2));
    }

    // Size of axis.
    Rect r;
    r.x = 0;
    r.y = 0;
    r.w = tip.x + 1;
    r.h = base.y;
    return r;
  }

  Plot::Plot(Window *a_window, PlotState *a_state,
      char const *a_title, Pos const &a_size,
      Point const &a_min_lin, Point const &a_max_lin,
      bool a_is_log_x, bool a_is_log_y, bool a_is_log_z,
      bool a_is_2d):
    m_window(a_window),
    m_state(a_state),
    m_rect_tot(),
    m_rect_graph(),
    m_min(a_min_lin),
    m_max(a_max_lin),
    m_is_log(),
    m_is_2d(a_is_2d)
  {
    m_is_log.x = a_is_log_x;
    m_is_log.y = a_is_log_y;
    m_is_log.z = a_is_log_z;

    // Render title.
    FontSetBold(g_font, true);
    auto title_size = m_window->RenderTextMeasure(a_title, Window::TEXT_BOLD);
    auto dx = (a_size.x - title_size.x) / 2;
    m_window->RenderText(a_title, Window::TEXT_BOLD, STYLE_TEXT,
        Pos(dx, PAD_EXT));
    m_rect_tot.x = 0;
    m_rect_tot.y = PAD_EXT + title_size.y + PAD_INT;
    m_rect_tot.w = a_size.x;
    m_rect_tot.h = a_size.y - m_rect_tot.y;
    m_window->LevelAdvance(Pos(0, PAD_EXT + title_size.y + PAD_INT));
    m_window->Newline();

    auto const &l = m_window->LevelGet();

    // Override incoming limits when user is/has been interacting.
    if (Time_get_ms() < m_state->cut.t + TENSION_TIMEOUT_MS ||
        Time_get_ms() < m_state->proj.t + TENSION_TIMEOUT_MS) {
      m_min = m_state->min_lin;
      m_max = m_state->max_lin;
    }
    if (Time_get_ms() < m_state->zooming.x.t + TENSION_TIMEOUT_MS) {
      m_min.x = m_state->min_lin.x;
      m_max.x = m_state->max_lin.x;
    }
    if (Time_get_ms() < m_state->zooming.y.t + TENSION_TIMEOUT_MS) {
      m_min.y = m_state->min_lin.y;
      m_max.y = m_state->max_lin.y;
    }
    if (Time_get_ms() >= m_state->cut.t + TENSION_TIMEOUT_MS) {
      m_state->CutClear();
    }
    if (m_state->proj.window &&
        (Time_get_ms() >= m_state->proj.t + TENSION_TIMEOUT_MS ||
         m_state->proj.window->DoClose())) {
      m_state->Unproject();
    }

    // Note: These only rely on m_is_log, should be safe!
    m_min.x = LinOrLogFromLinX(m_min.x);
    m_min.y = LinOrLogFromLinY(m_min.y);
    m_max.x = LinOrLogFromLinX(m_max.x);
    m_max.y = LinOrLogFromLinY(m_max.y);

    // Y-axis first, depends on label widths.
    auto r_y = PlotYaxis(m_rect_tot, m_min.y, m_max.y, a_is_log_y);

    // Construct X-axis.
    Rect r_x;
    r_x.x = r_y.x + r_y.w;
    r_x.y = r_y.y + r_y.h;
    r_x.w = m_rect_tot.w - r_y.w - PAD_EXT;
    r_x.h = m_rect_tot.h - r_y.h - PAD_EXT;
    PlotXaxis(r_x, m_min.x, m_max.x);

    // Graph rect, maps to m_min/m_max.
    m_rect_graph.x = r_x.x;
    m_rect_graph.y = r_y.y;
    m_rect_graph.w = r_x.w;
    m_rect_graph.h = r_y.h;

    // Interaction, work through buffered events.

    auto event_list = a_window->m_event_list;
    for (auto it = event_list.begin(); event_list.end() != it; ++it) {
      auto const &sdle = it->sdl_ev;
      switch (sdle.type) {

        case SDL_KEYDOWN:
          switch (m_state->user_state) {
            case PlotState::DEFAULT:
              switch (sdle.key.keysym.sym) {
                case SDLK_c:
                  if (m_window->ContainsLocal(m_rect_graph, it->pointer)) {
                    // Starting cut.
                    if (m_state->GoTo(PlotState::CUT)) {
                      m_state->cut.t = Time_get_ms();
                      m_state->cut.title = a_title;
                      m_state->min_lin.x = LinFromLinOrLogX(m_min.x);
                      m_state->min_lin.y = LinFromLinOrLogY(m_min.y);
                      m_state->max_lin.x = LinFromLinOrLogX(m_max.x);
                      m_state->max_lin.y = LinFromLinOrLogY(m_max.y);
                      SDL_SetCursor(g_cursor_xhair);
                    }
                  }
                  break;
                case SDLK_u:
                  if (m_window->ContainsLocal(m_rect_graph, it->pointer) ||
                      m_window->ContainsLocal(r_x, it->pointer) ||
                      m_window->ContainsLocal(r_y, it->pointer)) {
                    // Unzoom.
                    m_state->zooming.x.t = 0;
                    m_state->zooming.y.t = 0;
                  }
                  break;
                case SDLK_x:
                  if (m_window->ContainsLocal(m_rect_graph, it->pointer)) {
                    // X-projection.
                    m_state->Project(PlotState::PROJ_X, a_title, m_min,
                        m_max);
                  }
                  break;
                case SDLK_y:
                  if (m_window->ContainsLocal(m_rect_graph, it->pointer)) {
                    // Y-projection.
                    m_state->Project(PlotState::PROJ_Y, a_title, m_min,
                        m_max);
                  }
                  break;
                case SDLK_w:
                  if (m_window->ContainsLocal(m_rect_graph, it->pointer) &&
                      m_state->proj.window) {
                    ++m_state->proj.width;
                  }
                  break;
                case SDLK_s:
                  if (m_window->ContainsLocal(m_rect_graph, it->pointer) &&
                      m_state->proj.window &&
                      m_state->proj.width > 1) {
                    --m_state->proj.width;
                  }
                  break;
              }
              break;
            case PlotState::CUT:
              switch (sdle.key.keysym.sym) {
                case SDLK_ESCAPE:
                  // Cancel cut.
                  m_state->CutClear();
                  break;
                case SDLK_RETURN:
                  // Accept the cut, maybe.
                  if (SaveCut()) {
                    m_state->CutClear();
                  }
                  break;
                case SDLK_BACKSPACE:
                  if (!m_state->cut.list.empty()) {
                    // Undo the last point.
                    m_state->cut.t = Time_get_ms();
                    m_state->cut.list.pop_back();
                  }
                  break;
              }
              break;
            default:
              break;
          }
          break;

        case SDL_MOUSEBUTTONDOWN:
          memset(&m_state->zooming.band_r, 0, sizeof m_state->zooming.band_r);
          switch (m_state->user_state) {
            case PlotState::DEFAULT:
              if (m_window->ContainsLocal(r_x, it->pointer)) {
                m_state->GoTo(PlotState::ZOOM_X);
              } else if (m_window->ContainsLocal(r_y, it->pointer)) {
                m_state->GoTo(PlotState::ZOOM_Y);
              } else if (m_window->ContainsLocal(m_rect_graph, it->pointer)) {
                m_state->GoTo(PlotState::ZOOM_XY);
              }
              if (PlotState::ZOOM_X == m_state->user_state ||
                  PlotState::ZOOM_Y == m_state->user_state ||
                  PlotState::ZOOM_XY == m_state->user_state) {
                m_state->min_lin.x = LinFromLinOrLogX(m_min.x);
                m_state->min_lin.y = LinFromLinOrLogY(m_min.y);
                m_state->max_lin.x = LinFromLinOrLogX(m_max.x);
                m_state->max_lin.y = LinFromLinOrLogY(m_max.y);
              }
              if (PlotState::ZOOM_X == m_state->user_state ||
                  PlotState::ZOOM_XY == m_state->user_state) {
                m_state->zooming.x.t = Time_get_ms();
                m_state->zooming.x.min = r_x.x;
                m_state->zooming.x.max = r_x.x + r_x.w;
                m_state->zooming.x.end = m_state->zooming.x.start =
                    it->pointer.x - l.cursor.x;
              }
              if (PlotState::ZOOM_Y == m_state->user_state ||
                  PlotState::ZOOM_XY == m_state->user_state) {
                m_state->zooming.y.t = Time_get_ms();
                m_state->zooming.y.min = r_y.y;
                m_state->zooming.y.max = r_y.y + r_y.h;
                m_state->zooming.y.end = m_state->zooming.y.start =
                    it->pointer.y - l.cursor.y;
              }
              break;
            case PlotState::CUT:
              if (m_window->ContainsLocal(m_rect_graph, it->pointer)) {
                // Next cut point.
                m_state->cut.t = Time_get_ms();
                m_state->cut.list.push_back(Point(
                    PointFromPosX(it->pointer.x - l.cursor.x),
                    PointFromPosY(it->pointer.y - l.cursor.y)));
              }
              break;
            default:
              break;
          }
          break;

        case SDL_MOUSEBUTTONUP:
          if (PlotState::ZOOM_X == m_state->user_state ||
              PlotState::ZOOM_XY == m_state->user_state) {
            // Finished drag-zooming in X.
            m_state->zooming.x.t = Time_get_ms();
            m_state->zooming.x.end = it->pointer.x - l.cursor.x;
            auto left = std::min(
                m_state->zooming.x.start,
                m_state->zooming.x.end);
            auto right = std::max(
                m_state->zooming.x.start,
                m_state->zooming.x.end);
            if (right - left > 5) {
              auto min = LinOrLogFromLinX(m_state->min_lin.x);
              auto max = LinOrLogFromLinX(m_state->max_lin.x);
              auto span = max - min;
              auto width = m_state->zooming.x.max - m_state->zooming.x.min;
              min += span * (left - m_state->zooming.x.min) / width;
              max -= span * (m_state->zooming.x.max - right) / width;
              min = LinFromLinOrLogX(min);
              max = LinFromLinOrLogX(max);
              min = TRUNC(min, a_min_lin.x, a_max_lin.x);
              max = TRUNC(max, a_min_lin.x, a_max_lin.x);
              m_state->min_lin.x = min;
              m_state->max_lin.x = max;
              m_min.x = LinOrLogFromLinX(min);
              m_max.x = LinOrLogFromLinX(max);
            }
          }
          if (PlotState::ZOOM_Y == m_state->user_state ||
              PlotState::ZOOM_XY == m_state->user_state) {
            // Finished drag-zooming in Y.
            m_state->zooming.y.t = Time_get_ms();
            m_state->zooming.y.end = it->pointer.y - l.cursor.y;
            auto top = std::min(
                m_state->zooming.y.start,
                m_state->zooming.y.end);
            auto bottom = std::max(
                m_state->zooming.y.start,
                m_state->zooming.y.end);
            if (bottom - top > 5) {
              auto min = LinOrLogFromLinY(m_state->min_lin.y);
              auto max = LinOrLogFromLinY(m_state->max_lin.y);
              auto span = max - min;
              auto height = m_state->zooming.y.max - m_state->zooming.y.min;
              min += span * (m_state->zooming.y.max - bottom) / height;
              max -= span * (top - m_state->zooming.y.min) / height;
              min = LinFromLinOrLogY(min);
              max = LinFromLinOrLogY(max);
              min = TRUNC(min, a_min_lin.y, a_max_lin.y);
              max = TRUNC(max, a_min_lin.y, a_max_lin.y);
              m_state->min_lin.y = min;
              m_state->max_lin.y = max;
              m_min.y = LinOrLogFromLinY(min);
              m_max.y = LinOrLogFromLinY(max);
            }
          }
          if (PlotState::ZOOM_X == m_state->user_state ||
              PlotState::ZOOM_Y == m_state->user_state ||
              PlotState::ZOOM_XY == m_state->user_state) {
            m_state->user_state = PlotState::DEFAULT;
          }
          break;

        case SDL_MOUSEMOTION:
          m_state->zooming.band_r.x = r_y.x;
          m_state->zooming.band_r.y = r_y.y;
          m_state->zooming.band_r.w = r_y.w + r_x.w;
          m_state->zooming.band_r.h = r_y.h + r_x.h;
          if (PlotState::ZOOM_X == m_state->user_state ||
              PlotState::ZOOM_XY == m_state->user_state) {
            // Update x-zooming vis.
            m_state->zooming.x.t = Time_get_ms();
            m_state->zooming.x.end = it->pointer.x - l.cursor.x;
            m_state->zooming.band_r.x = std::min(
                m_state->zooming.x.start, m_state->zooming.x.end);
            m_state->zooming.band_r.w = std::abs(
                m_state->zooming.x.end - m_state->zooming.x.start);
          }
          if (PlotState::ZOOM_Y == m_state->user_state ||
              PlotState::ZOOM_XY == m_state->user_state) {
            // Update y-zooming vis.
            m_state->zooming.y.t = Time_get_ms();
            m_state->zooming.y.end = it->pointer.y - l.cursor.y;
            m_state->zooming.band_r.y = std::min(
                m_state->zooming.y.start, m_state->zooming.y.end);
            m_state->zooming.band_r.h = std::abs(
                m_state->zooming.y.end - m_state->zooming.y.start);
          }
          break;

        case SDL_MOUSEWHEEL:
          // Mouse-wheel scrolling.
          if (sdle.wheel.y) {
            auto mod_state = SDL_GetModState();
            if (KMOD_NONE == mod_state) {
              auto f = pow(ZOOM_FACTOR, sdle.wheel.y);
              bool x_yes = false;
              bool y_yes = false;
              if (m_window->ContainsLocal(r_x, it->pointer)) {
                x_yes = true;
              } else if (m_window->ContainsLocal(r_y, it->pointer)) {
                y_yes = true;
              } else if (m_window->ContainsLocal(m_rect_graph, it->pointer)) {
                x_yes = true;
                y_yes = true;
              }
              if (x_yes) {
                auto pos_x = it->pointer.x - l.cursor.x;
                auto x = m_min.x + (m_max.x - m_min.x) * (pos_x - r_x.x) /
                    r_x.w;
                m_state->user_state = PlotState::DEFAULT;
                m_state->zooming.x.t = Time_get_ms();
                m_min.x = x - (x - m_min.x) * f;
                m_max.x = x + (m_max.x - x) * f;
              }
              if (y_yes) {
                auto pos_y = it->pointer.y - l.cursor.y;
                auto y = m_max.y - (m_max.y - m_min.y) * (pos_y - r_y.y) /
                    r_y.h;
                m_state->user_state = PlotState::DEFAULT;
                m_state->zooming.y.t = Time_get_ms();
                m_min.y = y - (y - m_min.y) * f;
                m_max.y = y + (m_max.y - y) * f;
              }
              if (x_yes || y_yes) {
                Point min_lin, max_lin;
                min_lin.x = LinFromLinOrLogX(m_min.x);
                min_lin.y = LinFromLinOrLogY(m_min.y);
                max_lin.x = LinFromLinOrLogX(m_max.x);
                max_lin.y = LinFromLinOrLogY(m_max.y);
                min_lin.x = TRUNC(min_lin.x, a_min_lin.x, a_max_lin.x);
                min_lin.y = TRUNC(min_lin.y, a_min_lin.y, a_max_lin.y);
                max_lin.x = TRUNC(max_lin.x, a_min_lin.x, a_max_lin.x);
                max_lin.y = TRUNC(max_lin.y, a_min_lin.y, a_max_lin.y);
                m_state->min_lin = min_lin;
                m_state->max_lin = max_lin;
                m_min.x = LinOrLogFromLinX(min_lin.x);
                m_min.y = LinOrLogFromLinY(min_lin.y);
                m_max.x = LinOrLogFromLinX(max_lin.x);
                m_max.y = LinOrLogFromLinY(max_lin.y);
                // Reset if user zoomed the whole way out.
                if (min_lin.x == a_min_lin.x && max_lin.x == a_max_lin.x) {
                  m_state->zooming.x.t = 0;
                }
                if (min_lin.y == a_min_lin.y && max_lin.y == a_max_lin.y) {
                  m_state->zooming.y.t = 0;
                }
              }
            } else if (KMOD_SHIFT & mod_state) {
              // Pan "10% * wheel" of view in X.
              auto step = 0.1 * (m_max.x - m_min.x) * sdle.wheel.y;
              auto const min_x = LinOrLogFromLinX(a_min_lin.x);
              auto const max_x = LinOrLogFromLinX(a_max_lin.x);
              if (m_min.x + step < min_x) {
                step = min_x - m_min.x;
              }
              if (m_max.x + step > max_x) {
                step = max_x - m_max.x;
              }
              m_min.x += step;
              m_max.x += step;
              m_state->min_lin.x = LinFromLinOrLogX(m_min.x);
              m_state->max_lin.x = LinFromLinOrLogX(m_max.x);
            } else if (KMOD_CTRL & mod_state) {
              // Pan in Y.
              auto step = 0.1 * (m_max.y - m_min.y) * sdle.wheel.y;
              auto const min_y = LinOrLogFromLinX(a_min_lin.y);
              auto const max_y = LinOrLogFromLinX(a_max_lin.y);
              if (m_min.y + step < min_y) {
                step = min_y - m_min.y;
              }
              if (m_max.y + step > max_y) {
                step = max_y - m_max.y;
              }
              m_min.y += step;
              m_max.y += step;
              m_state->min_lin.y = LinFromLinOrLogX(m_min.y);
              m_state->max_lin.y = LinFromLinOrLogX(m_max.y);
            }
          }
          break;
      }
    }

    if (m_state->proj.window) {
      // Projection user position.
      auto const &pointer = m_window->m_pointer;
      if (m_window->ContainsLocal(m_rect_graph, pointer)) {
        m_state->proj.point.x = PointFromPosX(pointer.x - l.cursor.x);
        m_state->proj.point.y = PointFromPosY(pointer.y - l.cursor.y);
      }
    }

    // Clip graph.
    m_window->Push(m_rect_graph);
    m_rect_graph.x = 0;
    m_rect_graph.y = 0;
  }

  Plot::~Plot()
  {
    auto const &l = m_window->LevelGet();
    Pos curr(
        m_window->m_pointer.x - l.cursor.x,
        m_window->m_pointer.y - l.cursor.y);
    if (!m_state->cut.list.empty()) {
      // Plot cut in progress.
      m_window->RenderColor(g_style[g_style_i][STYLE_PLOT_AXIS]);
      // Convert from points to pos:es.
      std::vector<Pos> pos_list(m_state->cut.list.size());
      size_t i = 0;
      for (auto it = m_state->cut.list.begin(); m_state->cut.list.end() != it;
          ++it, ++i) {
        auto &dst = pos_list.at(i);
        dst.x = PosFromPointX(it->x);
        dst.y = PosFromPointY(it->y);
      }
      auto it2 = pos_list.begin();
      auto it1 = it2++;
      m_window->RenderCross(*it1, 5);
      for (; pos_list.end() != it2; ++it2) {
        m_window->RenderLine(*it1, *it2);
        m_window->RenderCross(*it2, 5);
        it1 = it2;
      }
      if (pos_list.size() > 2) {
        m_window->RenderLineDashed(*it1, *pos_list.begin(), 2, 5);
        m_window->RenderLineDashed(*it1, curr, 10, 5);
        m_window->RenderLineDashed(*pos_list.begin(), curr, 10, 5);
      }
    }
    if (m_window->ContainsLocal(m_rect_graph, m_window->m_pointer)) {
      // Draw pointer coordinates.
      auto px = PointFromPosX(curr.x);
      auto py = PointFromPosY(curr.y);
      std::ostringstream oss;
      oss << px << ',' << py;
      auto str = oss.str();
      m_window->PlotText(this, str.c_str(), Point(px, py), TEXT_ABOVE, true,
          true);
    }
    // Transparent projection band.
    m_window->RenderColor(g_style[g_style_i][STYLE_PLOT_USER_PROJ]);
    m_window->RenderTransparent(true);
    m_window->RenderRect(m_state->proj.band_r, false);
    m_window->RenderTransparent(false);
    // Pop the graph clipping so we can render axis overlays.
    m_window->LevelPop();
    if (PlotState::ZOOM_X == m_state->user_state ||
        PlotState::ZOOM_Y == m_state->user_state ||
        PlotState::ZOOM_XY == m_state->user_state) {
      // Transparent zooming band.
      m_window->RenderColor(g_style[g_style_i][STYLE_PLOT_USER_ZOOM]);
      m_window->RenderTransparent(true);
      m_window->RenderRect(m_state->zooming.band_r, false);
      m_window->RenderTransparent(false);
    }
  }

  double Plot::LinFromLinOrLogX(double a_x) const
  {
    return m_is_log.x ? pow(10, a_x) : a_x;
  }

  double Plot::LinFromLinOrLogY(double a_y) const
  {
    return m_is_log.y ? pow(10, a_y) : a_y;
  }

  double Plot::LinFromLinOrLogZ(double a_z) const
  {
    return m_is_log.z ? pow(10, a_z) : a_z;
  }

  double Plot::LinOrLogFromLinX(double a_x) const
  {
    return m_is_log.x ? log10(std::max(a_x, 1e-1)) : a_x;
  }

  double Plot::LinOrLogFromLinY(double a_y) const
  {
    return m_is_log.y ? log10(std::max(a_y, 1e-1)) : a_y;
  }

  double Plot::LinOrLogFromLinZ(double a_z) const
  {
    return m_is_log.z ? log10(std::max(a_z, 1e-1)) : a_z;
  }

  double Plot::PointFromPosX(int a_x) const
  {
    auto x = m_min.x +
        (m_max.x - m_min.x) * (a_x - m_rect_graph.x) / m_rect_graph.w;
    return LinFromLinOrLogX(x);
  }

  double Plot::PointFromPosY(int a_y) const
  {
    auto y = m_max.y -
        (m_max.y - m_min.y) * (a_y - m_rect_graph.y) / m_rect_graph.h;
    return LinFromLinOrLogY(y);
  }

  int Plot::PosFromPointX(double a_x) const
  {
    return m_rect_graph.x + (int)(
        m_rect_graph.w * (LinOrLogFromLinX(a_x) - m_min.x) /
        (m_max.x - m_min.x));
  }

  int Plot::PosFromPointY(double a_y) const
  {
    return m_rect_graph.y + m_rect_graph.h - (int)(
        m_rect_graph.h * (LinOrLogFromLinY(a_y) - m_min.y) /
        (m_max.y - m_min.y));
  }

  bool Plot::SaveCut()
  {
    size_t limit = m_is_2d ? 3 : 2;
    if (m_state->cut.list.size() < limit) {
      m_state->cut.t = Time_get_ms();
      std::cerr << "Cut must have at least " << limit << " vertices!\n";
      return false;
    }
    // Generate a name suggestion.
    std::string fname;
    for (int id = 1;; ++id) {
      std::ostringstream oss;
      oss << "cut" << id << ".txt";
      fname = oss.str();
      struct stat st;
      if (-1 == stat(fname.c_str(), &st)) {
        break;
      }
    }
    std::ofstream of(fname);
    of << m_state->cut.title << '\n';
    // Save points.
    for (auto it2 = m_state->cut.list.begin(); m_state->cut.list.end() != it2;
        ++it2) {
      of << it2->x;
      if (m_is_2d) {
        of << ' ' << it2->y;
      }
      of << '\n';
    }
    std::cout << "Saved cut in " << fname << ".\n";
    return true;
  }

  //
  // 1D histo.
  //

#define PLOT_TMPL(T) \
  void Window::PlotHist1(Plot const *a_plot, \
      double a_min, double a_max, \
      std::vector<T> const &a_vec, size_t a_bins)
  template <typename T> PLOT_TMPL(T)
  {
    auto const &r = a_plot->m_rect_graph;
    RenderColor(g_style[g_style_i][STYLE_PLOT_BG]);
    RenderRect(r, false);

    // TODO: Plot texture and render that, faster?

    RenderColor(g_style[g_style_i][STYLE_PLOT_FG]);
    // Figure out how many pixels the histogram covers.
    auto left = a_plot->PosFromPointX(a_min);
    auto right = a_plot->PosFromPointX(a_max);
    auto pixels = right - left;
    if (pixels >= (int)a_bins) {
      // We have at least 1px/bin, render one rect per entry.
      auto pos_x_prev = left;
      for (size_t i = 0; i < a_bins; ++i) {
        auto x = a_min + (a_max - a_min) * ((double)i + 1) / (double)a_bins;
        auto pos_x = a_plot->PosFromPointX(x);

        auto v = (double)a_vec.at(i);
        auto h = a_plot->PosFromPointY(v);

        Rect r_col;
        r_col.x = pos_x_prev;
        r_col.y = h;
        r_col.w = pos_x - pos_x_prev;
        r_col.h = r.h - h;
        RenderRect(r_col, false);
        pos_x_prev = pos_x;
      }
    } else {
      // Fewer pixels than bins, do nearest-neighbour boundaries and draw
      // filtered pixel-columns from vertical min to max.
      struct Blurred {
        int min;
        int max;
      };
      std::vector<Blurred> blurred((size_t)r.w);
      size_t i0 = (size_t)(
          (double)a_bins * (a_plot->PointFromPosX(0) - a_min)
          / (a_max - a_min));
      for (int pi = 0; pi < r.w; ++pi) {
        size_t i1 = (size_t)(
            (double)a_bins * (a_plot->PointFromPosX(pi + 1) - a_min) /
            (a_max - a_min));
        assert(i0 < i1);

        double sum = a_vec.at(i0);
        double min = sum;
        double max = sum;
        for (auto i = i0 + 1; i < i1; ++i) {
          auto v = (double)a_vec.at(i);
          sum += v;
          min = std::min(min, v);
          max = std::max(max, v);
        }
        auto v_min = min;
        auto v_max = max;

        auto h_min = a_plot->PosFromPointY(v_min);
        auto h_max = a_plot->PosFromPointY(v_max);

        Rect r_col;
        r_col.x = pi;
        r_col.y = h_min;
        r_col.w = 1;
        r_col.h = r.h - h_min;
        RenderRect(r_col, false);

        auto &b = blurred.at((size_t)pi);
        b.min = h_min;
        b.max = h_max;

        i0 = i1;
      }
      // Draw blurred min-max columns.
      SDL_Color col = g_style[g_style_i][STYLE_PLOT_FG];
      col.a = 128;
      RenderColor(col);
      a_plot->m_window->RenderTransparent(true);
      for (int pi = 0; pi < r.w; ++pi) {
        auto const &b = blurred.at((size_t)pi);
        Rect r_col;
        r_col.x = pi;
        r_col.y = b.max;
        r_col.w = 1;
        r_col.h = b.min - b.max;
        RenderRect(r_col, false);
      }
    a_plot->m_window->RenderTransparent(false);
    }
  }
  PLOT_TMPL_INSTANTIATE;
#undef PLOT_TMPL

  //
  // 2D histo.
  //

#define PLOT_TMPL(T) \
  void Window::PlotHist2(Plot *a_plot, size_t a_colormap, \
      Point const &a_min, Point const &a_max, \
      std::vector<T> const &a_vec, size_t a_bins_y, size_t a_bins_x, \
      std::vector<uint8_t> &a_pixels)
  template <typename T> PLOT_TMPL(T)
  {
    if (!a_bins_y || !a_bins_x) {
      return;
    }
    assert(a_bins_y * a_bins_x <= a_vec.size());

    auto const &rect = a_plot->m_rect_graph;

    T min_t = a_vec[0];
    T max_t = a_vec[0];
    for (size_t ofs = 1; ofs < a_bins_y * a_bins_x; ++ofs) {
      auto v = a_vec[ofs];
      min_t = std::min(min_t, v);
      max_t = std::max(max_t, v);
    }
    auto min_z = a_plot->LinOrLogFromLinZ(min_t);
    auto max_z = a_plot->LinOrLogFromLinZ(max_t);
    max_z = std::max(max_z, 1.0);

    auto const &cmap = g_cmap_vec.at(a_colormap);

    auto const &bg_col = g_style[g_style_i][STYLE_PLOT_BG];
    size_t w, h;
    if ((size_t)rect.w >= a_bins_x && (size_t)rect.h >= a_bins_y) {
      w = a_bins_x;
      h = a_bins_y;
      // More pixels than bins, let SDL scale the texture.
      auto bytes = w * h * 4;
      if (a_pixels.size() < bytes) {
        a_pixels.resize(bytes);
      }
      uint8_t *p = &a_pixels.at(0);
      for (size_t i = 0; i < h; ++i) {
        auto t = &a_vec.at((h - i - 1) * w);
        for (size_t j = 0; j < w; ++j) {
          *p++ = 255;
          auto v = (double)*t++;
          if (v > 0.0) {
            v = a_plot->LinOrLogFromLinZ(v);
            auto f = (v - min_z) / (max_z - min_z);
            auto const ramp_i = (size_t)(
                (double)(cmap.ramp.size() - 1) * f);
            auto const &rgb = cmap.ramp.at(ramp_i);
            *p++ = rgb.b;
            *p++ = rgb.g;
            *p++ = rgb.r;
          } else {
            *p++ = bg_col.b;
            *p++ = bg_col.g;
            *p++ = bg_col.r;
          }
        }
      }
    } else {
      // <1px/bin, do nearest-neighbour filtering.
      w = std::min((size_t)rect.w, a_bins_x);
      h = std::min((size_t)rect.h, a_bins_y);
      auto bytes = w * h * 4;
      if (a_pixels.size() < bytes) {
        a_pixels.resize(bytes);
      }
      uint8_t *p = &a_pixels.at(0);
      size_t i0 = 0;
      for (size_t y = 0; y < h; ++y) {
        auto i1 = a_bins_y * (y + 1) / (size_t)h;
        assert(i1 <= a_bins_y);
        auto i2 = i0 == i1 ? i1 + 1 : i1;
        size_t j0 = 0;
        for (size_t x = 0; x < w; ++x) {
          auto j1 = a_bins_x * (x + 1) / (size_t)w;
          assert(j1 <= a_bins_x);
          auto j2 = j0 == j1 ? j1 + 1 : j1;
          *p++ = 255;
          double v = 0.0;
          unsigned n = 0;
          auto ofs = (a_bins_y - i2) * a_bins_x + j0;
          for (auto i = i0; i < i2; ++i) {
            for (auto j = j0; j < j2; ++j) {
              v += a_vec.at(ofs++);
              ++n;
            }
            ofs += a_bins_x - (j2 - j0);
          }
          v /= n;
          if (v > 0.0) {
            v = a_plot->LinOrLogFromLinZ(v);
            auto f = (v - min_z) / (max_z - min_z);
            auto const ramp_i = (size_t)(
                (double)(cmap.ramp.size() - 1) * f);
            auto const &rgb = cmap.ramp.at(ramp_i);
            *p++ = rgb.b;
            *p++ = rgb.g;
            *p++ = rgb.r;
          } else {
            *p++ = bg_col.b;
            *p++ = bg_col.g;
            *p++ = bg_col.r;
          }
          j0 = j1;
        }
        i0 = i1;
      }
    }
    SDL_Surface *surf;
    SDL_CALL(surf, SDL_CreateRGBSurfaceWithFormatFrom,
        (&a_pixels[0], (int)w, (int)h, 32, (int)w * 4,
         SDL_PIXELFORMAT_RGBA8888));
    SDL_Texture *tex;
    SDL_CALL(tex, SDL_CreateTextureFromSurface, (m_renderer, surf));
    SDL_FreeSurface(surf);
    // Scale a_min/a_max into a_plot->m_min/m_max.
    Rect r;
    r.w = (int)(
        (a_max.x - a_min.x) * rect.w /
        (a_plot->m_max.x - a_plot->m_min.x)
    );
    r.h = (int)(
        (a_max.y - a_min.y) * rect.h /
        (a_plot->m_max.y - a_plot->m_min.y)
    );
    r.x = a_plot->PosFromPointX(a_min.x);
    r.y = a_plot->PosFromPointY(a_max.y);
    RenderTexture(tex, r);
    m_tex_destroy_list.push_back(tex);

    auto state = a_plot->m_state;
    if (state->proj.window) {
      state->proj.band_r = rect;
      // Draw the projection window.
      auto win = state->proj.window;
      win->Begin();
      double min_x, max_x;
      if (PlotState::PROJ_X == state->proj.state) {
        min_x = a_plot->m_min.x;
        max_x = a_plot->m_max.x;
        int i = (int)((state->proj.point.y - a_min.y) * (double)a_bins_y /
            (a_max.y - a_min.y));
        int i0 = i - state->proj.width / 2;
        i0 = std::max(i0, 0);
        int i1 = i0 + state->proj.width;
        i1 = std::min(i1, (int)a_bins_y);
        i0 = i1 - state->proj.width;
        i0 = std::max(i0, 0);
        if (a_bins_x > state->proj.vec.size()) {
          state->proj.vec.resize(a_bins_x);
        }
        for (size_t j = 0; j < a_bins_x; ++j) {
          state->proj.vec.at(j) = 0;
        }
        auto ap = &a_vec.at((size_t)i0 * a_bins_x);
        for (i = i0; i < i1; ++i) {
          for (size_t j = 0; j < a_bins_x; ++j) {
            state->proj.vec.at(j) += (float)*ap++;
          }
        }
        auto y0 = a_min.y + (a_max.y - a_min.y) * i0 / (int)a_bins_y;
        auto yy0 = a_plot->PosFromPointY(y0);
        auto y1 = a_min.y + (a_max.y - a_min.y) * i1 / (int)a_bins_y;
        auto yy1 = a_plot->PosFromPointY(y1);
        state->proj.band_r.y = yy0;
        state->proj.band_r.h = yy1 - yy0;
      } else if (PlotState::PROJ_Y == state->proj.state) {
        min_x = a_plot->m_min.y;
        max_x = a_plot->m_max.y;
        int j = (int)((state->proj.point.x - a_plot->m_min.x) *
            (double)a_bins_x / (a_plot->m_max.x - a_plot->m_min.x));
        int j0 = j - state->proj.width / 2;
        j0 = std::max(j0, 0);
        int j1 = j0 + state->proj.width;
        j1 = std::min(j1, (int)a_bins_x);
        j0 = j1 - state->proj.width;
        j0 = std::max(j0, 0);
        if (a_bins_y > state->proj.vec.size()) {
          state->proj.vec.resize(a_bins_y);
        }
        for (size_t i = 0; i < a_bins_y; ++i) {
          state->proj.vec.at(i) = 0;
        }
        auto ap = &a_vec.at((size_t)j0);
        for (size_t i = 0; i < a_bins_y; ++i) {
          for (j = j0; j < j1; ++j) {
            state->proj.vec.at(i) += (float)*ap++;
          }
          ap += a_bins_x - (size_t)(j1 - j0);
        }
        state->proj.band_r.x += rect.w * j0 / (int)a_bins_x;
        state->proj.band_r.w = rect.w * (j1 - j0) / (int)a_bins_x + 1;
      } else {
        throw std::runtime_error(__func__);
      }
      if (!state->proj.vec.empty()) {
        auto max_y = state->proj.vec[0];
        for (size_t i = 1; i < state->proj.vec.size(); ++i) {
          max_y = std::max(max_y, state->proj.vec.at(i));
        }
        auto size = win->GetSize();
        ImPlutt::Plot plot(win, state->proj.plot_state,
            state->proj.title.c_str(), size,
            ImPlutt::Point(min_x, 0.0),
            ImPlutt::Point(max_x, max_y * 1.1),
            false, a_plot->m_is_log.z, false, false);
        win->PlotHist1(&plot, min_x, max_x, state->proj.vec,
            state->proj.vec.size());
      }
      win->End();
    }
  }
  PLOT_TMPL_INSTANTIATE;
#undef PLOT_TMPL

  //
  // Poly-line.
  //

  void Window::PlotLines(Plot const *a_plot, std::vector<Point> const &a_v)
  {
    RenderColor(g_style[g_style_i][STYLE_PLOT_FIT]);
    auto s1 = &a_v.at(0);
    Pos d1(a_plot->PosFromPointX(s1->x), a_plot->PosFromPointY(s1->y));
    for (size_t i = 1; i < a_v.size(); ++i) {
      auto s2 = &a_v.at(i);
      Pos d2(a_plot->PosFromPointX(s2->x), a_plot->PosFromPointY(s2->y));
      RenderLine(d1, d2);
      s1 = s2;
      d1 = d2;
    }
  }

  //
  // Text.
  //

  void Window::PlotText(Plot const *a_plot, char const *a_str, Point const
      &a_pos, TextAlign a_align, bool a_do_fit, bool a_do_box)
  {
    auto size = RenderTextMeasure(a_str, TEXT_NORMAL);
    Pos pos(
        a_plot->PosFromPointX(a_pos.x),
        a_plot->PosFromPointY(a_pos.y));
    switch (a_align) {
      case TEXT_ABOVE:
        pos.x -= size.x / 2;
        pos.y -= size.y + PAD_INT;
        break;
      case TEXT_LEFT:
        pos.x -= size.x + PAD_INT;
        break;
      case TEXT_RIGHT:
        pos.x += PAD_INT;
        break;
      default:
        throw std::runtime_error(__func__);
    }
    if (a_do_fit) {
      pos.x = std::min(pos.x, a_plot->m_rect_graph.w - size.x);
      pos.x = std::max(pos.x, 0);
      pos.y = std::min(pos.y, a_plot->m_rect_graph.h - size.y);
      pos.y = std::max(pos.y, 0);
    }
    if (a_do_box) {
      Rect r;
      r.x = pos.x - 1;
      r.y = pos.y - 1;
      r.w = size.x + 2;
      r.h = size.y + 2;
      RenderBox(r, STYLE_PLOT_AXIS, STYLE_PLOT_OVERLAY);
    }
    RenderText(a_str, TEXT_NORMAL, STYLE_PLOT_AXIS, pos);
  }

}
