.PHONY: tools_clean

CARGO := cargo build
CARGO_CLEAN := cargo clean
CARGOFLAGS := --release
CARGO_TARGET := target/release

GSCRC := gscrc/$(CARGO_TARGET)/gscrc

tools_clean:
	(cd gscrc && $(CARGO_CLEAN))

$(GSCRC):
	(cd $(CURDIR)/$(TOOLS_DIR)/gscrc && $(CARGO) $(CARGOFLAGS) --no-default-features)

-include $(GSCRC_BASE).d
