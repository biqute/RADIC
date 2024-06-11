#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define sgn(x) (x < 0) ? -1 : (x > 0)
#define PI 3.14159265358979323846
#define ALSA_PCM_NEW_HW_PARAMS_API

void play_sin(snd_pcm_t *handle, int frames, int num_frames, int rate, double frequency, int max_amplitude, int precision)
{   
    if (precision == 24)
    {
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
    }
    else if (precision == 16)
    {
        char *buffer;
        int size = frames * 4;
        buffer = (char *)malloc(size);
        int j = 0;
        short sample;
        for (int i = 0; i < num_frames; i++)
        {
            // Create a sample and convert it back to an integer
            double x = (double)i / (double)rate;
            double y = sin(2 * PI * frequency * x);
            sample = max_amplitude*y;

            // Divide the sample created into 4 bytes and assign them to the buffer
            // Since 24 bit precision and Little Endian format are setted only the lower three bytes should be used
            buffer[0 + 4 * j] = sample & 0xff; // This operation gives me the last 8 bit of the number contained in sample
            buffer[1 + 4 * j] = (sample & 0xff00) >> 8; // This give the first 8 bit in sample
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
    }
}

void play_triangular(snd_pcm_t *handle, int frames, int num_frames, int rate, double frequency, int max_amplitude, int precision)
{
    if (precision == 24)
    {
        char *buffer;
        int size = frames * 8;
        buffer = (char *)malloc(size);
        int j = 0;
        long sample;
        for (int i = 0; i < num_frames; i++)
        {
            // Create a sample and convert it back to an integer
            double x = 2*max_amplitude/PI;
            double y = asin(sin(2*PI*frequency*i/rate));
            sample = x*y;

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
    }
    else if (precision == 16)
    {
        char *buffer;
        int size = frames * 4;
        buffer = (char *)malloc(size);
        int j = 0;
        short sample;
        for (int i = 0; i < num_frames; i++)
        {
            // Create a sample and convert it back to an integer
            double x = 2*max_amplitude/PI;
            double y = asin(sin(2*PI*frequency*i/rate));
            sample = x*y;

            // Divide the sample created into 4 bytes and assign them to the buffer
            // Since 24 bit precision and Little Endian format are setted only the lower three bytes should be used
            buffer[0 + 4 * j] = sample & 0xff; // This operation gives me the last 8 bit of the number contained in sample
            buffer[1 + 4 * j] = (sample & 0xff00) >> 8; // This give the first 8 bit in sample
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
    }    
}

void play_square(snd_pcm_t *handle, int frames, int num_frames, int rate, double frequency, int max_amplitude, int precision)
{
    if (precision == 24)
    {
        char *buffer;
        int size = frames * 8;
        buffer = (char *)malloc(size);
        int j = 0;
        long sample;
        for (int i = 0; i < num_frames; i++)
        {
            // Create a sample and convert it back to an integer
            float y = sin(2*PI*(float)frequency*i/(float)rate);
            sample = max_amplitude*(sgn(y));

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
    }
    else if (precision == 16)
    {
        char *buffer;
        int size = frames * 4;
        buffer = (char *)malloc(size);
        int j = 0;
        short sample;
        for (int i = 0; i < num_frames; i++)
        {
            // Create a sample and convert it back to an integer
            float y = sin(2*PI*(float)frequency*i/(float)rate);
            sample = max_amplitude*(sgn(y));

            // Divide the sample created into 4 bytes and assign them to the buffer
            // Since 24 bit precision and Little Endian format are setted only the lower three bytes should be used
            buffer[0 + 4 * j] = sample & 0xff; // This operation gives me the last 8 bit of the number contained in sample
            buffer[1 + 4 * j] = (sample & 0xff00) >> 8; // This give the first 8 bit in sample
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
    }  
}