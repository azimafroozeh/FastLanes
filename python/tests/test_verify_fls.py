"""Tests for verifying FastLanes files."""

from pathlib import Path

import pyfastlanes


def test_verify_valid_fls():
    valid_fls = Path(__file__).resolve().parents[2] / "data/test/verify_fastlanes_file_tests/valid.fls"
    assert pyfastlanes.verify_fls(str(valid_fls))

