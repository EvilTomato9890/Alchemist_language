# Root Makefile for alchemist_language project

# debug | release
CFG ?= debug

CXX ?= g++
AR  ?= ar
ARFLAGS ?= rcs

STD ?= -std=c++20

# Build directories
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
BIN_DIR   := $(BUILD_DIR)/bin

# Include directories
INCS := -I. -Icommon -Icommon/file_operations/include -Icommon/keywords/include \
        -Icommon/logger/include -Icommon/asserts/include -Icommon/console_colors/include \
        -Ilibs/AST/include -Ilibs/Vector/include -Ilibs/My_string/include \
        -Ilibs/Unordered_map/include -Ilibs/Stack/include \
        -Iproject/frontend -Iproject/frontend/error_logger/include \
        -Iproject/frontend/lexer/include -Iproject/frontend/parser/include \
        -Iproject/backend/include -Iproject/midend/include

# Compiler flags
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

DBG_DEFS := -D_DEBUG -D_EJUDGE_CLIENT_SIDE

DEFS ?= -DTREE_VERIFY_DEBUG -DSTACK_VERIFY_DEBUG -DLOGGER_ALL

CXXFLAGS_COMMON := $(STD) $(INCS) $(WARN_FLAGS) -MMD -MP -pipe -fexceptions
CXXFLAGS := $(CXXFLAGS_COMMON) $(DEFS)

LDFLAGS ?=
LDLIBS  ?=

ifeq ($(CFG),release)
  CXXFLAGS += -O2 -DNDEBUG
else
  CXXFLAGS += -O0 -g $(DBG_DEFS) $(SAN_FLAGS)
  LDFLAGS  += $(SAN_FLAGS)
endif

# Source files
MAIN_SRC := main.cpp
MAIN_OBJ := $(OBJ_DIR)/main.o
MAIN_DEPS := $(MAIN_OBJ:.o=.d)

# Executable
EXE := $(BIN_DIR)/alchemist

# ================================================================================

# Common library
COMMON_LIB_DIR := common
COMMON_LIB      := $(COMMON_LIB_DIR)/build/lib/libcommon.a

# Libraries from libs/
LIBS_LIB_DIR_1 := libs/AST
LIBS_LIB_1      := $(LIBS_LIB_DIR_1)/build/lib/libAST.a

LIBS_LIB_DIR_2 := libs/Vector
LIBS_LIB_2      := $(LIBS_LIB_DIR_2)/build/lib/libVector.a

LIBS_LIB_DIR_3 := libs/My_string
LIBS_LIB_3      := $(LIBS_LIB_DIR_3)/build/lib/libMy_string.a

LIBS_LIB_DIR_4 := libs/Unordered_map
LIBS_LIB_4      := $(LIBS_LIB_DIR_4)/build/lib/libUnordered_map.a

LIBS_LIB_DIR_5 := libs/Stack
LIBS_LIB_5      := $(LIBS_LIB_DIR_5)/build/lib/libStack.a

LIBS_LIBS := $(LIBS_LIB_1) $(LIBS_LIB_2) $(LIBS_LIB_3) $(LIBS_LIB_4) $(LIBS_LIB_5)

# Project libraries
PROJECT_LIB_DIR_1 := project/frontend/error_logger
PROJECT_LIB_1      := $(PROJECT_LIB_DIR_1)/build/lib/liberror_logger.a

PROJECT_LIB_DIR_2 := project/frontend/lexer
PROJECT_LIB_2      := $(PROJECT_LIB_DIR_2)/build/lib/liblexer.a

PROJECT_LIB_DIR_3 := project/frontend/parser
PROJECT_LIB_3      := $(PROJECT_LIB_DIR_3)/build/lib/libparser.a

PROJECT_LIB_DIR_4 := project/midend
PROJECT_LIB_4      := $(PROJECT_LIB_DIR_4)/build/lib/libmidend.a

PROJECT_LIB_DIR_5 := project/backend
PROJECT_LIB_5      := $(PROJECT_LIB_DIR_5)/build/lib/libbackend.a

PROJECT_LIBS := $(PROJECT_LIB_1) $(PROJECT_LIB_2) $(PROJECT_LIB_3) $(PROJECT_LIB_4) $(PROJECT_LIB_5)

# All libraries
ALL_LIBS := $(COMMON_LIB) $(LIBS_LIBS) $(PROJECT_LIBS)

# Project headers
PROJECT_HEADERS := \
	project/backend/include/backend.h \
	project/frontend/error_logger/include/frontend_err_logger.h \
	project/midend/include/tree_optimize.h \
	project/frontend/lexer/include/lexer_tokenizer.h \
	project/frontend/parser/include/frontend_parser.h

PROJECT_HEADERS_FILE := project_headers.h

# ================================================================================

.PHONY: all clean help project_headers project_libs libs all_libs exe

all: all_libs project_headers exe

help:
	@echo "Targets:"
	@echo "  make all              - build all libraries, headers and executable ($(EXE))"
	@echo "  make exe               - build executable from main.cpp ($(EXE))"
	@echo "  make all_libs         - build all libraries (common, libs/, project/)"
	@echo "  make project_libs     - build only project libraries"
	@echo "  make project_headers  - build file with all project headers ($(PROJECT_HEADERS_FILE))"
	@echo "  make clean            - remove generated files"
	@echo "  CFG=debug|release     - build configuration (default: debug)"

$(PROJECT_HEADERS_FILE): $(PROJECT_HEADERS)
	@echo "// Auto-generated file with all project headers" > $@
	@echo "// DO NOT EDIT MANUALLY" >> $@
	@echo "" >> $@
	@for header in $(PROJECT_HEADERS); do \
		echo "#include \"$$header\"" >> $@; \
	done

project_headers: $(PROJECT_HEADERS_FILE)

# Build all libraries
all_libs: $(ALL_LIBS)

# Build only project libraries
project_libs: $(PROJECT_LIBS)

# Common library
$(COMMON_LIB):
	@$(MAKE) -C "$(COMMON_LIB_DIR)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

# Libraries from libs/
$(LIBS_LIB_1):
	@$(MAKE) -C "$(LIBS_LIB_DIR_1)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(LIBS_LIB_2):
	@$(MAKE) -C "$(LIBS_LIB_DIR_2)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(LIBS_LIB_3):
	@$(MAKE) -C "$(LIBS_LIB_DIR_3)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(LIBS_LIB_4):
	@$(MAKE) -C "$(LIBS_LIB_DIR_4)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(LIBS_LIB_5):
	@$(MAKE) -C "$(LIBS_LIB_DIR_5)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

# Project libraries
$(PROJECT_LIB_1):
	@$(MAKE) -C "$(PROJECT_LIB_DIR_1)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(PROJECT_LIB_2):
	@$(MAKE) -C "$(PROJECT_LIB_DIR_2)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(PROJECT_LIB_3):
	@$(MAKE) -C "$(PROJECT_LIB_DIR_3)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(PROJECT_LIB_4):
	@$(MAKE) -C "$(PROJECT_LIB_DIR_4)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(PROJECT_LIB_5):
	@$(MAKE) -C "$(PROJECT_LIB_DIR_5)" lib \
		CFG="$(CFG)" CXX="$(CXX)" STD="$(STD)" DEFS="$(DEFS)" \
		SAN_FLAGS="$(SAN_FLAGS)" WARN_FLAGS="$(WARN_FLAGS)" \
		LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

# Build executable
exe: $(EXE)

$(EXE): $(MAIN_OBJ) $(ALL_LIBS)
	@mkdir -p $(dir $@)
	@$(CXX) $(MAIN_OBJ) -Wl,--start-group $(ALL_LIBS) -Wl,--end-group -o $@ $(LDFLAGS) $(LDLIBS)

$(MAIN_OBJ): $(MAIN_SRC)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(MAIN_DEPS)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf dumps
	rm -f $(PROJECT_HEADERS_FILE)
	@$(MAKE) -C "$(COMMON_LIB_DIR)" clean || true
	@$(MAKE) -C "$(LIBS_LIB_DIR_1)" clean || true
	@$(MAKE) -C "$(LIBS_LIB_DIR_2)" clean || true
	@$(MAKE) -C "$(LIBS_LIB_DIR_3)" clean || true
	@$(MAKE) -C "$(LIBS_LIB_DIR_4)" clean || true
	@$(MAKE) -C "$(LIBS_LIB_DIR_5)" clean || true
	@$(MAKE) -C "$(PROJECT_LIB_DIR_1)" clean || true
	@$(MAKE) -C "$(PROJECT_LIB_DIR_2)" clean || true
	@$(MAKE) -C "$(PROJECT_LIB_DIR_3)" clean || true
	@$(MAKE) -C "$(PROJECT_LIB_DIR_4)" clean || true
	@$(MAKE) -C "$(PROJECT_LIB_DIR_5)" clean || true