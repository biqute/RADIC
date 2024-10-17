"""
Functions for the Marcj client
"""

import ipaddress
import socket
import time
import json
from typing import Union
from logger import Logger
import logging
import re

mylogger = Logger().logger

class Client():
    """Class to communicate with the server"""

    def __init__(
        self,
        name: str, 
        address: str, 
        port: int
    ):
        ipaddress.ip_address(address)
        self.name = name
        self.address = address
        self.port = port
        self.timeout = 10
        self.no_delay = True
        self.sleep = 0.1
        self.__is_connected = False
        self.socket = None

    def connect(self):
        """Connect to the device"""
        if not self.__is_connected:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(self.timeout)
            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, int(self.no_delay))
            self.socket.connect((self.address, int(self.port)))
            self.socket.setblocking(True)
            self.__is_connected = True
            mylogger.info(f"Instrument {self.name} connected")
        else:
            mylogger.info(f"Instrument {self.name} already connected.")

    def disconnect(self):
        """Disconnect the device"""
        if self.__is_connected:
            self.socket.shutdown(socket.SHUT_RDWR)
            self.socket.close()
            self.__is_connected = False
            mylogger.info(f'Instrument {self.name} disconnected')
        else:
            mylogger.info(f'Instrument {self.name} is not connected')

    def write(self, cmd: str, sleep: bool = False):
        """Write a command to the instrument"""
        if self.socket is None:
            mylogger.warning('Socket not initialized')
            return
        self.socket.sendall(cmd.encode())
        if sleep:
            time.sleep(self.sleep)

    def read(self) -> dict:
        """Read a command from the instrument"""
        count = int.from_bytes(self.socket.recv(4), "little")
        received = self.socket.recv(count, socket.MSG_WAITALL)
        results = received.decode("utf-8")
        results = json.loads(results)
        return results

    def query(self, cmd: str) -> Union[dict, str]:
        """Query a command to the instrument.\n
        Returns a dictionary when we fetch the data from the device otherwise it returns a string."""
        if "?" not in cmd:
            mylogger.error("A query must include ?")
            raise ValueError('A query must include "?"')
        self.write(cmd, True)
        result = self.read()

        if re.split(":", cmd)[0] != "FETC":
            if re.split(":", cmd)[0] != "*IDN?":
                return self.return_command_value(re.split(":", cmd)[0], result)[0]

        return result

    def get_id(self) -> str:
        """Return name of the device from SCPI standard query."""
        return self.query("*IDN?")
    
    def return_command_value(self, cmd: str, result: dict) -> str:
        """Used to return just the value queried with a command.\n
        This is needed since the server sends back a dictionary which is not very convenient to work with."""
        return result[cmd]
    
    def validate_opt(self, opt: Union[str, int], allowed: tuple):
        """Check if provided option is between allowed ones."""
        if opt not in allowed:
            mylogger.error(f"Invalid option provided, choose between {allowed}")
            raise RuntimeError(f"Invalid option provided, choose between {allowed}")
        
    def validate_range(self, n: float, n_min: float, n_max: float) -> float:
        """Check if provided number is in allowed range."""
        if not n_min <= n <= n_max:
            valid = max(n_min, min(n_max, n))
            mylogger.warning(f"Provided value {n} not in range ({n_min}, {n_max}), will be set to {valid}.")
            return valid
        return n