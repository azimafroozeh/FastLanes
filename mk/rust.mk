# ────────────────────────────────────────────────────────────────
# Rust bindings – build, install, publish
# (auto-included from the root Makefile)
# ────────────────────────────────────────────────────────────────

ifndef RUST_MK_INCLUDED
RUST_MK_INCLUDED := yes

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/preamble.mk

# ----------------------------------------------------------------
# Paths
# ----------------------------------------------------------------
MK_DIR       := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PROJECT_DIR  := $(abspath $(MK_DIR)/..)
CRATE_ROOT   := $(PROJECT_DIR)/rust
PREFIX       := $(PROJECT_DIR)/build/install

# Output directory for generated artifacts (ignored by git)
OUT_DIR      := $(CRATE_ROOT)/target
FASTLANES_TAR := $(OUT_DIR)/fastlanes-src.tar.gz

NUM_JOBS     ?= $(shell sysctl -n hw.ncpu 2>/dev/null || echo 8)

CARGO        ?= cargo
C_ENV        := \
  C_INCLUDE_PATH=$(PREFIX)/include \
  LIBRARY_PATH=$(PREFIX)/lib \
  CXXFLAGS=-I$(PREFIX)/include

###############################################################################
# rust/mk/rust.mk  – only the package-fastlanes target shown
###############################################################################
.PHONY: package-fastlanes
package-fastlanes:
	@echo "Packaging FastLanes sources → $(FASTLANES_TAR)"
	@mkdir -p $(OUT_DIR)

	# tar the minimal C++ tree (no tests, docs, data)
	git -C $(PROJECT_DIR) archive --format=tar HEAD \
	    CMakeLists.txt include src alp primitives \
	    | gzip -9n > $(FASTLANES_TAR).tmp
	mv $(FASTLANES_TAR).tmp $(FASTLANES_TAR)
	@ls -lh $(FASTLANES_TAR)

# ----------------------------------------------------------------
# Build / install / publish
# ----------------------------------------------------------------
.PHONY: build-rust install-rust publish-rust dry-run-rust clean-rust rust-format

build-rust: package-fastlanes
	@echo "Building Rust crate (release, $(NUM_JOBS) jobs)…"
	$(CARGO) build --release \
	  --manifest-path $(CRATE_ROOT)/Cargo.toml \
	  --jobs $(NUM_JOBS)
	@echo "Rust build complete."

install-rust: build-rust
	@echo "Installing Rust crate into $(PREFIX)…"
	$(C_ENV) \
	$(CARGO) install --path $(CRATE_ROOT) --root $(PREFIX) --jobs $(NUM_JOBS)

publish-rust: package-fastlanes
	@echo "Publishing Rust crate to crates.io…"
	$(C_ENV) \
	RUSTFLAGS="-L$(PREFIX)/lib" \
	$(CARGO) publish --manifest-path $(CRATE_ROOT)/Cargo.toml

dry-run-rust: package-fastlanes
	@echo "Dry-run publishing Rust crate…"
	$(C_ENV) \
	RUSTFLAGS="-L$(PREFIX)/lib" \
	$(CARGO) publish --manifest-path $(CRATE_ROOT)/Cargo.toml --dry-run

clean-rust:
	@echo "Cleaning Rust build…"
	$(CARGO) clean --manifest-path $(CRATE_ROOT)/Cargo.toml
	@rm -rf $(FASTLANES_TAR)
	@echo "Rust clean complete."

run-rust-example: build-rust
	@echo "Running Rust example ‘rust_example’…"
	cd $(CRATE_ROOT) && \
	C_INCLUDE_PATH="$(PREFIX)/include" \
	LIBRARY_PATH="$(PREFIX)/lib" \
	cargo run --example rust_example

endif   # RUST_MK_INCLUDED
