#include "board_setup.h"

// Set the hardware parameters of the board
int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params, snd_pcm_access_t access)
{
    unsigned int rrate = rate;
    snd_pcm_uframes_t size;

    // Choose all parameters
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set hardware resampling
    err = snd_pcm_hw_params_set_rate_resample(handle, params, resample);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set access format (interleaved access)
    err = snd_pcm_hw_params_set_access(handle, params, access);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set sample format 
    err = snd_pcm_hw_params_set_format(handle, params, format);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set number of channels
    err = snd_pcm_hw_params_set_channels(handle, params, n_channels);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }

    // Set stream rate
    err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }   
    if (rrate != rate)
    {
        printf("Rate doesn't match (requested %u Hz, get %i Hz)\n", rate, err);
        return -EINVAL;
    } 

    // Set buffer time
    err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_get_buffer_size(params, &size);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }  
    buffer_size = size;

    // Set period time
    err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 
    err = snd_pcm_hw_params_get_period_size(params, &size, &dir);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }  
    period_size = size;
    
    // Write parameters to the device
    err = snd_pcm_hw_params(handle, params);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    }  

    return 0;
}


// Set the software parameters of the board
int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams)
{
    // Get current software parameters
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 

    /*
    Start the transfer when the buffer is almost full: (buffer_size/avail_min)*avail_min
    */
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 

    /*
    Allow the transfer when at least period_size samples can be processed or disable when period_event is enabled
    */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 
    // Enable period events when requested
    if (period_event)
    {
        err = snd_pcm_sw_params_set_period_event(handle, swparams, 1);
        if (err < 0)
        {
            printf("An error occurred: %s\n", snd_strerror(err));
            return err;
        } 
    }

    // Write parameters to playback device
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0)
    {
        printf("An error occurred: %s\n", snd_strerror(err));
        return err;
    } 

    return 0;
}


// Set the hardware parameters of the board for the reading process
int set_read_hwparameters(snd_pcm_t *capture_handle, snd_pcm_hw_params_t *hw_params, snd_pcm_uframes_t frames)
{
    snd_pcm_hw_params_any(capture_handle, hw_params);
    snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    
    snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S24_LE);

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

    return 0;
}