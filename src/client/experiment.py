"""
Contains a class that makes it easier to execute an IV curve measuring experiment
"""

import numpy as np
import matplotlib.pyplot as plt
from time import sleep
from instrument import Marcj
import os
from iminuit.cost import LeastSquares
from iminuit import Minuit

def diode_IV(x, I_0, a, offset):
    return I_0*(np.exp(a*x) - 1) + offset

def resistor_IV(x, R, b):
    return x/R + b

class IVCurve():
    """IV curve experiment"""

    def __init__(
            self, 
            resistance: int,
            inst = Marcj("JJ"), 
            freqs: tuple = [500], 
            ampls: tuple = [3],
            offset: float = 0,
            play_type: str = "CONT",
            averages: int = 1,
            wave: str = "TRIA",
            acq_time: float = 0.03,
            sleep_time: float = 0.1,
            folder_path: str = "data",
            save_file: bool = False
        ):
        """Creates an object on which we can run an IV curve measure.
        
        Parameters
        ----------
        resistance : int
            value of the shunt resistance (in Ohm) used to measure the current through the device.
        inst : optional
            name of the device.
        freqs : tuple
            different frequencies (in Hz) used to test the device.
        ampls : tuple
            different amplitudes (in V) used to test the device.
        play_type : str
            type of data playing. Can be "CONT" for continuous data playing or "LIM" for time limited data playing.
        averages : int
            number of averages wanted on the read signal.
        wave : str
            type of wave to play. Can be "TRIA" for a triangular signal or "SIN" for a sinusoidal signal.
        acq_time : float
            sets the length of the data acquisition (in seconds).
        sleep_time : float
            optional sleep time (in seconds).    
        folder_path : str
            path of the folder where data will be saved.
        save_file : bool
            choose if the data will be saved or not.
        """
    
        self.inst = inst
        self.freqs = freqs
        self.ampls = ampls
        self.offset = offset
        self.play_type = play_type
        self.averages = averages
        self.wave = wave
        self.acq_time = acq_time
        self.sleep_time = sleep_time
        self.full_data = []
        self.path = folder_path
        self.resistance = resistance
        self.save_file = save_file

        self.inst.connect()

        # Set common settings of the device
        self.inst.set_wave(self.wave)
        self.inst.set_offset(self.offset)

    def run_experiment(self, plot: bool = False, fit: bool = False, device: str = "diode"):
        """Runs an IV-curve measure
        
        Parameters
        ----------
        plot : bool
            Choose if you want to plot the results.
        fit : bool
            Choose if you want to fit the results.
        device : str
            Set the device under test (useful when we want to fit the result). Possible devices are "diode" and "resistance".       
        """

        for freq in self.freqs:                    # cycle over the frequencies
            for a in self.ampls:                   # cycle over the amplitudes
                self.inst.set_frequency(freq)
                self.inst.set_amplitude(a)

                self.inst.play(self.play_type)

                n_points = int(self.inst.max_rate/freq)
                
                try:
                    # Now start the acquisition process

                    if self.save_file:
                        path = self.path + f"/freq_{freq}Hz_ampl_{a}V"
                        os.mkdir(path)

                    nloops = self.inst.read_signal(self.acq_time)

                    ch0 = np.array(nloops["ch0"])*2.96/self.inst.maxval
                    ch1 = np.array(nloops["ch1"])*2.96/self.inst.maxval

                    full_ch0 = np.empty(shape=(self.averages, n_points))
                    full_ch1 = np.empty(shape=(self.averages, n_points))

                    for avg in range(self.averages):
                        if len(ch0) < n_points*self.averages:
                            self.inst.disconnect()
                            raise ValueError("You chose too many averages with respect to the acquisition time and/or frequency\n"
                                            "Try reducing the number of averages or increasing the acquisition time")

                        temp_0 = ch0[avg*n_points: n_points + avg*n_points]
                        temp_1 = ch1[avg*n_points: n_points + avg*n_points]

                        full_ch0[avg] = temp_0
                        full_ch1[avg] = temp_1

                    averaged_ch0 = np.mean(full_ch0, axis = 0)
                    averaged_ch1 = np.mean(full_ch1, axis = 0)

                    # Write the full data to a .txt file
                    if self.save_file:
                        with open(path + f"/averages_{self.averages}_resistance_{self.resistance}Ohm.txt", "w") as f:
                            for idx in range(len(averaged_ch0)):
                                f.write(str(averaged_ch0[idx]) + " " + str(averaged_ch1[idx]) + "\n")
                        f.close()

                    I_dut = averaged_ch0/self.resistance
                    V_dut = averaged_ch1-averaged_ch0

                    if plot:
                        plt.plot(V_dut, I_dut, 'o', markersize = 1.5, color = "darkblue")
                        plt.grid(alpha = .2)
                        plt.title(f"IV of a {device} with frequency of {freq} Hz and amplitude of {a}V")
                        plt.xlabel("voltage [V]")
                        plt.ylabel("current [A]")
                        #plt.show()

                    if fit:
                        self.IV_fit(V_dut, I_dut, device)

                except Exception as error:
                    print(error)
                    self.inst.stop_signal()

        self.inst.stop_signal()

    def IV_fit(self, x, y, device: str):
        """Method to fit the IV curve"""

        y_errors = .1*np.ones(len(y))

        if device == "diode":
            least_square = LeastSquares(x, y, y_errors, diode_IV)
            m = Minuit(least_square, I_0 = -30, a = 40, offset = 0)
        elif device == "resistance":
            least_square = LeastSquares(x, y, y_errors, resistor_IV)
            m = Minuit(least_square, R = 100, b = 0)

        fit = m.migrad()
        print(fit)
        
        plt.plot(x, y, 'o', markersize = .5, color = "darkblue", label="data")
        if device == "diode":
            plt.plot(x, diode_IV(x, *m.values), markersize=.5, color="darkorange", label="fit")
        elif device == "resistance":
            plt.plot(x, resistor_IV(x, *m.values), markersize=.5, color="darkorange", label="fit")

        plt.grid(alpha = .2)
        plt.title(f"IV of a {device} with fit")
        plt.legend(loc = "best")
        plt.xlabel("voltage [V]")
        plt.ylabel("current [A]")
        #plt.show()

    def close_instrument(self):
        """Closes the instrument."""
        self.inst.disconnect()