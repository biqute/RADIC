"""
Main program. It runs at startup and opens the server.
"""

import sys
import configuration as cfg
from server import open_server, mylogger

try:
    mylogger.info(f"Server starting at port {cfg.PORT} and IP {cfg.HOST}")
    open_server(cfg.HOST, cfg.PORT)
except KeyboardInterrupt:
    mylogger.info("Server closed by Keyboard combination")
    sys.exit(0)