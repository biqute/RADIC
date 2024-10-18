"""
This is the script to control the DAC+ADC board. 
 - Need to find a name for it beacuse I don't like "instrument.py".
 - Maybe I can implement some command for closing the server remotely 
        --> Something similar to close_audio.sh
        --> Don't know if it's doable or even useful.
"""

import configuration as cfg
from client import Client

class Marcj(Client):
    """Defines the commands to set the various board options."""

    def __init__(
        self,
        name: str,
        address: str = cfg.HOST,
        port: int = cfg.PORT
    ):
        """Initialize the instrument"""
        super().__init__(name, address, port)
        self.max_rate = 192000
        self.maxval = (2**24)/2 - 1

    def set_frequency(self, freq: float) -> dict:
        '''Sets the frequency f of the signal (in Hz).\n
        Returns the setted frequency also in Hz.'''
        self.validate_range(freq, 0, self.max_rate)
        return self.query(f'SOUR:FREQ:{freq}Hz?')
    
    def set_duration(self, time: float) -> dict:
        '''Sets the lenght of the wave (in seconds)'''
        return self.query(f'SOUR:LEN:{time}?')
    
    def set_wave(self, opt: str) -> dict:
        """Sets the type of wave to be generated.\n
        Valid options to query:\n
        - `SIN`: sinusoidal wave\n
        - `TRIA`: triangular wave\n
        - `SQUA`: square wave\n
        - `CONST`: constant wave"""
        self.validate_opt(opt, ('SIN', 'TRIA', 'SQUA', 'CONST'))
        return self.query('SOUR:FUNC:' + str(opt) + '?')
    
    def set_amplitude(self, ampl: float) -> dict:
        '''Sets the maximum voltage of the wave (in Volts).'''
        return self.query(f'SOUR:VOLT:{ampl}V?')
    
    def set_offset(self, offset: float) -> dict:
        """Sets the offset of the signal in Volts"""
        return self.query(f"SOUR:OFFSET:{offset}V?")
    
    def play(self, opt: str = 'CONT') -> dict:
        """Initiate the stream of data with selected mode\n
        Valid options to query:\n
        - `CONT`: continuous writing of the data\n
        - `LIM`: time limited writing of the data"""
        self.validate_opt(opt, ("CONT", "LIM"))
        return self.query("INIT:" + str(opt) + "?")
    
    def read_signal(self, time: float) -> dict:
        """Read data from the two channels for approximately the specified amount of time (in seconds).\n
        It returns a dictionary containing two arrays, each for one of the channels."""
        return self.query(f'FETC:{time}?')
    
    def stop_signal(self) -> dict:
        """Stops the playing of the data.\n
        Can be used at any time but it is useful when continuous data playing is selected."""
        return self.query("OUTPUT:STATE OFF?")
    
    #def close_server(self) -> dict:
    #    """Closes the server.\n
    #    Note that to reopen the server you will need to restart the device or connect to the Pi and restart it manually."""
    #    return self.query("CLOSE:SERVER?")