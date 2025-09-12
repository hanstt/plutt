# plutt, a scriptable monitor for experimental data.
#
# Copyright (C) 2023-2025
# Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
# Håkan T Johansson <f96hajo@chalmers.se>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301  USA

.SECONDARY:

include gmake/build_dir.mk

ROOT_CONFIG:=root-config
SDL2_CONFIG=sdl2-config
ifeq ($(UCESB_DIR),)
UCESB_DIR:=../ucesb
endif

QUIET=@

# ccache?

ifeq ($(shell (ccache --version 2>/dev/null && echo YesBox) | grep YesBox),YesBox)
CXX:=ccache g++
$(info ccache: yes)
else
$(info ccache: no)
endif

# nlopt?

ALLOW_NLOPT=Box
ifeq ($(shell (pkg-config nlopt 2>/dev/null && echo YesBox) | grep YesBox),Yes$(ALLOW_NLOPT))
CPPFLAGS+=-DPLUTT_NLOPT=1
CXXFLAGS:=$(CXXFLAGS) $(shell pkg-config nlopt --cflags)
NLOPT_LIBS:=$(shell pkg-config nlopt --libs)
LIBS+=$(NLOPT_LIBS) $(shell echo $(filter -L%,$(NLOPT_LIBS)) | sed 's/-L/-Wl,-rpath,/')
$(info nlopt: yes)
else
$(info nlopt: no)
endif

# ROOT input?

ALLOW_ROOT=Box
ifeq ($(shell ($(ROOT_CONFIG) --version 2>/dev/null && echo YesBox) | grep YesBox),Yes$(ALLOW_ROOT))
CPPFLAGS+=-DPLUTT_ROOT=1
ROOT_CFLAGS:=$(shell $(ROOT_CONFIG) --cflags | sed 's/-I/-isystem/')
LIBS+=$(shell $(ROOT_CONFIG) --libs)
ROOT_CLING:=$(shell $(ROOT_CONFIG) --prefix)/bin/rootcling
ROOT_DICT_O:=$(BUILD_DIR)/test/test_root_dict.o
ROOT_DICT_PCM:=$(BUILD_DIR)/test_root_dict_rdict.pcm
$(info ROOT: yes)
else
$(info ROOT: no)
endif

# ROOT GUI?

ALLOW_ROOT_HTTP=Box
ifeq ($(shell $(ROOT_CONFIG) --features 2>/dev/null | grep -o http)Box,http$(ALLOW_ROOT_HTTP))
CPPFLAGS+=-DPLUTT_ROOT_HTTP=1
LIBS+=-lRHTTP
$(info ROOT_HTTP: yes)
else
$(info ROOT_HTTP: no)
endif

# SDL2?

ALLOW_SDL2=Box
ifeq ($(shell ($(SDL2_CONFIG) --version 2>/dev/null && echo YesBox) | grep YesBox),Yes$(ALLOW_SDL2))
CPPFLAGS+=-DPLUTT_SDL2=1
CXXFLAGS:=$(CXXFLAGS) \
	$(shell pkg-config freetype2 --cflags) \
	$(shell $(SDL2_CONFIG) --cflags)
LIBS:=$(LIBS) \
	$(shell pkg-config freetype2 --libs) \
	$(shell $(SDL2_CONFIG) --libs)
$(info SDL2: yes)
else
$(info SDL2: no)
endif

# UCESB?

ALLOW_UCESB=Box
ifeq ($(shell ($(UCESB_DIR)/hbook/struct_writer 2>/dev/null && echo YesBox) | grep YesBox),Yes$(ALLOW_UCESB))
CPPFLAGS+=-DPLUTT_UCESB=1 -isystem $(UCESB_DIR)/hbook
LIBS+=$(UCESB_DIR)/hbook/ext_data_clnt.o \
	$(UCESB_DIR)/hbook/ext_data_client.o
$(info UCESB: yes)
else
$(info UCESB: no)
endif

# MacOS CoreServices for file watching.

ifeq (Darwin,$(shell uname -s))
LDFLAGS:=-framework CoreServices
endif

# FreeBSD inotify for file watching.

ifeq (FreeBSD,$(shell uname -s))
LIBS+=-linotify
endif

# Bison flags.

# --version prints info to stdout if the -W flags are accepted, else stderr.
BISON_SUPPORTS_WERROR:=$(shell bison -Werror -Wcounterexamples --version 2>/dev/null)
ifneq ($(BISON_SUPPORTS_WERROR),)
BISON_FLAGS=-Werror -Wcounterexamples
# name-prefix -> api.prefix happened around the same time...
BISON_PREFIX_YYCP:=-Dapi.prefix={yycp}
BISON_PREFIX_YYTM:=-Dapi.prefix={yytm}
else
BISON_PREFIX_YYCP:=--name-prefix=yycp
BISON_PREFIX_YYTM:=--name-prefix=yytm
endif

BUILD_MODE=debug
ifeq (debug,$(BUILD_MODE))
CXXFLAGS+=-ggdb
endif
ifeq (release,$(BUILD_MODE))
CXXFLAGS+=-O3
endif

CPPFLAGS:=$(CPPFLAGS) -MMD \
	-I$(BUILD_DIR)/src -Iinclude -I.
CXXFLAGS_UNSAFE:=$(CXXFLAGS) -fPIC -std=c++11
CXXFLAGS:=$(CXXFLAGS_UNSAFE) -Wall -Wconversion -Werror -Wshadow -Wswitch-enum
LDFLAGS:=$(LDFLAGS) -fPIC

MKDIR=[ -d $(@D) ] || mkdir -p $(@D)

SRC:=$(wildcard src/*.cpp)
OBJ:=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC)) \
	$(addprefix $(BUILD_DIR)/src/,config_parser.yy.o config_parser.tab.o) \
	$(addprefix $(BUILD_DIR)/src/,trig_map_parser.yy.o trig_map_parser.tab.o)
PLUTT:=$(BUILD_DIR)/plutt

TEST_SRC:=$(wildcard test/*.cpp)
TEST_OBJ:=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_SRC)) $(ROOT_DICT_O)
TEST_OK:=$(BUILD_DIR)/test.ok
TEST:=$(BUILD_DIR)/tests

.PHONY: clean
all: $(PLUTT)

.PHONY: test
test: $(TEST)
	$(QUIET)./$<

$(PLUTT): $(OBJ) $(TEST_OK)
	@echo LD $@
	$(QUIET)$(CXX) -o $@ $(filter %.o,$^) $(LDFLAGS) $(LIBS)

$(TEST_OK): $(TEST)
	$(QUIET)./$< && touch $@

$(TEST): $(TEST_OBJ) $(filter-out %main.o,$(OBJ)) $(ROOT_DICT_PCM)
	@echo LD $@
	$(QUIET)$(CXX) -o $@ $(filter %.o,$^) $(LDFLAGS) $(LIBS)

$(BUILD_DIR)/%.o: %.cpp Makefile
	@echo O $@
	$(QUIET)$(MKDIR)
	$(QUIET)$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS)

$(BUILD_DIR)/%.o: %.c Makefile
	@echo O $@
	$(QUIET)$(MKDIR)
	$(QUIET)$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS)

$(BUILD_DIR)/%.yy.o: $(BUILD_DIR)/%.yy.c
	@echo LEXO $@
	$(QUIET)$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS_UNSAFE)
$(BUILD_DIR)/%.yy.c: %.l
	@echo LEXC $@
	$(QUIET)$(MKDIR)
	$(QUIET)flex -o $@ $<

$(BUILD_DIR)/%.tab.o: $(BUILD_DIR)/%.tab.c
	@echo TABO $@
	$(QUIET)$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS_UNSAFE)
$(BUILD_DIR)/src/config_parser.tab.c: src/config_parser.y Makefile
	@echo TABC $@
	$(QUIET)$(MKDIR)
	$(QUIET)bison $(BISON_FLAGS) $(BISON_PREFIX_YYCP) -d -o $@ $<
$(BUILD_DIR)/src/trig_map_parser.tab.c: src/trig_map_parser.y Makefile
	@echo TABC $@
	$(QUIET)$(MKDIR)
	$(QUIET)bison $(BISON_FLAGS) $(BISON_PREFIX_YYTM) -d -o $@ $<

# These cannot be generalized...
$(BUILD_DIR)/src/config_parser.yy.h: $(BUILD_DIR)/src/config_parser.yy.c
$(BUILD_DIR)/src/config_parser.yy.c: $(BUILD_DIR)/src/config_parser.tab.h
$(BUILD_DIR)/src/config_parser.tab.h: $(BUILD_DIR)/src/config_parser.tab.c
$(BUILD_DIR)/src/trig_map_parser.yy.h: $(BUILD_DIR)/src/trig_map_parser.yy.c
$(BUILD_DIR)/src/trig_map_parser.yy.c: $(BUILD_DIR)/src/trig_map_parser.tab.h
$(BUILD_DIR)/src/trig_map_parser.tab.h: $(BUILD_DIR)/src/trig_map_parser.tab.c

$(BUILD_DIR)/src/root.o: src/root.cpp Makefile
$(BUILD_DIR)/src/root_gui.o: src/root_gui.cpp Makefile
$(BUILD_DIR)/src/root_output.o: src/root_output.cpp Makefile
$(BUILD_DIR)/src/root.o \
    $(BUILD_DIR)/src/root_gui.o \
    $(BUILD_DIR)/src/root_output.o:
	@echo ROOTO $@
	$(QUIET)$(MKDIR)
	$(QUIET)$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS) $(ROOT_CFLAGS)

$(BUILD_DIR)/test/test_root.o: test/test_root.cpp Makefile
	@echo ROOTO $@
	$(QUIET)$(MKDIR)
	$(QUIET)$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS) $(ROOT_CFLAGS)

$(BUILD_DIR)/test/test_root_dict.o: $(BUILD_DIR)/test/test_root_dict.cpp
	@echo ROOTO $@
	$(QUIET)$(MKDIR)
	$(QUIET)$(CXX) -c -o $@ $< $(CPPFLAGS) $(ROOT_CFLAGS)

$(BUILD_DIR)/test/test_root_dict.cpp: test/test_root.hpp test/test_root_linkdef.hpp
	@echo CLING $@
	$(QUIET)$(MKDIR)
	$(QUIET)$(ROOT_CLING) -f -DPLUTT_ROOT=1 -Iinclude $@ $^

$(BUILD_DIR)/test/test_root_dict_rdict.pcm: $(BUILD_DIR)/test/test_root_dict.cpp
$(BUILD_DIR)/test_root_dict_rdict.pcm: $(BUILD_DIR)/test/test_root_dict_rdict.pcm
	@echo CP $@
	$(QUIET)cp $< $@

$(BUILD_DIR)/src/config.o: $(BUILD_DIR)/src/config_parser.yy.h
$(BUILD_DIR)/src/trig_map.o: $(BUILD_DIR)/src/trig_map_parser.yy.h

# Vim config file support.

VIM_SYNTAX_PATH:=$(HOME)/.vim/syntax/plutt.vim
VIM_FTDETECT_PATH:=$(HOME)/.vim/ftdetect/plutt.vim
.PHONY: vim
vim: $(VIM_SYNTAX_PATH) $(VIM_FTDETECT_PATH)
$(VIM_SYNTAX_PATH): plutt.vim Makefile
	$(MKDIR)
	cp $< $@
$(VIM_FTDETECT_PATH): Makefile
	$(MKDIR)
	echo "au BufRead,BufNewFile *.plutt set filetype=plutt" > $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

-include $(OBJ:.o=.d) $(TEST_OBJ:.o=.d)
