"""
Contains a class that makes it easier to execute an IV curve measuring experiment.

THIS WORKS ONLY UP TO 5 SECONDS OF ACQUISITION. THE IMPROVED VERSION IN IS experiment_new.py

CHECK THE I-V PART AND IMPLEMENT IT WITH EXPERIMENT_NEW.PY BECAUSE THERE ARE SOME DIFFERENCES.
"""

from typing import Tuple
import numpy as np
import matplotlib.pyplot as plt
from time import sleep
from instrument import Marcj
import os
from iminuit.cost import LeastSquares
from iminuit import Minuit
from scipy.optimize import curve_fit
from scipy.signal import butter, filtfilt

class IVCurve():
    """IV curve experiment"""

    def __init__(
            self, 
            resistance: int = 10000,
            inst = Marcj("JJ", address="10.30.44.176"), 
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

                #print(f"We need {self.acq_cycles} acquistion cycles.")

                if self.acq_cycles > 1:
                    print("WARNING: You chose too many averages with respect to the acquisition time and/or frequency\n"
                          f"It will be used {n_cycles} as a number of averages.")
                    self.averages = n_cycles

                self.inst.play(self.play_type)

                # "Artificial current vector"
                current_vector_rise0 = np.linspace(-a/self.resistance, a/self.resistance, int(np.ceil(n_points/2)))
                current_vector_fall0 = np.linspace(a/self.resistance, -a/self.resistance, int(np.floor(n_points/2)))
                full_current_vector0 = np.concat((current_vector_rise0, current_vector_fall0))

                current_vector_rise1 = np.linspace(-a/self.resistance, a/self.resistance, int(np.ceil(n_points/2)))
                current_vector_fall1 = np.linspace(a/self.resistance, -a/self.resistance, int(np.floor(n_points/2)))
                full_current_vector1 = np.concat((current_vector_rise1, current_vector_fall1))
 
                # Now start the acquisition process
                try: 

                    # Check if folder exists and in case create it
                    if self.save_data:
                        path = self.path + f"/freq_{freq}Hz_ampl_{np.round(a,3)}V" # each (freq, ampl) is saved in a different folder
                        if not os.path.exists(path):
                            os.makedirs(path)

                    nloops = self.inst.read_signal(self.acq_time)

                    full_ch0 = np.empty(shape=(self.averages, n_points))
                    full_ch1 = np.empty(shape=(self.averages, n_points))

                    # Save raw amplitudes that will be converted to voltages later on
                    ch0 = np.array(nloops["ch0"])*3.05/self.inst.maxval
                    ch1 = np.array(nloops["ch1"])*3.05/self.inst.maxval

                    # Write the raw data to a .npz file
                    if self.save_data:
                        np.savez(path + f"/RAW_resistance_{self.resistance}Ohm.npz", raw_ch0=ch0, raw_ch1=ch1)


                    for avg in range(self.averages):
                        temp_0 = ch0[avg*n_points: n_points + avg*n_points]
                        temp_1 = ch1[avg*n_points: n_points + avg*n_points]

                        full_ch0[avg] = temp_0
                        full_ch1[avg] = temp_1

                    avg_ch0 = np.mean(full_ch0, axis = 0)
                    avg_ch1 = np.mean(full_ch1, axis = 0)

                    # Write the full data to a .npz file
                    if self.save_data:
                        np.savez(path + f"/AVERAGE_averages_{self.averages}_resistance_{self.resistance}Ohm.npz", 
                                 avg_ch0=avg_ch0,
                                 avg_ch1=avg_ch1,
                                 bias_current0=full_current_vector0,
                                 bias_current1=full_current_vector1
                                )

                    
                    # Now I have to order the avg_ch0 vector and align it with the currents
                    # I THINK THAT IS BETTER TO ALIGN THE CURRENT VECTOR TO THE VOLTAGE ONE --> TO BE ADDED
                    min_idx0 = np.argmin(avg_ch0)
                    V_dut_0 = np.concatenate((avg_ch0[min_idx0:], avg_ch0[:min_idx0])) # This is the shifted array

                    min_idx1 = np.argmin(avg_ch1)
                    V_dut_1 = np.concatenate((avg_ch1[min_idx1:], avg_ch1[:min_idx1])) # This is the shifted array
                    #V_dut_0 = avg_ch0

                    if plot:
                        plt.plot(full_current_vector0, V_dut_0, 'o', markersize = .5, color = "darkblue");
                        plt.grid(alpha = .2)
                        #plt.title(f"IV of a {device} with frequency {freq} Hz and amplitude {a}V")
                        plt.ylabel("voltage [V]")
                        plt.xlabel("current [A]")
                        plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm_ch0.pdf");
                        plt.clf();
                        plt.close();

                        fig, ax1 = plt.subplots();
                        color = "darkblue"
                        ax1.set_xlabel("time [a.u.]")
                        ax1.set_ylabel(r"current [$\mu$A]", color = color)
                        ax1.plot(full_current_vector0*1e6, 'o', markersize=.5, color = color);
                        ax1.tick_params(axis='y', labelcolor=color)
                        ax2 = ax1.twinx()
                        color = "darkorange"
                        ax2.set_xlabel("time [a.u.]")
                        ax2.set_ylabel("amplitude [mV]", color = color)
                        ax2.plot(V_dut_0*1e3, 'o', markersize=.5, color = color);
                        ax2.tick_params(axis='y', labelcolor=color)
                        ax1.grid(alpha=.2)
                        ax2.grid(alpha=.2);
                        plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm_timetrace_ch0.pdf");
                        plt.clf();
                        plt.close();
                    
                        plt.plot(full_current_vector1, V_dut_1, 'o', markersize = .5, color = "darkblue");
                        plt.grid(alpha = .2)
                        #plt.title(f"IV of a {device} with frequency {freq} Hz and amplitude {a}V")
                        plt.ylabel("voltage [V]")
                        plt.xlabel("current [A]")
                        plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm_ch1.pdf");
                        plt.clf();
                        plt.close();

                        fig, ax1 = plt.subplots();
                        color = "darkblue"
                        ax1.set_xlabel("time [a.u.]")
                        ax1.set_ylabel(r"current [$\mu$A]", color = color)
                        ax1.plot(full_current_vector1*1e6, 'o', markersize=.5, color = color);
                        ax1.tick_params(axis='y', labelcolor=color)
                        ax2 = ax1.twinx()
                        color = "darkorange"
                        ax2.set_xlabel("time [a.u.]")
                        ax2.set_ylabel("amplitude [mV]", color = color)
                        ax2.plot(V_dut_1*1e3, 'o', markersize=.5, color = color);
                        ax2.tick_params(axis='y', labelcolor=color)
                        ax1.grid(alpha=.2)
                        ax2.grid(alpha=.2);
                        plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm_timetrace_ch1.pdf");
                        plt.clf();
                        plt.close();

                    if fit:
                        self.IV_fit(V_dut_0, full_current_vector0, device, path)

                    sleep(self.sleep_time)

                    self.inst.stop_signal()

                except Exception as error:
                    print(error)
                    self.inst.stop_signal()

        #self.inst.stop_signal()

    def IV_fit(
            self, 
            x: Tuple, 
            y: Tuple, 
            device: str,
            path: str
        ):
        """Method to fit the IV curve"""

        def diode_IV(x, I_0, a, offset):
            return I_0*(np.exp(a*x) - 1) + offset

        def resistor_IV(x, R, b):
            return x/R + b

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


class LockIn():
    """Lock-in function of the board"""

    def __init__(
            self, 
            inst = Marcj("LockIn"), 
            freq: int = 500, 
            ampl: float = [2],
            offset: float = 0,
            play_type: str = "CONT",
            filter_type: str = "default",
            filter_order: int = 2,
            cutoff_freq: int = 1000,
            averages: int = 2,
            wave: str = "SIN",
            acq_time: float = 1,
            acq_cycles: int = 1,
            sleep_time: float = 1,
            folder_path: str = "dataLockIn",
            save_data: bool = True
        ):
        """Create an object on which we can run a Lock-In measure.
        
        Parameters
        ----------
        inst : str (optional)
            name of the device.
        freq : int
            frequency (in Hz) of the signal supplied to the DUT and used as a reference.
        ampls : tuple
            amplitude (in V) of the signal supplied to the DUT and used as a reference.
        offset : float (optional)
            dc offset of the signal (in V)
        play_type : str
            type of data playing. Can be `CONT` for continuous data playing or `LIM` for time limited data playing.
        filter_type : str
            type of low-pass filter to use (WILL BE IMPLEMENTED IN THE FUTURE).
        filer_order : int
            order of the low-pass filter to use.
        cutoff_freq : int
            3dB cutoff frequency of the low-pass filter.
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
        self.freq = freq
        self.ampl = ampl
        self.offset = offset
        self.play_type = play_type
        self.filter_type = filter_type
        self.filter_order = filter_order
        self.cutoff_freq = cutoff_freq
        self.averages = averages
        self.wave = wave
        self.acq_time = acq_time
        self.acq_cycles = acq_cycles
        self.sleep_time = sleep_time
        self.full_data = []
        self.path = folder_path
        self.save_data = save_data

        # Now we just connect to the device and set some options
        try:
            self.inst.connect()

            self.inst.set_amplitude(self.ampl)
            self.inst.set_frequency(self.freq)
            self.inst.set_offset(self.offset)
            self.inst.set_wave(self.wave)
        
        except Exception as error:
            print(error)
            self.close_instrument()

    def close_instrument(self):
        """Closes the instrument."""
        self.inst.disconnect()
            
    def run_LockIN(
            self,
            plot : bool = True
    ):
        """Run the Lock-In functionality of the board.
               
        Parameters
        ----------
        plot : bool
            Choose if you want to save a plot of the results.
        """

        try:
            # Play the signal that biases the DUT
            self.inst.play(self.play_type)
            sleep(self.sleep_time)

            # Check if folder exists and in case create it
            if self.save_data:
                path = self.path + f"/freq_{self.freq}Hz_ampl_{np.round(self.ampl,3)}V"
                if not os.path.exists(path):
                    os.makedirs(path)

            # Acquire both the signal from the DUT and the reference signal for at most 5 seconds (IT WILL BE IMPROVED)
            nloops = self.inst.read_signal(self.acq_time)

            ref_signal = np.array(nloops["ch0"])*self.inst.volt_maxval/self.inst.maxval
            DUT_signal = np.array(nloops["ch1"])*self.inst.volt_maxval/self.inst.maxval

            t = np.linspace(0, self.acq_time, len(ref_signal)) # Time vectore

            # Write the raw data to a .npz file
            if self.save_data:
                np.savez(path+"raw.npz", ref_signal=ref_signal, DUT_signal=DUT_signal, time=t)

            # Shift a copy of the reference signal by pi/2
            # Fit the reference signal and then shift it
            def sin_func(x, a, w, p):
                return a*np.sin(w*x + p)
            
            def shift_sin(x, a, w, p):
                return a*np.sin(w*x + p + np.pi/2)

            params = curve_fit(sin_func, t, ref_signal, p0=[self.ampl, self.freq, 0])
            amplt_fitted, freq_fitted, phase_fitted = params
            
            ref_signal_shifted = np.array(shift_sin(t, amplt_fitted, freq_fitted, phase_fitted))

            # Mix of the signals
            ref_with_DUT = ref_signal*DUT_signal
            shifted_ref_with_DUT = ref_signal_shifted*DUT_signal

            # Low-pass filtering
            nyq = 0.5 * self.inst.max_rate
            normal_cutoff = self.cutoff_freq / nyq
            b, a = butter(self.filter_order, normal_cutoff, btype='lowpass')

            filtered_ref_with_DUT = filtfilt(b, a, ref_with_DUT)
            filtered_shifted_ref_with_DUT = filtfilt(b, a, shifted_ref_with_DUT)

            R = np.sqrt(filtered_ref_with_DUT**2 + filtered_shifted_ref_with_DUT**2)
            theta = np.atan2(filtered_shifted_ref_with_DUT, filtered_ref_with_DUT)

            reconstructed_signal = R*np.cos(theta)

            # Write the processed data to a .npz file
            if self.save_data:
                np.savez(path+"processed.npz", R=R, theta=theta, signal=reconstructed_signal)
            
            if plot:
                plt.plot(filtered_ref_with_DUT, filtered_shifted_ref_with_DUT, 'o', markersize = .5, color = "darkblue");
                plt.grid(alpha = .2)
                #plt.title(f"IV of a {device} with frequency {freq} Hz and amplitude {a}V")
                plt.xlabel("X [V]")
                plt.ylabel("Y [V]")
                plt.savefig(path + f"/polar_signal.pdf");
                plt.clf();
                plt.close();
        
                plt.plot(t, reconstructed_signal, 'o', markersize = .5, color = "darkblue");
                plt.grid(alpha = .2)
                #plt.title(f"IV of a {device} with frequency {freq} Hz and amplitude {a}V")
                plt.xlabel("time [s]")
                plt.ylabel("amplitude [V]")
                plt.savefig(path + f"/time_trace.pdf");
                plt.clf();
                plt.close();

        except Exception as error:
            print(error)
            self.inst.stop_signal()
            self.close_instrument()