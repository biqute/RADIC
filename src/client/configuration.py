"""
Client-Server configuration file

Taken from qtics, I don't really know what it does and maybe it can be simplified
"""

import os

def from_env(name: str, default = None):
    """Get the value for an environment variable."""
    return os.getenv(f"Marcj_{name}", default)

address = "10.30.41.155" # Just a placeholder for now
#address = "0.0.0.0"
port = 7000 # Just a placeholder for now

HOST = from_env("HOST", address)
"""Server IP address"""

PORT = from_env("PORT", str(port))
"""Server port"""