#!/usr/bin/env python3
"""
analyze.py

Hardcoded to analyze '5547758_eea9edfd54_n_rec_dct.csv'.
Casts each numeric column to int16 (smallint) and computes:
  - per-column min / max (and flags if OUT_OF_RANGE)
  - per-column percentage of zeros
  - per-column number of unique values
Also computes, across ALL numeric entries:
  - total number of values
  - count and % of zeros
  - count and % of non-zeros
  - global count of unique values (+ percentage)
  - global min / max
  - ceil(log2(global_max − global_min))
  - estimated compressed size (bytes) = nonzeros * range_bits / 8
Prints a “Global summary” as a two-column table (with calculation details embedded), then the per-column table.
Assumes pipe-delimited input ("|") and assigns proper column names
even if the file lacks a header.
"""

import pandas as pd
import numpy as np
import math
import sys


def print_table(df: pd.DataFrame):
    """Pretty-print a DataFrame as an ASCII table with borders."""
    cols = df.columns.tolist()
    widths = {col: max(df[col].astype(str).map(len).max(), len(col))
              for col in cols}
    sep = '+' + '+'.join('-' * (widths[col] + 2) for col in cols) + '+'
    header = '|' + '|'.join(f' {col.ljust(widths[col])} ' for col in cols) + '|'

    print(sep)
    print(header)
    print(sep)
    for _, row in df.iterrows():
        line = '|' + '|'.join(f' {str(row[col]).ljust(widths[col])} '
                              for col in cols) + '|'
        print(line)
        print(sep)


def analyze_csv(path: str):
    try:
        df = pd.read_csv(path, sep='|', header=None)
    except Exception as e:
        print(f"Error reading '{path}': {e}")
        return

    if df.shape[1] < 3:
        print(f"Unexpected number of columns: {df.shape[1]}")
        return

    # Assign column names
    cols = ['component', 'block_row', 'block_col'] + [
        f'COL{k}' for k in range(df.shape[1] - 3)
    ]
    df.columns = cols

    # Select only numeric columns
    numeric_df = df.select_dtypes(include=[np.number])

    # Global stats
    total_numeric = numeric_df.size
    zero_count = (numeric_df == 0).sum().sum()
    nonzero_count = total_numeric - zero_count
    zero_pct = zero_count / total_numeric * 100
    nonzero_pct = nonzero_count / total_numeric * 100

    global_min = numeric_df.min().min()
    global_max = numeric_df.max().max()

    global_unique = pd.unique(numeric_df.values.ravel()).size
    unique_pct = global_unique / total_numeric * 100

    # Compute bits needed to represent the range
    range_val = int(global_max - global_min)
    bits_needed = math.ceil(math.log2(range_val)) if range_val > 0 else 0

    # Estimate compressed size in bytes
    est_bytes = nonzero_count * bits_needed / 8

    # Build a columnar global summary with calculation
    stats = [
        ('total', total_numeric),
        ('zeros', zero_count),
        ('zero %', f"{zero_pct:.2f}"),
        ('nonzeros', nonzero_count),
        ('nonzero %', f"{nonzero_pct:.2f}"),
        ('unique', global_unique),
        ('unique %', f"{unique_pct:.2f}"),
        ('global_min', int(global_min)),
        ('global_max', int(global_max)),
        ('range', range_val),
        ('range_bits', bits_needed),
        ('est_bytes', f"{est_bytes:.2f}"),
        ('est_calc', f"{nonzero_count}×{bits_needed}/8 = {est_bytes:.2f}"),
    ]
    global_summary = pd.DataFrame(stats, columns=['statistic', 'value'])

    print("\nGlobal summary:")
    print_table(global_summary)

    # Per-column summary
    summary = []
    int16_min = np.iinfo(np.int16).min
    int16_max = np.iinfo(np.int16).max

    for col in cols:
        rec = {'column': col}
        series = df[col]
        unique_count = series.nunique()

        if not pd.api.types.is_numeric_dtype(series):
            rec.update(
                status='non-numeric',
                min='',
                max='',
                **{'zero %': ''},
                unique=unique_count
            )
        else:
            min_val = series.min()
            max_val = series.max()
            zero_pct = (series == 0).sum() / len(series) * 100
            status = ('OUT_OF_RANGE'
                      if (min_val < int16_min or max_val > int16_max)
                      else 'OK')

            rec.update(
                status=status,
                min=int(min_val),
                max=int(max_val),
                **{'zero %': f"{zero_pct:.2f}"},
                unique=unique_count
            )
        summary.append(rec)

    summary_df = pd.DataFrame(
        summary, columns=['column', 'status', 'min', 'max', 'zero %', 'unique']
    )
    print("\nPer-column summary:")
    print_table(summary_df)


if __name__ == "__main__":
    csv_path = "raw64/5547758_eea9edfd54_n.csv"
    analyze_csv(csv_path)
