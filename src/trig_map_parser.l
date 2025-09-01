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
#include <trig_map_parser.tab.h>

#define YY_USER_ACTION yytmlloc_update();

void yytmerror(char const *);

void
yytmlloc_update()
{
	yytmlloc.first_line = yytmlloc.last_line;
	yytmlloc.first_column = yytmlloc.last_column;
	for (size_t i = 0; yytmtext[i] != '\0'; ++i) {
		if ('\n' == yytmtext[i]) {
			++yytmlloc.last_line;
			yytmlloc.last_column = 1;
		} else {
			++yytmlloc.last_column;
		}
	}
}

%}

%option noyywrap
%option prefix="yytm"

%%

#.*    ;
\/\/.* ;

[-+]?[0-9]+ {
	char *end;
	yytmlval.u32 = strtol(yytmtext, &end, 10);
	if (end == yytmtext) {
		yytmerror("Invalid integer.");
	}
	return TK_INTEGER;
}
[_A-Za-z][_A-Za-z0-9]* {
	yytmlval.str = strdup(yytmtext);
	return TK_IDENT;
}

[:=] return *yytmtext;

[ \t\n] ;

. yytmerror("Unexpected symbol.");

%%
