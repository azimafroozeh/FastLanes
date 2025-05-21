// build.rs
//
// Self-contained FastLanes build: unpacks the embedded tarball when
// CMakeLists.txt isn’t found one level up (i.e. in a crates.io package),
// otherwise builds from the real repo root.

use std::{env, fs, io::Cursor, path::Path, path::PathBuf};

/// The embedded FastLanes sources as a .tar.gz
static FLS_TARBALL: &[u8] = include_bytes!("fastlanes-src.tar.gz");

fn main() {
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let out_dir   = PathBuf::from(env::var("OUT_DIR").unwrap());
    let unpack_dst = out_dir.join("fastlanes-src");

    // Decide where the CMake tree lives:
    // - if we're in the git repo, it's the parent of rust/
    // - otherwise unpack the embedded tarball
    let repo_root = crate_dir.parent().unwrap();
    let src_dir: PathBuf = if repo_root.join("CMakeLists.txt").exists() {
        repo_root.to_path_buf()
    } else {
        ensure_unpacked(&unpack_dst);
        unpack_dst
    };

    // Run CMake build+install (verbose)
    println!("cargo:warning=--- Running CMake build+install in {:?} ---", src_dir);
    let dst = cmake::Config::new(&src_dir)
        .define("CMAKE_VERBOSE_MAKEFILE", "ON")
        .define("BUILD_SHARED_LIBS", "OFF")
        .define("BUILD_TESTS", "OFF")
        .define("BUILD_EXAMPLES", "OFF")
        .profile("Release")
        .build_target("install")
        .build();
    println!("cargo:warning=--- CMake done, install prefix is {:?} ---", dst);

    let include_dir = dst.join("include");
    let lib_dir     = dst.join("lib");

    // Build the CXX bridge
    cxx_build::bridge("src/lib.rs")
        .include(&include_dir)
        .include(&crate_dir)
        .file("bridge_shim.cpp")
        .flag_if_supported("-std=c++20")
        .compile("fastlanes_version");

    // Link static libs
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=static=FastLanes");
    println!("cargo:rustc-link-lib=static=ALP");
    println!("cargo:rustc-link-lib=static=primitives");
    println!("cargo:rustc-link-lib=c++");

    // Re-run triggers
    println!("cargo:rerun-if-changed=fastlanes-src.tar.gz");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge_shim.cpp");
}

fn ensure_unpacked(dst: &Path) {
    if dst.exists() {
        return;
    }
    println!("cargo:warning=Unpacking embedded FastLanes sources to {:?}", dst);
    fs::create_dir_all(dst).expect("create unpack dir");

    // Decompress gzip + untar
    let cursor = Cursor::new(FLS_TARBALL);
    let gz = flate2::read::GzDecoder::new(cursor);
    let mut archive = tar::Archive::new(gz);
    archive.unpack(dst).expect("unpack FastLanes tarball");
}
