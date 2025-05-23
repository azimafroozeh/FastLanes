//! Self-contained FastLanes builder + Git-tag version emitter

use std::{
    env, fs,
    io::Cursor,
    path::{Path, PathBuf},
};

use flate2::read::GzDecoder;
use tar::Archive;
use vergen::{vergen, Config}; // ①

const FLS_TARBALL: &[u8] = include_bytes!("fastlanes-src.tar.gz");

fn main() {
    // ── 1) Emit Git tag version into VERGEN_GIT_SEMVER  ─────────────
    // Re-run if HEAD or tags change
    println!("cargo:rerun-if-changed=.git/HEAD");
    println!("cargo:rerun-if-changed=.git/refs/tags");
    let mut cfg = Config::default();
    *cfg.git_mut().semver_mut() = true; // enable semver from annotated tag
    vergen(cfg).expect("Unable to emit version info");

    // ── 2) Unpack & build the C++ sources ───────────────────────────
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let unpack_dst = out_dir.join("fastlanes-src");

    let repo_root = crate_dir.parent().expect("rust/ has a parent");
    let src_dir: PathBuf = if repo_root.join("CMakeLists.txt").exists() {
        // dev build: build from workspace checkout
        repo_root.into()
    } else {
        // crates.io build: unpack the embedded archive
        ensure_unpacked(&unpack_dst);
        dive_one_level(&unpack_dst)
    };

    println!("cargo:warning=--- Running CMake in {:?} ---", src_dir);

    let install_prefix = cmake::Config::new(&src_dir)
        .define("CMAKE_VERBOSE_MAKEFILE", "ON")
        .profile("Release")
        .build_target("install")
        .build();

    let include_dir = install_prefix.join("include");
    let lib_dir = install_prefix.join("lib");

    // ── 3) Generate & compile the CXX bridge ───────────────────────
    let mut bridge = cxx_build::bridge("src/lib.rs");
    bridge
        .include(&include_dir)
        .include(&crate_dir)
        .file("bridge_shim.cpp")
        .flag_if_supported("-std=c++20")
        .flag_if_supported("-Wno-changes-meaning")
        .flag_if_supported("-Wno-error=unknown-warning-option")
        .compile("fastlanes_rs");

    // ── 4) Link the static C++ library ──────────────────────────────
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=static=FastLanes");
    println!("cargo:rustc-link-lib=c++");

    // ── 5) Re-run triggers ──────────────────────────────────────────
    println!("cargo:rerun-if-changed=fastlanes-src.tar.gz");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=bridge_shim.cpp");
}

fn ensure_unpacked(dst: &Path) {
    if dst.exists() {
        return;
    }
    println!("cargo:warning=Unpacking FastLanes sources to {:?}", dst);

    fs::create_dir_all(dst).expect("create unpack dir");
    let gz = GzDecoder::new(Cursor::new(FLS_TARBALL));
    let mut ar = Archive::new(gz);
    ar.unpack(dst).expect("untar FastLanes");
}

fn dive_one_level(dir: &Path) -> PathBuf {
    let mut entries = fs::read_dir(dir).expect("read_dir").flatten();
    let first = entries.next().expect("tar has at least one entry");
    if entries.next().is_none() && first.file_type().unwrap().is_dir() {
        first.path()
    } else {
        dir.to_owned()
    }
}
