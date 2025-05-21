// build.rs
use std::{env, path::PathBuf};

fn main() {
    // ────────────────────────────────────────────────
    // 1) Locate FastLanes source (parent of `rust/` by default)
    // ────────────────────────────────────────────────
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let src_dir = env::var("FASTLANES_SRC_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| {
            crate_dir
                .parent()
                .expect("crate must live inside the FastLanes repo")
                .to_path_buf()
        });

    // ────────────────────────────────────────────────
    // 2) Configure, build & install via CMake
    // ────────────────────────────────────────────────
    println!("cargo:warning=--- Running CMake build+install in {:?} ---", src_dir);
    let dst = cmake::Config::new(&src_dir)
        // verbose Makefiles so you see each compile/link
        .define("CMAKE_VERBOSE_MAKEFILE", "ON")
        // static libs only
        .define("BUILD_SHARED_LIBS", "OFF")
        .define("BUILD_TESTS", "OFF")
        .define("BUILD_EXAMPLES", "OFF")
        .profile("Release")
        // this makes `cmake --build <out> --target install`
        .build_target("install")
        .build();
    println!("cargo:warning=--- CMake done, install prefix is {:?} ---", dst);

    // ────────────────────────────────────────────────
    // 3) Compute include & lib dirs under the install prefix
    // ────────────────────────────────────────────────
    let install_include = dst.join("include");
    let install_lib     = dst.join("lib");

    // ────────────────────────────────────────────────
    // 4) Generate the cxx bridge & compile the C++ shim
    // ────────────────────────────────────────────────
    cxx_build::bridge("src/lib.rs")
        .include(&install_include)          // headers from CMake install
        .include(&crate_dir)                // your crate’s own headers
        .file("bridge_shim.cpp")            // implements get_version()
        .flag_if_supported("-std=c++20")
        .compile("fastlanes_version");

    // ────────────────────────────────────────────────
    // 5) Tell rustc where to find & link the FastLanes libs
    // ────────────────────────────────────────────────
    println!("cargo:rustc-link-search=native={}", install_lib.display());
    println!("cargo:rustc-link-lib=static=FastLanes");
    println!("cargo:rustc-link-lib=static=ALP");
    println!("cargo:rustc-link-lib=static=primitives");
    // link the C++ stdlib
    println!("cargo:rustc-link-lib=c++");

    // ────────────────────────────────────────────────
    // 6) Re-run this script if anything important changes
    // ────────────────────────────────────────────────
    println!("cargo:rerun-if-env-changed=FASTLANES_INSTALL_PREFIX");
    println!("cargo:rerun-if-env-changed=FASTLANES_SRC_DIR");
    println!("cargo:rerun-if-changed=CMakeLists.txt");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge_shim.cpp");
}
