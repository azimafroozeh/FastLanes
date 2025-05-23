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
# rust/mk/rust.mk  – revised package-fastlanes target (Git ≥ 1.8 compatible)
###############################################################################
.PHONY: package-fastlanes
package-fastlanes:
	@echo "Packaging FastLanes sources → $(FASTLANES_TAR)"
	@mkdir -p $(OUT_DIR)

	# --- 1. figure out whether our Git supports --mtime (≥ 2.35) ----------
	$(eval GIT_MTIME_OPT := $(shell git -C $(PROJECT_DIR) archive --help 2>&1 \
	                               | grep -q -- '--mtime' && echo "--mtime=@0" || echo ""))

	# --- 2. create a temporary archive ------------------------------------
	@tmp=$(FASTLANES_TAR).tmp && \
	    git -C $(PROJECT_DIR) archive --format=tar $(GIT_MTIME_OPT) -o $$tmp \
	        HEAD CMakeLists.txt include src && \
	    gzip -9n $$tmp && mv $$tmp.gz $$tmp

	# --- 3. install it if the content is different ------------------------
	@if ! test -f $(FASTLANES_TAR) || ! cmp -s $(FASTLANES_TAR).tmp $(FASTLANES_TAR); then \
	    mv $(FASTLANES_TAR).tmp $(FASTLANES_TAR); \
	    cp $(FASTLANES_TAR) $(CRATE_ROOT)/fastlanes-src.tar.gz; \
	    echo "  ↪ updated tarball"; \
	else \
	    rm $(FASTLANES_TAR).tmp; \
	    echo "  ↪ tarball already up-to-date"; \
	fi



# ----------------------------------------------------------------
# Build / install / publish
# ----------------------------------------------------------------
.PHONY: build-rust install-rust publish-rust dry-run-rust clean-rust rust-format

build-rust: package-fastlanes
	@echo "Building Rust crate (release, $(NUM_JOBS) jobs)…"
	CMAKE_BUILD_PARALLEL_LEVEL=$(NUM_JOBS) \
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
	CC="$(CC)" CXX="$(CXX)" \
	C_INCLUDE_PATH="$(PREFIX)/include" \
	LIBRARY_PATH="$(PREFIX)/lib" \
	cargo run --jobs $(NUM_JOBS) --example rust_example

# ----------------------------------------------------------------
# Format Rust source
# ----------------------------------------------------------------
.PHONY: rust-format
rust-format:
	@echo "Formatting Rust sources…"
	$(CARGO) fmt --manifest-path $(CRATE_ROOT)/Cargo.toml
	@echo "Rust formatting complete."

endif   # RUST_MK_INCLUDED
