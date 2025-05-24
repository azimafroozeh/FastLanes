//! Self-contained FastLanes builder + Git-tag version emitter
use std::{
    env, fs,
    io::Cursor,
    path::{Path, PathBuf},
};

use flate2::read::GzDecoder;
use tar::Archive;
use vergen::Emitter;
use vergen_gix::GixBuilder;

const FLS_TARBALL: &[u8] = include_bytes!("fastlanes-src.tar.gz");

fn main() -> Result<(), Box<dyn std::error::Error>> {
    /* ── 1) Git-derived build info ─────────────────────────────── */
    println!("cargo:rerun-if-changed=.git/HEAD");
    println!("cargo:rerun-if-changed=.git/refs/tags");

    // emit all VERGEN_GIT_* variables
    let git = GixBuilder::all_git()?;          // -> Instructions
    Emitter::default()
        .add_instructions(&git)?
        .emit()?;                              // writes the cargo: directives

    /* ── 2) Locate the C++ sources ─────────────────────────────── */
    let crate_dir  = PathBuf::from(env::var("CARGO_MANIFEST_DIR")?);
    let out_dir    = PathBuf::from(env::var("OUT_DIR")?);
    let unpack_dst = out_dir.join("fastlanes-src");

    let repo_root = crate_dir.parent().expect("rust/ has a parent");
    let src_dir: PathBuf = if repo_root.join("CMakeLists.txt").exists() {
        repo_root.into()                 // workspace checkout
    } else {
        ensure_unpacked(&unpack_dst)?;
        dive_one_level(&unpack_dst)
    };

    println!("cargo:warning=--- Running CMake in {:?} ---", src_dir);

    let install = cmake::Config::new(&src_dir)
        .define("CMAKE_VERBOSE_MAKEFILE", "ON")
        .profile("Release")
        .build_target("install")
        .build();

    let include_dir = install.join("include");
    let lib_dir     = install.join("lib");

    /* ── 3) Build the CXX bridge ───────────────────────────────── */
    cxx_build::bridge("src/lib.rs")
        .include(&include_dir)
        .include(&crate_dir)
        .file("bridge_shim.cpp")
        .flag_if_supported("-std=c++20")
        .flag_if_supported("-Wno-changes-meaning")
        .flag_if_supported("-Wno-error=unknown-warning-option")
        .compile("fastlanes_rs");

    /* ── 4) Link the static FastLanes library ──────────────────── */
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=static=FastLanes");
    println!("cargo:rustc-link-lib=c++");

    /* ── 5) Re-run triggers for local files ────────────────────── */
    println!("cargo:rerun-if-changed=fastlanes-src.tar.gz");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=bridge_shim.cpp");

    Ok(())
}

/* ------------------------------------------------------------------- */
fn ensure_unpacked(dst: &Path) -> Result<(), Box<dyn std::error::Error>> {
    if dst.exists() {
        return Ok(());
    }
    println!("cargo:warning=Unpacking FastLanes sources to {:?}", dst);

    fs::create_dir_all(dst)?;
    let gz = GzDecoder::new(Cursor::new(FLS_TARBALL));
    Archive::new(gz).unpack(dst)?;
    Ok(())
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
