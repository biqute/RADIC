#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define ALSA_PCM_NEW_HW_PARAMS_API

int write_data(const char *filename, short *data, int size)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror("fopen failed");
        return 1;
    }

    size_t written = fwrite(data, sizeof(short), size/sizeof(short), fp);
    printf("Number of data written to file: %d\n", written);
    if (written != size/sizeof(short))
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
    int err;
    int rc, dir = 0;
    char *device = "hw:2";
    int precision = atoi(argv[1]); // Precision to use. It can be 16, 24 or 32
    int rate = 192000;

    /* 
    Number of loops to capture. 
        1 loop means that we capture 512 frames which corresponds to approx. 3 ms of data
        To aquire 1 second of data we have to acquire 375 loops
    */
    int loops = atoi(argv[2]);       

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
    else if (precision == 32)
    {
        snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S32_LE);
    }

    // We use 2 channels (left audio and right audio)
    snd_pcm_hw_params_set_channels(capture_handle, hw_params, 2);

    // Here we set our sampling rate.
    snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, &dir);

    // This sets the period size
    snd_pcm_hw_params_set_period_size_near(capture_handle, hw_params, &frames, &dir);

    // Finally, the parameters get written to the sound card
    rc = snd_pcm_hw_params(capture_handle, hw_params);
    if (rc < 0)
    {
        fprintf(stderr, "unable to set the hw params: %s\n", snd_strerror(rc));
        exit(1);
    }

    int significant_bits = snd_pcm_hw_params_get_sbits(hw_params);
    printf("Number of significant bits: %i\n", significant_bits);

    if ((err = snd_pcm_prepare(capture_handle)) < 0)
    {
        fprintf(stderr, "Cannot prepare audio interfate for use (%s)\n", snd_strerror(err));
        exit(1);
    }

    // Now we acquire the data
    if (precision == 16)
    {
        //int dimension = frames * 4;
        //int dataSize = dimension;
        int num_samples = 2*frames*loops;
        short data[num_samples]; // This will contain all the samples

        printf("Number of samples we expect: %i\n", num_samples);

        int j = 0;

        while (loops > 0)
        {
            loops--;
            short buffer[frames];
            //short *buf = buffer; // This will contain the frames i.e. stuff that contain the samples from the two channels
            rc = snd_pcm_readi(capture_handle, buffer, frames);

            printf("Number of frames actually read (rc): %i\n", rc);
            printf("Number of elements in buffer: %i\n", sizeof(buffer)/sizeof(buffer[0]));
            printf("Dimension of one element in the buffer: %i\n", sizeof(buffer[0]));

            if (rc == -EPIPE)
            {
                perror("Reading failed");
            }

            for (int i = 0; i < rc; i += 2)
            {   
                //printf("%hi\n", buffer[i]);
                data[j*frames + i] = buffer[i];
                data[j*frames + i + 1] = buffer[i+1];
            }
            
            j++;
        }
        
        printf("\tReading process finished\n");

        printf("Size of data: %i\n", sizeof(data)/sizeof(data[0]));

        if (write_data("data.txt", data, sizeof(data)) != 0)
        {
            return 1;
        }
    }

    snd_pcm_close(capture_handle);

    return 0;
}