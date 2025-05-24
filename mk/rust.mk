# ────────────────────────────────────────────────────────────────
#  Rust bindings – build, install, publish
#  (auto-included from the project-root Makefile)
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
OUT_DIR     := $(CRATE_ROOT)/target          # Cargo’s build dir

NUM_JOBS ?= $(shell sysctl -n hw.ncpu 2>/dev/null || echo 8)

CARGO ?= cargo
C_ENV  := C_INCLUDE_PATH=$(PREFIX)/include \
          LIBRARY_PATH=$(PREFIX)/lib \
          CXXFLAGS=-I$(PREFIX)/include

# ----------------------------------------------------------------
# Build / install / publish
# ----------------------------------------------------------------
.PHONY: build-rust install-rust publish-rust dry-run-rust \
        clean-rust rust-format run-rust-example update-fastlanes-src

build-rust:
	@echo "Building Rust crate (release, $(NUM_JOBS) jobs)…"
	CMAKE_BUILD_PARALLEL_LEVEL=$(NUM_JOBS) \
	$(CARGO) build \
	  --release \
	  --manifest-path $(CRATE_ROOT)/Cargo.toml \
	  --jobs $(NUM_JOBS)
	@echo "Rust build complete."

install-rust: build-rust
	@echo "Installing Rust crate into $(PREFIX)…"
	$(C_ENV) \
	$(CARGO) install \
	  --path $(CRATE_ROOT) \
	  --root $(PREFIX) \
	  --jobs $(NUM_JOBS)

# Always pull the latest upstream C++ sources before packaging
publish-rust: update-fastlanes-src
	@echo "Publishing Rust crate to crates.io…"
	$(C_ENV) \
	RUSTFLAGS="-L$(PREFIX)/lib" \
	$(CARGO) publish --manifest-path $(CRATE_ROOT)/Cargo.toml

dry-run-rust: update-fastlanes-src
	@echo "Dry-run publishing Rust crate…"
	$(C_ENV) \
	RUSTFLAGS="-L$(PREFIX)/lib" \
	$(CARGO) publish \
	  --manifest-path $(CRATE_ROOT)/Cargo.toml \
	  --dry-run

# ----------------------------------------------------------------
# Clean & misc
# ----------------------------------------------------------------
clean-rust:
	@echo "Cleaning Rust build…"
	$(CARGO) clean --manifest-path $(CRATE_ROOT)/Cargo.toml
	@echo "Rust clean complete."

run-rust-example: build-rust
	@echo "Running Rust example ‘rust_example’…"
	cd $(CRATE_ROOT) && \
	CC="$(CC)" CXX="$(CXX)" \
	C_INCLUDE_PATH="$(PREFIX)/include" \
	LIBRARY_PATH="$(PREFIX)/lib" \
	cargo run --jobs $(NUM_JOBS) --example rust_example

rust-format:
	@echo "Formatting Rust sources…"
	$(CARGO) fmt --manifest-path $(CRATE_ROOT)/Cargo.toml
	@echo "Rust formatting complete."

# ----------------------------------------------------------------
# Vendored-source maintenance helper
# ----------------------------------------------------------------
update-fastlanes-src:
	@git subtree pull \
	    --prefix=rust/vendor/fastlanes \
	    https://github.com/cwida/FastLanes.git \
	    main --squash || true   # tolerate no-net environments

endif  # RUST_MK_INCLUDED
