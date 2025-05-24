//! Self-contained FastLanes builder + Git-tag version emitter
use std::{env, path::PathBuf};

use vergen::Emitter;
use vergen_gix::GixBuilder;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    /* ── 1) Git-derived build info ───────────────────────────── */
    println!("cargo:rerun-if-changed=.git/HEAD");
    println!("cargo:rerun-if-changed=.git/refs/tags");

    let git = GixBuilder::all_git()?;
    Emitter::default().add_instructions(&git)?.emit()?;

    /* ── 2) Locate the C++ sources ───────────────────────────── */
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR")?);
    let repo_root = crate_dir
        .parent()
        .expect("crate directory has a parent (workspace root)");

    // Workspace checkout → build in repo-root; crates.io checkout → build in vendored folder
    let src_dir = if repo_root.join("CMakeLists.txt").exists() {
        repo_root.to_path_buf()               // local workspace
    } else {
        crate_dir.join("vendor/fastlanes")    // vendored copy inside the crate
    };

    println!("cargo:warning=--- Running CMake in {src_dir:?} ---");

    let install = cmake::Config::new(&src_dir)
        .define("CMAKE_VERBOSE_MAKEFILE", "ON")
        .profile("Release")
        .build_target("install")
        .build();

    let include_dir = install.join("include");
    let lib_dir     = install.join("lib");

    /* ── 3) Build the CXX bridge ─────────────────────────────── */
    cxx_build::bridge("src/lib.rs")
        .include(&include_dir)
        .include(&crate_dir)
        .file("bridge_shim.cpp")
        .flag_if_supported("-std=c++20")
        .compile("fastlanes_rs");

    /* ── 4) Link against the static FastLanes lib ────────────── */
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=static=FastLanes");
    println!("cargo:rustc-link-lib=c++");

    /* ── 5) Re-run triggers ──────────────────────────────────── */
    println!("cargo:rerun-if-changed=vendor/fastlanes");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=bridge_shim.cpp");

    Ok(())
}
