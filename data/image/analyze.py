#!/usr/bin/env python3
"""
analyze.py

Hardcoded to analyze '5547758_eea9edfd54_n_rec_dct.csv'.
Casts each numeric column to int16 (smallint) and computes:
  - min / max (and flags if OUT_OF_RANGE)
  - percentage of zeros
  - number of unique values
Prints all results in a single ASCII table with borders.
Assumes pipe-delimited input ("|") and assigns proper column names even if the file lacks a header.
"""

import pandas as pd
import numpy as np
import sys


def print_table(df: pd.DataFrame):
    # Determine column widths
    cols = df.columns.tolist()
    widths = {
        col: max(df[col].astype(str).map(len).max(), len(col))
        for col in cols
    }

    # Horizontal separator line
    sep = '+' + '+'.join('-' * (widths[col] + 2) for col in cols) + '+'

    # Header row
    header = '|' + '|'.join(f' {col.ljust(widths[col])} ' for col in cols) + '|'

    print(sep)
    print(header)
    print(sep)

    # Data rows
    for _, row in df.iterrows():
        line = '|' + '|'.join(f' {str(row[col]).ljust(widths[col])} ' for col in cols) + '|'
        print(line)
        print(sep)


def analyze_csv(path: str):
    try:
        df = pd.read_csv(path, sep='|', header=None)
    except Exception as e:
        print(f"Error reading '{path}': {e}")
        return

    num_cols = df.shape[1]
    if num_cols < 3:
        print(f"Unexpected number of columns: {num_cols}")
        return

    # Assign column names
    cols = ['component', 'block_row', 'block_col'] + [f'COL{k}' for k in range(num_cols - 3)]
    df.columns = cols

    summary = []
    int16_min = np.iinfo(np.int16).min
    int16_max = np.iinfo(np.int16).max

    for col in cols:
        rec = {'column': col}
        series = df[col]

        # Count unique values for all columns
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
            status = 'OUT_OF_RANGE' if (min_val < int16_min or max_val > int16_max) else 'OK'

            rec.update(
                status=status,
                min=int(min_val),
                max=int(max_val),
                **{'zero %': f"{zero_pct:.2f}"},
                unique=unique_count
            )

        summary.append(rec)

    # Build and print the summary table
    summary_df = pd.DataFrame(summary, columns=['column', 'status', 'min', 'max', 'zero %', 'unique'])
    print_table(summary_df)


if __name__ == "__main__":
    csv_path = "5547758_eea9edfd54_n_rec_dct.csv"
    analyze_csv(csv_path)
