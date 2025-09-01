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

%{
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <trig_map.hpp>
#include <trig_map_parser.tab.h>
TrigMap *yytm_trig_map;
char const *yytmpath;
void
yytmerror(char const *s)
{
	fprintf(stderr, "%s:%d:%d: %s\n",
	    yytmpath, yytmlloc.first_line, yytmlloc.first_column, s);
	abort();
}
int yytmlex(void);
%}

%locations

%union {
	uint32_t u32;
	char *str;
}

%token <u32> TK_INTEGER
%token <str> TK_IDENT

%%

map_list
	: map
	| map_list map

map
	: TK_IDENT ':' TK_INTEGER '=' TK_INTEGER {
		yytm_trig_map->AddMapping($1, $3, $5);
		free($1);
	}

%%
