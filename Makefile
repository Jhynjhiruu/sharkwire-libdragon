all: fw.bin
.PHONY: all

BUILD_DIR = build
SOURCE_DIR = src
TOOLS_DIR = tools
include $(N64_INST)/include/n64.mk

OBJS = $(BUILD_DIR)/main.o

N64_CFLAGS += -std=gnu23

shark.z64: N64_ROM_TITLE = "SharkWire Firmware"

$(BUILD_DIR)/shark.elf: $(OBJS)

DD := dd

include $(TOOLS_DIR)/Makefile

fw.bin.tmp: shark.z64
	$(DD) if=$< of=$@ bs=256K conv=sync status=none

fw.bin: fw.bin.tmp $(GSCRC)
	$(TOOLS_DIR)/$(GSCRC) $< $@ 7101::80201000

# Setup repo
setup:
	$(MAKE) -C $(TOOLS_DIR) tools_setup

# Clean code
clean:
	rm -rf $(BUILD_DIR) *.z64 *.bin *.tmp
.PHONY: clean

# Clean tools
distclean: clean
	$(MAKE) -C $(TOOLS_DIR) tools_clean
.PHONY: distclean

-include $(wildcard $(BUILD_DIR)/*.d)
