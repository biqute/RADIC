// All the other standard libraries are in "board_setup.h"
#include "board_setup.h"
#include "functions.h"

// Generate a sinusoidal function with specified parameters and assign it to memory areas
void generate_sine(
        const snd_pcm_channel_area_t *areas, 
        snd_pcm_uframes_t offset, 
        int count, 
        double *_phase, 
        int amplitude
    )
{
    static double max_phase = 2. * M_PI;
    double phase = *_phase;
    double step = max_phase*frequency/(double)rate;
    unsigned char *samples[n_channels];
    int steps[n_channels];
    unsigned int chn;
    
    int format_bits = snd_pcm_format_width(format);
    int bps = format_bits / 8;
    int phys_bps = snd_pcm_format_physical_width(format) / 8;
    int big_endian = snd_pcm_format_big_endian(format) == 1;
    int to_unsigned = snd_pcm_format_unsigned(format) == 1;
    int is_float = (format == SND_PCM_FORMAT_FLOAT_LE || format == SND_PCM_FORMAT_FLOAT_BE);

    // Verify and preapare the contents of areas
    for (chn = 0; chn < n_channels; chn++) {
        if ((areas[chn].first % 8) != 0) {
            printf("areas[%u].first == %u, aborting...\n", chn, areas[chn].first);
            exit(EXIT_FAILURE);
        }
        samples[chn] = /*(signed short *)*/(((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));
        if ((areas[chn].step % 16) != 0) {
            printf("areas[%u].step == %u, aborting...\n", chn, areas[chn].step);
            exit(EXIT_FAILURE);
        }
        steps[chn] = areas[chn].step / 8;
        samples[chn] += offset * steps[chn];
    }

    // Fill the channels areas
    while (count-- > 0) {
        union {
            float f;
            int i;
        } fval;
        int res, i;
        if (is_float) {
            fval.f = sin(phase);
            res = fval.i;
        } else
            res = sin(phase) * amplitude;
        if (to_unsigned)
            res ^= 1U << (format_bits - 1);
        for (chn = 0; chn < n_channels; chn++) {
            /* Generate data in native endian format */
            if (big_endian) {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
            } else {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + i) = (res >>  i * 8) & 0xff;
            }
            samples[chn] += steps[chn];
        }
        phase += step;
        if (phase >= max_phase)
            phase -= max_phase;
    }
    *_phase = phase;
}


// Generate a triangular function with specified parameters and assign it to memory areas
void generate_triangular(
        const snd_pcm_channel_area_t *areas, 
        snd_pcm_uframes_t offset, 
        int count, 
        double *_phase, 
        int amplitude
    )
{
    static double max_phase = 2. * M_PI;
    double phase = *_phase;
    double step = max_phase*frequency/(double)rate;
    unsigned char *samples[n_channels];
    int steps[n_channels];
    unsigned int chn;
    
    int format_bits = snd_pcm_format_width(format);
    int bps = format_bits / 8;
    int phys_bps = snd_pcm_format_physical_width(format) / 8;
    int big_endian = snd_pcm_format_big_endian(format) == 1;
    int to_unsigned = snd_pcm_format_unsigned(format) == 1;
    int is_float = (format == SND_PCM_FORMAT_FLOAT_LE || format == SND_PCM_FORMAT_FLOAT_BE);

    // Verify and preapare the contents of areas
    for (chn = 0; chn < n_channels; chn++) {
        if ((areas[chn].first % 8) != 0) {
            printf("areas[%u].first == %u, aborting...\n", chn, areas[chn].first);
            exit(EXIT_FAILURE);
        }
        samples[chn] = /*(signed short *)*/(((unsigned char *)areas[chn].addr) + (areas[chn].first / 8));
        if ((areas[chn].step % 16) != 0) {
            printf("areas[%u].step == %u, aborting...\n", chn, areas[chn].step);
            exit(EXIT_FAILURE);
        }
        steps[chn] = areas[chn].step / 8;
        samples[chn] += offset * steps[chn];
    }

    // Fill the channels areas
    while (count-- > 0) {
        union {
            float f;
            int i;
        } fval;
        int res, i;
        if (is_float) {
            fval.f = sin(phase);
            res = fval.i;
        } else
            res = asin(sin(phase))*(2*amplitude/M_PI);
        if (to_unsigned)
            res ^= 1U << (format_bits - 1);
        for (chn = 0; chn < n_channels; chn++) {
            /* Generate data in native endian format */
            if (big_endian) {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + phys_bps - 1 - i) = (res >> i * 8) & 0xff;
            } else {
                for (i = 0; i < bps; i++)
                    *(samples[chn] + i) = (res >>  i * 8) & 0xff;
            }
            samples[chn] += steps[chn];
        }
        phase += step;
        if (phase >= max_phase)
            phase -= max_phase;
    }
    *_phase = phase;
}

// Recovery of the board if an error occours
int xrun_recovery(snd_pcm_t *handle, int err)
{
    if (verbose)
    {
        printf("stream recovery\n");
    }
    if (err == -EPIPE)
    {
        err = snd_pcm_prepare(handle);
        if (err < 0)
        {
            printf("An error occurred: %s\n", snd_strerror(err));
        }
        return 0;
    }
    else if (err == -ESTRPIPE)
    {
        while ((err == snd_pcm_resume(handle)) == -EAGAIN)
        {
            sleep(1);
        }
        if (err < 0)
        {
            err = snd_pcm_prepare(handle);
            if (err < 0)
            {
                printf("An error occurred: %s\n", snd_strerror(err));
            }
        }
        return 0;
    }
    return err;
}


// Write data to the board
int write_loop(snd_pcm_t *handle, signed long *samples, snd_pcm_channel_area_t *areas, 
                unsigned int seconds, char *type, unsigned int amplitude, char *write_type)
{
    double phase = 0;
    signed long *ptr;
    int err, cptr;
    int loops = 375*seconds;

    if (*write_type == 'c')
    {
        while(1)
        {   
            if (*type == 's') 
            {
                generate_sine(areas, 0, period_size, &phase, amplitude);
            }
            else if (*type == 't')
            {
                generate_triangular(areas, 0, period_size, &phase, amplitude);
            }
            else
            {
                printf("Error: wave format not recognized");
                exit(EXIT_FAILURE);
            }
                
            ptr = samples;
            cptr = period_size;
            while (cptr > 0)
            {
                err = snd_pcm_writei(handle, ptr, cptr);
                if (err == -EAGAIN) 
                    continue;
                if (err < 0)
                {
                    if (xrun_recovery(handle, err) < 0)
                    {
                        printf("Write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
                ptr += err*n_channels;
                cptr -= err;
            }
            loops--;
        }
    }
    else if (*write_type == 'l')
    {
        while(loops > 0)
        {   
            if (*type == 's') 
            {
                generate_sine(areas, 0, period_size, &phase, amplitude);
            }
            else if (*type == 't')
            {
                generate_triangular(areas, 0, period_size, &phase, amplitude);
            }
            else
            {
                printf("Error: wave format not recognized");
                exit(EXIT_FAILURE);
            }
                
            ptr = samples;
            cptr = period_size;
            while (cptr > 0)
            {
                err = snd_pcm_writei(handle, ptr, cptr);
                if (err == -EAGAIN) 
                    continue;
                if (err < 0)
                {
                    if (xrun_recovery(handle, err) < 0)
                    {
                        printf("Write error: %s\n", snd_strerror(err));
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
                ptr += err*n_channels;
                cptr -= err;
            }
            loops--;
        }
    }
}



// Read data from the board
int read_loop(snd_pcm_t *capture_handle, snd_pcm_uframes_t frames, int loops, long long *data)
{
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

    return 0;
}


// Write an array of data to a file
int data_to_file(const char *filename, long long *data, int size)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        perror("fopen failed");
        return 1;
    }

    size_t written = fwrite(data, sizeof(long long), size/sizeof(long long), fp);
    printf("Number of frames written to file: %d\n", written);
    if (written != size/sizeof(long long))
    {
        perror("fwrite failed");
        fclose(fp);
        return 1;
    }

    fclose(fp);
    return 0;
}