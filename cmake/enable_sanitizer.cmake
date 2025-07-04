# --- cmake/enable_sanitizer.cmake ---
#
# Defines fls_enable_sanitizers(<target>) to apply
# AddressSanitizer, UBSan, and related checks on all builds
# *except* fully‐static macOS executables, which are unsupported.
#
# RATIONALE:
#   • ASan on Apple platforms is provided only as a dynamic
#     runtime (libclang_rt.asan_osx_dynamic.dylib).
#   • Clang docs state: “Static linking of executables is
#     not supported.”
#   • On macOS-14 (Sonoma) & macOS-15 (Ventura), a truly
#     static binary with -fsanitize=address will abort at
#     startup (you see the shadow-byte legend then “ABORTING”).
#   • Linux “static” builds still load the dynamic libasan,
#     so only macOS static+ASan fails.
#
# IMPACT ON CI:
#   OS         | Shared        | Static
#   -----------------------------------------
#   Ubuntu     | ✅ ASan       | ✅ ASan
#   macOS-14   | ✅ ASan       | ❌ Abort at startup
#   macOS-15   | ✅ ASan       | ❌ Abort at startup
#

function(fls_enable_sanitizers target)
    # 1) Skip on static macOS
    if (APPLE AND NOT FLS_BUILD_SHARED_LIBS)
        message(STATUS
                "[Sanitizers] Skipping sanitizers for '${target}' "
                "(static build on macOS is unsupported by ASan)")
        return()
    endif ()

    # 2) Apply on all other platforms
    if (NOT WIN32)
        # unified list of sanitizers
        set(_san_flags
                -fsanitize=address
                -fsanitize=undefined
                -fsanitize=vptr
                -fsanitize=function
                -fsanitize=null
                -fno-sanitize-recover=all
                -fno-omit-frame-pointer
        )

        # Log exactly what flags we’re about to add
        string(REPLACE ";" " " _san_flags_str "${_san_flags}")
        message(STATUS
                "[Sanitizers] Enabling sanitizers for '${target}': ${_san_flags_str}")

        target_compile_options(${target} PRIVATE ${_san_flags})
        target_link_options(${target} PRIVATE ${_san_flags})
    endif ()
endfunction()
