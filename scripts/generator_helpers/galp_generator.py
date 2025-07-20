"""
Module: scripts/generate_helpers/galp_generator.py
Description: For two FLOAT columns—one random two‑decimal GALP and one constant 1.00—
             writes each as its own single‑column CSV + schema.json, in both
             ROW_GROUP_SIZE and VEC_SIZE variants.
"""

import random
from decimal import Decimal, ROUND_HALF_UP
from pathlib import Path
import json

from .write_helpers import write_csv
from .common import ROW_GROUP_SIZE, VEC_SIZE


def generate_fls_galp(_faker, row_id):
    """Random FLOAT in [0.0,100.0], two decimals."""
    raw = random.uniform(0.0, 100.0)
    galp_val = float(
        Decimal(str(raw))
        .quantize(Decimal("0.01"), rounding=ROUND_HALF_UP)
    )
    return [galp_val]


def generate_fls_constant(_faker, row_id):
    """Constant FLOAT = 1.00."""
    return [1.00]


def fls_galp():
    """
    For each of two fields—GALP and CONSTANT—write:

      data/generated/galp/<field>/single_column/generated.csv   (ROW_GROUP_SIZE rows)
      data/generated/galp/<field>/single_column/schema.json

      data/generated/galp/<field>/one_vector/generated.csv     (VEC_SIZE rows)
      data/generated/galp/<field>/one_vector/schema.json
    """
    base_dir = Path.cwd() / 'data' / 'generated' / 'galp'

    tasks = [
        ('galp', 'SYNTHETIC_DATA_GALP', generate_fls_galp),
        ('constant', 'SYNTHETIC_DATA_CONSTANT', generate_fls_constant),
    ]

    for folder_key, column_name, gen_fn in tasks:
        # 1) Row‑group version
        dir_rg = base_dir / folder_key / 'single_column'
        write_csv(dir_rg, gen_fn, ROW_GROUP_SIZE)
        schema = {"columns": [{"name": column_name, "type": "FLOAT"}]}
        (dir_rg / 'schema.json').write_text(json.dumps(schema, indent=2) + "\n")

        # 2) One‑vector version
        dir_ov = base_dir / folder_key / 'one_vector'
        write_csv(dir_ov, gen_fn, VEC_SIZE)
        (dir_ov / 'schema.json').write_text(json.dumps(schema, indent=2) + "\n")
