"""
Contains a class that makes it easier to execute an IV curve measuring experiment.

THIS WORKS FOR ARBITRARY ACQUISITION TIMES

TO-DO:
    add a correction for the stabilization time correction in the acq_cycles computation --> don't know if it is needed
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
from scipy.signal import butter, filtfilt, sawtooth, correlate

class IVCurve():
    """IV curve acquistion"""

    def __init__(
            self, 
            resistance: int,
            inst = Marcj("JJ", address="10.30.44.176"), 
            freqs: tuple = [143], 
            ampls: tuple = [0.5],
            offset: float = 0,
            play_type: str = "CONT",
            averages: int = 2,
            wave: str = "TRIA",
            acq_time: float = 1,
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
            sets the length of the data acquisition (in seconds)
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
        self.sleep_time = sleep_time
        self.full_data = []
        self.path = folder_path
        self.resistance = resistance
        self.save_data = save_data

        # How many acquisitions we have to do to have enough data for the setted number of averages
        self.acq_cycles = 1 

        self.inst.connect()

        # Set common settings of the device
        self.inst.set_wave(self.wave)
        self.inst.set_offset(self.offset) 

    def run_experiment(
            self, 
            plot: bool = True, 
            data_processing: bool = True,
            amplitude_correction: bool = True,
            verbose: bool = False,
            stabilization_time: float = 0
            #fit: bool = False, 
            #device: str = "jj"
        ):
        """Run an IV-curve measure
        
        Parameters
        ---------- 
        plot : bool
            Choose if you want to save a plot of the results.
        data_processing : bool
            Choose if you want to do some data processing on the results.
        amplitude_correction : bool
            Choose if you want to apply the amplitude correction due to the presence of the pcb. To use when the pcb is present    
        verbose : bool
            Choose if the code will spit out informations during runtime.
        """

        self.plot = plot
        self.verbose = verbose
        self.stabilization_time = stabilization_time

        for freq in self.freqs:                    # cycle over the frequencies
            for a in self.ampls:                   # cycle over the amplitudes
                self.inst.set_frequency(freq)
                self.inst.set_amplitude(a)

                if self.verbose:
                    print(f"Setted frequency: {freq} Hz")
                    print(f"Setted voltage amplitude: {a} V --> Current amplitude: {a/self.resistance} A")

                n_points = self.inst.max_rate//freq # Number of points in a cycle at given acquisition rate and signal frequency
                n_waveforms = self.acq_time*self.inst.max_rate//n_points # Number of waveforms in a given acquisition time

                # At the end of this cycle we should know how many acquisition we have to do
                while n_waveforms*self.acq_cycles < self.averages:
                    self.acq_cycles +=1

                if self.verbose:
                    print(f"We need {self.acq_cycles} acquistion cycles each lasting {self.acq_time} seconds.")

                # Create the folder that will contain the data
                if self.save_data:
                    path = self.path + f"/freq_{freq}Hz_ampl_{a:.3f}V"
                    if not os.path.exists(path):
                        os.makedirs(path)

                self.inst.play(self.play_type)
                
                # Now start the acquisition process
                try: 
                    for idx in range(self.acq_cycles):
                        sleep(1)
                        nloops = self.inst.read_signal(self.acq_time)

                        ch0 = np.array(nloops["ch0"])*3.05/self.inst.maxval
                        ch1 = np.array(nloops["ch1"])*3.05/self.inst.maxval

                        # Write the raw data to a .npz file
                        # Each 5 seconds acquisition is saved in a different .npz file
                        # Save both the full array and the sliced array for the stabilization time correction
                        if self.save_data:
                            np.savez(path + f"/RAW_data_averages{self.averages}_resistance_{self.resistance}Ohm_{idx}.npz", 
                                    raw_ch0 = ch0, 
                                    raw_ch1 = ch1,
                                    cut_raw_ch0 = ch0[self.inst.max_rate:],
                                    cut_raw_ch1 = ch1[self.inst.max_rate:]
                                )

                        if verbose:
                            print(f"Acqusition cycle {idx+1} out of {self.acq_cycles} done.\n")


                except Exception as error:
                    print(error)
                    self.inst.stop_signal()

                self.inst.stop_signal()

                # Now call the data_processing function to do stuff on the signal
                if data_processing:
                    self.basic_data_processing(a, freq, n_points, path, amplitude_correction)

    def close_instrument(self):
        """Closes the instrument."""
        self.inst.disconnect()

    def triangular_signal(self, x, a, w, p):
        """Return a triangular signal with specified parameter used to fit and align phase shifted time traces"""
        return a * sawtooth(2*np.pi*w*x + p, 0.5) 
    
    def output_conversion_function(self, nominal_amplitude):
        """Returns the real amplitude"""
        m = 0.91
        real_amplitude =  m*nominal_amplitude
        return real_amplitude

    def input_conversion_function(self, measured_amplitude):
        """Returns the real amplitude"""
        m = 1/0.91
        real_amplitude =  m*measured_amplitude
        return real_amplitude

    def basic_data_processing(
            self,
            amplitude: float,
            frequency: float,
            n_points: int,
            path: str,
            amplitude_correction: bool = True,
        ):
        """
        Performs a preliminary data processing. 
        It reads the .npz files containing the data, apply the stabilization time correction, align the time traces and averges.
        """
        
        # "Artificial current vector" and time
        current_vector_rise = np.linspace(-amplitude/self.resistance, amplitude/self.resistance, int(np.ceil(n_points/2)))
        current_vector_fall = np.linspace(amplitude/self.resistance, -amplitude/self.resistance, int(np.floor(n_points/2)))
        I = np.concat((current_vector_rise, current_vector_fall))

        if amplitude_correction:
            I = self.output_conversion_function(I) # corrected values for the presence of the pcb

        # Read the .npz files
        # These arrays contains the full acquistions
        full_ch0 = np.empty(shape=(self.acq_cycles, self.inst.max_rate*self.acq_time))
        full_ch1 = np.empty(shape=(self.acq_cycles, self.inst.max_rate*self.acq_time))

        for idx in range(self.acq_cycles):
            full_ch0[idx,:] = np.load(path + f"/RAW_data_averages{self.averages}_resistance_{self.resistance}Ohm_{idx}.npz")["raw_ch0"]#*self.inst.volt_maxval/self.inst.maxval
            full_ch1[idx,:] = np.load(path + f"/RAW_data_averages{self.averages}_resistance_{self.resistance}Ohm_{idx}.npz")["raw_ch1"]#*self.inst.volt_maxval/self.inst.maxval

        # Delete last cycle of reference signal and create new array
        ref0 = full_ch0[0,:-n_points]
        ref1 = full_ch1[0,:-n_points]

        data_new0 = np.empty(shape=(self.acq_cycles,((self.acq_time-self.stabilization_time)*self.inst.max_rate-n_points)))
        data_new1 = np.empty(shape=(self.acq_cycles,((self.acq_time-self.stabilization_time)*self.inst.max_rate-n_points)))
        data_new0[0,:] = ref0[self.inst.max_rate*self.stabilization_time:]
        data_new1[0,:] = ref1[self.inst.max_rate*self.stabilization_time:]
        
        for i in range(self.acq_cycles-1):
            corr0 = correlate(full_ch0[0,:], full_ch0[i+1,:], mode="full")
            corr1 = correlate(full_ch1[0,:], full_ch1[i+1,:], mode="full")

            lags0 = np.arange(-len(full_ch0[0,:])+1, len(full_ch0[0,:]))
            lags1 = np.arange(-len(full_ch1[0,:])+1, len(full_ch1[0,:]))

            lag0 = lags0[np.argmax(corr0)]
            lag1 = lags1[np.argmax(corr1)]

            full_ch0[i+1,:] = np.roll(full_ch0[i+1,:], lag0)
            full_ch1[i+1,:] = np.roll(full_ch1[i+1,:], lag1)

            data_new0[i+1,:] = full_ch0[i+1,self.inst.max_rate*self.stabilization_time:-n_points]
            data_new1[i+1,:] = full_ch1[i+1,self.inst.max_rate*self.stabilization_time:-n_points]

        
        # Now do the average
        data_new0 = data_new0.ravel()
        data_new1 = data_new1.ravel()

        waveforms0 = []
        waveforms1 = []
        n_waveforms = (self.acq_time-self.stabilization_time)*self.inst.max_rate//n_points

        #for i in range(self.acq_cycles):
        #    for n in range(n_waveforms):
        #        try:
        #            waveforms0.append(data_new0[i,n*n_points:(n+1)*n_points])
        #            waveforms1.append(data_new1[i,n*n_points:(n+1)*n_points])
        #        except:
        #            continue

        for avg in range(self.averages):
            try:
                waveforms0.append(data_new0[avg*n_points:(avg+1)*n_points])
                waveforms1.append(data_new1[avg*n_points:(avg+1)*n_points])
            except :
                ValueError("Too many averages selected")
        
        avg_ch0 = np.average(waveforms0, axis=0)
        avg_ch1 = np.average(waveforms1, axis=0)

        time = np.linspace(0, 1/frequency, n_points)

        # Now I have to order the avg vector and align it with the currents
        min_idx_0 = np.argmin(avg_ch0)
        V_dut_0 = np.concatenate((avg_ch0[min_idx_0:], avg_ch0[:min_idx_0])) # This is the shifted array

        min_idx_1 = np.argmin(avg_ch1)
        V_dut_1 = np.concatenate((avg_ch1[min_idx_1:], avg_ch1[:min_idx_1])) # This is the shifted array

        if amplitude_correction:
            V_dut_0 = self.input_conversion_function(V_dut_0) # corrected values for the pcb presence
            V_dut_1 = self.input_conversion_function(V_dut_1) # corrected values for the pcb presence


        # Save and plot
        # Write averaged data to .npz file
        if self.save_data:
            np.savez(path + f"/AVERAGED_DATA_averages_{self.averages}_resistance_{self.resistance}Ohm.npz",
                    avg_ch0 = avg_ch0,
                    avg_ch1 = avg_ch1,
                    V_dut_0 = V_dut_0,
                    V_dut_1 = V_dut_1,
                    bias_current = I,
                    cycle_time = time
                )

            if self.plot:
                plt.plot(I, V_dut_0, 'o', markersize = .5, color = "darkblue");
                plt.grid(alpha = .2)
                plt.ylabel("voltage [V]")
                plt.xlabel("current [A]")
                plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm_channel0.pdf");
                plt.clf();
                plt.close();

                plt.plot(I, V_dut_1, 'o', markersize = .5, color = "darkblue");
                plt.grid(alpha = .2)
                plt.ylabel("voltage [V]")
                plt.xlabel("current [A]")
                plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm_channel1.pdf");
                plt.clf();
                plt.close();

                fig, ax1 = plt.subplots();
                color = "darkblue"
                ax1.set_xlabel("time [s]")
                ax1.set_ylabel(r"current [$\mu$A]", color = color)
                ax1.plot(time, I*1e6, 'o', markersize=.5, color = color);
                ax1.tick_params(axis='y', labelcolor=color)
                ax2 = ax1.twinx()
                color = "darkorange"
                ax2.set_xlabel("time [s]")
                ax2.set_ylabel("amplitude [mV]", color = color)
                ax2.plot(time, V_dut_0*1e3, 'o', markersize=.5, color = color);
                ax2.tick_params(axis='y', labelcolor=color)
                ax1.grid(alpha=.2)
                ax2.grid(alpha=.2);
                plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm_timetrace_ch0.pdf");
                plt.clf();
                plt.close();

                fig, ax1 = plt.subplots();
                color = "darkblue"
                ax1.set_xlabel("time [s]")
                ax1.set_ylabel(r"current [$\mu$A]", color = color)
                ax1.plot(time, I*1e6, 'o', markersize=.5, color = color);
                ax1.tick_params(axis='y', labelcolor=color)
                ax2 = ax1.twinx()
                color = "darkorange"
                ax2.set_xlabel("time [s]")
                ax2.set_ylabel("amplitude [mV]", color = color)
                ax2.plot(time, V_dut_1*1e3, 'o', markersize=.5, color = color);
                ax2.tick_params(axis='y', labelcolor=color)
                ax1.grid(alpha=.2)
                ax2.grid(alpha=.2);
                plt.savefig(path + f"/IV_averages_{self.averages}_resistance_{self.resistance}Ohm_timetrace_ch1.pdf");
                plt.clf();
                plt.close();


class LockIn():
    """Lock-in function of the board"""

    def __init__(
            self, 
            inst = Marcj("LockIn"), 
            resistance : int = 11000,
            points : int = 10000,
            freq: int = 500, 
            ac_ampl: float = 0.5,
            dc_bias: float = 0.5,
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
        self.resistance = resistance
        self.points = points
        self.freq = freq
        self.ac_ampl = ac_ampl
        self.dc_bias = dc_bias
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

        self.inst.connect()

        self.inst.set_wave(self.wave)

    def close_instrument(self):
        """Closes the instrument."""
        #self.inst.stop_signal()
        self.inst.disconnect()

    def signal(self, t, a, w, p):
        return a*np.cos(2*np.pi*w*t + p)

    def shift_signal(self, t, a, w, p):
        return a*np.cos(2*np.pi*w*t + p + np.pi/2)
            
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
            self.inst.set_amplitude(self.ac_ampl)
            self.inst.set_frequency(self.freq)

            # DC bias vector
            dc_rise = np.linspace(-self.dc_bias, self.dc_bias, int(np.ceil(self.points/2)))
            dc_fall = np.linspace(self.dc_bias, -self.dc_bias, int(np.floor(self.points/2)))
            dc_bias = np.concatenate((dc_rise, dc_fall))

            # Create the folder that will contain the data
            if self.save_data:
                path = self.path + f"/freq_{self.freq}Hz_ampl_{self.ac_ampl:.3f}V/"
                if not os.path.exists(path):
                    os.makedirs(path)

            time = np.linspace(0, self.acq_time, int(self.acq_time*self.inst.max_rate))

            R = []
            Theta = []

            # Swipe the DC values
            for i in dc_bias:

                print(i)
                
                Ir = i/self.resistance + self.signal(time, self.ac_ampl/self.resistance, self.freq, 0)
                Ir_s = i/self.resistance + self.shift_signal(time, self.ac_ampl/self.resistance, self.freq, 0)

                self.inst.set_offset(i)
                self.inst.play(self.play_type)

                sleep(1)
                nloops = self.inst.read_signal(self.acq_time)

                # Save raw amplitudes that will be converted to voltages later on
                ch0 = (np.array(nloops["ch0"])*self.inst.volt_maxval/self.inst.maxval)[:len(Ir)]
                ch1 = np.array(nloops["ch1"])*self.inst.volt_maxval/self.inst.maxval

                self.inst.stop_signal()

                # Write the raw data to a .npz file
                if self.save_data:
                    np.savez(path + f"/RAW_data_resistance_{self.resistance}Ohm_{np.round(i,3)}dc_value.npz", 
                            raw_ch0 = ch0, 
                            raw_ch1 = ch1
                            #cut_ch0 = ch0[self.inst.max_rate:],
                            #cut_ch1 = ch1[self.inst.max_rate:]
                        )
                    
                VrVs = Ir*ch0
                VrVs_s = Ir_s*ch0

                nyq = 0.5 * self.inst.max_rate
                normal_cutoff = self.cutoff_freq / nyq
                b, a = butter(self.filter_order, normal_cutoff, btype='lowpass')

                VrVs_lp = filtfilt(b, a, VrVs)
                VrVs_s_lp = filtfilt(b, a, VrVs_s) 

                # Remove initial wiggle
                X = VrVs_lp[1920:]
                Y = VrVs_s_lp[1920:]

                # Average and transform to polar coordinates
                X = np.average(X)
                Y = np.average(Y)

                R.append((np.sqrt(X**2+Y**2)/self.ac_ampl)*2*np.sqrt(2))
                Theta.append(np.atan2(Y,X))

            # Write the processed data to a .npz file
            if self.save_data:
                np.savez(path+"dVdI.npz", R = R, theta = Theta, current = dc_bias/self.resistance)

            if plot:
                plt.plot(dc_bias/self.resistance, R, 'o', markersize = .5, color = "darkblue");
                plt.grid(alpha = .2)
                plt.xlabel("I [A]")
                plt.ylabel(r"$\frac{dV}{dI}$ [$\Omega$]")
                plt.savefig(path + f"/1Fmode.pdf");
                plt.clf();
                plt.close();

        except Exception as error:
            print(error)
            self.inst.stop_signal()
            self.close_instrument()