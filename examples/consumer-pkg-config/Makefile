CXX ?= c++
PKG_CONFIG ?= pkg-config
PKG_CONFIG_FLAGS ?= --static
BUILD_DIR ?= .
TARGET ?= $(BUILD_DIR)/texgui_pkg_config_example
OUTPUT ?= $(BUILD_DIR)/pkg-config-example.png

CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic
TEXGUI_CFLAGS := $(shell $(PKG_CONFIG) --cflags texgui)
TEXGUI_LIBS := $(shell $(PKG_CONFIG) --libs $(PKG_CONFIG_FLAGS) texgui)
TEXGUI_RES_DIR := $(shell $(PKG_CONFIG) --variable=resdir texgui)

CPPFLAGS += -DTEXGUI_RES_DIR=\"$(TEXGUI_RES_DIR)\"
CPPFLAGS += -DTEXGUI_EXAMPLE_OUTPUT=\"$(OUTPUT)\"

.PHONY: all run clean

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p "$@"

$(TARGET): main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TEXGUI_CFLAGS) $< $(TEXGUI_LIBS) -o $@

run: $(TARGET)
	"$(TARGET)"

clean:
	rm -f "$(TARGET)" "$(OUTPUT)"
