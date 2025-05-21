# ────────────────────────────────────────────────────────────────
# Rust bindings ­– build, install, publish
# (auto-included from the root Makefile)
# ────────────────────────────────────────────────────────────────

ifndef RUST_MK_INCLUDED
RUST_MK_INCLUDED := yes

include $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/preamble.mk

# ----------------------------------------------------------------
# Paths
# ----------------------------------------------------------------
MK_DIR      := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PROJECT_DIR := $(abspath $(MK_DIR)/..)
CRATE_ROOT  := $(PROJECT_DIR)/rust
PREFIX      := $(PROJECT_DIR)/build/install

NUM_JOBS    ?= $(shell sysctl -n hw.ncpu 2>/dev/null || echo 8)

CARGO       ?= cargo
C_ENV       := \
  C_INCLUDE_PATH=$(PREFIX)/include \
  LIBRARY_PATH=$(PREFIX)/lib \
  CXXFLAGS=-I$(PREFIX)/include

###############################################################################
# rust/mk/rust.mk  – only the package-fastlanes target shown
###############################################################################
.PHONY: package-fastlanes
package-fastlanes:
	@echo "Packaging FastLanes sources → $(CRATE_ROOT)/fastlanes-src.tar.gz"

	# tar the minimal C++ tree (no tests, docs, data)
	git -C $(PROJECT_DIR) archive --format=tar HEAD \
	    CMakeLists.txt include src alp primitives \
	    | gzip -9n > $(CRATE_ROOT)/fastlanes-src.tar.gz.tmp

	mv $(CRATE_ROOT)/fastlanes-src.tar.gz.tmp $(CRATE_ROOT)/fastlanes-src.tar.gz
	@ls -lh $(CRATE_ROOT)/fastlanes-src.tar.gz



$(CRATE_ROOT)/fastlanes-src.tar.gz:
	@echo "Packaging FastLanes sources → $@"
	@rm -f $@.tmp
	git -C $(PROJECT_DIR) archive --format=tar HEAD | gzip -n > $@.tmp
	mv $@.tmp $@

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
	@rm -f $(CRATE_ROOT)/fastlanes-src.tar.gz
	@echo "Rust clean complete."

run-rust-example: build-rust
	@echo "Running Rust example ‘rust_example’…"
	cd $(CRATE_ROOT) && \
	C_INCLUDE_PATH="$(PREFIX)/include" \
	LIBRARY_PATH="$(PREFIX)/lib" \
	cargo run --example rust_example
	
endif   # RUST_MK_INCLUDED
