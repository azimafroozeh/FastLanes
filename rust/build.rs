//! Self-contained FastLanes builder

use std::{
    env, fs,
    io::Cursor,
    path::{Path, PathBuf},
};

const FLS_TARBALL: &[u8] = include_bytes!(concat!(
    env!("CARGO_MANIFEST_DIR"),
    "/target/fastlanes-src.tar.gz"
));

fn main() {
    // --- figure out where the CMakeLists.txt lives ----------------------------
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let unpack_dst = out_dir.join("fastlanes-src");

    let repo_root = crate_dir.parent().expect("rust/ has a parent");
    let src_dir: PathBuf = if repo_root.join("CMakeLists.txt").exists() {
        repo_root.into() // dev/CI build
    } else {
        ensure_unpacked(&unpack_dst); // crates.io build
        dive_one_level(&unpack_dst) // inside git-archive top dir
    };

    // --- CMake build + install -----------------------------------------------
    println!(
        "cargo:warning=--- Running CMake build+install in {:?} ---",
        src_dir
    );

    let install_prefix = cmake::Config::new(&src_dir)
        .define("CMAKE_VERBOSE_MAKEFILE", "ON")
        .profile("Release")
        .build_target("install")
        .build();

    println!(
        "cargo:warning=--- CMake done, install prefix is {:?} ---",
        install_prefix
    );

    let include_dir = install_prefix.join("include");
    let lib_dir = install_prefix.join("lib");

    // --- CXX bridge -----------------------------------------------------------
    cxx_build::bridge("src/lib.rs")
        .include(&include_dir)
        .include(&crate_dir)
        .file("bridge_shim.cpp")
        .flag_if_supported("-std=c++20")
        .compile("fastlanes_version");

    // --- Linkage --------------------------------------------------------------
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=static=FastLanes");
    println!("cargo:rustc-link-lib=c++");

    // --- Re-run triggers ------------------------------------------------------
    println!("cargo:rerun-if-changed=fastlanes-src.tar.gz");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=bridge_shim.cpp");
}

/// unpack the embedded tar-ball once
fn ensure_unpacked(dst: &Path) {
    if dst.exists() {
        return;
    }
    println!(
        "cargo:warning=Unpacking embedded FastLanes sources to {:?}",
        dst
    );

    fs::create_dir_all(dst).expect("create unpack dir");
    let gz = flate2::read::GzDecoder::new(Cursor::new(FLS_TARBALL));
    let mut ar = tar::Archive::new(gz);
    ar.unpack(dst).expect("untar FastLanes");
}

/// git-generated tar-balls contain a single top-level directory: `FastLanes-<hash>/`
fn dive_one_level(dir: &Path) -> PathBuf {
    let mut it = fs::read_dir(dir).expect("read_dir").flatten();
    let first = it.next().expect("tar has at least one entry");
    if it.next().is_none() && first.file_type().unwrap().is_dir() {
        first.path()
    } else {
        dir.to_owned()
    }
}
