"""
Server that runs on the Raspberry Pi.

TO IMPLEMENT: error correction
"""

import json
import socket
from typing import Union
import re
import configuration as cfg
from logger import Logger
import marcj as mj
from marcj import mylogger

####### Translate from SCPI command to functions used to communicate with the board ########

def return_idn():
    return str(cfg.HOST)

default_cmd = {
    "*IDN?": return_idn
}

measure_units = [
    "Hz", 
    "V",
    "COUN"
]

commands = {
    "SOUR": {
        "FREQ": mj.set_frequency,
        "LEN": mj.set_duration,
        "VOLT": mj.set_amplitude,
        "FUNC": mj.set_waveform,
        "OFFSET": mj.set_offset
    },
    "SENS": {
        "AVER": mj.set_nmeans
    },
    "INIT": {
        "CONT": mj.play_type,
        "LIM": mj.play_type
    },
    "FETC":
        mj.read_data,
    "OUTPUT": 
        mj.stop_play
}

def check_default_cmd(opt: Union[str, int], allowed: tuple) -> bool:
    """Check if provided command is among the 'deafult' ones. Options are:\n
    - IDN -> ask the identity of the instrument"""
    if opt in allowed:
        return True
    return False

def validate_opt(opt: Union[str, int], allowed: tuple):
    """Check if provided option is between allowed ones."""
    if opt not in allowed:
        raise RuntimeError(f"Invalid option provided, choose between {allowed}")
    
def find_query(cmd: str) -> bool:
    return re.search(".\?$", cmd)

def find_digits(cmd: str) -> bool:
    return re.search(r"\d+(\.\d*)?", cmd[-1])

def remove_units(cmd: str) -> str:
    """Removes question mark and measurement unit indication from the command to obtain just a numerical value (still contained in a text string)"""
    cmd = cmd.replace("?", "")
    for unit in measure_units:
        cmd = cmd.replace(unit, "")
    return cmd

def write_commands(cmd: tuple, value: str):
    return commands[cmd[0]][cmd[1]](value)

def send_value_back(cmd: str, *value):
    if cmd == 'FETC':
        """We want to send back a dictionary with the two vectors"""
        ch0, ch1 = mj.data_analysis()
        my_dict = {"ch0": ch0, "ch1": ch1}
        return my_dict
    return {cmd: value}


def decode_string(cmd: str):
    splitted = re.split(":", cmd)
    mylogger.info(splitted)

    """Check if command is among the \"default\" ones (e.g. IDN?)"""
    if check_default_cmd(splitted[0], default_cmd):
        return default_cmd[splitted[0]]()

    """value will contain the numerical value of a certain setting"""
    value = remove_units(splitted[-1])

    """Check if SCPI command received is valid"""
    validate_opt(splitted[0], commands) and validate_opt(splitted[1], commands[splitted[0]])

    """Find out if SCPI command recevide is a query (basically everything will be a query and for now is the only thing that works)"""
    if find_query(splitted[-1]):
        mylogger.info("The command just received is a query")

        if splitted[0] == "INIT":
            """This means we want to play data"""
            splitted[-1] = splitted[-1].replace("?", "")
            if splitted[1] == "CONT":
                commands[splitted[0]][splitted[1]]('c')
                mj.play()
            else:
                commands[splitted[0]][splitted[1]]('l')
                mj.play()
            return send_value_back(splitted[0], "Playing the data")
        
        if splitted[0] == 'FETC':
            """This means we want to read data"""
            value = splitted[-1].replace("?", "")
            commands[splitted[0]](value)
            return send_value_back(cmd = splitted[0])
        
        if splitted[0] == "OUTPUT":
            mylogger.info("Sending the stop streaming data")
            value = commands[splitted[0]]()
            return send_value_back(splitted[0], value)

        a = write_commands(splitted, value)
        return send_value_back(splitted[0], a)
    return

#################################################################################################
        
def open_server(server_ip, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.bind((server_ip, port))
        server.listen() 
        while True:
            client_socket, client_address = server.accept()

            with client_socket:
                mylogger.info(f"Connected by {client_address}")
                while True:
                    data = client_socket.recv(1024)
                    if not data:
                        break

                    """Encoding of the data to be sent back to the client"""
                    data = data.decode()
                    result = decode_string(data)
                    data = json.dumps(result)
                    data = data.encode("utf-8")
                    dim = len(data)

                    """Sending the data back to the client"""
                    client_socket.sendall(dim.to_bytes(4, "little"))                
                    client_socket.sendall(data)            