# plutt, a scriptable monitor for experimental data.
#
# Copyright (C) 2023  Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
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

ROOT_CONFIG:=root-config
ifeq ($(UCESB_DIR),)
UCESB_DIR:=../ucesb
endif

QUIET=@

# ccache?

ifeq ($(shell (ccache --version 2>/dev/null && echo Yes) | grep Yes),Yes)
CXX:=ccache g++
$(info ccache: yes)
else
$(info ccache: no)
endif

# nlopt?

ifeq ($(shell (pkg-config nlopt 2>/dev/null && echo Yes) | grep Yes),Yes)
CPPFLAGS+=-DPLUTT_NLOPT=1
CXXFLAGS:=$(CXXFLAGS) $(shell pkg-config nlopt --cflags)
LIBS+=$(shell pkg-config nlopt --libs)
$(info nlopt: yes)
else
$(info nlopt: no)
endif

# ROOT?

ifeq ($(shell ($(ROOT_CONFIG) --version 2>/dev/null && echo Yes) | grep Yes),Yes)
CPPFLAGS+=-DPLUTT_ROOT=1
ROOT_CFLAGS:=$(shell $(ROOT_CONFIG) --cflags | sed 's/-I/-isystem/')
LIBS+=$(shell $(ROOT_CONFIG) --libs)
$(info ROOT: yes)
else
$(info ROOT: no)
endif

# UCESB?

ifeq ($(shell ($(UCESB_DIR)/hbook/struct_writer 2>/dev/null && echo Yes) | grep Yes),Yes)
CPPFLAGS+=-DPLUTT_UCESB=1 -isystem $(UCESB_DIR)/hbook
LIBS+=$(UCESB_DIR)/hbook/ext_data_clnt.o \
	$(UCESB_DIR)/hbook/ext_data_client.o
$(info UCESB: yes)
else
$(info UCESB: no)
endif

BUILD_MODE=debug
ifeq (debug,$(BUILD_MODE))
CXXFLAGS+=-ggdb
endif
ifeq (release,$(BUILD_MODE))
CXXFLAGS+=-O3
endif

BASE_CFLAGS:=$(shell pkg-config freetype2 sdl2 --cflags)
LIBS+=$(shell pkg-config freetype2 sdl2 --libs)

include build_dir.mk
CPPFLAGS:=$(CPPFLAGS) -MMD \
	-I$(BUILD_DIR) -I. \
	$(BASE_CFLAGS)
CXXFLAGS_UNSAFE:=$(CXXFLAGS) -fPIC -std=c++11
CXXFLAGS:=$(CXXFLAGS_UNSAFE) -Wall -Wconversion -Weffc++ -Werror -Wshadow
LDFLAGS:=$(LDFLAGS) -fPIC
LIBS+=-ldl

MKDIR=[ -d $(@D) ] || mkdir -p $(@D)

SRC:=$(wildcard *.cpp)
OBJ:=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC)) \
	$(addprefix $(BUILD_DIR)/,config_parser.yy.o config_parser.tab.o) \
	$(addprefix $(BUILD_DIR)/,trig_map_parser.yy.o trig_map_parser.tab.o)

TEST_SRC:=$(wildcard test/*.cpp)
TEST_OBJ:=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_SRC))

.PHONY: clean
all: $(BUILD_DIR)/plutt test

.PHONY: test
test: $(BUILD_DIR)/tests
	$(QUIET)./$<

$(BUILD_DIR)/plutt: $(OBJ)
	@echo LD $@
	$(QUIET)$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BUILD_DIR)/tests: $(TEST_OBJ) $(filter-out %main.o,$(OBJ))
	@echo LD $@
	$(QUIET)$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

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
$(BUILD_DIR)/%.yy.c: %.l $(BUILD_DIR)/%.tab.c Makefile
	@echo LEXC $@
	$(QUIET)$(MKDIR)
	$(QUIET)flex -o $@ $<

$(BUILD_DIR)/%.tab.o: $(BUILD_DIR)/%.tab.c
	@echo TABO $@
	$(QUIET)$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS_UNSAFE)
$(BUILD_DIR)/%.tab.c: %.y Makefile
	@echo TABC $@
	$(QUIET)$(MKDIR)
	$(QUIET)bison -Werror -d -o $@ $<

$(BUILD_DIR)/root.o: root.cpp Makefile
	@echo ROOTO $@
	$(QUIET)$(MKDIR)
	$(QUIET)$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS) $(ROOT_CFLAGS)

$(BUILD_DIR)/config.o: $(BUILD_DIR)/config_parser.yy.c
$(BUILD_DIR)/trig_map.o: $(BUILD_DIR)/trig_map_parser.yy.c

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
