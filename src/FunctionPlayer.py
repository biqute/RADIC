# Imports
import numpy as np
from matplotlib import pyplot as plt
from scipy import signal as sg
from math import sin, pi

import alsaaudio as aa

import time
import struct
import itertools
from multiprocessing import Queue
from queue import Empty
from threading import Thread

# Initialization of some values
# Format parameters
#data_format = aa.PCM_FORMAT_S16_LE # Signed 24 bit integer in Little Endian format
pack_format = 'l' # long int (4 bytes) to convert the data to bytes

channels = 2 # This is practically the only possibility as far as I managed to understand :|
rate = 192000 # Maximum rate supported by the board

# Define parameters
precision = 16
freq = 500 # Works for sinusoidal, triangular and square waves
length = 10 # Length in time of the signal
amp = (2**precision)/2-1

#
##### FUNCTIONS #####
#
def replace_byte_in_place(byte_string, index, new_byte):
    '''Replaces a byte at a specific index in a byte string'''
    if not isinstance(byte_string, bytes):
        raise TypeError("Input must be a byte string")
    if not isinstance(index, int) or not isinstance(new_byte, int):
        raise TypeError("Index and new_byte must be integers")
    if not (0 <= index < len(byte_string)):
        raise ValueError("Index is outside the byte string range")
    if not (0 <= new_byte <= 255):
        raise ValueError("New byte value must be between 0 and 255")
    byte_array = bytearray(byte_string)
    byte_array[index] = new_byte
    return bytes(byte_array)

def remove_byte(byte_string, index):
    '''Removes a byte at a specific index in a byte string'''
    if not isinstance(byte_string, bytes):
        raise TypeError("Input must be a byte string")
    if not isinstance(index, int):
        raise TypeError("Index must be an integer")
    if not (0 <= index < len(byte_string)):
        raise ValueError("Index is outside the byte string range")
    return byte_string[:index] + byte_string[index + 1:]

def insert_byte(byte_string, index, new_byte):
    '''Insert a byte at a specific index in a byte string'''
    if not isinstance(byte_string, bytes):
        raise TypeError("Input must be a byte string")
    if not isinstance(index, int) or not isinstance(new_byte, int):
        raise TypeError("Index and new_byte must be integers")
    if not (0 <= index <= len(byte_string)):
        raise ValueError("Index is outside the byte string range")
    if not (0 <= new_byte <= 255):
        raise ValueError("New byte value must be between 0 and 255")
    byte_array = bytearray(byte_string)
    byte_array.insert(index, new_byte)
    return bytes(byte_array)

def byte_conversion__OLD__(bytes_list):
    '''Takes a list composed of chunks of 4 bytes and should return
    a list composed of chunks of 3 bytes'''

    print(len(bytes_list))

    listed = [bytes_list[i:i+4] for i in range(0, len(bytes_list), 4)]

    write_bytes = [[],[]]

    for idx in range(len(listed)):
        if idx%2 == 0:
            removed_bytes = remove_byte(listed[idx], 0)
            write_bytes[0].append(removed_bytes)
        else:
            removed_bytes = remove_byte(listed[idx], 0)
            write_bytes[1].append(removed_bytes)

    bytes_array = bytearray()

    for ii in range(len(write_bytes[0])):
        bytes_array.extend(write_bytes[0][ii])
        bytes_array.extend(write_bytes[1][ii])

    return bytes(bytes_array)

def byte_conversion(data_list):
    '''Converts a list of integers into a list of bytes with specified length'''

    num_bytes = int(precision/8)

    if num_bytes != 2 and num_bytes != 3 and num_bytes != 4:
        raise ValueError('Precision specified is not 2, 3 or 4 bytes')

    bytes_array = [(integer).to_bytes(num_bytes, byteorder='little', signed=True) for integer in data_list]
    
    bytes_arr = bytearray()

    for b in bytes_array:
        bytes_arr.extend(b)

    return bytes(bytes_arr)

def generate_sin(frequency, amplitude, duration = length):
    '''Generates a sinusoidal function with specified parametrs. 
    It returns a bytes list ready to be sent to the board'''

    cycle_size = int(rate / frequency)
    factor = int(duration * frequency)

    # Total number of frames
    frames = cycle_size * factor

    print('We have %i frames' % frames)

    signal = [int(amplitude * sin(2 * pi * frequency * i / rate)) for i in range(frames)]

    if channels > 1:
        signal = list(itertools.chain.from_iterable(itertools.repeat(x, channels) for x in signal))

    #signal = np.array(signal).astype(np.int32) # This is an array composed of 32 bit samples
    #return byte_conversion(struct.pack(str(frames * channels) + pack_format, *signal))

    return byte_conversion(signal)

def generate_triangular(frequency, duration, amplitude):
    '''Generates a constant function with specified parametrs. 
    It returns a bytes list ready to be sent to the board'''

    cycle_size = int(rate / frequency)
    factor = int(duration * frequency)

    # Total number of frames
    frames = cycle_size * factor

    length = np.linspace(0,frames,frames)

    signal = (amplitude*sg.sawtooth(2*pi*frequency*length/rate, width=0.5)).astype(np.int16)
    # width=0.5 is to have a symmetrical triangle

    if channels > 1:
        signal = list(itertools.chain.from_iterable(itertools.repeat(x, channels) for x in signal))

    return struct.pack(str(frames * channels) + pack_format, *signal)

def generate_constant(duration, amplitude):
    '''Generates a constant function with specified parametrs. 
    It returns a bytes list ready to be sent to the board'''

    frames = int(rate * duration)

    signal = (amplitude * np.ones(frames)).astype(np.int16)

    if channels > 1:
        signal = list(itertools.chain.from_iterable(itertools.repeat(x, channels) for x in signal))

    return struct.pack(str(frames * channels) + pack_format, *signal)

def generate_square(frequency, duration, amplitude):
    '''Generates a constant function with specified parametrs. 
    It returns a bytes list ready to be sent to the board'''

    cycle_size = int(rate / frequency)
    factor = int(duration * frequency)

    # Total number of frames
    frames = cycle_size * factor

    length = np.linspace(0,frames,frames)

    signal = (amplitude*sg.square(2*pi*frequency*length/rate, duty=0.5)).astype(np.int16)

    if channels > 1:
        signal = list(itertools.chain.from_iterable(itertools.repeat(x, channels) for x in signal))

    return struct.pack(str(frames * channels) + pack_format, *signal)


#
##### CLASSES #####
#
class SinePlayer(Thread):
    '''Class to play a sinusoidal wave.'''
    def __init__(self, frequency = freq, duration = length, amplitude = amp):
        Thread.__init__(self, daemon=True)
        if precision == 16:
            self.device = aa.PCM(rate=rate, channels=channels, format=aa.PCM_FORMAT_S16_LE, periodsize=512, cardindex=2)
        elif precision == 24:
            self.device = aa.PCM(rate=rate, channels=channels, format=aa.PCM_FORMAT_S24_LE, periodsize=512, cardindex=2)
        elif precision == 32:
            self.device = aa.PCM(rate=rate, channels=channels, format=aa.PCM_FORMAT_S32_LE, periodsize=512, cardindex=2)
        else:
            raise ValueError('Precision specified is not 16, 24 or 32 bits.')
        # We open the device
        # Defaul settings are:
        #   - type = PCM_PLAYBACK
        #   - mode = PCM_NORMAL  -->  will block the 'write' function if the buffer is full
        #   - periods = 4

        self.queue = Queue()
        self.change(frequency, amplitude, duration)

    def change(self, frequency, amplitude, duration = length):
        '''Generate the buffer with the data and put it in the queue.
        Parameters needed:
            - frequency --> frequency of the sinusoidal in Hz
            - duration  --> approximate duration of the signal in seconds
            - amplitude --> amplitude of the signal. The maximum value depends on the resolution we set (e.g for S24int is approx. 8 millions)'''

        print('Ciao I\'m in the change function')

        if frequency > rate / 2:
            raise ValueError('maximum frequency is %d' % (rate / 2))

        buf = generate_sin(frequency, amplitude, duration)
        print('len buf:', len(buf))
        self.queue.put(buf)
        print('putted the data in the queue')

    def run(self):
        print('Entered the run function')
        buffer = None
        while True:
            try:
                buffer = self.queue.get(False)
                print('len buffer:', len(buffer))
                #print(buffer)
                print('Got the queued data, data stream should start :)')
            except Empty:
                pass
            if buffer:
                if self.device.write(buffer) < 0:
                    print("Playback buffer underrun! Continuing nonetheless ...")

class ConstPlayer(Thread):
    '''Class to play a constant wave.'''
    def __init__(self, duration = length, amplitude = amp):
        Thread.__init__(self, daemon=True)
        self.device = aa.PCM(rate = rate, channels=channels, format=format, periodsize=128, cardindex=2)
        # We open the device
        # Defaul settings are:
        #   - type = PCM_PLAYBACK
        #   - mode = PCM_NORMAL  -->  will block the 'write' function if the buffer is full
        #   - periods = 4

        self.queue = Queue()
        self.change(duration, amplitude)

    def change(self, duration, amplitude):
        '''Generate the buffer with the data and put it in the queue.
        Parameters needed:
            - duration  --> approximate duration of the signal in seconds
            - amplitude --> amplitude of the signal. The maximum value depends on the resolution we set (e.g for S24int is approx. 8 millions)'''

        buf = generate_constant(duration, amplitude)
        self.queue.put(buf)

    def run(self):
        print('Entered the run function')
        buffer = None
        while True:
            try:
                buffer = self.queue.get(False)
                print('Got the queued data, data stream should start :)')
            except Empty:
                pass
            if buffer:
                if self.device.write(buffer) < 0:
                    print("Playback buffer underrun! Continuing nonetheless ...")

class TriangularPlayer(Thread):
    '''Class to play a triangular wave.'''
    def __init__(self, frequency = freq, duration = length, amplitude = amp):
        Thread.__init__(self, daemon=True)
        self.device = aa.PCM(rate = rate, channels=channels, format=format, periodsize=128, cardindex=2)
        # We open the device
        # Defaul settings are:
        #   - type = PCM_PLAYBACK
        #   - mode = PCM_NORMAL  -->  will block the 'write' function if the buffer is full
        #   - periods = 4

        self.queue = Queue()
        self.change(frequency, duration, amplitude)

    def change(self, frequency, duration, amplitude):
        '''Generate the buffer with the data and put it in the queue.
        Parameters needed:
            - duration  --> approximate duration of the signal in seconds
            - amplitude --> amplitude of the signal. The maximum value depends on the resolution we set (e.g for S24int is approx. 8 millions)'''

        if frequency > rate / 2:
            raise ValueError('maximum frequency is %d' % (rate / 2))

        buf = generate_triangular(frequency, duration, amplitude)
        self.queue.put(buf)

    def run(self):
        print('Entered the run function')
        buffer = None
        while True:
            try:
                buffer = self.queue.get(False)
                print('Got the queued data, data stream should start :)')
            except Empty:
                pass
            if buffer:
                if self.device.write(buffer) < 0:
                    print("Playback buffer underrun! Continuing nonetheless ...")

class SquarePlayer(Thread):
    '''Class to play a square wave.'''
    def __init__(self, frequency = freq, duration = length, amplitude = amp):
        Thread.__init__(self, daemon=True)
        self.device = aa.PCM(rate = rate, channels=channels, format=format, periodsize=128, cardindex=2)
        # We open the device
        # Defaul settings are:
        #   - type = PCM_PLAYBACK
        #   - mode = PCM_NORMAL  -->  will block the 'write' function if the buffer is full
        #   - periods = 4
   
        self.queue = Queue()
        self.change(frequency, duration, amplitude)

    def change(self, frequency, duration, amplitude):
        '''Generate the buffer with the data and put it in the queue.
        Parameters needed:
            - duration  --> approximate duration of the signal in seconds
            - amplitude --> amplitude of the signal. The maximum value depends on the resolution we set (e.g for S24int is approx. 8 millions)'''

        if frequency > rate / 2:
            raise ValueError('maximum frequency is %d' % (rate / 2))

        buf = generate_square(frequency, duration, amplitude)
        self.queue.put(buf)

    def run(self):
        print('Entered the run function')
        buffer = None
        while True:
            try:
                buffer = self.queue.get(False)
                print('Got the queued data, data stream should start :)')
            except Empty:
                pass
            if buffer:
                if self.device.write(buffer) < 0:
                    print("Playback buffer underrun! Continuing nonetheless ...")