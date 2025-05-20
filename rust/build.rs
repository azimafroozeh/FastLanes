// build.rs
use std::{env, path::PathBuf};

fn main() {
    // ────────────────────────────────────────────────
    // 1) Build (or find) FastLanes locally
    // ────────────────────────────────────────────────
    // Allow the caller to short-circuit and supply a pre-built prefix.
    let prefix: PathBuf = if let Ok(prefix) = env::var("FASTLANES_INSTALL_PREFIX") {
        PathBuf::from(prefix)
    } else {
        // Build from source (default: vendor/FastLanes) with CMake.
        let src = env::var("FASTLANES_SRC_DIR")
            .map(PathBuf::from)
            .unwrap_or_else(|_| PathBuf::from(".."));

        let dst = cmake::Config::new(&src)
            .define("BUILD_SHARED_LIBS", "OFF")          // static libs make linking simpler
            .define("BUILD_TESTS", "OFF")
            .define("BUILD_EXAMPLES", "OFF")
            .profile("Release")
            .build();                                    // <- runs cmake & ninja/make

        dst
    };

    let include_dir = prefix.join("include");
    let lib_dir     = prefix.join("lib");

    // ────────────────────────────────────────────────
    // 2) Generate the cxx bridge & build the C++ shim
    // ────────────────────────────────────────────────
    cxx_build::bridge("src/lib.rs")
        .include(&include_dir)                           // FastLanes headers
        .include(env::var("CARGO_MANIFEST_DIR").unwrap())// your own headers
        .file("bridge_shim.cpp")                         // implements get_version()
        .flag_if_supported("-std=c++20")
        .compile("fastlanes_version");

    // ────────────────────────────────────────────────
    // 3) Tell rustc where to find & link the libraries
    // ────────────────────────────────────────────────
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-lib=static=FastLanes");
    println!("cargo:rustc-link-lib=static=ALP");
    println!("cargo:rustc-link-lib=static=primitives");
    println!("cargo:rustc-link-lib=c++");

    // ────────────────────────────────────────────────
    // 4) Re-run rules
    // ────────────────────────────────────────────────
    println!("cargo:rerun-if-env-changed=FASTLANES_INSTALL_PREFIX");
    println!("cargo:rerun-if-env-changed=FASTLANES_SRC_DIR");
    println!("cargo:rerun-if-changed=vendor/FastLanes"); // rebuild if vendored tree changes
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge_shim.cpp");
}
