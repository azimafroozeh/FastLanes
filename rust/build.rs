//! FastLanes build script
//!  • Emits VERGEN_* Git metadata
//!  • Builds the FastLanes C++ library via CMake
//!  • Links the resulting static lib into Rust

use std::{env, path::PathBuf};

use vergen::Emitter;
use vergen_gix::GixBuilder;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    /* ── 1. Git metadata ──────────────────────────────────────────── */
    println!("cargo:rerun-if-changed=.git/HEAD");
    println!("cargo:rerun-if-changed=.git/refs/tags");

    let git = GixBuilder::all_git()?;
    Emitter::default().add_instructions(&git)?.emit()?;

    /* ── 2. Locate sources ────────────────────────────────────────── */
    let crate_dir  = PathBuf::from(env::var("CARGO_MANIFEST_DIR")?);     // …/rust/fls-rs
    let repo_root  = crate_dir.ancestors().nth(2).expect("repo root");   // …/FastLanes
    let src_dir = if repo_root.join("CMakeLists.txt").exists() {
        repo_root.to_path_buf()                  // workspace checkout
    } else {
        crate_dir.join("vendor/fastlanes")       // crates-io build
    };

    println!("cargo:warning=--- Running CMake in {src_dir:?} ---");

    /* ── 3. Build C++ with CMake ──────────────────────────────────── */
    let dst = cmake::Config::new(&src_dir)
        .define("CMAKE_VERBOSE_MAKEFILE", "ON")
        .profile("Release")
        .build();

    let include_src    = src_dir.join("include").canonicalize()?;        // FastLanes headers
    let include_dst    = dst.join("include").canonicalize()?;            // “make install” copy
    let lib_dir        = dst.join("lib");                                // libfastlanes.a

    /* ── 4. Build the CXX bridge ──────────────────────────────────── */
    cxx_build::bridge("src/lib.rs")
        .file("bridge_shim.cpp")
        .include(&include_src)      // absolute → works everywhere
        .include(&include_dst)
        .include(&crate_dir)        // bridge_shim.hpp
        .flag_if_supported("-std=c++20")
        .compile("fastlanes_rs");

    /* ── 5. Link the static library ───────────────────────────────── */
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=static=fastlanes");
    println!("cargo:rustc-link-lib=c++");        // libc++ on clang

    /* ── 6. Re-run triggers ───────────────────────────────────────── */
    println!("cargo:rerun-if-changed=vendor/fastlanes");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=bridge_shim.cpp");

    Ok(())
}
