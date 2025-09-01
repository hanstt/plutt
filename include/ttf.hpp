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

#ifndef TTF_HPP
#define TTF_HPP

struct Font;
struct FontSize {
  int x;
  int y;
  unsigned w;
  unsigned h;
};
struct FontPrinted {
  FontPrinted();
  FontSize size;
  std::vector<uint8_t> bmap;
};

int FontGetHeight(Font *);
Font *FontLoad(char const *);
FontSize FontMeasure(Font *, char const *);
FontPrinted FontPrintf(Font *, char const *, uint8_t, uint8_t, uint8_t);
void FontSetBold(Font *, bool);
void FontSetSize(Font *, int);
void FontUnload(Font **);

void TtfSetup();
void TtfShutdown();

#endif
