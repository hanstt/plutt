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

#include <ttf.hpp>
#include <iostream>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

#define FT_CALL(func, args) do { \
  auto ret_ = func args; \
  if (FT_Err_Ok != ret_) { \
    std::cerr << #func << ": Failed: " << ft_error(ret_) << '\n'; \
    throw std::runtime_error(__func__); \
  } \
} while (0)

namespace {

  char const *ft_error(int a_ret)
  {
#undef FTERRORS_H_
#define FT_ERROR_START_LIST switch (a_ret) {
#define FT_ERRORDEF(e, v, s) case v: return s;
#define FT_ERROR_END_LIST default: return "Unknown"; }
#include <freetype/fterrors.h>
  }

  FT_Library g_lib;

}

struct Font {
  FT_Face face;
  int size;
  bool is_bold;
};
struct GlyphKey {
  Font *font;
  int size;
  unsigned index;
  bool operator()(GlyphKey const &a_l, GlyphKey const &a_r) const {
#define CMP(field) \
    if (a_l.field < a_r.field) return true; \
    if (a_l.field > a_r.field) return false
    CMP(font);
    CMP(size);
    return a_l.index < a_r.index;
  }
};
struct GlyphValue {
  std::vector<uint8_t> bmap;
  int x;
  int y;
  unsigned w;
  unsigned h;
  int advance_x;
  time_t t_last;
};
std::map<GlyphKey, GlyphValue, GlyphKey> g_glyph_map;

Font *FontLoad(char const *a_path)
{
  auto f = new Font;
  // Don't use FT_CALL, we don't want to fail with fireworks.
  auto ret = FT_New_Face(g_lib, a_path, 0, &f->face);
  if (FT_Err_Ok != ret) {
    delete f;
    return nullptr;
  }
  f->size = 0;
  return f;
}

GlyphValue *GetRenderedGlyph(Font *a_f, int a_size, unsigned a_index)
{
  // Look in glyph cache.
  GlyphKey key;
  key.font = a_f;
  key.size = a_size;
  key.index = a_index;
  auto it = g_glyph_map.find(key);
  if (g_glyph_map.end() == it) {
    // Have Freetype make the glyph bitmap.
    FT_CALL(FT_Load_Glyph, (a_f->face, (unsigned)a_index, FT_LOAD_DEFAULT));
    FT_CALL(FT_Render_Glyph, (a_f->face->glyph, FT_RENDER_MODE_LIGHT));
    auto slot = a_f->face->glyph;
    auto bmap = slot->bitmap;
    if (FT_PIXEL_MODE_GRAY != bmap.pixel_mode) {
      std::cerr << "Font bitmap not 8-bit grayscale!\n";
      throw std::runtime_error(__func__);
    }
    auto ret = g_glyph_map.insert(std::make_pair(key, GlyphValue()));
    it = ret.first;
    auto &value = it->second;
    // Render it to our own bitmap.
    value.x = slot->bitmap_left;
    value.y = -slot->bitmap_top;
    value.w = bmap.width;
    value.h = bmap.rows;
    value.bmap.resize(value.w * value.h);
    size_t ofs = 0;
    uint8_t const *p = bmap.buffer;
    for (unsigned i = 0; i < value.h; ++i) {
      for (unsigned j = 0; j < value.w; ++j) {
        value.bmap.at(ofs) = (uint8_t)(256 * p[j] / bmap.num_grays);
        ++ofs;
      }
      p += bmap.pitch;
    }
    value.advance_x = (int)(slot->advance.x >> 6);
  }
  auto &value = it->second;
  value.t_last = time(nullptr);
  return &value;
}

int FontGetHeight(Font *a_f)
{
  return a_f->size;
}

FontSize FontMeasure(Font *a_f, char const *a_str)
{
  int boldness = a_f->is_bold ? 1 : 0;
  int x1 = 0;
  int x2 = 0;
  int x = 0;
  auto has_kerning = FT_HAS_KERNING(a_f->face);
  unsigned index_prev = 0;
  // TODO: utf-8.
  for (char const *p = a_str; *p; ++p) {
    auto index_curr = FT_Get_Char_Index(a_f->face, (unsigned)*p);
    // TODO: index == 0.
    if (has_kerning && index_prev && index_curr) {
      FT_Vector delta;
      FT_Get_Kerning(a_f->face, index_prev, index_curr, FT_KERNING_DEFAULT,
          &delta);
      x += delta.x >> 6;
    }
    auto glyph = GetRenderedGlyph(a_f, a_f->size, index_curr);
    x1 = std::min(x1, x + glyph->x);
    x2 = std::max(x2, x + glyph->x + (int)glyph->w + boldness);
    x += glyph->advance_x;
    index_prev = index_curr;
  }
  assert(x1 <= x2);
  auto const &metrics = a_f->face->size->metrics;
  FontSize s;
  s.x = x1;
  s.y = 0;
  s.w = (unsigned)(x2 - x1);
  s.h = (unsigned)(metrics.ascender - metrics.descender) >> 6;
  return s;
}

FontPrinted FontPrintf(Font *a_f, char const *a_str, uint8_t a_r, uint8_t a_g,
    uint8_t a_b)
{
  unsigned boldness = a_f->is_bold ? 1 : 0;
  auto ascender = a_f->face->size->metrics.ascender >> 6;
  FontPrinted fp;
  // TODO: This is a waste...
  fp.size = FontMeasure(a_f, a_str);
  fp.bmap.resize(4 * fp.size.w * fp.size.h);
  int x = 0;
  auto has_kerning = FT_HAS_KERNING(a_f->face);
  unsigned index_prev = 0;
  // TODO: utf-8.
  for (char const *p = a_str; *p; ++p) {
    auto index_curr = FT_Get_Char_Index(a_f->face, (unsigned)*p);
    // TODO: index == 0.
    if (has_kerning && index_prev && index_curr) {
      FT_Vector delta;
      FT_Get_Kerning(a_f->face, index_prev, index_curr, FT_KERNING_DEFAULT,
          &delta);
      x += delta.x >> 6;
    }
    auto glyph = GetRenderedGlyph(a_f, a_f->size, index_curr);
    auto w = glyph->w + boldness;
    uint8_t const *srcp = glyph->bmap.data();
    for (unsigned i = 0; i < glyph->h; ++i) {
      auto yy = glyph->y + (int)i + ascender;
      for (unsigned j = 0; j < w; ++j) {
        auto xx = x + glyph->x + (int)j - fp.size.x;
        auto dst_ofs = 4 * (size_t)(yy * (int)fp.size.w + xx);
        unsigned v;
        if (!a_f->is_bold) {
          v = *srcp++;
        } else if (j == 0) {
          v = *srcp++;
        } else if (j + 1 == w) {
          v = srcp[-1];
        } else {
          v = srcp[-1] + srcp[0];
          v = std::min(v, 255U);
          ++srcp;
        }
        fp.bmap.at(dst_ofs + 0) = (uint8_t)v;
        fp.bmap.at(dst_ofs + 1) = a_r;
        fp.bmap.at(dst_ofs + 2) = a_g;
        fp.bmap.at(dst_ofs + 3) = a_b;
      }
    }
    x += glyph->advance_x;
  }
  return fp;
}

void FontSetBold(Font *a_f, bool a_yes)
{
  a_f->is_bold = a_yes;
}

void FontSetSize(Font *a_f, int a_size)
{
  FT_CALL(FT_Set_Pixel_Sizes, (a_f->face, 0, (unsigned)a_size));
  a_f->size = a_size;
}

void FontUnload(Font **a_f)
{
  auto f = *a_f;
  if (f) {
    FT_CALL(FT_Done_Face, (f->face));
    delete f;
    *a_f = nullptr;
  }
}

void TtfSetup()
{
  FT_CALL(FT_Init_FreeType, (&g_lib));
}

void TtfShutdown()
{
  FT_CALL(FT_Done_FreeType, (g_lib));
}
