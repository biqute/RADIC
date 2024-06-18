/* 
gcc reader.c -lasound -lm -o reader

./reader
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

int write_data_24(const char *filename, long long *data, int size)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("fopen failed");
        return 1;
    }

    size_t written = fwrite(data, sizeof(long long), size/sizeof(long long), fp);
    if (written != size/sizeof(long long))
    {
        perror("fwrite failed");
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}

int main(int argc, char *argv[]) 
{
    int precision = 24;
    int loops = 100;                                    // Number of loops to read
    double frequency = 5000;                            // in Hz
    int rate = 192000;                                  // Work at the maximum possible sampling rate
    int n_channels = 2;                                 // This is really the only choice since I didn't manage to make it works with just 1 channel
    int max_amplitude = pow(2, precision) / 2 - 1;      // Maximum number obtainable with specified precision

    int err;
    int rc, dir = 0;
    char *device = "hw:CARD=sndrpihifiberry,DEV=0";     // Name of the device
    snd_pcm_t *capture_handle;                                  // A reference to the sound card
    snd_pcm_hw_params_t *hw_params;                        // Information about hardware params
    snd_pcm_uframes_t frames = 512;                     // The size of the period   

/***** 
 ***** Setting of the parameters 
*****/

    if ((err = snd_pcm_open(&capture_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        fprintf (stderr, "Cannot open audio device %s (%s)\n", device, snd_strerror(err));
        exit(1);
    }

    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(capture_handle, hw_params);
    snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    
    snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S24_LE);

    // We use 2 channels (left audio and right audio)
    snd_pcm_hw_params_set_channels(capture_handle, hw_params, 2);

    // Here we set our sampling rate.
    snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, &dir);

    // This sets the period size
    snd_pcm_hw_params_set_period_size(capture_handle, hw_params, frames, dir);

    snd_pcm_hw_params_set_periods(capture_handle, hw_params, 4, dir);

    // Finally, the parameters get written to the sound card
    rc = snd_pcm_hw_params(capture_handle, hw_params);
    if (rc < 0)
    {
        fprintf(stderr, "unable to set the hw params: %s\n", snd_strerror(rc));
        exit(1);
    }

    if ((err = snd_pcm_prepare(capture_handle)) < 0)
    {
        fprintf(stderr, "Cannot prepare audio interfate for use (%s)\n", snd_strerror(err));
        exit(1);
    }

/***** 
 ***** Read the signal
*****/

    int num_samples = frames*loops;
    long long data[num_samples]; // This will contain all the samples
    int j = 0;

    while (loops > 0)
    {
        loops--;
        long long buffer[frames];
        rc = snd_pcm_readi(capture_handle, buffer, frames);

        if (rc == -EPIPE || rc == -EBADFD || rc == -ESTRPIPE) 
        {
            perror("Reading failed");
        }

        for (int i = 0; i < rc; i++)
        {   
            data[j*frames + i] = buffer[i];
        }
            
        j++;
    }

    // This is just to save the data in a file
    if (write_data_24("data.txt", data, sizeof(data)) != 0)
    {
        return 1;
    }

    snd_pcm_close(capture_handle);

    return 0;
}