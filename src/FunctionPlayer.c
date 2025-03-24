/*
 *  Script to play more or less arbitrary functions
 *  
 *  gcc board_setup.c functions.c FunctionPlayer.c -lasound -lm -o FunctionPlayer
 * 
 *  ./FunctionPlayer
 *  --> If run like this the script will use the default values setted in "board_setup.h"
*/

#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <sys/time.h>
#include "board_setup.h"
#include "functions.h"

struct transfer_method 
{
    const char *name;
    snd_pcm_access_t access;
    int (*transfer_loop)(snd_pcm_t *handle, signed long *samples, snd_pcm_channel_area_t *areas, 
                    unsigned int seconds, char *type, unsigned int amplitude, char *write_type);
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
"-c,--channels  number of channels in stream\n"
"-f,--frequency wave frequency in Hz\n"
"-d,--duration  duration of the signal in seconds\n"
"-a,--amplitude  amplitude of the signal in V\n"
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
        if ((c = getopt_long(argc, argv, "hD:w:r:t:c:f:d:a:b:p:m:o:vne", long_option, NULL)) < 0)
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
            amplitude = (read_amplitude*maxval)/3.06;        // Conversion of the amplitude from Volts to a.u.
            amplitude = amplitude < 1 ? 1 : amplitude;
            amplitude = amplitude > maxval ? maxval : amplitude;
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
    printf("Waveform generated: %s\n", *type == 's' ? "sin" : "triangular");
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

    err = transfer_methods[method].transfer_loop(handle, samples, areas, duration, type, amplitude, write_type);
    if (err < 0)
        printf("Transfer failed: %s\n", snd_strerror(err));
 
    free(areas);
    free(samples);
    snd_pcm_close(handle);
    return 0;
}