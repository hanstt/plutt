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

#ifndef IMPLUTT_HPP
#define IMPLUTT_HPP

#include <cstdint>
#include <cstdlib>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <SDL.h>
#include <vector.hpp>

union SDL_Event;
struct SDL_Renderer;
struct SDL_Window;

/*
 * ImGui+ImPlot replacement, small specialized version for SDL full-window
 * plotting.
 */

namespace ImPlutt {

  enum Style {
    STYLE_LIGHT,
    STYLE_DARK,
    STYLE_MAX_
  };
  enum TextAlign {
    TEXT_ABOVE,
    TEXT_LEFT,
    TEXT_RIGHT
  };
  enum InputStatus {
    STATUS_NOP,
    STATUS_OK
  };

  struct CheckboxState {
    // User code should listen to this.
    bool is_on;
    // Not this.
    bool is_locked;
  };
  struct Point {
    Point();
    Point(double, double);
    double x;
    double y;
  };
  struct Pos {
    Pos();
    Pos(int, int);
    int x;
    int y;
  };
  struct Rect {
    int x;
    int y;
    int w;
    int h;
  };
  struct Event {
    Event();
    SDL_Event sdl_ev;
    // Old versions of SDL don't keep the pointer position for all pointer
    // event types, so we do it.
    Pos pointer;
  };
  // This struct must be created before showing a TextInput and destroyed
  // after it's finished!
  struct TextInputState {
    TextInputState();
    ~TextInputState();
    std::string str;
    size_t pos;
    std::string comp_str;
    size_t comp_ofs;
  };

  class Window;

  struct PlotState {
    PlotState(uint32_t);
    ~PlotState();
    enum UserState {
      DEFAULT = 0,
      // When the user is creating a cut.
      CUT = 1 << 0,
      // X/Y-projection.
      PROJ_X = 1 << 1,
      PROJ_Y = 1 << 2,
      // When the user is zooming in X. Note that zoomed view has no state,
      // the zoom_x_t timeout keeps the zoomed extents.
      ZOOM_X = 1 << 3,
      // Ditto for y.
      ZOOM_Y = 1 << 4,
      // Both at once.
      ZOOM_XY = 1 << 5
    };
    uint32_t state_mask;
    UserState user_state;
    Point min_lin;
    Point max_lin;
    struct Cut {
      Cut();
      uint64_t t;
      std::string title;
      std::list<Point> list;
    };
    Cut cut;
    struct Proj {
      Proj();
      UserState state;
      uint64_t t;
      std::string title;
      Window *window;
      PlotState *plot_state;
      std::vector<float> vec;
      Point point;
      int width;
      Rect band_r;
      private:
        Proj(Proj const &);
        Proj &operator=(Proj const &);
    };
    Proj proj;
    struct {
      struct {
        uint64_t t;
        int min;
        int max;
        int start;
        int end;
      } x, y;
      Rect band_r;
    } zooming;
    void CutClear();
    bool GoTo(UserState);
    void Project(UserState, char const *, Point const &, Point const &);
    void Unproject();
    private:
      PlotState(PlotState const &);
      PlotState &operator=(PlotState const &);
  };

  class Plot {
    public:
      Plot(Window *, PlotState *, char const *, Pos const &, Point const &,
          Point const &, bool, bool, bool, bool);
      ~Plot();
      void DrawOverlay(PlotState const &);
      double LinFromLinOrLogX(double) const;
      double LinFromLinOrLogY(double) const;
      double LinFromLinOrLogZ(double) const;
      double LinOrLogFromLinX(double) const;
      double LinOrLogFromLinY(double) const;
      double LinOrLogFromLinZ(double) const;
      void PlotXaxis(Rect const &, double, double);
      Rect PlotYaxis(Rect const &, double, double, bool);
      double PointFromPosX(int) const;
      double PointFromPosY(int) const;
      int PosFromPointX(double) const;
      int PosFromPointY(double) const;
      bool SaveCut();
      Window *m_window;
      PlotState *m_state;
      Rect m_rect_tot;
      Rect m_rect_graph;
      Point m_min;
      Point m_max;
      struct {
        bool x;
        bool y;
        bool z;
      } m_is_log;
      bool m_is_2d;

    private:
      Plot(Plot const &);
      Plot &operator=(Plot const &);
  };

  void Setup();
  void Destroy();

  void StyleSet(Style);

  // Loads colormap from *.lut-file, or nullptr for a default.
  size_t ColormapGet(char const *);
  void ColormapPop();
  void ColormapPush(size_t);

  void Begin();
  void End();
  void ProcessEvent(SDL_Event const &);
  bool DoQuit();

  // UI space/clip stack.
  struct Level {
    Level(Rect const &a_rect);
    // Total size of working area, coordinates are global.
    Rect const rect;
    // Position for next widget, coordinates are global.
    Pos cursor;
    // Max height of widgets until 'Newline'.
    int max_h;
  };

  // Text texture cache.
  struct TextKey {
    TextKey();
    std::string str;
    int font_style;
    int style_i;
    int style_sub_i;
    bool operator()(TextKey const &, TextKey const &) const;
  };
  struct TextTexture {
    Rect r;
    SDL_Texture *tex;
    uint64_t t_last;
  };

  class Window {
    public:
      enum TextStyle {
        TEXT_NORMAL,
        TEXT_BOLD
      };

      Window(char const *, int, int);
      ~Window();

      bool DoClose() const;
      void ProcessEvent(SDL_Event const &);

      void Begin();
      void End();
      Pos GetSize();
      int Newline();
      void Pop();
      void Push(Rect const &);
      void Rewind();

      bool Button(char const *);
      void Checkbox(char const *, CheckboxState *);
      void Panel(void);
      void Text(char const *, ...);
      enum InputStatus TextInput(TextInputState *);

      template <typename T> void PlotHist1(Plot const *, double, double,
          std::vector<T> const &, size_t);
      template <typename T> void PlotHist2(Plot  *, size_t, Point const &,
          Point const &, std::vector<T> const &, size_t, size_t,
          std::vector<uint8_t> &);
      void PlotLines(Plot const *, std::vector<Point> const &);
      void PlotText(Plot const *, char const *, Point const &, TextAlign,
          bool, bool);

      void Render();

    private:
      Window(Window const &);
      Window &operator=(Window const &);

      void LevelAdvance(Pos const &);
      Level &LevelGet();
      void LevelPop();
      void LevelPush(Rect const &);

      bool ContainsLocal(Rect const &, Pos const &);
      bool ContainsWorld(Rect const &, Pos const &);

      void DrawLineClipped(Pos const &, Pos const &);

      bool InputPointerButtonDown(Event const &, Rect const &);

      void RenderBox(Rect const &, size_t, size_t);
      void RenderColor(SDL_Color);
      void RenderLine(Pos const &, Pos const &);
      void RenderLineDashed(Pos const &, Pos const &, double, double);
      void RenderRect(Rect const &, bool);
      void RenderTexture(SDL_Texture *, Rect const &);
      void RenderCross(Pos const &, int);
      void RenderText(char const *, TextStyle, int, Pos const &);
      Pos RenderTextMeasure(char const *, TextStyle);
      void RenderTransparent(bool);

      SDL_Window *m_window;
      uint32_t m_window_id;
      SDL_Renderer *m_renderer;
      SDL_Texture *m_checkmark_tex;
      Pos m_wsize;
      Pos m_rsize;
      std::list<Event> m_event_list;
      Pos m_pointer;
      std::list<Level> m_level_stack;
      std::map<TextKey, TextTexture, TextKey> m_text_tex_map;
      bool m_do_close;
      std::list<SDL_Texture *> m_tex_destroy_list;

      friend class Plot;
  };

}

#endif
