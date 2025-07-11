#!/usr/bin/env python3
"""
generate_binaries.py
────────────────────
Creates 25 600 × 1 024 = 26 214 400 random values (two decimals, 0.00–99.99)
and streams them to *two* raw binary files:

    floats/1.bin   – 32-bit IEEE-754 big-endian floats
    doubles/1.bin  – 64-bit IEEE-754 big-endian doubles
"""

from pathlib import Path
from time import perf_counter
import random
import struct

# ---------------------------------------------------------------------
TOTAL   = 25_600 * 1_024          # 26 214 400 numbers
CHUNK   = 100_000                 # how many numbers to buffer before writing

OUT_F32 = Path("floats/1.bin")    # 32-bit output
OUT_F64 = Path("doubles/1.bin")   # 64-bit output
# ---------------------------------------------------------------------


def main() -> None:
    print(f"Generating {TOTAL:,} numbers →")
    print(f"  • {OUT_F32.resolve()}  (float32, {TOTAL*4/1024/1024:.1f} MiB)")
    print(f"  • {OUT_F64.resolve()}  (float64, {TOTAL*8/1024/1024:.1f} MiB)")

    t0      = perf_counter()
    pack_f  = struct.pack          # alias: >f
    pack_d  = struct.pack          # alias: >d
    buf_f32 = bytearray()
    buf_f64 = bytearray()

    # Ensure parent directories exist
    OUT_F32.parent.mkdir(parents=True, exist_ok=True)
    OUT_F64.parent.mkdir(parents=True, exist_ok=True)

    with OUT_F32.open("wb") as fp32, OUT_F64.open("wb") as fp64:
        for i in range(1, TOTAL + 1):
            val = round(random.uniform(0, 100), 2)

            buf_f32.extend(pack_f(">f", val))   # 32-bit
            buf_f64.extend(pack_d(">d", val))   # 64-bit

            if i % CHUNK == 0:
                fp32.write(buf_f32); buf_f32.clear()
                fp64.write(buf_f64); buf_f64.clear()

                if i % (CHUNK * 10) == 0:
                    print(f"  …{i / TOTAL * 100:5.1f}%")

        # final partial flush
        if buf_f32:
            fp32.write(buf_f32)
            fp64.write(buf_f64)

    dt = perf_counter() - t0
    print(f"Done in {dt:,.1f}s "
          f"({TOTAL/dt:,.0f} numbers/s total, "
          f"{TOTAL*12/dt/1024/1024:,.1f} MiB/s combined I/O)")

if __name__ == "__main__":
    main()
