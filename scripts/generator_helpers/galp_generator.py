"""
Module: scripts/generate_helpers/galp_generator.py
Description: Generates synthetic FLOAT(-like) data values—e.g. 10.23, 24.46—and
             writes them to CSV files with accompanying schema definitions.
"""

import random
from decimal import Decimal, ROUND_HALF_UP
from pathlib import Path
import json

from .write_helpers import *  # write_csv(...)
from .common import *         # ROW_GROUP_SIZE, VEC_SIZE


# ----------------------------------------------------------------------
# 1) Wrap generate_galp() to match other “generate_fls_*” signatures
# ----------------------------------------------------------------------
def generate_fls_galp(_faker, row_id):
    """Generates a list containing a single float-like (two-decimal) value."""
    return generate_galp(_faker, row_id)


# ----------------------------------------------------------------------
# 2) Writer for the GALP column (with schema.json)
# ----------------------------------------------------------------------
def fls_galp():
    """
    Write a single-column CSV of FLOAT values (two decimal places).
    Creates two outputs, each with a schema.json alongside generated.csv:
      - data/generated/single_columns/galp/generated.csv   (ROW_GROUP_SIZE rows)
      - data/generated/one_vector/galp/generated.csv       (VEC_SIZE rows)
    """

    # Directory for the “rowgroup” version
    dir_rowgroup = Path.cwd() / 'data' / 'generated' / 'galp' / 'single_column'
    write_csv(dir_rowgroup, generate_fls_galp, ROW_GROUP_SIZE)

    # Write schema.json in the same folder with trailing newline
    schema_rg = {
        "columns": [
            {"name": "SYNTHETIC_DATA_GALP", "type": "FLOAT"}
        ]
    }
    (dir_rowgroup / 'schema.json').write_text(json.dumps(schema_rg, indent=2) + "\n")

    # Directory for the “one_vector” (VEC_SIZE rows) version
    dir_onevec = Path.cwd() / 'data' / 'generated' / 'galp' / 'one_vector'
    write_csv(dir_onevec, generate_fls_galp, VEC_SIZE)

    # Write schema.json in that folder as well with trailing newline
    schema_ov = {
        "columns": [
            {"name": "SYNTHETIC_DATA_GALP", "type": "FLOAT"}
        ]
    }
    (dir_onevec / 'schema.json').write_text(json.dumps(schema_ov, indent=2) + "\n")


# ----------------------------------------------------------------------
# Core galp generator
# ----------------------------------------------------------------------
def generate_galp(_faker, row_id, *, low: float = 0.0, high: float = 100.0):
    """
    Generates a single random decimal number in [low, high] with exactly
    two digits after the decimal point. Returned as a one-element list so it
    works seamlessly with write_csv(...).
    """
    raw = random.uniform(low, high)
    # Round to 2 decimal places using Decimal so CSVs stay nicely formatted
    value = float(Decimal(str(raw)).quantize(Decimal("0.01"), rounding=ROUND_HALF_UP))
    return [value]
