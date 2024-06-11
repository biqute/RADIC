/*
gcc functions.c FunctionPlayer.c -lasound -lm -o FunctionPlayer

./FunctionPlayer ww xx yy zz
    ww --> precision (16 or 24)
    xx --> time duration of the signal (in seconds)
    yy --> frequency of the sine wave (in Hz)
    zz --> type of wave to be generated (s for sinusoidal wave, t for triangular wave or q for a square wave)
*/

#include <math.h>
#include <alsa/asoundlib.h>
#include "functions.h"

int main(int argc, char *argv[]) 
{
//***************** Set the parameters and the board **********************//
    int precision = atoi(argv[1]);
    double duration = atof(argv[2]);
    double frequency = atof(argv[3]);

    int rate = 192000;                                  // Work at the maximum possible sampling rate
    int n_channels = 2;                                 // This is really the only choice since I didn't manage to make it works with just 1 channel
    int max_amplitude = pow(2, precision) / 2 - 1;      // Maximum number obtainable with specified precision
    int num_frames = rate * duration;                   // Number of frames
    
    int rc, dir  = 0;
    char *device = "hw:CARD=sndrpihifiberry,DEV=0";     // Name of the device
    snd_pcm_t *handle;                                  // A reference to the sound card
    snd_pcm_hw_params_t *params;                        // Information about hardware params
    snd_pcm_uframes_t frames = 512;                     // The size of the period

    // Here we open a reference to the sound card
    rc = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0)
    {
        fprintf(stderr, "unable to open defualt device: %s\n", snd_strerror(rc));
        exit(1);
    }

    // Now we allocate memory for the parameters structure on the stack
    snd_pcm_hw_params_alloca(&params);

    // Next, we're going to set all the parameters for the sound card

    // This sets up the soundcard with some default parameters and we'll customize it a bit afterwards
    snd_pcm_hw_params_any(handle, params);

    // Set the samples for each channel to be interleaved
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // Sets how our samples are represented
    if (precision == 16) 
    {
        snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    }
    else if (precision == 24)
    {
        snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S24_LE);
    }
    
    // We use 2 channels (left audio and right audio)
    snd_pcm_hw_params_set_channels(handle, params, n_channels);

    // Here we set our sampling rate.
    snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);

    // This sets the period size
    snd_pcm_hw_params_set_period_size(handle, params, frames, dir);

    snd_pcm_hw_params_set_periods(handle, params, 4, dir);

    // Parameters get written to the sound card
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0)
    {
        fprintf(stderr, "unable to set the hw params: %s\n", snd_strerror(rc));
        exit(1);
    }

    int significant_bits = snd_pcm_hw_params_get_sbits(params);
    printf("Number of significant bits: %i\n", significant_bits);

//*************************************************************************//    

    if (*argv[4] == 's')
    {
        play_sin(handle, frames, num_frames, rate, frequency, max_amplitude, precision);
    }
    else if (*argv[4] == 't')
    {
        play_triangular(handle, frames, num_frames, rate, frequency, max_amplitude, precision); 
    }
    else if (*argv[4] == 'q')
    {
        play_square(handle, frames, num_frames, rate, frequency, max_amplitude, precision); 
    }

    // Play all remaining samples before exitting
    snd_pcm_drain(handle);

    // Close the sound card handle
    snd_pcm_close(handle);

    return 0;
}