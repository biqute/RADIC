"""
Script to link the python code with the C code that will actually run the board.

Since the C codes (both writing and reading) use flags to set the various options the idea is to create a dictionary 
where we define the values/options and then use a subprocess to lunch the C script.

Note that some of the functions still need an implementation on the client side so for now they can only be called locally
"""

import subprocess
import threading
import time
from logger import Logger 
import numpy as np
import matplotlib.pyplot as plt
from scipy.fft import rfft, rfftfreq
from math import pi, ceil
import scipy.constants as sc

maxval = str((2**24)/2-1)
mylogger = Logger().logger

write_options = {}
read_options = {}
w_filename = "./FunctionPlayer"
r_filename = "./FunctionReader"

# To be implemented but it also works like this
def help_request():
    """Help function. Must be called only by herself since it overrides all other options."""
    write_options.update({"-h": ""})

# To be implemented but it works also like this
def set_device(device: str = "hw:CARD=sndrpihifiberry,DEV=0") -> str:
    """Set the name of the device to be used."""
    write_options.update({"-D": device})
    return device

def set_waveform(wave: str = "SIN") -> str:
    """Set the waveform to be generated\n.
    "SIN" --> sinusoidal\n
    "TRIA" --> triangular"""
    if wave == "SIN":
        write_options.update({"-w": "s"})
    elif wave == "TRIA":
        write_options.update({"-w": "t"})
    return wave

# To be implemented but it also works like this
def set_rate(rate = 192000):
    """Set the sampling rate in Hz. Maximum value is 192000"""
    write_options.update({"-r": rate})
    return rate

# To be implemented but it also works like this

def set_num_channels(channels = 2):
    """Set the number of channels"""
    write_options.update({"-c": channels})
    return channels

def set_frequency(frequency: str = "700") -> str:
    """Set the frequency of the wave in Hz"""
    write_options.update({"-f": frequency})
    return frequency

def set_amplitude(amplitude: str = maxval) -> str:
    """Set the amplitude of the wave in Volts"""
    write_options.update({"-a": amplitude})
    return amplitude

def set_offset(offset: str = "0") -> str:
    """Set the offset of the wave"""
    write_options.update({"-s": offset})
    return offset

# To be implemented but it also works like this
def set_buffer_size(size = 500000):
    write_options.update({"-b": size})
    return size

# To be implemented but it also works like this
def set_period_size(period_size = 100000):
    write_options.update({"-p": period_size})
    return period_size

# To be implemented but it also works like this
def set_method(method = "write"):
    write_options.update({"-m": method})
    return method

# To be implemented but it also works like this
def set_format(format = "S24_LE"):
    write_options.update({"-o": format})
    return format

# To be implemented but it also works like this
def verbose(v = False):
    if v == True:
        write_options.update({"-v": ""})

def play_type(type: str = "c") -> str:
    """Define the writing method\n.
    "c" --> continuous i.e. play data until closing of the stream\n
    "l" --> time-limited i.e. define a duration of the signal"""
    write_options.update({"-t": type})
    return type

def set_duration(seconds: str = "1") -> str:
    """Set the duration of the wave in seconds"""
    write_options.update({"-d": seconds})
    return seconds
    

# Command the board to play the data
def set_play():
    command = [w_filename]
    print(write_options)
    for option, value in write_options.items():
        command.append(option)
        command.append(str(value))

    result = subprocess.run(command)
    mylogger.info(result.stdout)

def play():
    play_thread = threading.Thread(target = set_play, args=())
    play_thread.start()

def stop_play():
    command = "./close_audio.sh"
    subprocess.run(command)
    return "Data stream interrupted"


def set_read_thread(seconds):
    nloops = ceil(375*float(seconds)/15)
    mylogger.info(f"We are acquiring {15*nloops} loops.")
    read_command = [r_filename, str(nloops*15)]

    try:
        result = subprocess.run(read_command)
        result.stdout
    except:
        read_command = [r_filename, str(1*15)]
        result = subprocess.run(read_command)
        result.stdout
        
def read_data(seconds):
    read_thread = threading.Thread(target = set_read_thread, args=(seconds,))
    read_thread.start()

    read_thread.join()

def snr(signal, axis=0, ddof=0):
    signal = np.asanyarray(signal)
    m = signal.mean(axis)
    sd = signal.std(axis=axis, ddof=ddof)
    return 20*np.log10(abs(np.where(sd == 0, 0, m/sd)))

def set_nmeans(nmeans = 1):
    nmeans = nmeans

def data_analysis(*args, verbose = 0, rate = 192000):
    """Typical use of *args: set the frequency for the data analysis."""
    # Reading of the data 
    with open("data.txt", "rb") as f:
        data = f.read()

    f.close()

    # Extract 24 bit data from 4 bytes data
    listed = [data[i:i+4] for i in range(0,len(data), 4)]
    new_data = bytearray()

    for el in listed:
        l = bytearray(el)
        del l[3]
        new_data.extend(l)

    bytes_list = bytes(new_data)
    listed_2 = [bytes_list[i:i+3] for i in range(0,len(bytes_list), 3)]

    data_array_0 = [int.from_bytes(b, byteorder="little", signed=True) for b in listed_2[::2]]
    data_array_1 = [int.from_bytes(c, byteorder="little", signed=True) for c in listed_2[1::2]]

    if verbose == 1:
        print("S/N of the signal: ", snr(data_array_0))
        print("Total length of read data: ", len(data))
        print("First 10 bytes read: ", data[0:10])
        print("First 10 bytes of channel 0: ", listed[0:10])
        print("First 10 bytes of channel 0 after LSB removal: ", listed_2[0:10])
        print("First 10 values of channel 0:", data_array_0[0:10])

        plot_option = 5 # approximate number of waveorms to be displayed

        # Show data
        try:
            frequency = args[0]

            plt.plot(data_array_0, "o", markersize = 1)
            plt.title("Channel 0");
            plt.xlim(0,int(plot_option*rate/frequency));
            plt.show()

            plt.plot(data_array_1, "o", markersize = 1)
            plt.title("Channel 1");
            plt.xlim(0,int(plot_option*rate/frequency));
            plt.show()
        except:
            print("Signals plotted with default settings. May not be visually pleasing")
            plt.plot(data_array_0, "o", markersize = 1)
            plt.title("Channel 0");
            plt.xlim(0,int(len(data_array_0)/plot_option));
            plt.show()

            plt.plot(data_array_1, "o", markersize = 1)
            plt.title("Channel 1");
            plt.xlim(0,int(len(data_array_0)/plot_option));
            plt.show()

        # Spectrum of the signal and peak detection
        yft = rfft(data_array_0)
        xft = rfftfreq(int(len(data_array_0)), 1/rate)
        spectrum = 20*np.log10(np.abs(yft)/np.max(np.abs(yft)))

        plt.plot(xft, spectrum, "o", markersize = .5)
        plt.xscale("log")
        plt.title("Spectrum of the signal")
        plt.show()

        maxima = []

        for i in range(len(spectrum)):
            if spectrum[i] > -95:
                maxima.append(i)

        print("Intensity of the peaks are:", spectrum[maxima])
        print("Frequencies of peaks are: ", xft[maxima])

        plt.plot(xft,spectrum,"o",markersize=.5)
        plt.plot(xft[maxima], spectrum[maxima],"x")
        plt.xscale("log")

    return data_array_0, data_array_1    