LIB_NAME  ?= common

BUILD_DIR ?= build
OBJ_DIR   := $(BUILD_DIR)/obj
LIB_DIR   := $(BUILD_DIR)/lib
LIB_PATH  := $(LIB_DIR)/lib$(LIB_NAME).a

CXX     ?= g++
AR      ?= ar
ARFLAGS ?= rcs

# debug | release
CFG ?= debug

STD  ?= -std=c++20
DEFS ?=

INC_DIRS := $(shell find . -type d -name include)
INCS     := $(addprefix -I,$(INC_DIRS)) -I..

SAN_FLAGS  ?= -fsanitize=address,undefined,leak -fno-omit-frame-pointer

WARN_FLAGS ?= \
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

CXXFLAGS_COMMON := $(STD) $(INCS) $(WARN_FLAGS) -MMD -MP -pipe -fexceptions $(DEFS)
CXXFLAGS := $(CXXFLAGS_COMMON)
LDFLAGS  ?=
LDLIBS   ?=

ifeq ($(CFG),release)
  CXXFLAGS += -O2 -DNDEBUG
else
  CXXFLAGS += -O0 -g $(SAN_FLAGS)
  LDFLAGS  += $(SAN_FLAGS)
endif

# ==============================================================================

SRC := $(shell find . -type f -path './*/source/*' \( -name '*.cpp' -o -name '*.c' \))
SRC := $(patsubst ./%,%,$(SRC))

OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC)))
DEPS := $(OBJS:.o=.d)

# ==============================================================================

.PHONY: all lib clean help
all: lib

help:
	@echo "Targets:"
	@echo "  make lib        - build $(LIB_PATH)"
	@echo "  make clean"
	@echo "  CFG=debug|release "

lib: $(LIB_PATH)

$(LIB_PATH): $(OBJS)
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
