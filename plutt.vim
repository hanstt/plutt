" plutt, a scriptable monitor for experimental data.
"
" Copyright (C) 2023-2025
" Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
"
" This library is free software; you can redistribute it and/or
" modify it under the terms of the GNU Lesser General Public
" License as published by the Free Software Foundation; either
" version 2.1 of the License, or (at your option) any later version.
"
" This library is distributed in the hope that it will be useful,
" but WITHOUT ANY WARRANTY; without even the implied warranty of
" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
" Lesser General Public License for more details.
"
" You should have received a copy of the GNU Lesser General Public
" License along with this library; if not, write to the Free Software
" Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
" MA  02110-1301  USA

" Vim syntax file
" Language:	    plutt
" Maintainer:	    Hans Toernqvist <hans.tornqvist@gmail.com>
" Latest Revision:  2023-01-25

if exists("b:current_syntax")
  finish
endif

syn match pluttComment "#.*$"
syn match pluttComment "\/\/.*$"
syn match pluttNumber "\<\d\+"
syn match pluttString "\"[^\"]*\""

syn keyword pluttFunctions annular appearance binsx binsy bitfield clock_match cluster coarse_fine colormap contoured ctdc cut drop_counts drop_stats filled filter_range fit hist hist2d length logy logz match_index match_value max mean_arith mean_geom merge min page pedestal select_index signal sub_mod tamex3 tot tpat transformx transformy trig_map ui_rate vftx2 zero_suppress

hi def link pluttComment Comment
hi def link pluttFunctions Type
hi def link pluttNumber Constant
hi def link pluttString Constant

let b:current_syntax = "plutt"
