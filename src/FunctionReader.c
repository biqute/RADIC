#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define ALSA_PCM_NEW_HW_PARAMS_API

int write_bytes_to_binary(const char *filename, unsigned char *data, int size)
{
    FILE *fp = fopen(filename, "wb");
    
    if (fp == NULL)
    {
        perror("fopen failed");
        return 1;
    }

    size_t written = fwrite(data, sizeof(unsigned char), size, fp);
    if (written != size)
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
    int precision = atoi(argv[1]);
    int rate = 192000;
    int channels = 2;

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
    snd_pcm_hw_params_set_channels(capture_handle, hw_params, channels);

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
        int dimension = frames * 4;
        int dataSize = dimension;

        printf("Dimension is: %d \n", dimension);

        int data[dimension*sizeof(int)*loops];
        /*if (data = NULL) 
        {
            fprintf(stderr, "Array not allocated");
            return 1;
        }
        */

        printf("Initial data dimension: %ld\n", sizeof(data));

        int j = 0;

        while (loops > 0)
        {
            loops--;

            /*int *buffer = (int*)malloc(dimension*sizeof(int));
            if (buffer == NULL)
            {
                fprintf(stderr, "Buffer not allocated");
                return 1;
            }*/

            int buffer[dimension*sizeof(int)];
            int *buf = buffer;

            printf("Initial dimension of the buffer: %ld\n", sizeof(buffer));

            rc = snd_pcm_readi(capture_handle, buf, dimension);

            if (rc == -EPIPE)
            {
                perror("Reading failed");
            }

            printf("Frames obtained: %d\n", rc);
            printf("Buffer dimension: %ld\n", sizeof(buf));

            for (int i = 0; i < rc; i++)
            {
                data[j*dimension + i] = buf[i];
                printf("%d\n", buf[i]);
            }

            //data[j*dimension] = *buffer; I guess this command doesn't work
            /*for (int i = 0; i < sizeof(buffer); i++)
            {
                //data[j*dimension + i] = buffer[i];
                printf("%d\n", buffer[i]);
            }*/

            /*dataSize += dimension;
            data = realloc(data, dataSize*sizeof(int));
            if (data == NULL)
            {
                fprintf(stderr, "Array not reallocated");
                return 1;
            }*/

            printf("ciao7\n");

            //free(buffer);
            j++;
        }

        printf("Reading process finished\n");

        /*if (write_bytes_to_binary("data.bin", data, sizeof(data)) != 0)
        {
            return 1;
        }
        */

        //free(data);
    }

    snd_pcm_close(capture_handle);

    return 0;
}