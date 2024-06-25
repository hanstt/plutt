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
struct Node;
struct NodeValue;
struct NodeVector;
#include <cstdint>
#include <config_parser.hpp>
#include <config_parser.tab.h>

#define YY_USER_ACTION yycplloc_update();

void yycperror(char const *);

void
yycplloc_update()
{
	yycplloc.first_line = yycplloc.last_line;
	yycplloc.first_column = yycplloc.last_column;
	for (size_t i = 0; yycptext[i] != '\0'; ++i) {
		if ('\n' == yycptext[i]) {
			++yycplloc.last_line;
			yycplloc.last_column = 1;
		} else {
			++yycplloc.last_column;
		}
	}
}

%}

%option noyywrap
%option prefix="yycp"

%%

abs                    return TK_ABS;
acos                   return TK_ACOS;
asin                   return TK_ASIN;
atan                   return TK_ATAN;
appearance             return TK_APPEARANCE;
binsx                  return TK_BINSX;
binsy                  return TK_BINSY;
bitfield               return TK_BITFIELD;
clock_match            return TK_CLOCK_MATCH;
cluster                return TK_CLUSTER;
coarse_fine            return TK_COARSE_FINE;
colormap               return TK_COLORMAP;
cos                    return TK_COS;
ctdc                   return TK_CTDC;
cut                    return TK_CUT;
drop_counts            return TK_DROP_COUNTS;
drop_stats             return TK_DROP_STATS;
exp                    return TK_EXP;
filter_range           return TK_FILTER_RANGE;
fit                    return TK_FIT;
hist                   return TK_HIST;
hist2d                 return TK_HIST2D;
length                 return TK_LENGTH;
log                    return TK_LOG;
logy                   return TK_LOGY;
logz                   return TK_LOGZ;
match_index            return TK_MATCH_INDEX;
match_value            return TK_MATCH_VALUE;
max                    return TK_MAX;
mean_arith             return TK_MEAN_ARITH;
mean_geom              return TK_MEAN_GEOM;
min                    return TK_MIN;
page                   return TK_PAGE;
pedestal               return TK_PEDESTAL;
pow                    return TK_POW;
s                      return TK_S;
select_index           return TK_SELECT_INDEX;
sin                    return TK_SIN;
sqrt                   return TK_SQRT;
sub_mod                return TK_SUB_MOD;
tamex3                 return TK_TAMEX3;
tan                    return TK_TAN;
tot                    return TK_TOT;
tpat                   return TK_TPAT;
transformx             return TK_TRANSFORMX;
transformy             return TK_TRANSFORMY;
trig_map               return TK_TRIG_MAP;
ui_rate                return TK_UI_RATE;
vftx2                  return TK_VFTX2;
zero_suppress          return TK_ZERO_SUPPRESS;

#.*    ;
\/\/.* ;

== return TK_OP_EQ;
\< return TK_OP_LESS;
\<= return TK_OP_LESSEQ;
\> return TK_OP_GREATER;
\>= return TK_OP_GREATEREQ;
-- return TK_OP_DDASH;

\"[^\"\n]*\" {
	char *p;
	yycplval.str = strdup(yycptext + 1);
	p = strrchr(yycplval.str, '"');
	if (!p) {
		yycperror("Internal error, missing '\"'!");
	}
	*p = '\0';
	return TK_STRING;
}
(([0-9]*\.[0-9]+|[0-9]+\.)([eE][-+]?[0-9]+)?|[0-9]+[eE][-+]?[0-9]+) {
	char *end;
	yycplval.c.is_i64 = false;
	yycplval.c.dbl = strtod(yycptext, &end);
	if (end == yycptext) {
		yycperror("Invalid double.");
	}
	return TK_DOUBLE;
}
[0-9]+ {
	char *end;
	yycplval.c.is_i64 = true;
	yycplval.c.i64 = strtoll(yycptext, &end, 10);
	if (end == yycptext) {
		yycperror("Invalid integer.");
	}
	return TK_INTEGER;
}
[_A-Za-z][_A-Za-z0-9]* {
	yycplval.str = strdup(yycptext);
	return TK_IDENT;
}

[\+\-*\/(),\.:=\[\]] return *yycptext;

[ \t\n] ;

. yycperror("Unexpected symbol.");

%%
