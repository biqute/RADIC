/*
 *  Script to play more or less arbitrary functions
 *  
 *  gcc FunctionPlayer.c -lasound -lm -o FunctionPlayer
 * 
 *  ./FunctionPlayer
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <sys/time.h>

#ifndef M_PI
    #define M_PI 3.14159
#endif

#define maxvalue 8388607

static unsigned int maxval = maxvalue;
static int precision = 24;                          // Generate a signal 10 seconds long
static double frequency = 700;                     // in Hz
static unsigned int rate = 192000;                   // Work at the maximum possible sampling rate
static int n_channels = 2;                          // This is really the only choice since I didn't manage to make it works with just 1 channel
static char *device = "hw:CARD=sndrpihifiberry,DEV=0";                 // Name of the device 
static char *type = "s";
static char *write_type = "c";                  // Continuous write method
static int amplitude = maxvalue;
static int duration = 2;                        // Duration of the signal in seconds
static int dc_offset = 0;                           // Offset of the signal, in arbitrary units

static unsigned int buffer_time = 500000;
static unsigned int period_time = 100000;
static int resample = 1;
static int period_event = 0;
static int verbose = 0;

static snd_pcm_format_t format = SND_PCM_FORMAT_S24_LE;

static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;
static snd_output_t *output = NULL;


/*
 *  Generate a sinusoidal function with specified parameters and assign it to memory areas
*/
static void generate_sine(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset, int count, double *_phase, int amplitude, int dc_offset)
{
    static double max_phase = 2. * M_PI;
    double phase = *_phase;
    double step = max_phase*frequency/(double)rate;
    unsigned char *samples[n_channels];
    int steps[n_channels];
    unsigned int chn;
    
    int format_bits = snd_pcm_format_width(format);
    int bps = format_bits / 8;
    int phys_bps = snd_pcm_format_physical_width(format) / 8;
    int big_endian = snd_pcm_format_big_endian(format) == 1;
    int to_unsigned = snd_pcm_format_unsigned(format) == 1;
    int is_float = (format == SND_PCM_FORMAT_FLOAT_LE || format == SND_PCM_FORMAT_FLOAT_BE);

    // Verify and preapare the contents of areas
    for (chn = 0; chn < n_channels; chn++) {
        if ((areas[chn].first % 8) != 0) {
            printf("areas[%u].first == %u, aborting...\n", chn, areas[chn].first);
            exit(EXIT_FAILURE);
        }
        samples[chn] = /*(signed short *)*/(((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));
        if ((areas[chn].step % 16) != 0) {
            printf("areas[%u].step == %u, aborting...\n", chn, areas[chn].step);
            exit(EXIT_FAILURE);
        }
        steps[chn] = areas[chn].step / 8;
        samples[chn] += offset * steps[chn];
    }

    // Fill the channels areas
    while (count-- > 0) {
        union {
            float f;
            int i;
        } fval;
        int res, i;
        if (is_float) {
            fval.f = sin(phase);
            res = fval.i;
        } else
            res = sin(phase) * amplitude;// + dc_offset;
        if (to_unsigned)
            res ^= 1U << (format_bits - 1);
        for (chn = 0; chn < n_channels; chn++) {
            /* Generate data in native endian format */
            if (big_endian) {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
            } else {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + i) = (res >>  i * 8) & 0xff;
            }
            samples[chn] += steps[chn];
        }
        phase += step;
        if (phase >= max_phase)
            phase -= max_phase;
    }
    *_phase = phase;
}

/*
 *  Generate a triangular function with specified parameters and assign it to memory areas
*/
static void generate_triangular(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset, int count, double *_phase, int amplitude, int dc_offset)
{

    // Try to implement an asymmetric triangul wave --> dc_offst in this situation is the rise time
    static double max_phase = 2. * M_PI;
    double phase = *_phase;
    double step = max_phase*frequency/(double)rate;
    unsigned char *samples[n_channels];
    int steps[n_channels];
    unsigned int chn;
    
    int format_bits = snd_pcm_format_width(format);
    int bps = format_bits / 8;
    int phys_bps = snd_pcm_format_physical_width(format) / 8;
    int big_endian = snd_pcm_format_big_endian(format) == 1;
    int to_unsigned = snd_pcm_format_unsigned(format) == 1;
    int is_float = (format == SND_PCM_FORMAT_FLOAT_LE || format == SND_PCM_FORMAT_FLOAT_BE);

    // Verify and preapare the contents of areas
    for (chn = 0; chn < n_channels; chn++) {
        if ((areas[chn].first % 8) != 0) {
            printf("areas[%u].first == %u, aborting...\n", chn, areas[chn].first);
            exit(EXIT_FAILURE);
        }
        samples[chn] = /*(signed short *)*/(((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));
        if ((areas[chn].step % 16) != 0) {
            printf("areas[%u].step == %u, aborting...\n", chn, areas[chn].step);
            exit(EXIT_FAILURE);
        }
        steps[chn] = areas[chn].step / 8;
        samples[chn] += offset * steps[chn];
    }

    // Fill the channels areas
    while (count-- > 0) {
        union {
            float f;
            int i;
        } fval;
        int res, i;
        if (is_float) 
        {
            fval.f = sin(phase);
            res = fval.i;
        } 
        else
        {
            res = asin(sin(phase))*(2*amplitude/M_PI);
        }
        if (to_unsigned)
            res ^= 1U << (format_bits - 1);
        for (chn = 0; chn < n_channels; chn++) {
            /* Generate data in native endian format */
            if (big_endian) {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
            } else {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + i) = (res >>  i * 8) & 0xff;
            }
            samples[chn] += steps[chn];
        }
        phase += step;
        if (phase >= max_phase)
            phase -= max_phase;
    }
    *_phase = phase;
}

/*
 *  Generate a square function with specified parameters and assign it to memory areas
*/
static void generate_square(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset, int count, double *_phase, int amplitude, int dc_offset)
{

    static double max_phase = 2. * M_PI;
    double phase = *_phase;
    double step = max_phase*frequency/(double)rate;
    unsigned char *samples[n_channels];
    int steps[n_channels];
    unsigned int chn;
    
    int format_bits = snd_pcm_format_width(format);
    int bps = format_bits / 8;
    int phys_bps = snd_pcm_format_physical_width(format) / 8;
    int big_endian = snd_pcm_format_big_endian(format) == 1;
    int to_unsigned = snd_pcm_format_unsigned(format) == 1;
    int is_float = (format == SND_PCM_FORMAT_FLOAT_LE || format == SND_PCM_FORMAT_FLOAT_BE);

    // Verify and preapare the contents of areas
    for (chn = 0; chn < n_channels; chn++) {
        if ((areas[chn].first % 8) != 0) {
            printf("areas[%u].first == %u, aborting...\n", chn, areas[chn].first);
            exit(EXIT_FAILURE);
        }
        samples[chn] = /*(signed short *)*/(((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));
        if ((areas[chn].step % 16) != 0) {
            printf("areas[%u].step == %u, aborting...\n", chn, areas[chn].step);
            exit(EXIT_FAILURE);
        }
        steps[chn] = areas[chn].step / 8;
        samples[chn] += offset * steps[chn];
    }



    // Fill the channels areas
    while (count-- > 0) {
        union {
            float f;
            int i;
        } fval;
        int res, i;
        if (is_float) 
        {
            fval.f = sin(phase);
            res = fval.i;
        } 
        else
        {
            res = amplitude*(2/M_PI*atan(tan(phase)) + 2/M_PI*atan(cos(phase)/sin(phase)));
        }
        if (to_unsigned)
            res ^= 1U << (format_bits - 1);
        for (chn = 0; chn < n_channels; chn++) {
            /* Generate data in native endian format */
            if (big_endian) {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
            } else {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + i) = (res >>  i * 8) & 0xff;
            }
            samples[chn] += steps[chn];
        }
        phase += step;
        if (phase >= max_phase)
            phase -= max_phase;
    }
    *_phase = phase;
}

/*
 *  Generate a constant function with specified parameters and assign it to memory areas
*/
static void generate_constant(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset, int count, double *_phase, int amplitude, int dc_offset)
{

    static double max_phase = 2. * M_PI;
    double phase = *_phase;
    double step = max_phase*frequency/(double)rate;
    unsigned char *samples[n_channels];
    int steps[n_channels];
    unsigned int chn;
    
    int format_bits = snd_pcm_format_width(format);
    int bps = format_bits / 8;
    int phys_bps = snd_pcm_format_physical_width(format) / 8;
    int big_endian = snd_pcm_format_big_endian(format) == 1;
    int to_unsigned = snd_pcm_format_unsigned(format) == 1;
    int is_float = (format == SND_PCM_FORMAT_FLOAT_LE || format == SND_PCM_FORMAT_FLOAT_BE);

    // Verify and preapare the contents of areas
    for (chn = 0; chn < n_channels; chn++) {
        if ((areas[chn].first % 8) != 0) {
            printf("areas[%u].first == %u, aborting...\n", chn, areas[chn].first);
            exit(EXIT_FAILURE);
        }
        samples[chn] = /*(signed short *)*/(((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));
        if ((areas[chn].step % 16) != 0) {
            printf("areas[%u].step == %u, aborting...\n", chn, areas[chn].step);
            exit(EXIT_FAILURE);
        }
        steps[chn] = areas[chn].step / 8;
        samples[chn] += offset * steps[chn];
    }



    // Fill the channels areas
    while (count-- > 0) {
        union {
            float f;
            int i;
        } fval;
        int res, i;
        if (is_float) 
        {
            fval.f = sin(phase);
            res = fval.i;
        } 
        else
        {
            res = amplitude;
        }
        if (to_unsigned)
            res ^= 1U << (format_bits - 1);
        for (chn = 0; chn < n_channels; chn++) {
            /* Generate data in native endian format */
            if (big_endian) {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
            } else {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + i) = (res >>  i * 8) & 0xff;
            }
            samples[chn] += steps[chn];
        }
        phase += step;
        if (phase >= max_phase)
            phase -= max_phase;
    }
    *_phase = phase;
}

/*
 *  Set the hardware parameters of the board
*/
static int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params, snd_pcm_access_t access)
{
    unsigned int rrate = rate;
    snd_pcm_uframes_t size;
    int err, dir;

    // Choose all parameters
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set hardware resampling
    err = snd_pcm_hw_params_set_rate_resample(handle, params, resample);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set access format (interleaved access)
    err = snd_pcm_hw_params_set_access(handle, params, access);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set sample format 
    err = snd_pcm_hw_params_set_format(handle, params, format);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set number of channels
    err = snd_pcm_hw_params_set_channels(handle, params, n_channels);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set stream rate
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }   
    if (rrate != rate)
    {
        printf("Rate doesn't match (requested %u Hz, get %i Hz)\n", rate, err);
        return -EINVAL;
    } 

    // Set buffer time
    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_buffer_size(params, &size);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }  
    buffer_size = size;

    // Set period time
    err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 
    err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }  
    period_size = size;
    
    // Write parameters to the device
    err = snd_pcm_hw_params(handle, params);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }  

    return 0;
}

/*
 * Set the software parameters of the board
*/
static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams)
{
    int err; 

    // Get current software parameters
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 

    /*
    Start the transfer when the buffer is almost full: (buffer_size/avail_min)*avail_min
    */
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 

    /*
    Allow the transfer when at least period_size samples can be processed or disable when period_event is enabled
    */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 
    // Enable period events when requested
    if (period_event)
    {
        err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
        if (err < 0)
        {
            printf("An error occurred: %s\n", snd_strerror(err));
            return err;
        } 
    }

    // Write parameters to playback device
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 

    return 0;
}

/*
 * Recovery of the board if an error occour
 */
static int xrun_recovery(snd_pcm_t *handle, int err)
{
    if (verbose)
    {
        printf("stream recovery\n");
    }
    if (err == -EPIPE)
    {
        err = snd_pcm_prepare(handle);
        if (err < 0)
        {
            printf("An error occurred: %s\n", snd_strerror(err));
        }
        return 0;
    }
    else if (err == -ESTRPIPE)
    {
        while ((err == snd_pcm_resume(handle)) == -EAGAIN)
        {
            sleep(1);
        }
        if (err < 0)
        {
            err = snd_pcm_prepare(handle);
            if (err < 0)
            {
                printf("An error occurred: %s\n", snd_strerror(err));
            }
        }
        return 0;
    }
    return err;
}

/*
 * Write data to the board
*/
static int write_loop(snd_pcm_t *handle, signed long *samples, snd_pcm_channel_area_t *areas, 
                unsigned int seconds, char *type, unsigned int amplitude, char *write_type, int offset)
{
    double phase = 0;
    signed long *ptr;
    int err, cptr;
    int loops = 375*seconds;

    if (*write_type == 'c')
    {
        while(1)
        {   
            if (*type == 's') 
            {
                generate_sine(areas, 0, period_size, &phase, amplitude, offset);
            }
            else if (*type == 't')
            {
                generate_triangular(areas, 0, period_size, &phase, amplitude, offset);
            }
            else if (*type == 'q')
            {
                generate_square(areas, 0, period_size, &phase, amplitude, offset);
            }
            else if (*type == 'c')
            {
                generate_constant(areas, 0, period_size, &phase, amplitude, offset);
            }
            else
            {
                printf("Error: wave format not recognized");
                exit(EXIT_FAILURE);
            }
                
            ptr = samples;
            cptr = period_size;
            while (cptr > 0)
            {
                err = snd_pcm_writei(handle, ptr, cptr);
                if (err == -EAGAIN) 
                    continue;
                if (err < 0)
                {
                    if (xrun_recovery(handle, err) < 0)
                    {
                        printf("Write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
                ptr += err*n_channels;
                cptr -= err;
            }
            loops--;
        }
    }
    else if (*write_type == 'l')
    {
        while(loops > 0)
        {   
            if (*type == 's') 
            {
                generate_sine(areas, 0, period_size, &phase, amplitude, offset);
            }
            else if (*type == 't')
            {
                generate_triangular(areas, 0, period_size, &phase, amplitude, offset);
            }
            else if (*type == 'q')
            {
                generate_square(areas, 0, period_size, &phase, amplitude, offset);
            }
            else if (*type == 'c')
            {
                generate_constant(areas, 0, period_size, &phase, amplitude, offset);
            }
            else
            {
                printf("Error: wave format not recognized");
                exit(EXIT_FAILURE);
            }
                
            ptr = samples;
            cptr = period_size;
            while (cptr > 0)
            {
                err = snd_pcm_writei(handle, ptr, cptr);
                if (err == -EAGAIN) 
                    continue;
                if (err < 0)
                {
                    if (xrun_recovery(handle, err) < 0)
                    {
                        printf("Write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
                ptr += err*n_channels;
                cptr -= err;
            }
            loops--;
        }
    }


}


struct transfer_method 
{
    const char *name;
    snd_pcm_access_t access;
    int (*transfer_loop)(snd_pcm_t *handle, signed long *samples, snd_pcm_channel_area_t *areas, 
                    unsigned int seconds, char *type, unsigned int amplitude, char *write_type, int offset);
};

static struct transfer_method transfer_methods[] = 
{
    {"write", SND_PCM_ACCESS_RW_INTERLEAVED, write_loop},
    {NULL, SND_PCM_ACCESS_RW_INTERLEAVED, NULL}
};

static void help(void)
{
    int k;
    printf(
"Usage: pcm [OPTION]... [FILE]...\n"
"-h,--help  help\n"
"-D,--device    playback device\n"
"-w,--waveform  waveform to be generated\n"
"-r,--rate  stream rate in Hz\n"
"-t,--type   continuous or time-limited signal\n"
"-c,--channels  count of channels in stream\n"
"-f,--frequency wave frequency in Hz\n"
"-d,--duration  duration of the signal in seconds\n"
"-a,--amplitude  amplitude of the signal in V\n"
"-s,--offset    dc offset of the signal in V\n"
"-b,--buffer    ring buffer size in us\n"
"-p,--period    period size in us\n"
"-m,--method    transfer method\n"
"-o,--format    sample format\n"
"-v,--verbose   show the PCM setup parameters\n"
"-n,--noresample  do not resample\n"
"\n");
        printf("Recognized waveform to be generated are: s - sinusoidal wave, t - triangular wave");
        printf("\n");
        printf("Recognized sample formats are:");
        for (k = 0; k < SND_PCM_FORMAT_LAST; ++k) {
                const char *s = snd_pcm_format_name(k);
                if (s)
                        printf(" %s", s);
        }
        printf("\n");
        printf("Recognized transfer methods are:");
        for (k = 0; transfer_methods[k].name; k++)
            printf(" \'%s\'", transfer_methods[k].name);
    printf("\n");
}


int main(int argc, char*argv[])
{

    struct option long_option[] = 
    {
        {"help", 0, NULL, 'h'},
        {"device", 1, NULL, 'D'},
        {"waveform", 1, NULL, 'w'},
        {"rate", 1, NULL, 'r'},
        {"type", 1, NULL, 't'},
        {"channels", 1, NULL, 'c'},
        {"frequency", 1, NULL, 'f'},
        {"duration", 1, NULL, 'd'},
        {"amplitude", 1, NULL, 'a'},
        {"offset", 1, NULL, 's'},
        {"buffer", 1, NULL, 'b'},
        {"period", 1, NULL, 'p'},
        {"method", 1, NULL, 'm'},
        {"format", 1, NULL, 'o'},
        {"verbose", 1, NULL, 'v'},
        {"noresample", 1, NULL, 'n'},
        {"pevent", 1, NULL, 'e'},
        {NULL, 0, NULL, 0},
    };

    int err;
    int morehelp = 0;
    int method = 0;
    signed long *samples;
    unsigned int chn;

    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_channel_area_t *areas;

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);    

    while (1) {
        int c;
        if ((c = getopt_long(argc, argv, "hD:w:r:t:c:f:d:a:s:b:p:m:o:vne", long_option, NULL)) < 0)
            break;
        switch (c) {
        case 'h':
            morehelp++;
            break;
        case 'D':
            device = strdup(optarg);
            break;
        case 'w':
            type = strdup(optarg);
            break;
        case 'r':
            rate = atoi(optarg);
            rate = rate < 4000 ? 4000 : rate;
            rate = rate > 196000 ? 196000 : rate;
            break;
        case 't':
            write_type = strdup(optarg);
            break;
        case 'c':
            n_channels = atoi(optarg);
            n_channels = n_channels < 1 ? 1 : n_channels;
            n_channels = n_channels > 1024 ? 1024 : n_channels;
            break;
        case 'f':
            frequency = atoi(optarg);
            frequency = frequency < 50 ? 50 : frequency;
            frequency = frequency > 5000 ? 5000 : frequency;
            break;
        case 'd':
            duration = atoi(optarg);
            break;
        case 'a':
            double read_amplitude = atof(optarg);
            amplitude = (read_amplitude*maxval)/2.96;        // Conversion of the amplitude from Volts to a.u.
            amplitude = amplitude < 1 ? 1 : amplitude;
            amplitude = amplitude > maxval ? maxval : amplitude;
            break;
        case 's':
            double read_offset = atof(optarg);
            dc_offset = (read_offset*maxval)/2.96;        // Conversion of the offset from Volts to a.u.
            dc_offset = dc_offset < 0 ? 0 : dc_offset;
            dc_offset = dc_offset > maxval ? maxval : dc_offset;
            break;
        case 'b':
            buffer_time = atoi(optarg);
            buffer_time = buffer_time < 1000 ? 1000 : buffer_time;
            buffer_time = buffer_time > 1000000 ? 1000000 : buffer_time;
            break;
        case 'p':
            period_time = atoi(optarg);
            period_time = period_time < 1000 ? 1000 : period_time;
            period_time = period_time > 1000000 ? 1000000 : period_time;
            break;
        case 'm':
            for (method = 0; transfer_methods[method].name; method++)
                    if (!strcasecmp(transfer_methods[method].name, optarg))
                    break;
            if (transfer_methods[method].name == NULL)
                method = 0;
            break;
        case 'o':
            for (format = 0; format < SND_PCM_FORMAT_LAST; format++) {
                const char *format_name = snd_pcm_format_name(format);
                if (format_name)
                    if (!strcasecmp(format_name, optarg))
                    break;
            }
            if (format == SND_PCM_FORMAT_LAST)
                format = SND_PCM_FORMAT_S16;
            if (!snd_pcm_format_linear(format) &&
                !(format == SND_PCM_FORMAT_FLOAT_LE ||
                  format == SND_PCM_FORMAT_FLOAT_BE)) {
                printf("Invalid (non-linear/float) format %s\n",
                       optarg);
                return 1;
            }
            break;
        case 'v':
            verbose = 1;
            break;
        case 'n':
            resample = 0;
            break;
        case 'e':
            period_event = 1;
            break;
        }
    }
 
    if (morehelp) {
        help();
        return 0;
    }
 
    err = snd_output_stdio_attach(&output, stdout, 0);
    if (err < 0) {
        printf("Output failed: %s\n", snd_strerror(err));
        return 0;
    }
 
    printf("Playback device is %s\n", device);
    printf("Stream parameters are %uHz, %s, %u channels\n", rate, snd_pcm_format_name(format), n_channels);
    printf("Waveform generated: %s\n", *type == 's' ? "sin" : *type == 't' ? "triangular" :  *type == 'q' ? "square" : "constant");
    printf("Wave frequency is %.4fHz\n", frequency);
    printf("Using transfer method: %s\n", transfer_methods[method].name);

    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("Playback open error: %s\n", snd_strerror(err));
        return 0;
    }
    
    if ((err = set_hwparams(handle, hwparams, transfer_methods[method].access)) < 0) {
        printf("Setting of hwparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if ((err = set_swparams(handle, swparams)) < 0) {
        printf("Setting of swparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
 
    if (verbose > 0)
        snd_pcm_dump(handle, output);
 
    samples = malloc((period_size * n_channels * snd_pcm_format_physical_width(format)) / 8);
    if (samples == NULL) {
        printf("No enough memory\n");
        exit(EXIT_FAILURE);
    }
    
    areas = calloc(n_channels, sizeof(snd_pcm_channel_area_t));
    if (areas == NULL) {
        printf("No enough memory\n");
        exit(EXIT_FAILURE);
    }
    for (chn = 0; chn < n_channels; chn++) {
        areas[chn].addr = samples;
        areas[chn].first = chn * snd_pcm_format_physical_width(format);
        areas[chn].step = n_channels * snd_pcm_format_physical_width(format);
    }

    err = transfer_methods[method].transfer_loop(handle, samples, areas, duration, type, amplitude, write_type, dc_offset);
    if (err < 0)
        printf("Transfer failed: %s\n", snd_strerror(err));
 
    free(areas);
    free(samples);
    snd_pcm_close(handle);
    return 0;
}