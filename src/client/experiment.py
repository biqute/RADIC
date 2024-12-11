"""
Contains a class that makes it easier to execute an IV curve measuring experiment.

TO-DO:

OTHER MODIFICATIONS: 
since the idea is to assoume that we know the current that we provide, we create the "current" 
vector artificially. This implies that we have to do a calibration to know which voltage point corresponds to what current point. 
This I think can be done by finding the lowest point of the voltage and considering a period after that --> I'M NOT ENTIRELY SURE 
ON HOW TO DO THIS CALIBRATION PROCESS.
"""

from ast import Tuple
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
            ampls: tuple = [2],
            offset: float = 0,
            play_type: str = "CONT",
            averages: int = 2,
            wave: str = "TRIA",
            acq_time: float = 1,
            acq_cycles: int = 1,
            sleep_time: float = 0.5,
            folder_path: str = "dataIV",
            save_data: bool = True
        ):
        """Create an object on which we can run an IV curve measure.
        
        Parameters
        ----------
        resistance : int
            value of the resistance (in Ohm) used to current bias the device.
        inst : str (optional)
            name of the device.
        freqs : tuple
            different frequencies (in Hz) used to test the device.
        ampls : tuple
            different amplitudes (in V) used to test the device.
        offset : float (optional)
            dc offset of the signal (in V) --> !Note that it cannot be detected by the board since it's ac coupled!
        play_type : str
            type of data playing. Can be `CONT` for continuous data playing or `LIM` for time limited data playing.
        averages : int
            number of averages wanted on the read signal. If the number of averages is set to `0` the program will save 
            all the data acquired during the "acq_time" period.\n
            Default is `2` i.e. we average between two cycles.
        wave : str
            type of wave to play. Can be `TRIA` for a triangular signal or `SIN` for a sinusoidal signal.
        acq_time : float
            sets the length of the data acquisition (in seconds)\n
            --> note that acquisition longer than approx. 5 seconds may "broke" the code (MAYBE this will be improved in the future).
        sleep_time : float (optional)
            sleep time (in seconds).    
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
        self.acq_cycles = acq_cycles
        self.sleep_time = sleep_time
        self.full_data = []
        self.path = folder_path
        self.resistance = resistance
        self.save_data = save_data

        self.inst.connect()

        # Set common settings of the device
        self.inst.set_wave(self.wave)
        self.inst.set_offset(self.offset) 
    
    def first_local_minimum(self, arr):
        # Check that array has at least three elements
        if len(arr) < 3:
            raise ValueError("Array must have at least three elements to have a local minimum.")
        
        # Iterate through the array to find the first local minimum
        for i in range(1, len(arr) - 1):  # Exclude the first and last elements
            if arr[i - 1] > arr[i] < arr[i + 1]:
                return i
        return None  # No local minimum found

    def run_experiment(
            self, 
            plot: bool = True, 
            fit: bool = False, 
            device: str = "jj"
        ):
        """Run an IV-curve measure
        
        Parameters
        ----------
        plot : bool
            Choose if you want to save a plot of the results.
        fit : bool
            Choose if you want to fit the results.
        device : str
            Set the device under test (useful when we want to fit the result). 
            Possible devices are `diode`, `resistance` and `jj` (josephson junction).       
        """

        for freq in self.freqs:                    # cycle over the frequencies
            for a in self.ampls:                   # cycle over the amplitudes
                self.inst.set_frequency(freq)
                self.inst.set_amplitude(a)

                print(f"Setted frequency: {freq} Hz")
                print(f"Setted amplitude: {a} V")

                n_points = self.inst.max_rate//freq # Number of points in a cycle at given acquisition rate and signal frequency
                n_cycles = self.acq_time*self.inst.max_rate//n_points # Number of cycles in a given acquisition time

                # At the end of this cycle we should know how many acquisition we have to do
                while n_cycles*self.acq_cycles < self.averages:
                    self.acq_cycles +=1

                self.inst.play(self.play_type)

                #print(f"We need {self.acq_cycles} acquistion cycles.")

                if self.acq_cycles > 1:
                    print("The code is about to broke :(")
                    self.inst.disconnect()
                    raise ValueError("You chose too many averages with respect to the acquisition time and/or frequency\n"
                                    "Try reducing the number of averages or increasing the acquisition time")

                # "Artificial current vector"
                current_vector_rise = np.linspace(-a/self.resistance, a/self.resistance, int(np.ceil(n_points/2)))
                current_vector_fall = np.linspace(a/self.resistance, -a/self.resistance, int(np.floor(n_points/2)))
                full_current_vector = np.concat((current_vector_rise, current_vector_fall))
                
                # Now start the acquisition process
                try: 
                   # Now start the acquisition process

                    # Check if folder exists and in case create it
                    if self.save_data:
                        path = self.path + f"/freq_{freq}Hz_ampl_{a}V" # each (freq, ampl) is saved in a different folder
                        if not os.path.exists(path):
                            os.makedirs(path)

                    nloops = self.inst.read_signal(self.acq_time)

                    full_ch0 = np.empty(shape=(self.averages, n_points))
                    full_ch1 = np.empty(shape=(self.averages, n_points))

                    ch0 = np.array(nloops["ch0"])*2.96/self.inst.maxval
                    ch1 = np.array(nloops["ch1"])*2.96/self.inst.maxval

                    # Write the raw data to a .txt file
                    if self.save_data:
                        with open(path + f"/RAW_resistance_{self.resistance}Ohm.txt", "a") as f:
                            for idx in range(len(ch0)):
                                f.write(str(ch0[idx]) + " " + str(ch1[idx]) + "\n")
                        f.close()

                    for avg in range(self.averages):
                        temp_0 = ch0[avg*n_points: n_points + avg*n_points]
                        temp_1 = ch1[avg*n_points: n_points + avg*n_points]

                        full_ch0[avg] = temp_0
                        full_ch1[avg] = temp_1

                    avg_ch0 = np.mean(full_ch0, axis = 0)
                    avg_ch1 = np.mean(full_ch1, axis = 0)

                    # Write the full data to a .txt file
                    if self.save_data:
                        with open(path + f"/AVERAGE_averages_{self.averages}_resistance_{self.resistance}Ohm.txt", "w") as f:
                            for idx in range(len(avg_ch0)):
                                f.write(str(avg_ch0[idx]) + " " + str(avg_ch1[idx]) + " " + str(full_current_vector[idx]) + "\n")
                        f.close()
                    
                    # Now I have to order the avg_ch0 vector and align it with the currents
                    min_idx = np.argmin(avg_ch0)
                    V_dut_0 = np.concatenate((avg_ch0[min_idx:], avg_ch0[:min_idx])) # This is the shifted array
                    #V_dut_0 = avg_ch0

                    if plot:
                        plt.plot(V_dut_0, full_current_vector, 'o', markersize = .5, color = "darkblue");
                        plt.grid(alpha = .2)
                        #plt.title(f"IV of a {device} with frequency {freq} Hz and amplitude {a}V")
                        plt.xlabel("voltage [V]")
                        plt.ylabel("current [A]")
                        plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm.pdf");
                        plt.clf();
                        plt.close();

                        fig, ax1 = plt.subplots();
                        color = "darkblue"
                        ax1.set_xlabel("time")
                        ax1.set_ylabel("current [A]", color = color)
                        ax1.plot(full_current_vector, 'o', markersize=.5, color = color);
                        ax1.tick_params(axis='y', labelcolor=color)
                        ax2 = ax1.twinx()
                        color = "darkorange"
                        ax2.set_xlabel("time")
                        ax2.set_ylabel("amplitude [V]", color = color)
                        ax2.plot(V_dut_0, 'o', markersize=.5, color = color);
                        ax2.tick_params(axis='y', labelcolor=color)
                        ax1.grid(alpha=.2)
                        ax2.grid(alpha=.2)
                        fig.tight_layout() 
                        plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm_timetrace.pdf");
                        plt.clf();
                        plt.close();

                    if fit:
                        self.IV_fit(V_dut_0, full_current_vector, device, path)

                    sleep(self.sleep_time)

                    self.inst.stop_signal()

                except Exception as error:
                    print(error)
                    self.inst.stop_signal()

        #self.inst.stop_signal()

    def IV_fit(self, x, y, device: str, path: str):
        """Method to fit the IV curve"""

        y_errors = .1*np.ones(len(y))

        if device == "diode":
            least_square = LeastSquares(x, y, y_errors, diode_IV)
            m = Minuit(least_square, I_0 = -30, a = 40, offset = 0)
        elif device == "resistance":
            least_square = LeastSquares(x, y, y_errors, resistor_IV)
            m = Minuit(least_square, R = 100, b = 0)
        elif device == "jj":
            print("Fit method yet to be implemented --> It's a pain...")

        fit = m.migrad()
        print(fit)
        
        plt.plot(x, y, 'o', markersize = .5, color = "darkblue", label="data")
        if device == "diode":
            plt.plot(x, diode_IV(x, *m.values), markersize=.5, color="darkorange", label="fit")
        elif device == "resistance":
            plt.plot(x, resistor_IV(x, *m.values), markersize=.5, color="darkorange", label="fit")
        # Other ELIF yet to be implemented

        plt.grid(alpha = .2)
        #plt.title(f"IV of a {device} with fit")
        plt.legend(loc = "best")
        plt.xlabel("voltage [V]")
        plt.ylabel("current [A]")
        plt.savefig(path + f"/IV+fit_averages_{self.averages}_resistance_{self.resistance}Ohm.pdf")
        plt.clf()

    def close_instrument(self):
        """Closes the instrument."""
        self.inst.disconnect()