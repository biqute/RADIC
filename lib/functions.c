#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define sgn(x) (x < 0) ? -1 : (x > 0)
#define ALSA_PCM_NEW_HW_PARAMS_API

// ************************* Writing Functions ************************* //

void play_sin(snd_pcm_t *handle, int frames, int num_frames, double rate, double frequency, int max_amplitude, int precision)
{   
    float PI = 3.14159;
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

void play_triangular(snd_pcm_t *handle, int frames, int num_frames, double rate, double frequency, int max_amplitude, int precision)
{
    float PI = 3.14159;
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

void play_square(snd_pcm_t *handle, int frames, int num_frames, double rate, double frequency, int max_amplitude, int precision)
{
    float PI = 3.14159;
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


// ************************* Reading Functions ************************* //
int write_data_16(const char *filename, int *data, int size)
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

int acquire_data(snd_pcm_t *handle, int frames, int loops, int precision)
{
    if (precision == 24)
    {
        int num_samples = frames*loops;
        long long data[num_samples]; // This will contain all the samples
        int j, rc = 0;

        while (loops > 0)
        {
            loops--;
            long long buffer[frames];
            rc = snd_pcm_readi(handle, buffer, frames);

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
    }
    else if (precision == 16)
    {
        int num_samples = 2*frames*loops;
        int data[num_samples]; // This will contain all the samples
        int j = 0;
        int rc = 0;
        while (loops > 0)
        {
            loops--;
            int buffer[frames];
            rc = snd_pcm_readi(handle, buffer, frames);

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

        if (write_data_16("data.txt", data, sizeof(data)) != 0)
        {
            return 1;
        }
    }
}