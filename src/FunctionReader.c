/* 
gcc FunctionReader.c -lasound -lm -o FunctionReader

./FunctionReader xx yy
    xx --> precision (16 or 32). Use 32 to read samples generated with 24 bits precision and the in data processing remove the first byte
    yy --> Number of loops to acquire
*/
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define ALSA_PCM_NEW_HW_PARAMS_API

// Write read 16 bits data to file
int write_data_16(const char *filename, short *data, int size)
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

// Write read 32 bits (which in reality are 24) data to file
int write_data_32(const char *filename, int *data, int size)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror("fopen failed");
        return 1;
    }

    size_t written = fwrite(data, sizeof(int), size/sizeof(int), fp);
    printf("Number of data written to file: %d\n", written);
    if (written != size/sizeof(int))
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
    // Setting of the parameters
    int precision = atoi(argv[1]);
    int err;
    int rc, dir = 0;
    char *device = "hw:1";  // Name of the device
    int rate = 192000;      // Use maximum samplig rate

    /* 
    Number of loops to capture. 
        1 loop means that we capture 512 frames which corresponds to approx. 3 ms of data with specified sampling rate
        To aquire around 1 second of data we have to acquire 375 loops
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

    //int significant_bits = snd_pcm_hw_params_get_sbits(hw_params);
    //printf("Number of significant bits: %i\n", significant_bits);
    //printf("Format width: %i\n", snd_pcm_format_width(SND_PCM_FORMAT_S16_LE));
    //printf("Bytes for sample: %i\n", snd_pcm_format_size(SND_PCM_FORMAT_S24_LE, 1));

    if ((err = snd_pcm_prepare(capture_handle)) < 0)
    {
        fprintf(stderr, "Cannot prepare audio interfate for use (%s)\n", snd_strerror(err));
        exit(1);
    }

    // Now we acquire the data
    if (precision == 16)
    {
        int num_samples = 2*frames*loops;
        short data[num_samples]; // This will contain all the samples

        //printf("Number of samples we expect: %i\n", num_samples);

        int j = 0;

        while (loops > 0)
        {
            loops--;
            short buffer[2*frames*snd_pcm_format_width(SND_PCM_FORMAT_S16_LE)/8];
            //short *buf = buffer; // This will contain the frames i.e. stuff that contain the samples from the two channels
            rc = snd_pcm_readi(capture_handle, buffer, frames);

            //printf("Number of frames actually read (rc): %i\n", rc);
            //printf("Number of elements in buffer: %i\n", sizeof(buffer)/sizeof(buffer[0]));
            //printf("Dimension of one element in the buffer: %i\n", sizeof(buffer[0]));

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

        //printf("Size of data: %i\n", sizeof(data)/sizeof(data[0]));

        if (write_data_16("data.txt", data, sizeof(data)) != 0)
        {
            return 1;
        }
    }
    else if (precision == 24)
    {
        //int dimension = frames * 4;
        //int dataSize = dimension;
        int num_samples = 2*frames*loops;
        int data[num_samples]; // This will contain all the samples

        //printf("Number of samples we expect: %i\n", num_samples);

        int j = 0;

        while (loops > 0)
        {
            loops--;
            int buffer[frames];
            //short *buf = buffer; // This will contain the frames i.e. stuff that contain the samples from the two channels
            rc = snd_pcm_readi(capture_handle, buffer, frames);

            //printf("Number of frames actually read (rc): %i\n", rc);
            //printf("Number of elements in buffer: %i\n", sizeof(buffer)/sizeof(buffer[0]));
            //printf("Dimension of one element in the buffer: %i\n", sizeof(buffer[0]));

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

        //printf("Size of data: %i\n", sizeof(data)/sizeof(data[0]));

        if (write_data_24("data.txt", data, sizeof(data)) != 0)
        {
            return 1;
        }
    }
    else if (precision == 32)
    {
        int num_samples = 2*frames*loops;
        int data[num_samples]; // This will contain all the samples

        //printf("Number of samples we expect: %i\n", num_samples);

        int j = 0;

        while (loops > 0)
        {
            loops--;
            int buffer[frames];
            //short *buf = buffer; // This will contain the frames i.e. stuff that contain the samples from the two channels
            rc = snd_pcm_readi(capture_handle, buffer, frames);

            //printf("Number of frames actually read (rc): %i\n", rc);
            //printf("Number of elements in buffer: %i\n", sizeof(buffer)/sizeof(buffer[0]));
            //printf("Dimension of one element in the buffer: %i\n", sizeof(buffer[0]));

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

        //printf("Number of elements in data: %i\n", sizeof(data)/sizeof(data[0]));

        if (write_data_32("data.txt", data, sizeof(data)) != 0)
        {
            return 1;
        }
    }

    snd_pcm_close(capture_handle);

    return 0;
}