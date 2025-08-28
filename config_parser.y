/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2025
 * Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
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
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <config.hpp>
#include <config_parser.hpp>
#include <config_parser.tab.h>
#include <cut.hpp>
#include <gui.hpp>
#include <node_bitfield.hpp>
#include <node_filter_range.hpp>
#include <node_merge.hpp>
#include <util.hpp>

#define LOC_SAVE(arg) g_config->SetLoc(arg.first_line, arg.first_column)

#define MEXPR(arg, ret, l, r, d, op) do { \
	LOC_SAVE(arg); \
	ret = g_config->AddMExpr(l, r, d, NodeMExpr::op); \
} while (0)

class Node;
class NodeValue;

char const *yycppath;
Config *g_config;
GuiCollection g_gui;
void
yycperror(char const *s)
{
	fprintf(stderr, "%s:%d:%d: %s\n",
	    yycppath, yycplloc.first_line, yycplloc.first_column, s);
	abort();
}
int yylex(void);

#define CONST_OP(l, op, r) do { \
		Constant c_; \
		if ((l).is_i64 && (r).is_i64) { \
			c_.is_i64 = true; \
			c_.i64 = (l).i64 op (r).i64; \
		} else { \
			c_.is_i64 = false; \
			c_.dbl = (l).GetDouble() op (r).GetDouble(); \
		} \
		return c_; \
	} while (0)
Constant Constant::Add(Constant const &a_c) const {
	CONST_OP(*this, +, a_c);
}
Constant Constant::Div(Constant const &a_c) const {
	CONST_OP(*this, /, a_c);
}
Constant Constant::Mul(Constant const &a_c) const {
	CONST_OP(*this, *, a_c);
}
Constant Constant::Sub(Constant const &a_c) const {
	CONST_OP(*this, -, a_c);
}
double Constant::GetDouble() const {
	return is_i64 ? i64 : dbl;
}
int64_t Constant::GetI64() const {
	if (!is_i64) {
		std::cerr << g_config->GetLocStr() << ": Must be integer!\n";
		throw std::runtime_error(__func__);
	}
	return i64;
}

CutPolygon *g_cut_poly;

NodeFilterRange::CondVec g_filter_cond_vec;
std::vector<std::string> g_filter_dst_vec;
std::vector<NodeValue *> g_filter_src_vec;

struct FitEntry {
	FitEntry(double a_x, double a_y): x(a_x), y(a_y) {}
	double x;
	double y;
};
std::vector<FitEntry> g_fit_vec;

static struct {
	double r_min;
	double r_max;
} g_annular;
static NodeValue *g_pedestal_tpat;
static uint32_t g_binsx;
static uint32_t g_binsy;
static char *g_transformx;
static char *g_transformy;
static char *g_fit;
static int g_logy, g_logz;
static int g_contour;
static struct {
	double time;
	unsigned slice_num;
} g_drop_counts = {-1.0, 1};
static double g_drop_stats = -1.0;

static void ResetDrawArgs() {
	g_binsx = 0;
	g_binsy = 0;
	free(g_transformx);
	g_transformx = nullptr;
	free(g_transformy);
	g_transformy = nullptr;
	free(g_fit);
	g_fit = nullptr;
	g_logy = 0;
	g_logz = 0;
	g_contour = 0;
	g_drop_counts.time = -1.0;
	g_drop_counts.slice_num = 1;
	g_drop_stats = -1.0;
}

#define CTDC_BITS 12
#define TAMEX3_BITS 11
#define VFTX2_BITS 11

#define CTDC_FREQ 150e6
#define TAMEX3_FREQ 200e6
#define VFTX2_FREQ  200e6
%}

%locations

%union {
	Constant c;
	uint32_t u32;
	double dbl;
	char *str;
	struct {
		int64_t first;
		int64_t last;
	} range;
	Node *node;
	NodeValue *value;
	struct BitfieldArg *bitfield;
	struct MergeArg *merge;
}

%token <c> TK_DOUBLE
%token <c> TK_INTEGER
%token <str> TK_IDENT
%token <str> TK_STRING

%token TK_ABS
%token TK_ACOS
%token TK_ANNULAR
%token TK_APPEARANCE
%token TK_ASIN
%token TK_ATAN
%token TK_BINSX
%token TK_BINSY
%token TK_BITFIELD
%token TK_CLOCK_MATCH
%token TK_CLUSTER
%token TK_COARSE_FINE
%token TK_COLORMAP
%token TK_CONTOURED
%token TK_COS
%token TK_CTDC
%token TK_CUT
%token TK_DROP_COUNTS
%token TK_DROP_STATS
%token TK_EXP
%token TK_FILLED
%token TK_FILTER_RANGE
%token TK_FIT
%token TK_HIST
%token TK_HIST2D
%token TK_LENGTH
%token TK_LOG
%token TK_LOGY
%token TK_LOGZ
%token TK_MATCH_INDEX
%token TK_MATCH_VALUE
%token TK_MAX
%token TK_MEAN_ARITH
%token TK_MEAN_GEOM
%token TK_MERGE
%token TK_MIN
%token TK_PAGE
%token TK_PEDESTAL
%token TK_POW
%token TK_S
%token TK_SELECT_INDEX
%token TK_SIGNAL
%token TK_SIN
%token TK_SQRT
%token TK_SUB_MOD
%token TK_TAMEX3
%token TK_TAN
%token TK_TOT
%token TK_TPAT
%token TK_TRANSFORMX
%token TK_TRANSFORMY
%token TK_TRIG_MAP
%token TK_UI_RATE
%token TK_VFTX2
%token TK_ZERO_SUPPRESS

%token TK_OP_EQ
%token TK_OP_LESS
%token TK_OP_LESSEQ
%token TK_OP_GREATER
%token TK_OP_GREATEREQ
%token TK_OP_DDASH

%type <value> alias
%type <value> array
%type <value> bitfield
%type <bitfield> bitfield_arg
%type <bitfield> bitfield_args
%type <u32> bitmask
%type <u32> bitmask_item
%type <dbl> clock_period
%type <u32> cmp_less
%type <value> coarse_fine
%type <c> const
%type <range> integer_range
%type <value> length
%type <value> max
%type <value> mean_arith
%type <value> mean_geom
%type <value> member
%type <value> merge
%type <merge> merge_arg
%type <merge> merge_args
%type <value> mexpr
%type <dbl> clock_range
%type <value> select_index
%type <value> signal
%type <value> sub_mod
%type <value> tot
%type <value> tpat
%type <value> trig_map
%type <dbl> unit_time
%type <value> value
%type <value> zero_suppress

%left '-' '+'
%left '*' '/'
%left ':'

%%

stmt_list
	: stmt
	| stmt_list stmt

stmt
	: assign
	| annular
	| appearance
	| clock_match
	| cluster
	| colormap
	| cut
	| filter_range
	| fit
	| hist
	| match_index
	| match_value
	| page
	| pedestal
	| ui_rate

appearance
	: TK_APPEARANCE '(' TK_STRING ')' {
		LOC_SAVE(@1);
		g_config->AppearanceSet($3);
		free($3);
	}

clock_match
	: TK_CLOCK_MATCH '(' value ',' const ')' {
		g_config->ClockMatch($3, $5.dbl);
	}

colormap
	: TK_COLORMAP '(' ')' {
		g_config->ColormapSet(nullptr);
	}
	| TK_COLORMAP '(' TK_STRING ')' {
		LOC_SAVE(@1);
		g_config->ColormapSet($3);
		free($3);
	}

cut_inline_args
	:
	| cut_inline_args1
	| cut_inline_args2
cut_inline_args1
	: cut_inline_arg1
	| cut_inline_args1 ',' cut_inline_arg1
cut_inline_arg1
	: const {
		LOC_SAVE(@1);
		g_cut_poly->AddPoint($1.GetDouble());
	}
cut_inline_args2
	: cut_inline_arg2
	| cut_inline_args2 ',' cut_inline_arg2
cut_inline_arg2
	: '(' const ',' const ')' {
		LOC_SAVE(@1);
		g_cut_poly->AddPoint($2.GetDouble(), $4.GetDouble());
	}
cut_invoc
	: TK_CUT '(' TK_STRING ')' {
		g_cut_poly = new CutPolygon($3, true);
		free($3);
	}
	| TK_CUT '(' TK_STRING ',' {
		g_cut_poly = new CutPolygon($3, false);
		free($3);
	} cut_inline_args ')'
cut
	: TK_IDENT '=' cut_invoc {
		LOC_SAVE(@3);
		auto node = g_config->AddCut(g_cut_poly);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		free($1);
		g_cut_poly = nullptr;
	}
	| TK_IDENT ',' TK_IDENT '=' cut_invoc {
		LOC_SAVE(@5);
		auto node = g_config->AddCut(g_cut_poly);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		free($1);
		free($3);
		g_cut_poly = nullptr;
	}

page
	: TK_PAGE '(' TK_STRING ')' {
		g_gui.AddPage($3);
		free($3);
	}

const
	: TK_DOUBLE { $$ = $1; }
	| TK_INTEGER { $$ = $1; }
	| const '+' const { $$ = $1.Add($3); }
	| const '-' const { $$ = $1.Sub($3); }
	| const '*' const { $$ = $1.Mul($3); }
	| const '/' const { $$ = $1.Div($3); }
	| '-' const {
		Constant zero;
		zero.is_i64 = true;
		zero.i64 = 0;
		$$ = zero.Sub($2);
	}
	| '(' const ')' { $$ = $2; }

integer_range
	: const TK_OP_DDASH const {
		LOC_SAVE(@1);
		auto l = $1.GetI64();
		auto r = $3.GetI64();
		if (l > r) {
			std::cerr << g_config->GetLocStr() <<
			    ": Integer range inverted!\n";
			throw std::runtime_error(__func__);
		}
		$$.first = l;
		$$.last = r;
	}

bitmask
	: bitmask_item             { $$ = $1; }
	| bitmask ',' bitmask_item { $$ = $1 | $3; }
bitmask_item
	: TK_INTEGER { $$ = 1 << $1.i64; }
	| TK_INTEGER TK_OP_DDASH TK_INTEGER {
		$$ = (~(uint32_t)0 << $1.i64) &
		    (~(uint32_t)0 >> (31 - $3.i64));
	}

tpat
	: TK_TPAT '(' value ',' bitmask ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddTpat($3, $5);
	}

clock_period
	: const     { $$ = $1.GetDouble(); }
	| TK_CTDC   { $$ = 1e9 / CTDC_FREQ; }
	| TK_TAMEX3 { $$ = 1e9 / TAMEX3_FREQ; }
	| TK_VFTX2  { $$ = 1e9 / VFTX2_FREQ; }
clock_range
	: const { $$ = $1.GetDouble(); }
	| TK_CTDC {
		$$ = (1 << CTDC_BITS) * 1e9 / CTDC_FREQ;
	}
	| TK_TAMEX3 {
		$$ = (1 << TAMEX3_BITS) * 1e9 / TAMEX3_FREQ;
	}
	| TK_VFTX2 {
		$$ = (1 << VFTX2_BITS) * 1e9 / VFTX2_FREQ;
	}

unit_time
	: TK_S   { $$ = 1.0; }
	| TK_MIN { $$ = 60.0; }

alias
	: TK_IDENT {
		LOC_SAVE(@1);
		$$ = g_config->AddAlias($1, NULL, 0);
		free($1);
	}

cmp_less
	: TK_OP_LESS {
		$$ = 0;
	}
	| TK_OP_LESSEQ {
		$$ = 1;
	}
filter_range_conds
	: filter_range_cond
	| filter_range_conds ',' filter_range_cond
filter_range_cond
	: const TK_OP_EQ value {
		g_filter_cond_vec.push_back(FilterRangeCond());
		auto &c = g_filter_cond_vec.back();
		c.node = $3;
		c.lower = $1.GetDouble();
		c.lower_le = 1;
		c.upper = $1.GetDouble();
		c.upper_le = 1;
	}
	| value TK_OP_EQ const {
		g_filter_cond_vec.push_back(FilterRangeCond());
		auto &c = g_filter_cond_vec.back();
		c.node = $1;
		c.lower = $3.GetDouble();
		c.lower_le = 1;
		c.upper = $3.GetDouble();
		c.upper_le = 1;
	}
	| const cmp_less value cmp_less const {
		g_filter_cond_vec.push_back(FilterRangeCond());
		auto &c = g_filter_cond_vec.back();
		c.node = $3;
		c.lower = $1.GetDouble();
		c.lower_le = $2;
		c.upper = $5.GetDouble();
		c.upper_le = $4;
	}
filter_args
	: filter_arg
	| filter_args ',' filter_arg
filter_arg
	: '(' TK_IDENT '=' value ')' {
		g_filter_dst_vec.push_back($2);
		g_filter_src_vec.push_back($4);
		free($2);
	}
filter_range
	: TK_FILTER_RANGE '(' filter_range_conds ',' filter_args ')' {
		LOC_SAVE(@1);
		auto node = g_config->AddFilterRange(
		    g_filter_cond_vec, g_filter_src_vec);
		// Assign destinations.
		unsigned i = 0;
		for (auto it = g_filter_dst_vec.begin();
		    g_filter_dst_vec.end() != it; ++it, ++i) {
			g_config->AddAlias(it->c_str(), node, i);
		}
		g_filter_cond_vec.clear();
		g_filter_dst_vec.clear();
		g_filter_src_vec.clear();
	}

fit_args
	: fit_arg
	| fit_args ',' fit_arg
fit_arg
	: '(' const ',' const ')' {
		LOC_SAVE(@1);
		g_fit_vec.push_back(FitEntry($2.GetDouble(), $4.GetDouble()));
	}
fit
	: TK_IDENT '=' TK_FIT '(' fit_args ')' {
		double k, m;
		FitLinear(
		    &g_fit_vec.at(0).x, sizeof g_fit_vec.at(0),
		    &g_fit_vec.at(0).y, sizeof g_fit_vec.at(0),
		    g_fit_vec.size(), &k, &m);
		g_config->AddFit($1, k, m);
		g_fit_vec.clear();
		free($1);
	}

signal
	: TK_SIGNAL '(' TK_IDENT ',' TK_IDENT ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddSignalUser($3, nullptr, $5);
		free($3);
		free($5);
	}
	| TK_SIGNAL '(' TK_IDENT ',' TK_IDENT ',' TK_IDENT ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddSignalUser($3, $5, $7);
		free($3);
		free($5);
		free($7);
	}

value
	: alias         { $$ = $1; }
	| array         { $$ = $1; }
	| bitfield      { $$ = $1; }
	| coarse_fine   { $$ = $1; }
	| mexpr         { $$ = $1; }
	| length        { $$ = $1; }
	| max           { $$ = $1; }
	| mean_arith    { $$ = $1; }
	| mean_geom     { $$ = $1; }
	| member        { $$ = $1; }
	| merge         { $$ = $1; }
	| select_index  { $$ = $1; }
	| signal        { $$ = $1; }
	| sub_mod       { $$ = $1; }
	| tot           { $$ = $1; }
	| tpat          { $$ = $1; }
	| trig_map      { $$ = $1; }
	| zero_suppress { $$ = $1; }

member
	: value ':' TK_IDENT {
		LOC_SAVE(@1);
		$$ = g_config->AddMember($1, $3);
		free($3);
	}

array
	: alias '[' TK_INTEGER ']' {
		LOC_SAVE(@1);
		$$ = g_config->AddArray($1, $3.GetI64());
	}
	| alias '[' TK_INTEGER ']' '[' TK_INTEGER ']' {
		LOC_SAVE(@1);
		$$ = g_config->AddArray($1, $3.GetI64(), $6.GetI64());
	}

assign
	: TK_IDENT '=' value {
		LOC_SAVE(@1);
		g_config->AddAlias($1, $3, 0);
		free($1);
	}

r_limits
	: '(' const ',' const ')' {
		LOC_SAVE(@1);
		g_annular.r_min = $2.GetDouble();
		g_annular.r_max = $4.GetDouble();
	}
annular_opts
	:
	| annular_opt_list
annular_opt_list
	: TK_LOGZ { g_logz = 1; }
	| drop_counts
	| drop_stats
annular_opt_list
	: annular_opt
	| annular_opt_list annular_opt
annular_opt
	: ',' annular_arg
annular_arg
	: TK_LOGZ { g_logz = 1; }
	| drop_counts
	| drop_stats
annular
	: TK_ANNULAR '(' TK_STRING ',' value ',' r_limits ',' value ',' const annular_opts ')' {
		LOC_SAVE(@1);
		g_config->AddAnnular($3, $5, g_annular.r_min, g_annular.r_max,
		    $9, $11.GetDouble(), g_logz, g_drop_counts.time,
		    g_drop_counts.slice_num, g_drop_stats);
		ResetDrawArgs();
		free($3);
	}

merge_args
	: merge_arg { $$ = $1; }
	| merge_args ',' merge_arg {
		/* Beware, builds the list in reverse order! */
		$3->next = $1;
		$$ = $3;
	}
merge_arg
	: alias {
		LOC_SAVE(@1);
		$$ = new MergeArg(g_config->GetLocStr(), $1);
	}
merge
	: TK_MERGE '(' merge_args ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMerge($3);
	}

mexpr
	: '(' value ')' { $$ = $2; }
	| value '+' const { MEXPR(@1, $$, $1, nullptr, $3.GetDouble(), ADD); }
	| const '+' value { MEXPR(@1, $$, nullptr, $3, $1.GetDouble(), ADD); }
	| value '+' value { MEXPR(@1, $$, $1,      $3,            0.0, ADD); }
	| value '-' const { MEXPR(@1, $$, $1, nullptr, $3.GetDouble(), SUB); }
	| const '-' value { MEXPR(@1, $$, nullptr, $3, $1.GetDouble(), SUB); }
	| value '-' value { MEXPR(@1, $$, $1,      $3,            0.0, SUB); }
	| value '*' const { MEXPR(@1, $$, $1, nullptr, $3.GetDouble(), MUL); }
	| const '*' value { MEXPR(@1, $$, nullptr, $3, $1.GetDouble(), MUL); }
	| value '*' value { MEXPR(@1, $$, $1,      $3,            0.0, MUL); }
	| value '/' const { MEXPR(@1, $$, $1, nullptr, $3.GetDouble(), DIV); }
	| const '/' value { MEXPR(@1, $$, nullptr, $3, $1.GetDouble(), DIV); }
	| value '/' value { MEXPR(@1, $$, $1,      $3,            0.0, DIV); }
	| '-' value       { MEXPR(@1, $$, nullptr, $2,            0.0, SUB); }
	| TK_COS  '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0,  COS); }
	| TK_SIN  '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0,  SIN); }
	| TK_TAN  '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0,  TAN); }
	| TK_ACOS '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0, ACOS); }
	| TK_ASIN '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0, ASIN); }
	| TK_ATAN '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0, ATAN); }
	| TK_SQRT '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0, SQRT); }
	| TK_EXP  '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0,  EXP); }
	| TK_LOG  '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0,  LOG); }
	| TK_LOG  '(' const ',' value ')' {
		MEXPR(@1, $$, nullptr, $5, $3.GetDouble(),  LOG);
	}
	| TK_ABS  '(' value ')' { MEXPR(@1, $$, $3, nullptr, 0.0,  ABS); }
	| TK_POW  '(' value ',' const ')' {
		MEXPR(@1, $$, $3, nullptr, $5.GetDouble(), POW);
	}
	| TK_POW  '(' const ',' value ')' {
		MEXPR(@1, $$, nullptr, $5, $3.GetDouble(), POW);
	}

bitfield_args
	: bitfield_arg                   { $$ = $1; }
	| bitfield_args ',' bitfield_arg {
		/* Beware, builds the list in reverse order! */
		$3->next = $1;
		$$ = $3;
	}
bitfield_arg
	: alias ',' const {
		LOC_SAVE(@1);
		$$ = new BitfieldArg(g_config->GetLocStr(), $1, $3.GetI64());
	}
bitfield
	: TK_BITFIELD '(' bitfield_args ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddBitfield($3);
	}
cluster
	: TK_IDENT '=' TK_CLUSTER '(' value ')' {
		LOC_SAVE(@3);
		auto node = g_config->AddCluster($5);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		free($1);
	}
	| TK_IDENT ',' TK_IDENT '=' TK_CLUSTER '(' value ')' {
		LOC_SAVE(@5);
		auto node = g_config->AddCluster($7);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		free($1);
		free($3);
	}

hist_cut
	: cut_invoc {
		LOC_SAVE(@1);
		g_config->HistCutAdd(g_cut_poly);
		g_cut_poly = nullptr;
	}
drop_counts
	: TK_DROP_COUNTS '(' const unit_time ')' {
		LOC_SAVE(@1);
		g_drop_counts.time = $3.GetDouble() * $4;
		g_drop_counts.slice_num = 2;
	}
	| TK_DROP_COUNTS '(' const unit_time ',' const ')' {
		LOC_SAVE(@1);
		g_drop_counts.time = $3.GetDouble() * $4;
		g_drop_counts.slice_num = $6.GetI64();
		if (g_drop_counts.slice_num < 1) {
			std::cerr << g_config->GetLocStr() <<
			    ": Must have >= 1 drop-slice!\n";
			throw std::runtime_error(__func__);
		}
		if (g_drop_counts.slice_num > 5) {
			std::cerr << g_config->GetLocStr() <<
			    ": Must have <= 5 drop-slices!\n";
			throw std::runtime_error(__func__);
		}
	}
drop_stats
	: TK_DROP_STATS '=' const unit_time {
		LOC_SAVE(@1);
		g_drop_stats = $3.GetDouble() * $4;
	}

hist_opts
	:
	| hist_opt_list
hist_opt_list
	: hist_opt
	| hist_opt_list hist_opt
hist_opt
	: ',' hist_arg
hist_arg
	: TK_BINSX '=' const { g_binsx = $3.GetI64(); }
	| TK_CONTOURED { g_contour = 1; }
	| TK_FILLED { g_contour = 0; }
	| TK_FIT '=' TK_STRING { g_fit = $3; }
	| TK_LOGY { g_logy = 1; }
	| TK_TRANSFORMX '=' TK_IDENT { g_transformx = $3; }
	| hist_cut
	| drop_counts
	| drop_stats
hist2d_opts
	:
	| hist2d_opt_list
hist2d_opt_list
	: hist2d_opt
	| hist2d_opt_list hist2d_opt
hist2d_opt
	: ',' hist2d_arg
hist2d_arg
	: TK_BINSX '=' const { g_binsx = $3.GetI64(); }
	| TK_BINSY '=' const { g_binsy = $3.GetI64(); }
	| TK_LOGZ { g_logz = 1; }
	| TK_TRANSFORMX '=' TK_IDENT { g_transformx = $3; }
	| TK_TRANSFORMY '=' TK_IDENT { g_transformy = $3; }
	| hist_cut
	| drop_counts
	| drop_stats

coarse_fine
	: TK_COARSE_FINE '(' value ',' value ',' clock_period ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddCoarseFine($3, $5, $7);
	}
hist
	: TK_HIST '(' TK_STRING ',' value hist_opts ')' {
		LOC_SAVE(@1);
		g_config->AddHist1($3, $5, g_binsx, g_transformx, g_fit,
		    g_logy, g_contour, g_drop_counts.time,
		    g_drop_counts.slice_num, g_drop_stats);
		ResetDrawArgs();
		free($3);
	}
	| TK_HIST2D '(' TK_STRING ',' value hist2d_opts ')' {
		LOC_SAVE(@1);
		g_config->AddHist2($3, $5, nullptr, g_binsy, g_binsx,
		    g_transformy, g_transformx, g_fit, g_logz,
		    g_drop_counts.time, g_drop_counts.slice_num,
		    g_drop_stats);
		ResetDrawArgs();
		free($3);
	}
	| TK_HIST2D '(' TK_STRING ',' value ',' value hist2d_opts ')' {
		LOC_SAVE(@1);
		g_config->AddHist2($3, $5, $7, g_binsy, g_binsx,
		    g_transformy, g_transformx, g_fit, g_logz,
		    g_drop_counts.time, g_drop_counts.slice_num,
		    g_drop_stats);
		ResetDrawArgs();
		free($3);
	}
match_index
	: TK_IDENT ',' TK_IDENT '=' TK_MATCH_INDEX '(' value ',' value ')' {
		LOC_SAVE(@5);
		auto node = g_config->AddMatchId($7, $9);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		free($1);
		free($3);
	}
match_value
	: TK_IDENT ',' TK_IDENT '=' TK_MATCH_VALUE '(' value ',' value ','
	    const ')' {
		LOC_SAVE(@5);
		auto node = g_config->AddMatchValue($7, $9, $11.GetDouble());
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		free($1);
		free($3);
	}
length
	: TK_LENGTH '(' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddLength($3);
	}
max
	: TK_MAX '(' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMax($3);
	}
mean_arith
	: TK_MEAN_ARITH '(' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMeanArith($3, NULL);
	}
	| TK_MEAN_ARITH '(' value ',' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMeanArith($3, $5);
	}
mean_geom
	: TK_MEAN_GEOM '(' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMeanGeom($3, NULL);
	}
	| TK_MEAN_GEOM '(' value ',' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddMeanGeom($3, $5);
	}
pedestal_opts
	:
	| pedestal_opt_list
pedestal_opt_list
	: pedestal_opt
	| pedestal_opt_list pedestal_opt
pedestal_opt: ',' pedestal_arg
pedestal_arg
	: TK_TPAT '=' value {
		g_pedestal_tpat = $3;
	}
pedestal
	: TK_IDENT ',' TK_IDENT '=' TK_PEDESTAL
	    '(' value ',' const pedestal_opts ')' {
		LOC_SAVE(@5);
		auto node = g_config->AddPedestal($7, $9.GetDouble(),
		    g_pedestal_tpat);
		LOC_SAVE(@1);
		g_config->AddAlias($1, node, 0);
		LOC_SAVE(@3);
		g_config->AddAlias($3, node, 1);
		g_pedestal_tpat = nullptr;
		free($1);
		free($3);
	}
ui_rate
	: TK_UI_RATE '(' const ')' {
		LOC_SAVE(@1);
		g_config->UIRateSet($3.GetDouble());
	}
select_index
	: TK_SELECT_INDEX '(' value ',' const ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddSelectId($3, $5.GetI64(), $5.GetI64());
	}
	| TK_SELECT_INDEX '(' value ',' integer_range ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddSelectId($3, $5.first, $5.last);
	}
sub_mod
	: TK_SUB_MOD '(' value ',' value ',' clock_range ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddSubMod($3, $5, $7);
	}
tot
	: TK_TOT '(' value ',' value ',' clock_range ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddTot($3, $5, $7);
	}
trig_map
	: TK_TRIG_MAP '(' TK_STRING ',' TK_STRING ','
	                  value ',' value ',' clock_range ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddTrigMap($3, $5, $7, $9, $11);
		free($3);
		free($5);
	}
zero_suppress
	: TK_ZERO_SUPPRESS '(' value ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddZeroSuppress($3, 1e-15);
	}
	| TK_ZERO_SUPPRESS '(' value ',' const ')' {
		LOC_SAVE(@1);
		$$ = g_config->AddZeroSuppress($3, $5.GetDouble());
	}

%%
