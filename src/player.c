/*
gcc player.c -lasound -lm -o player

./player
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define PI 3.14159

int main(int argc, char*argv[])
{
    int precision = 24;
    double duration = 10;                               // Generate a signal 10 seconds long
    double frequency = 1000;                            // in Hz
    int rate = 192000;                                  // Work at the maximum possible sampling rate
    int n_channels = 2;                                 // This is really the only choice since I didn't manage to make it works with just 1 channel
    int max_amplitude = pow(2, precision) / 2 - 1;      // Maximum number obtainable with specified precision
    int num_frames = rate * duration;                   // Number of frames

    int rc, dir  = 0;
    char *device = "hw:CARD=sndrpihifiberry,DEV=0";     // Name of the device
    snd_pcm_t *handle;                                  // A reference to the sound card
    snd_pcm_hw_params_t *params;                        // Information about hardware params
    snd_pcm_uframes_t frames = 512;                     // The size of the period

/***** 
 ***** Setting of the parameters 
*****/

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
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S24_LE);
    
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

/***** 
 ***** Create and write the signal
*****/
    char *buffer;
    int size = frames * 8;
    buffer = (char *)malloc(size);
    int j = 0;
    long sample;
    for (int i = 0; i < num_frames; i++)
    {
        // Create a sample and convert it back to an integer
        double x = (double)i / (double)rate;
        double y = sin(2 * PI * frequency * x);
        sample = max_amplitude*y;

        // Divide the sample created into 4 bytes and assign them to the buffer
        // Since 24 bit precision and Little Endian format are setted only the lower three bytes should be used
        buffer[0 + 8*j] = sample & 0xff;
        buffer[1 + 8*j] = (sample >> 8) & 0xff;
        buffer[2 + 8*j] = (sample >> 16) & 0xff;
        buffer[3 + 8*j] = (sample >> 24) & 0xff;
        buffer[4 + 8*j] = sample & 0xff;
        buffer[5 + 8*j] = (sample >> 8) & 0xff;
        buffer[6 + 8*j] = (sample >> 16) & 0xff;
        buffer[7 + 8*j] = (sample >> 24) & 0xff;

        // If we have a buffer full of samples, write 1 period of samples to the sound card
        if (j++ == frames)
        {   
            j = snd_pcm_writei(handle, buffer, frames);

            // Check for under runs
            if (j < 0)
            {   
                snd_pcm_prepare(handle);
            }
            j = 0;
        }
    }

    // Play all remaining samples before exitting
    snd_pcm_drain(handle);

    // Close the sound card handle
    snd_pcm_close(handle);

    return 0;
}