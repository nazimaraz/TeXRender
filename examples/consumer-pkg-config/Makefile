CXX ?= c++
PKG_CONFIG ?= pkg-config
PKG_CONFIG_FLAGS ?= --static
BUILD_DIR ?= .
TARGET ?= $(BUILD_DIR)/texrender_pkg_config_example
OUTPUT ?= $(BUILD_DIR)/pkg-config-example.png

CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic
TEXRENDER_CFLAGS := $(shell $(PKG_CONFIG) --cflags texrender)
TEXRENDER_LIBS := $(shell $(PKG_CONFIG) --libs $(PKG_CONFIG_FLAGS) texrender)
TEXRENDER_RES_DIR := $(shell $(PKG_CONFIG) --variable=resdir texrender)

CPPFLAGS += -DTEXRENDER_RES_DIR=\"$(TEXRENDER_RES_DIR)\"
CPPFLAGS += -DTEXRENDER_EXAMPLE_OUTPUT=\"$(OUTPUT)\"

.PHONY: all run clean

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p "$@"

$(TARGET): main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TEXRENDER_CFLAGS) $< $(TEXRENDER_LIBS) -o $@

run: $(TARGET)
	"$(TARGET)"

clean:
	rm -f "$(TARGET)" "$(OUTPUT)"
