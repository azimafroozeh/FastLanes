//! Self-contained FastLanes builder
use std::{
    env, fs,
    io::Cursor,
    path::{Path, PathBuf},
};

/// Embed the C++ tarball. During crates.io builds, this file must be
/// included next to build.rs via Cargo.toml `include = ["fastlanes-src.tar.gz"]`.
const FLS_TARBALL: &[u8] = include_bytes!("fastlanes-src.tar.gz");

fn main() {
    // ── Locate sources ───────────────────────────────────────────
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let unpack_dst = out_dir.join("fastlanes-src");

    let repo_root = crate_dir.parent().expect("rust/ has a parent");
    let src_dir: PathBuf = if repo_root.join("CMakeLists.txt").exists() {
        // dev/CI build – use workspace sources
        repo_root.into()
    } else {
        // crates.io build – unpack embedded tarball
        ensure_unpacked(&unpack_dst);
        dive_one_level(&unpack_dst)
    };

    // ── CMake build & install ────────────────────────────────────
    println!("cargo:warning=--- Running CMake in {:?} ---", src_dir);

    let install_prefix = cmake::Config::new(&src_dir)
        .define("CMAKE_VERBOSE_MAKEFILE", "ON")
        .profile("Release")
        // Pass parallel build level from environment if set
        .build_arg(format!("-- -j{}", env::var("NUM_JOBS").unwrap_or_else(|_| "1".into())))
        .build_target("install")
        .build();

    let include_dir = install_prefix.join("include");
    let lib_dir = install_prefix.join("lib");

    // ── Generate & build the CXX bridge file ─────────────────────
    let mut bridge = cxx_build::bridge("src/lib.rs");
    bridge
        .include(&include_dir)
        .include(&crate_dir)
        .file("bridge_shim.cpp")
        .flag_if_supported("-std=c++20")
        .flag_if_supported("-Wno-changes-meaning")
        .flag_if_supported("-Wno-error=unknown-warning-option")
        .compile("fastlanes_rs");

    // ── Link against the freshly-built FastLanes static lib ──────
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=static=FastLanes");
    println!("cargo:rustc-link-lib=c++");

    // ── Re-run triggers ──────────────────────────────────────────
    println!("cargo:rerun-if-changed=fastlanes-src.tar.gz");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=bridge_shim.cpp");
}

/// Unpack the embedded tarball once per build directory.
fn ensure_unpacked(dst: &Path) {
    if dst.exists() {
        return;
    }
    println!("cargo:warning=Unpacking FastLanes sources to {:?}", dst);

    fs::create_dir_all(dst).expect("create unpack dir");
    let gz = flate2::read::GzDecoder::new(Cursor::new(FLS_TARBALL));
    let mut ar = tar::Archive::new(gz);
    ar.unpack(dst).expect("untar FastLanes");
}

/// Git-generated tarballs have a single top-level dir (`FastLanes-<hash>`).
fn dive_one_level(dir: &Path) -> PathBuf {
    let mut it = fs::read_dir(dir).expect("read_dir").flatten();
    let first = it.next().expect("tar has at least one entry");
    if it.next().is_none() && first.file_type().unwrap().is_dir() {
        first.path()
    } else {
        dir.to_owned()
    }
}
