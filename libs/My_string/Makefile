
PROJECT   ?= My_string

SRC_DIR   := source
INC_DIR   := include
INT_DIR   := internal
TST_DIR   := tests

BUILD_DIR ?= build
OBJ_DIR   := $(BUILD_DIR)/obj
BIN_DIR   := $(BUILD_DIR)/bin
LIB_DIR   := $(BUILD_DIR)/lib

CXX       ?= g++
AR        ?= ar
ARFLAGS   ?= rcs

# debug | release
CFG ?= debug

STD   ?= -std=c++17
INCS  := -I$(INC_DIR) -I$(INT_DIR) -I../..


SAN_FLAGS := -fsanitize=address,undefined,leak -fno-omit-frame-pointer

WARN_FLAGS := \
	-Wshadow -Winit-self -Wredundant-decls \
	-Wcast-align -Wundef -Wfloat-equal -Winline \
	-Wunreachable-code -Wmissing-declarations \
	-Wmissing-include-dirs -Wswitch-enum -Wswitch-default \
	-Weffc++ -Wmain -Wextra -Wall -Wpedantic \
	-Wcast-qual -Wconversion \
	-Wctor-dtor-privacy -Wempty-body -Wformat-security \
	-Wformat=2 -Wignored-qualifiers -Wlogical-op \
	-Wno-missing-field-initializers -Wnon-virtual-dtor \
	-Woverloaded-virtual -Wpointer-arith -Wsign-promo \
	-Wstack-usage=8192 -Wstrict-aliasing \
	-Wstrict-null-sentinel -Wtype-limits \
	-Wwrite-strings -Werror=vla

DBG_DEFS := -D_DEBUG -D_EJUDGE_CLIENT_SIDE

CXXFLAGS_COMMON := $(STD) $(INCS) $(WARN_FLAGS) -MMD -MP -pipe -fexceptions

#Свои дефайны
DEFS ?=
CXXFLAGS := $(CXXFLAGS_COMMON) $(DEFS)

LDFLAGS ?=
LDLIBS  ?=

ifeq ($(CFG),release)
  CXXFLAGS += -O2 -DNDEBUG
else
  CXXFLAGS += -O0 -g $(DBG_DEFS) $(SAN_FLAGS)
  LDFLAGS  += $(SAN_FLAGS)
endif

# ================================================================================

LIB_SRC  := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*.c)
TST_SRC  := $(wildcard $(TST_DIR)/*.cpp) $(wildcard $(TST_DIR)/*.c)

LIB_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(patsubst %.c,$(OBJ_DIR)/%.o,$(LIB_SRC)))
TST_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(patsubst %.c,$(OBJ_DIR)/%.o,$(TST_SRC)))

LIB_PATH := $(LIB_DIR)/lib$(PROJECT).a
TST_EXE  := $(BIN_DIR)/$(PROJECT)_tests

DEPS     := $(LIB_OBJS:.o=.d) $(TST_OBJS:.o=.d)

# ================================================================================

.PHONY: all test lib clean help

all: test

help:
	@echo "Targets:"
	@echo "  make test        - build tests executable ($(TST_EXE))"
	@echo "  make lib         - build static library   ($(LIB_PATH))"
	@echo "  make clean"
	@echo "  CFG=debug|release "

test: $(TST_EXE)

lib: $(LIB_PATH)

$(TST_EXE): $(LIB_OBJS) $(TST_OBJS)
	@mkdir -p $(dir $@)
	@$(CXX) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(LIB_PATH): $(LIB_OBJS)
	@mkdir -p $(dir $@)
	@$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)
