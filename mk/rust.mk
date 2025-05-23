# ────────────────────────────────────────────────────────────────
# Rust bindings – build, install, publish
# (auto-included from the project-root Makefile)
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

# Output directory for generated artifacts (ignored by Git)
OUT_DIR      := $(CRATE_ROOT)/target

# The tarball kept under version control inside the crate
CRATE_TAR    := $(CRATE_ROOT)/fastlanes-src.tar.gz

# The same tarball built into the build cache (never checked in)
FASTLANES_TAR := $(OUT_DIR)/fastlanes-src.tar.gz

NUM_JOBS ?= $(shell sysctl -n hw.ncpu 2>/dev/null || echo 8)

CARGO  ?= cargo
C_ENV   := C_INCLUDE_PATH=$(PREFIX)/include \
           LIBRARY_PATH=$(PREFIX)/lib \
           CXXFLAGS=-I$(PREFIX)/include

# ----------------------------------------------------------------
# Produce fastlanes-src.tar.gz reproducibly
# ----------------------------------------------------------------
.PHONY: package-fastlanes
package-fastlanes:
	@echo "Packaging FastLanes sources → $(FASTLANES_TAR)"
	@mkdir -p $(OUT_DIR)

	# Determine whether this Git supports --mtime (needs ≥ 2.35)
	$(eval GIT_MTIME_OPT := $(shell git -C $(PROJECT_DIR) archive --help 2>&1 | \
	                               grep -q -- '--mtime' && echo '--mtime=@0' || echo ''))

	# Create deterministic archive in a temporary file
	@tmp=$(FASTLANES_TAR).tmp && \
	    git -C $(PROJECT_DIR) archive --format=tar $(GIT_MTIME_OPT) -o $$tmp \
	        HEAD CMakeLists.txt include src && \
	    gzip -9n $$tmp && mv $$tmp.gz $$tmp

	# Refresh build-cache copy; copy into crate only if bytes differ
	@mv $(FASTLANES_TAR).tmp $(FASTLANES_TAR)
	@if ! test -f $(CRATE_TAR) || ! cmp -s $(FASTLANES_TAR) $(CRATE_TAR); then \
	    cp $(FASTLANES_TAR) $(CRATE_TAR); \
	    echo "  ↪ updated committed tarball"; \
	else \
	    echo "  ↪ committed tarball already up-to-date"; \
	fi

# Helper: regenerate the tarball and commit it (run after editing C/C++)
.PHONY: update-fastlanes-src
update-fastlanes-src: package-fastlanes
	@git add $(CRATE_TAR)
	@git commit -m "Update embedded FastLanes source blob" \
	  || echo "Tarball unchanged — nothing to commit."

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

publish-rust:
	@echo "Publishing Rust crate to crates.io…"
	$(C_ENV) \
	RUSTFLAGS="-L$(PREFIX)/lib" \
	$(CARGO) publish --manifest-path $(CRATE_ROOT)/Cargo.toml

# Dry-run publish without regenerating the tarball
.PHONY: dry-run-rust
dry-run-rust:
	@echo "Dry-run publishing Rust crate…"
	$(C_ENV) \
	RUSTFLAGS="-L$(PREFIX)/lib" \
	$(CARGO) publish --manifest-path $(CRATE_ROOT)/Cargo.toml --dry-run

# ----------------------------------------------------------------
# Clean & misc
# ----------------------------------------------------------------
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

rust-format:
	@echo "Formatting Rust sources…"
	$(CARGO) fmt --manifest-path $(CRATE_ROOT)/Cargo.toml
	@echo "Rust formatting complete."

endif   # RUST_MK_INCLUDED
