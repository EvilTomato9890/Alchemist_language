
PROJECT    ?= parser
PARENT_DIR ?= ../../..
SRC_DIR   := source
INC_DIR   := include
INT_DIR   := internal
TST_DIR   := tests

BUILD_DIR ?= build
OBJ_DIR   := $(BUILD_DIR)/obj
BIN_DIR   := $(BUILD_DIR)/bin
LIB_DIR   ?= $(BUILD_DIR)/lib

CXX       ?= g++
AR        ?= ar
ARFLAGS   ?= rcs

# debug | release
CFG ?= debug

STD   ?= -std=c++20
INCS  := -I$(INC_DIR) -I$(INT_DIR) -I$(PARENT_DIR) -I$(PARENT_DIR)/project/frontend


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

CXXFLAGS_COMMON := $(STD) $(INCS) $(WARN_FLAGS) -MMD -MP -pipe -fexceptions

#Свои дефайны
DEFS ?= -DTREE_VERIFY_DEBUG -DSTACK_VERIFY_DEBUG -DLOGGER_ALL
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

TST_EXE  := $(BIN_DIR)/$(PROJECT)_tests

DEPS     := $(LIB_OBJS:.o=.d) $(TST_OBJS:.o=.d)

# ================================================================================

EXT_LIB_DIR_1    ?= $(PARENT_DIR)/common
EXT_LIB_TARGET_1 ?= lib
EXT_LIB_A_1      ?= $(EXT_LIB_DIR_1)/build/lib/libcommon.a  

EXT_LIB_DIR_2    ?= $(PARENT_DIR)/libs/Stack
EXT_LIB_TARGET_2 ?= lib
EXT_LIB_A_2      ?= $(EXT_LIB_DIR_2)/build/lib/libStack.a  

EXT_LIB_DIR_3    ?= $(PARENT_DIR)/libs/My_string
EXT_LIB_TARGET_3 ?= lib
EXT_LIB_A_3      ?= $(EXT_LIB_DIR_3)/build/lib/libMy_string.a  

EXT_LIB_DIR_4    ?= $(PARENT_DIR)/libs/Vector
EXT_LIB_TARGET_4 ?= lib
EXT_LIB_A_4      ?= $(EXT_LIB_DIR_4)/build/lib/libVector.a  

EXT_LIB_DIR_5    ?= $(PARENT_DIR)/libs/Unordered_map
EXT_LIB_TARGET_5 ?= lib
EXT_LIB_A_5      ?= $(EXT_LIB_DIR_5)/build/lib/libUnordered_map.a  

EXT_LIB_DIR_6    ?= $(PARENT_DIR)/libs/AST
EXT_LIB_TARGET_6 ?= lib
EXT_LIB_A_6      ?= $(EXT_LIB_DIR_6)/build/lib/libAST.a  

EXT_LIB_DIR_7    ?= $(PARENT_DIR)/project/frontend/error_logger
EXT_LIB_TARGET_7 ?= lib
EXT_LIB_A_7      ?= $(EXT_LIB_DIR_7)/build/lib/liberror_logger.a  

EXT_LIB_DIR_8    ?= $(PARENT_DIR)/project/frontend/lexer
EXT_LIB_TARGET_8 ?= lib
EXT_LIB_A_8      ?= $(EXT_LIB_DIR_8)/build/lib/liblexer.a  

EXT_LIBS := $(EXT_LIB_A_1) $(EXT_LIB_A_2) $(EXT_LIB_A_3) $(EXT_LIB_A_4) $(EXT_LIB_A_5) $(EXT_LIB_A_6) $(EXT_LIB_A_7) $(EXT_LIB_A_8)

# ================================================================================

.PHONY: all test lib clean test_clean help

all: test


help:
	@echo "Targets:"
	@echo "  make test        - build tests executable ($(TST_EXE))"
	@echo "  make clean"
	@echo "  make test_clean  - clear after test"
	@echo "  CFG=debug|release "

test: $(TST_EXE)


$(TST_EXE): $(LIB_OBJS) $(TST_OBJS) $(EXT_LIBS)
	@mkdir -p $(dir $@)
	@$(CXX) $(TST_OBJS) $(LIB_OBJS) -Wl,--start-group $(EXT_LIBS) -Wl,--end-group -o $@ $(LDFLAGS) $(LDLIBS) #TODO -  Разобраться

$(EXT_LIB_A_1):
	@$(MAKE) -C "$(EXT_LIB_DIR_1)" "$(EXT_LIB_TARGET_1)" \
		CFG="$(CFG)" CXX="$(CXX)" DEFS="$(DEFS)" STD="$(STD)" SAN_FLAGS="$(SAN_FLAGS)" \
		WARN_FLAGS="$(WARN_FLAGS)" LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(EXT_LIB_A_2):
	@$(MAKE) -C "$(EXT_LIB_DIR_2)" "$(EXT_LIB_TARGET_2)" \
		CFG="$(CFG)" CXX="$(CXX)" DEFS="$(DEFS)" STD="$(STD)" SAN_FLAGS="$(SAN_FLAGS)" \
		WARN_FLAGS="$(WARN_FLAGS)" LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(EXT_LIB_A_3):
	@$(MAKE) -C "$(EXT_LIB_DIR_3)" "$(EXT_LIB_TARGET_3)" \
		CFG="$(CFG)" CXX="$(CXX)" DEFS="$(DEFS)" STD="$(STD)" SAN_FLAGS="$(SAN_FLAGS)" \
		WARN_FLAGS="$(WARN_FLAGS)" LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(EXT_LIB_A_4):
	@$(MAKE) -C "$(EXT_LIB_DIR_4)" "$(EXT_LIB_TARGET_4)" \
		CFG="$(CFG)" CXX="$(CXX)" DEFS="$(DEFS)" STD="$(STD)" SAN_FLAGS="$(SAN_FLAGS)" \
		WARN_FLAGS="$(WARN_FLAGS)" LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(EXT_LIB_A_5):
	@$(MAKE) -C "$(EXT_LIB_DIR_5)" "$(EXT_LIB_TARGET_5)" \
		CFG="$(CFG)" CXX="$(CXX)" DEFS="$(DEFS)" STD="$(STD)" SAN_FLAGS="$(SAN_FLAGS)" \
		WARN_FLAGS="$(WARN_FLAGS)" LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(EXT_LIB_A_6):
	@$(MAKE) -C "$(EXT_LIB_DIR_6)" "$(EXT_LIB_TARGET_6)" \
		CFG="$(CFG)" CXX="$(CXX)" DEFS="$(DEFS)" STD="$(STD)" SAN_FLAGS="$(SAN_FLAGS)" \
		WARN_FLAGS="$(WARN_FLAGS)" LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(EXT_LIB_A_7):
	@$(MAKE) -C "$(EXT_LIB_DIR_7)" "$(EXT_LIB_TARGET_7)" \
		CFG="$(CFG)" CXX="$(CXX)" DEFS="$(DEFS)" STD="$(STD)" SAN_FLAGS="$(SAN_FLAGS)" \
		WARN_FLAGS="$(WARN_FLAGS)" LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"

$(EXT_LIB_A_8):
	@$(MAKE) -C "$(EXT_LIB_DIR_8)" "$(EXT_LIB_TARGET_8)" \
		CFG="$(CFG)" CXX="$(CXX)" DEFS="$(DEFS)" STD="$(STD)" SAN_FLAGS="$(SAN_FLAGS)" \
		WARN_FLAGS="$(WARN_FLAGS)" LDFLAGS="$(LDFLAGS)" LDLIBS="$(LDLIBS)"


$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

test_clean:
	rm -rf $(BUILD_DIR)
	@$(MAKE) -C $(EXT_LIB_DIR_1) clean || true
	@$(MAKE) -C $(EXT_LIB_DIR_2) clean || true
	@$(MAKE) -C $(EXT_LIB_DIR_3) clean || true
	@$(MAKE) -C $(EXT_LIB_DIR_4) clean || true
	@$(MAKE) -C $(EXT_LIB_DIR_5) clean || true
	@$(MAKE) -C $(EXT_LIB_DIR_6) clean || true
	@$(MAKE) -C $(EXT_LIB_DIR_7) clean || true
	@$(MAKE) -C $(EXT_LIB_DIR_8) clean || true

clean:
	rm -rf $(BUILD_DIR)