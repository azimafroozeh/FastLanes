"""
PyFastLanes Python API
"""

from ._fastlanes import (
    get_version,
    Connection,
    connect,
    # Add any other bindings you expose here
)


def verify_fls(file_path: str) -> bool:
    """Convenience wrapper to verify a FastLanes file."""
    conn = connect()
    return conn.verify_fls(file_path)

__all__ = [
    "get_version",
    "Connection",
    "connect",
    "verify_fls",
    # Add others as needed
]
