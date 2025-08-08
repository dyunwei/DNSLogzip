# path macros
BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src
INC_PATH := include

# compile macros
TARGET_NAME := DNSLogzip
TARGET := $(BIN_PATH)/$(TARGET_NAME)
MACRO =

# tool macros
CXX := g++ -g -ggdb3 -Wall -std=c++11 -Wfatal-errors
CXXFLAGS := -O3
CCOBJFLAGS := $(CXXFLAGS) -I $(INC_PATH) -c $(MACRO)
LDFLAGS := -lm

# install macros
PREFIX ?= /usr/local
INSTALL_BIN_PATH := $(DESTDIR)$(PREFIX)/bin

# src files & obj files
SRCS := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
INCS := $(foreach x, $(INC_PATH), $(wildcard $(addprefix $(x)/*,.h*)))
OBJS := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRCS)))))

# clean files list
CLEAN_LIST := $(TARGET) \
				$(OBJS)

# default rule
default: makedir all

# non-phony targets
$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c* $(INC_PATH)/*.h*
	$(CXX) $(CCOBJFLAGS) -o $@ $<

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS)

# phony rules
.PHONY: makedir
makedir:
	@mkdir -p $(BIN_PATH) $(OBJ_PATH)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(CLEAN_LIST)

.PHONY: clean_all
clean_all:
	@echo CLEAN $(BIN_PATH) $(OBJ_PATH) 
	@rm -rf $(BIN_PATH) $(OBJ_PATH)

# install rule
.PHONY: install
install: $(TARGET)
	@echo "Installing $(TARGET) to $(INSTALL_BIN_PATH)"
	@mkdir -p $(INSTALL_BIN_PATH)
	@install -m 0755 $(TARGET) $(INSTALL_BIN_PATH)/$(TARGET_NAME)

# uninstall rule
.PHONY: uninstall
uninstall:
	@echo "Removing $(TARGET_NAME) from $(INSTALL_BIN_PATH)"
	@rm -f $(INSTALL_BIN_PATH)/$(TARGET_NAME)