/* 
gcc FunctionReader.c -lasound -lm -o FunctionReader

./FunctionReader yy
    yy --> Number of loops to acquire
*/

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define ALSA_PCM_NEW_HW_PARAMS_API

int write_data_24(const char *filename, long long *data, int size)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("fopen failed");
        return 1;
    }

    size_t written = fwrite(data, sizeof(long long), size/sizeof(long long), fp);
    printf("Number of data written to file: %d\n", written);
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
//***************** Set the parameters and the board **********************//
    int precision = 24;
    int err;
    int rc, dir = 0;
    char *device = "hw:CARD=sndrpihifiberry,DEV=0";  // Name of the device
    int rate = 192000;      // Use maximum samplig rate

    /* 
    Number of loops to capture. 
        1 loop means that we capture 512 frames which corresponds to approx. 3 ms of data with specified sampling rate
        To aquire around 1 second of data we have to acquire 375 loops
    */
    int loops = atoi(argv[1]);       

    snd_pcm_t *capture_handle;       // Reference to the sound card
    snd_pcm_hw_params_t *hw_params;  // Information about hardware parameters
    snd_pcm_uframes_t frames = 512;  // The size of the period

    if ((err = snd_pcm_open(&capture_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        fprintf (stderr, "Cannot open audio device %s (%s)\n", device, snd_strerror(err));
        exit(1);
    }

    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(capture_handle, hw_params);
    snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    
    if (precision == 16)
    {
        snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    }
    else if (precision == 24)
    {
        snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S24_LE);
    }

    // We use 2 channels (left audio and right audio)
    snd_pcm_hw_params_set_channels(capture_handle, hw_params, 2);

    // Here we set our sampling rate.
    snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, &dir);

    // This sets the period size
    snd_pcm_hw_params_set_period_size(capture_handle, hw_params, frames, dir);

    snd_pcm_hw_params_set_periods(capture_handle, hw_params, 19, dir);

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

    snd_pcm_hw_params_get_buffer_size(hw_params, &frames);
    printf("Buffer size: %i\n", frames);

    unsigned int periods;
    snd_pcm_hw_params_get_periods(hw_params, &periods, &dir);
    printf("Number of periods in the buffer: %lu\n", periods);

    snd_pcm_hw_params_get_period_size(hw_params, &frames, &dir);
    printf("Period size in frames: %i\n", frames);

//*************************************************************************//    

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
        
    printf("\tReading process finished\n");

    if (write_data_24("data.txt", data, sizeof(data)) != 0)
    {
        return 1;
    }

    snd_pcm_close(capture_handle);

    return 0;
}