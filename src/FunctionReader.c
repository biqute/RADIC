/* 
gcc functions.c board_setup.c FunctionReader.c -lasound -lm -o FunctionReader

./FunctionReader
*/

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <getopt.h>
#include "functions.h"
#include "board_setup.h"

#define ALSA_PCM_NEW_HW_PARAMS_API

struct transfer_method 
{
    const char *name;
    snd_pcm_access_t access;
    int (*transfer_loop)(snd_pcm_t *capture_handle, snd_pcm_uframes_t frames, int loops, long long *data);
};

static struct transfer_method transfer_methods[] = 
{
    {"read", SND_PCM_ACCESS_RW_INTERLEAVED, read_loop},
    {NULL, SND_PCM_ACCESS_RW_INTERLEAVED, NULL}
};

static void help(void)
{
    printf(
        "-h,--help  help\n"
        "-d,--device    name of the device\n"
        "-l,-loops  number of loops to acquire\n"
        "-v,--verbose   show the PCM setup parameters\n\n"
    );
}

int main(int argc, char *argv[]) 
{

    struct option long_option[] = 
    {
        {"help", 0, NULL, 'h'},
        {"device", 1, NULL, 'd'},
        {"loops", 1, NULL, 'l'},
        {"verbose", 1, NULL, 'v'},
        {NULL, 0, NULL, 0},
    };

    snd_pcm_t *capture_handle;       // Reference to the sound card
    snd_pcm_hw_params_t *hw_params;  // Information about hardware parameters
    snd_pcm_uframes_t frames = 512;  // The size of the period

    snd_pcm_hw_params_malloc(&hw_params);

    int morehelp = 0;

    while (1) 
    {
        int c;
        if ((c = getopt_long(argc, argv, "hd:l:v", long_option, NULL)) < 0)
            break;
        switch (c)
        {
        case 'h':
            morehelp++;
            break;
        case 'd':
            device = strdup(optarg);
            break;
        case 'l':
            loops = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        }
    }

    if (morehelp)
    {
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
    printf("Number of loops acquired: %i\n", loops);
    printf("Using transfer method: %s\n", transfer_methods[method].name);

    if ((err = snd_pcm_open(&capture_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        fprintf (stderr, "Cannot open audio device %s (%s)\n", device, snd_strerror(err));
        exit(1);
    }

    if ((err = set_read_hwparameters(capture_handle, hw_params, frames)) < 0) {
        printf("Setting of hwparams failed: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if (verbose > 0)
        snd_pcm_dump(capture_handle, output);

    /*
     *  Now the actual reading process begins
    */

    int num_samples = frames*loops;
    long long data[num_samples]; // This will contain all the samples

    read_loop(capture_handle, frames, loops, data);

    printf("Reading process finished\n");

    if (data_to_file("data.txt", data, sizeof(data)) != 0)
        return 1;

    snd_pcm_close(capture_handle);

    return 0;
}