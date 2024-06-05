/* 
Functions that will be called in a Python script to improve the speed and
most of all to use 24 bit data that in C should be easier to implement :\
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <alsa/asoundlib.h>

#define ALSA_PCM_NEW_HW_PARAMS_API

#define PI 3.14159265358979323846

int main(int argc, char *argv[]) 
{   
    int precision = atoi(argv[1]);
    int duration = atoi(argv[2]);
    int frequency = atoi(argv[3]);
    int rate = 192000;
    int rc, dir  = 0;
    int max_amplitude = pow(2, precision) / 2 - 1; // Maximum number obtainable with specified precision
    int num_frames = rate * duration;              // Number of frames
    char *device = "hw:2";

    snd_pcm_t *handle;            // A reference to the sound card
    snd_pcm_hw_params_t *params;  // Information about hardware params
    snd_pcm_uframes_t frames = 512; // The size of the period
    //snd_pcm_subformat_t subformat = 3;

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
    else if (precision == 32)
    {
        snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S32_LE);
    }

    //snd_pcm_hw_params_set_subformat(handle, params, SND_PCM_SUBFORMAT_LAST);
    
    // We use 2 channels (left audio and right audio)
    snd_pcm_hw_params_set_channels(handle, params, 2);

    // Here we set our sampling rate.
    snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);

    // This sets the period size
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    // Finally, the parameters get written to the sound card
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0)
    {
        fprintf(stderr, "unable to set the hw params: %s\n", snd_strerror(rc));
        exit(1);
    }

    int significant_bits = snd_pcm_hw_params_get_sbits(params);
    printf("Number of significant bits: %i\n", significant_bits);

    // We will do different things depending on the resolution (for now at least 16 bits works)
    if (precision == 16) 
    {
        // This allocates memory to hold our samples
        char *buffer;
        buffer = (char *)malloc(frames * 4);
        int j = 0;
        short sample;

        for (int i = 0; i < num_frames; i++)
        {
            // Create a sample and convert it back to an integer
            double x = (double)i / (double)rate;
            double y = sin(2 * PI * frequency * x);
            sample = max_amplitude*y;

            // Store the sample in our buffer using Little Endian format
            // I do it two times because I have two channels
            buffer[0 + 4 * j] = sample & 0xff; // This operation apparently gives me the last 8 bit of the number contained in sample
            buffer[1 + 4 * j] = (sample & 0xff00) >> 8;
            buffer[2 + 4 * j] = sample & 0xff;
            buffer[3 + 4 * j] = (sample & 0xff00) >> 8;

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
    }
    else if (precision == 32) 
    {
        // This allocates memory to hold our samples
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

            // The following two methods to allocate memory are equivalent
            buffer[0 + 8*j] = sample & 0xff;
            buffer[1 + 8*j] = (sample >> 8) & 0xff;
            buffer[2 + 8*j] = (sample >> 16) & 0xff;
            buffer[3 + 8*j] = (sample >> 24) & 0xff;
            buffer[4 + 8*j] = sample & 0xff;
            buffer[5 + 8*j] = (sample >> 8) & 0xff;
            buffer[6 + 8*j] = (sample >> 16) & 0xff;
            buffer[7 + 8*j] = (sample >> 24) & 0xff;

            /*memset(&buffer[0+6*j], sample&0xff, 1);
            memset(&buffer[1+6*j], (sample>>8)&0xff, 1);
            memset(&buffer[2+6*j], (sample>>16)&0xff, 1);
            memset(&buffer[3+6*j], sample&0xff, 1);
            memset(&buffer[4+6*j], (sample>>8)&0xff, 1);
            memset(&buffer[5+6*j], (sample>>16)&0xff, 1);*/

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
    }

    return 0;
}