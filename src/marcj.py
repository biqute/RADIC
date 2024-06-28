#   Each class will control the writing or reading process of the board

import subprocess
import threading
import time

import numpy as np
import matplotlib.pyplot as plt
from scipy.fft import rfft, rfftfreq
from scipy.signal import find_peaks

maxval = (2**24)/2-1

class FunctionPlayer():
    def __init__(self):
        self.options = {}
        self.filename = './FunctionPlayer'

    def help_request(self):
        '''Help function'''
        self.options.update({"-h": ''})

    def set_device(self, device = 'hw:CARD=sndrpihifiberry,DEV=0'):
        '''Set the name of the device to be used as a function player'''
        self.options.update({"-D": device})

    def set_waveform(self, wave = 's'):
        '''Set the waveform to be generated\n.
        's' --> sinusoidal\n
        't' --> triangular'''
        self.options.update({"-w": wave})

    def set_rate(self, rate = 192000):
        '''Set the sampling rate in Hz. Maximum value is 192000'''
        self.options.update({"-r": rate})
    
    def set_num_channels(self, channels = 2):
        '''Set the number of channels'''
        self.options.update({"-c": channels})

    def set_frequency(self, frequency = 700):
        '''Set the frequency of the wave in Hz'''
        self.options.update({"-f": frequency})

    def set_duration(self, seconds = 2):
        '''Set the duration of the wave in seconds'''
        self.options.update({"-d": seconds})

    def set_amplitude(self, amplitude = maxval):
        '''Set the amplitude of the wave in Volts'''
        self.options.update({"-a": amplitude})

    def set_buffer_size(self, size = 500000):
        self.options.update({"-b": size})

    def set_period_size(self, period_size = 100000):
        self.options.update({"-p": period_size})

    def set_method(self, method = 'write'):
        self.options.update({"-m": method})

    def set_format(self, format = 'S24_LE'):
        self.options.update({"-o": format})

    def verbose(self):
        self.options.update({"-v": ''})

    def set_play(self):
        command = [self.filename]
        for option, value in self.options.items():
            command.append(option)
            command.append(str(value))

        print('Writing the signal')

        result = subprocess.run(command)
        result.stdout

    def play(self):
        play_thread = threading.Thread(target = self.set_play)
        play_thread.start()


class FunctionReader():
    def __init__(self):
        #self.options = {}
        self.nloops = None
        self.filename = './FunctionReader'

    def set_nloops(self, loops = 15):
        '''Set number of loops. I don't know why but it works only with integers multiples of 15.
        For now we have to work with that until I discover what's wrong.'''
        self.nloops = loops

    def set_read(self):
        command = [self.filename, str(self.nloops)]

        print('Reading the signal')

        result = subprocess.run(command)
        result.stdout

    def read(self):
        read_thread = threading.Thread(target = self.set_read)
        read_thread.start()

def data_analysis(verbose = 0, rate = 192000):
    # Reading of the data 
    with open("data.txt", "rb") as f:
        data = f.read()

    # Extract 24 bit data from 4 bytes data
    listed = [data[i:i+4] for i in range(0,len(data), 4)]
    new_data = bytearray()

    for el in listed:
        l = bytearray(el)
        del l[3]
        new_data.extend(l)

    bytes_list = bytes(new_data)
    listed_2 = [bytes_list[i:i+3] for i in range(0,len(bytes_list), 3)]

    data_array_0 = [int.from_bytes(b, byteorder='little', signed=True) for b in listed_2[::2]]
    data_array_1 = [int.from_bytes(c, byteorder='little', signed=True) for c in listed_2[1::2]]

    # Show data
    plt.plot(data_array_0, 'o', markersize = 1)
    plt.title('Channel 0');
    plt.xlim(0,int(len(data_array_0)/5));
    plt.show()

    plt.plot(data_array_1, 'o', markersize = 1)
    plt.title('Channel 1');
    plt.xlim(0,int(len(data_array_1)/5));
    plt.show()

    # Spectrum of the signal and peak detection
    yft = rfft(data_array_0)
    xft = rfftfreq(int(len(data_array_0)), 1/rate)
    spectrum = 20*np.log10(np.abs(yft)/np.max(np.abs(yft)))

    plt.plot(xft, spectrum, 'o', markersize = .5)
    plt.xscale('log')
    plt.title("Spectrum of the signal")
    plt.show()

    maxima = []

    for i in range(len(spectrum)):
        if spectrum[i] > -95:
            maxima.append(i)

    print('Intensity of the peaks are:', spectrum[maxima])
    print('Frequencies of peaks are: ', xft[maxima])

    plt.plot(xft,spectrum,'o',markersize=.5)
    plt.plot(xft[maxima], spectrum[maxima],'x')
    plt.xscale('log')

    if verbose == 1:
        print('Total length of read data: ', len(data))
        print('First 10 bytes read: ', data[0:10])
        print('First 10 bytes of channel 0: ', listed[0:10])
        print('First 10 bytes of channel 0 after LSB removal: ', listed_2[0:10])
        print('First 10 values of channel 0:', data_array_0[0:10])

    return data_array_0, data_array_1    