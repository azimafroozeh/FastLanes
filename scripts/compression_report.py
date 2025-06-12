#!/usr/bin/env python3
import csv
import argparse


def load_data(path):
    rows = []
    with open(path, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            row['file_size'] = int(row['file_size'])
            rows.append(row)
    return rows


def main():
    parser = argparse.ArgumentParser(
        description="Generate a Markdown compression-regression report"
    )
    parser.add_argument(
        "--baseline", required=True,
        help="Path to the CSV from the dev baseline run"
    )
    parser.add_argument(
        "--current", required=True,
        help="Path to the CSV from the current run"
    )
    parser.add_argument(
        "--out", default="report.md",
        help="Output Markdown file (default: report.md)"
    )
    args = parser.parse_args()

    base_rows = load_data(args.baseline)
    curr_rows = load_data(args.current)

    # Sum totals
    total_base = sum(r['file_size'] for r in base_rows)
    total_curr = sum(r['file_size'] for r in curr_rows)
    pct_total = (
        (total_curr - total_base) / total_base * 100
        if total_base else 0.0
    )

    # Map table -> size
    base_map = {r['table_name']: r['file_size'] for r in base_rows}
    curr_map = {r['table_name']: r['file_size'] for r in curr_rows}
    tables = sorted(set(base_map) | set(curr_map))

    # Build Markdown
    lines = []
    lines.append("# Compression Regression Report\n")
    lines.append(f"**Dev total:**  `{total_base}` bytes  ")
    lines.append(f"**Current total:**  `{total_curr}` bytes  ")
    lines.append(f"**Total change:**  `{pct_total:.2f}%`  \n")
    lines.append("| table_name | dev (bytes) | current (bytes) | % change |")
    lines.append("|------------|-------------|-----------------|----------|")
    for table in tables:
        b = base_map.get(table, 0)
        c = curr_map.get(table, 0)
        pct = ((c - b) / b * 100) if b else 0.0
        lines.append(f"| {table} | {b} | {c} | {pct:.2f}% |")

    # Write out
    with open(args.out, "w") as f:
        f.write("\n".join(lines))


if __name__ == "__main__":
    main()
