//! FastLanes build script
//! • Emits VERGEN_* Git metadata
//! • Builds the FastLanes C++ library via CMake
//! • Links the resulting static lib into Rust

use std::{env, path::PathBuf};

use vergen::Emitter;
use vergen_gix::GixBuilder;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    /* ── 1) Emit Git-derived build info ───────────────────────────── */
    println!("cargo:rerun-if-changed=.git/HEAD");
    println!("cargo:rerun-if-changed=.git/refs/tags");

    let git = GixBuilder::all_git()?;
    Emitter::default().add_instructions(&git)?.emit()?;

    /* ── 2) Locate the C++ sources ───────────────────────────────── */
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR")?);
    let repo_root = crate_dir
        .parent()
        .expect("crate directory has a parent (workspace root)");

    // Workspace checkout → use repo root; crates.io build → use vendored copy
    let src_dir = if repo_root.join("CMakeLists.txt").exists() {
        repo_root.to_path_buf()
    } else {
        crate_dir.join("vendor/fastlanes")
    };

    println!("cargo:warning=--- Running CMake in {src_dir:?} ---");

    /* ── 3) Configure & build with CMake (default ‘all’ target) ──── */
    let dst = cmake::Config::new(&src_dir)
        .define("CMAKE_VERBOSE_MAKEFILE", "ON")
        .profile("Release")
        .build();

    // Headers live in the source tree; library comes from cmake-crate build dir
    let include_dir = src_dir.join("include");
    let lib_dir = dst.join("lib");

    /* ── 4) Compile the CXX bridge shim ─────────────────────────── */
    cxx_build::bridge("src/lib.rs")
        .include(&include_dir)
        .include(&crate_dir) // for bridge_shim.hpp if present
        .file("bridge_shim.cpp")
        .flag_if_supported("-std=c++20")
        .compile("fastlanes_rs");

    /* ── 5) Link the static FastLanes library ───────────────────── */
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=static=fastlanes"); // libfastlanes.a
    println!("cargo:rustc-link-lib=c++");

    /* ── 6) Re-run triggers ─────────────────────────────────────── */
    println!("cargo:rerun-if-changed=vendor/fastlanes");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=bridge_shim.cpp");

    Ok(())
}
