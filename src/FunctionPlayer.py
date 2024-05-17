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
format = aa.PCM_FORMAT_S16_LE # Signed 16 bit integer in Little Endian format
pack_format = 'h' # short int (2 bytes) matching the format

channels = 2 # This is practically the only possibility as far as I managed to understand :|
rate = 192000 # Maximum rate supported by the board

# Define parameters
freq = 500 # Works for sinusoidal, triangular and square waves
length = 3 # Length in time of the signal
amp = 32000 # Works for all signals

#
##### FUNCTIONS #####
#
def generate_sin(frequency, duration, amplitude):
    '''Generates a sinusoidal function with specified parametrs. 
    It returns a bytes list ready to be sent to the board'''

    cycle_size = int(rate / frequency)
    factor = int(duration * frequency)

    # Total number of frames
    frames = cycle_size * factor

    signal = [int(amplitude * sin(2 * pi * frequency * i / rate)) for i in range(frames)]

    if channels > 1:
        signal = list(itertools.chain.from_iterable(itertools.repeat(x, channels) for x in signal))

    return struct.pack(str(frames * channels) + pack_format, *signal)

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
        self.device = aa.PCM(rate=rate, channels=channels, format=format, periodsize=128*5, cardindex=2)
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
            - frequency --> frequency of the sinusoidal in Hz
            - duration  --> approximate duration of the signal in seconds
            - amplitude --> amplitude of the signal. The maximum value depends on the resolution we set (e.g for S24int is approx. 8 millions)'''

        if frequency > rate / 2:
            raise ValueError('maximum frequency is %d' % (rate / 2))

        buf = generate_sin(frequency, duration, amplitude)
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